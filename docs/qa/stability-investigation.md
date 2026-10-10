# Native launch stability investigation

2026-10-10, branch `codex/release-readiness`. Investigation uses the pinned SDK
read-only and private CPU fixtures; it does not change guest progress or impose
a watchdog, fake task completion, reduced rendering quality, or disabled audio.

## Proven SDK thread removal defect

Pinned `N64ModernRuntime/ultramodern/src/threadqueue.cpp` SHA-256:
`b9bb55edeb9cb99a153d00d80a13c5a09140113403499af7656fd9489d38f305`.
The original removal loop at lines 46–53 repeatedly reads the original queue
head. Although its `cur` variable changes, line 48 passes `queue_` to
`queue_to_ptr` again. If the requested thread is not the head and the head has
a successor, the loop never advances. A missing requested thread has the same
failure. An empty queue also dereferences the guest NULL pointer at line 53.

The real SDK consumer `threads.cpp:286–304`, `osDestroyThread`, calls that
removal at line 297 for a non-stopped target. The original Blast Corps guest
uses it when unloading the frontend: `hd.c:4835`, frontend object
`0x80218D30`, generated call at guest PC `0x80256BDC`
(`.local/generated/functions/funcs_0.c:51036`). This establishes a real game
path to the proven defect.

`tests/threadqueue_runtime.cpp` compiles the actual SDK queue implementation,
with actual SDK `threads.cpp` for the destruction consumer. No queue or
destruction implementation is substituted. Private compiler invocations use
`-O2 -DNDEBUG`; section garbage collection omits unrelated thread entrypoints
from this bounded CPU fixture. A parent subprocess timeout detects the old
infinite loop without changing its implementation.

| Actual-source case | Original pinned SDK | Prepared local source |
|---|---|---|
| Remove middle element from three threads | Timeout after 2 s | Exit 0 |
| Remove absent element from three threads | Timeout after 2 s | Exit 0 |
| Remove middle element from host running queue | Timeout after 2 s | Exit 0 |
| Remove from empty queue | SIGSEGV, exit -11 | Exit 0 |
| Real `osDestroyThread` removes middle queued thread | Timeout after 2 s | Exit 0 |

The prepared source walks addresses of successive `next` links, splices only
the requested thread, and returns false after reaching the end. It retains
head/tail order and the SDK's existing thread metadata behavior. The shared
SDK is untouched. `tools/prepare_threadqueue_runtime.py` rejects an unknown
fingerprint or non-unique transform boundaries before writing output and keeps
the generated file's modification time stable when its content is unchanged.
CMake substitutes this local file and tracks both generator and pinned source
for automatic reconfiguration. Native tests have explicit five-second limits.

Integrated verification: configure and production build exited 0, retaining
`RelWithDebInfo`; full CTest passed 35/35 (including the five new actual SDK
cases), full Python unittest passed 88/88, and `git diff --check` passed. The
read-only SDK fingerprint remained identical after the build. Current native
ELF is unstripped with debug information, SHA-256
`43adc6c54ee887a76daa6a32d242f0fbf5b820d6a0f3c92f33be6e747159998d`,
BuildID `216f81781aae815a970361dbe9e999f0c9c435c6`, built from HEAD
`6bf0e2d15daf8572095d2be80036a993d40efa51` plus the local threadqueue change.
Native gameplay regression of this binary is pending; fixture acceptance alone
does not claim the historical startup freeze closed.

Private evidence:

- `.local/qa/stability-launch/original-threadqueue-red.json`: exact original
  source fingerprint, compiler command and four original outcomes.
- `.local/qa/stability-launch/prepared-threadqueue-green.json`: prepared source
  fingerprint, compiler command and four corrected outcomes.
- `.local/qa/stability-launch/actual-sdk-destroy-red-green.json`: actual SDK
  destruction consumer before and after the local transform.
- `.local/qa/stability-launch/python-tests.log`: full Python suite.

## Relationship to previously observed native freezes

`gameplay-sfru9ccc` stopped graphics/audio/input polls at 968/488/762 around
VI 988, immediately after guest mode 28→15. Host VI continued through 2263;
RT64's graphics thread continued processing screen updates and metrics.
Scheduler message reception stopped, so the stall is on the guest execution
or scheduling path, rather than a globally blocked renderer thread. Frontend
thread 2 had been recreated and started just before this transition.

The broken thread removal and original frontend unload path are compatible
with that symptom and transition. However, the failed run predates native
debugger capture: its exact queue links and CPU stack at the stall were never
recorded. The VI 960 audio snapshot is a normal pre-stall state, also observed
in healthy runs; it cannot prove a blocked audio task. Thus the SDK defect and
its real consumer reproduction are proven, while attribution of that historic
freeze remains a hypothesis until a failed-native stack or an equivalent
instrumented native trigger establishes it. Earlier audio-only loss in
`gameplay-ja976uzj` likewise is not closed by this queue fixture.

After production integration, validation must include the full native suite
and a private native frontend transition/restart run. Existing live-hang
capture can identify a future stall before teardown. A passing repeated run
supports regression acceptance but alone does not retroactively prove the
historic failure's cause.

## New live stall after threadqueue correction

`gameplay-dn_rgo2e` reproduced a simultaneous stop after commit `85b45ba`,
using current production objects with only a private value-preserving
framebuffer lookup wrapper. Wrapper logging begins at VI 2500; this run stopped
around VI 997, so none of that logging executed. At VI 1202 and 1502,
graphics/audio/input polls stayed 969/486/765 while VI advanced. An actual
pre-teardown GDB attachment captured all 44 native threads.

No guest thread was in `thread_queue_remove`. Scheduler thread 5, main thread
3, audio thread 4, and frontend thread 2 were waiting in SDK `do_recv` /
`wait_for_resumed`. The SP worker waited for a task, and the graphics worker
waited for its next action; RT64/Vulkan workers were idle or waiting normally.
The active idle guest thread 1 was in SDK `pause_self` →
`wait_for_external_message` → `BlockingConcurrentQueue::wait_dequeue` line 364,
inside `ConcurrentQueue::try_dequeue` producer-size scanning. That point follows
successful semaphore acquisition, so another snapshot must determine whether
this is a persistent queue spin or a briefly sampled dequeue.

Captured guest queues:

- Scheduler interrupt queue: zero messages, receiver `0x803154F0`.
- Audio frame queue: full (8/8), no receiver.
- Audio reply queue: zero messages, receiver `0x80368088`.
- Pending audio list: `0x80396F48`; current SP and DP task: `0x8036E758`.

This rules out attributing the new stall to the already-corrected removal loop.
The external host-message queue is the next evidence boundary. A private CPU
fixture compiling actual SDK `mesgqueue.cpp`, with real pinned concurrentqueue
headers, exercised 1,200 producer thread lifetimes and 38,400 unique deliveries;
it exited 0 without loss or duplicate delivery. Simple producer recycling alone
therefore did not reproduce the native issue, and no speculative replacement
queue or wakeup fix is justified.

Native hang capture now takes two bounded snapshots, with detach and 150 ms of
normal execution between them. It records deeper guest stacks, the SDK running
queue head, each original guest thread's queue/state/context and resume
semaphore, the host external-message semaphore, and at most 16 producer links
with tail/head/optimistic/overcommit counts and cycle detection. The helper uses
debugger memory reads only. Producer and semaphore inspection was verified on
the actual SDK CPU event-delivery fixture; that fixture does not contain the
native game's thread or RSP symbols, which are reported unavailable there.
Current evidence is retained under `.local/qa/stability-launch/`.

## Proven destruction/reuse lifecycle race

Pinned SDK `ultramodern/src/threads.cpp` SHA-256:
`a2fef03027eece8d5ae6346b4f8695ed3fb2b03217069e6b8ef4a80995771468`.
External `osDestroyThread` cleared the guest context pointer and woke the old
host thread, then returned immediately. The old host thread still read that
guest pointer in `wait_for_resumed:157` and `_thread_func`, while a subsequent
`osCreateThread:265` could overwrite it. Reusing or unloading the guest thread
slot therefore happened before the cancelled host thread finished its reads.
Cooperative scheduling of the surviving guest threads does not serialize the
cancelled host thread's asynchronous teardown.

Two actual-source CPU reproductions reported ThreadSanitizer exit 66 on that
plain context-pointer read/write pair. One repeatedly created and destroyed a
never-started thread. The stronger reproduction used the real SDK thread,
message, priority queue and scheduling sources: a guest parent created and
started a higher-priority frontend, that frontend blocked in `osRecvMesg`, and
the parent destroyed and reused its slot. No queue, lifecycle, semaphore or
scheduling implementation was mocked; the fixture supplies guest entrypoints
instead of ROM code. The original game likewise destroys its frontend thread
and later recreates or unloads its memory, but this race is not claimed as the
cause of the captured external-message queue stall.

The fingerprinted local `tools/prepare_thread_lifecycle_runtime.py` transform
makes external destruction wait for the cancelled host thread's actual exit
before returning. A host mutex protects a set of contexts whose destruction
caller owns joining/deletion. The cancelled worker removes its marker and
skips the asynchronous cleaner; the caller joins it and deletes its context.
The mutex is released before signalling or joining. Thus one owner deletes
each context, and the caller's join establishes completion before guest RAM
reuse. Self-destruction and normal guest returns still use the original SDK
cleanup worker. No SDK header or ABI changes, artificial timeout, guest state
patch or fake wakeup was introduced; the shared SDK remains unchanged.

The prepared actual implementation passed ThreadSanitizer with exit 0 for
1,500 cycles each of never-started destruction, started/blocked destruction,
self-destruction and normal return. The preceding minimal 3,000-cycle
create/destroy/reuse fixture also passed with the correction. The final native
regression compiles these same real SDK components, verifies entry counts,
cleared context pointers and removal from the blocked receiver queue, and has
an explicit parent timeout. Full Python suite passed 103/103 after integration.
The production RelWithDebInfo configure and build both exited 0; CTest passed
39/39, including all four lifecycle modes. The executable retains debug info
and is not stripped. This checkpoint uses HEAD
`85b45bacc6399b513037ae1de656d21180e4666b` plus the shared working-tree changes:
ELF SHA-256 `6d2104d1c77f4a5f61b591b4d074d02f1ad558d00dfcdda321dc65203a02ba87`,
GNU Build ID `b93cf1ccb6f7b422fc8e84971e4248d08767f2f7`.
The generated `build/blastcorps-threads.cpp` SHA-256 is
`178239c32c1367d53810915872cc70f2e7a69db6fe63edc7d82cac43f9444f42`;
the pinned SDK source fingerprint remains unchanged. Native regression after
this checkpoint is a separate requirement; the intermittent gameplay stall
remains open until its cause is captured or otherwise demonstrated.

Private evidence: `lifecycle-tsan.log`, `original-started-tsan.log`,
`joined-started-tsan.log`, four `prepared-lifecycle-*-tsan.log` files,
`lifecycle-provenance.json` and `prepared-full-lifecycle-provenance.json` under
`.local/qa/stability-launch/`. They include original/prepared fingerprints,
the actual compile invocation and runtime outcomes. The private TSAN runtime
is the previously verified Fedora GCC 15.2.1-7.fc43 extraction retained in
`timer-race`; it is not installed or packaged with the game.
