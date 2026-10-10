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
