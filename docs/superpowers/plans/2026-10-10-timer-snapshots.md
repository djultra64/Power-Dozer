# Immutable timer scheduling

Scope authorized by overnight release-readiness work. ThreadSanitizer on actual
pinned SDK timer.cpp reports producer osSetTimer line187 racing with timer-thread
std::set comparison line79. Existing scheduler uses this API for audio replies.
This is independently proven undefined behavior; causal closure of the observed
native startup freeze still requires separate evidence.

1. Keep pinned shared Tetrisphere SDK unchanged. Fingerprint its timer.cpp and
   prepare a local compiled copy through a fail-closed source transform.
2. Snapshot timer address, deadline, interval, destination queue and message in
   the producer action. Order host-owned immutable records, replace by address,
   cancel by address, and advance periodic deadlines only on the timer thread.
   Preserve guest API writes in osSetTimer; background scheduling never reads
   or writes guest timer memory. Preserve other timer/time APIs and delivery.
3. Verify actual replacement/cancellation/deadline ordering/one-shot/periodic
   behavior, and rerun the real-source stress fixture with ThreadSanitizer.
   Verify source fingerprint rejection and original source remains unchanged.
4. Wait for all pre-fix native startup trials to terminate before rebuilding
   the executable, then run native/Python checks and live startup/gameplay tests.
   Do not mark the native freeze closed solely from successful repetitions.
