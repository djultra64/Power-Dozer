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

## Proven starvation of external producers by completion retries

New startup freeze `.local/qa/gameplay-zpzpxc35/native-hang.txt` includes two
read-only snapshots, separated by normal execution. All guest workers remain
asleep, the running queue is empty, and the idle thread is active in the SDK
external-message dequeue path. Its newest producer advances head/tail from
521,659,458/521,659,460 to 537,993,697/537,993,698. An older event producer's
head remains 1008 while its tail advances 1515→1532. External semaphore count
increases 512→529. These are successful repeated dequeues/re-enqueues with
other real messages still pending, not evidence of a lost semaphore token.

Pinned `mesgqueue.cpp` SHA-256:
`b504bd7e4290189e0e7e7b522486cc4b9ec22ae54f73297d4fa2c0e3f1575045`.
Its untimed/timed waits immediately re-enqueued each rejected critical event.
The pinned ConcurrentQueue tokenless dequeue (header 1125–1146) compares only
the first three non-empty producer streams. A retry stream plus two other
blocked streams can therefore hide the older event producer indefinitely.
The new snapshots show exactly that topology and rapidly advancing retry
indices; they identify a causal starvation mechanism beyond the previous
single-stack samples. Exact native retry payloads were not captured.

A CPU fixture compiling this actual SDK implementation and its pinned queue
headers keeps four real host producers alive: an older VI message to an
available guest queue, followed by three critical streams to a saturated
queue. The original implementation returns from 1,000,000 waits without
delivering VI (exit 1, ready queue 0), while retaining the blocked messages.
The prepared implementation delivers VI in its first wait (exit 0) and retains
all blocked completions. Scheduler callbacks are traps in this isolated
fixture because no guest thread waits on its queues; scheduling priority is
not simulated or claimed as exercised by it.

The fingerprinted `tools/prepare_mesgqueue_runtime.py` transform shares one
deferred-retry vector between the first rejected message and the existing
batch drain. It drains currently available streams before re-enqueuing any
rejected message. The first message stays ahead of later retries from its
producer; no replacement concurrent queue, discarded critical completion,
fake acknowledgement, sleep, timer patch or inferior state write is used.
Untimed and timed paths use the same helper. `do_send(...,block=false)` only
schedules a receiver; the existing idle/yield caller checks thread priority
after the wait returns. The batch does not invoke that check or recurse.
Its drain-until-empty behavior already existed in the SDK batch function;
continuous producers can still prolong a batch and are not given a new cap.

`tests/mesgqueue_runtime.cpp` compiles the actual prepared SDK source. Both
wait modes pass: every SP/DP/Timer/SI payload is recovered exactly once after
space becomes available; the older full queue's eight messages remain
unchanged; two messages from one producer preserve order; saturated VI/AI/PI
ticks keep their SDK discard policy; deferred jam insertion remains at the
head ahead of an existing tail. Mechanical transform tests validate stable
output, untouched reference source and rejection of unknown fingerprints
or changed sites. Full Python suite 110/110 passed after integration.

Private evidence includes `sdk-external-starvation.cpp`,
`mesgqueue-{red,green,normal,timed}.log` and `mesgqueue-results.json` under
`.local/qa/stability-launch/`. Production RelWithDebInfo configure/build both exited 0. CTest passed 41/41
(including the two new actual-source modes), and Python passed 110/110.
The unstripped ELF SHA-256 is
`b44d72c32cddf274b650b2dedea1fa84496cda540d7391121031d7afd8a9747e`,
GNU Build ID `99d965828e9c3f2b349f07e4e3902c8c2d2e0a6b`, from HEAD
`f3e927a25164b35546eb3ed0b99d9cb7f4235c56` plus these working-tree changes.
Generated message source SHA-256 is
`e2bf8af2828eb36fa8b239a39347d96d5e56fdfb4e9058e0d54d8e6c3db01715`;
original SDK fingerprint remains unchanged. Independent review found no
material issue in the transformation, formal fixtures or source substitution.
`mesgqueue-native-checkpoint.json` records the identity and results. Native
post-fix gameplay verification will be tracked separately. The starvation mechanism is
proven; post-fix gameplay and repeated cold startup are still needed before
claiming the intermittent native freeze is closed.


### First native regression after retry fairness correction

The production executable `b44d72c32cddf274b650b2dedea1fa84496cda540d7391121031d7afd8a9747e`
completed private `gameplay-7dgvn2n3` with exit 0, 180 seconds, VI 10801,
10956 graphics tasks and 5399 audio tasks. Existing ALF progress loaded and
replayed a completed Simian Acres; movement, pause/Continue and Restart were
verified by guest state. Seed EEPROM and Pak remained unchanged. This tests the
normal scheduler and audio/render paths beyond the isolated fairness fixtures.
Repeated cold launches and the updated independent-source executable remain
separate acceptance steps. It does not certify all missions or sustained 120 fps.

Four additional cold launches in private `coldboots-26vhqm_p` each completed
30 seconds and exited 0 at VI 1801, graphics 1784 and audio 898–899. The health
checker observed advancing graphics/audio beyond the previous startup stall,
and the PCM checker observed non-silent audio. All four used the same corrected
production SHA and fresh private copies of the same seeds; source hashes stayed
unchanged. This closes the demonstrated producer-starvation regression in the
checked startup/session paths, without claiming exhaustive stability coverage.

The independently reconstructed host with the same correction, ELF
`27d46626bbd6b3e48a0317b4e20191bcb2a68d9c949862289470d1bdfe1c23ad`,
also completed `gameplay-3ar19xmo`: 180 seconds, exit 0, VI 10801, 10951 graphics
and 5399 audio tasks. Its cwd and executable were outside this checkout in a
path with spaces. Movement, pause/Continue, Restart and completed-level replay
were verified; source EEPROM/Pak hashes stayed unchanged. Its SDK/RT64/tools
were built independently from exported sources; full CPU suites passed 41/110.
