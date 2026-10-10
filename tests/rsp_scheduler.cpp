#include "rsp_scheduler.h"
#include <cstdlib>
#include <cstdio>
static void check(bool value) { if (!value) std::abort(); }
int main() {
    RspScheduler rsp;
    check(rsp.start_graphics(100) == RspScheduler::Start::Submit);
    check(rsp.yield_graphics()); // Audio can run while RT64 parses the list.
    check(rsp.status() == 0x181);
    check(!rsp.finish_graphics(100)); // No completion attributed to the audio task.
    rsp.finish_audio();
    check(rsp.start_graphics(100) == RspScheduler::Start::Complete);
    check(rsp.status() == 0x201);
    check(rsp.start_graphics(100) == RspScheduler::Start::Submit); // Recycled OSTask.
    check(rsp.yield_graphics());
    rsp.finish_audio();
    check(rsp.start_graphics(100) == RspScheduler::Start::Resume);
    check(rsp.finish_graphics(100));
    check(!rsp.yield_graphics()); // Completion is already enqueued; no duplicate SP.
    check(rsp.status() == 0x201);
    check(rsp.start_graphics(200) == RspScheduler::Start::Submit);
    check(rsp.yield_graphics());
    check(rsp.start_graphics(200) == RspScheduler::Start::Resume);
    check(rsp.yield_graphics()); // A second audio deadline during the same list.
    check(!rsp.finish_graphics(200));
    check(rsp.start_graphics(200) == RspScheduler::Start::Complete);
    std::puts("RSP audio preemption: passed");
}
