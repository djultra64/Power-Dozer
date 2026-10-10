#include "audio_fifo.h"
#include <cstdio>
#include <cstdlib>
static void check(bool valid) { if (!valid) std::abort(); }
int main() {
    AudioFifo fifo;
    check(fifo.head_bytes(0) == 0);
    fifo.push(3136);
    fifo.push(2944);
    check(!fifo.room(6080));
    check(fifo.head_bytes(6080) == 3136);
    check(fifo.head_bytes(5000) == 2056);
    check(fifo.head_bytes(2944) == 2944);
    check(fifo.room(2944));
    check(fifo.head_bytes(0) == 0);
    fifo.reset();
    fifo.push(3200);
    check(fifo.head_bytes(3200) == 3200);
    bool rejected = false;
    try { fifo.head_bytes(3201); } catch (const std::logic_error&) { rejected = true; }
    check(rejected);
    std::puts("audio FIFO: passed");
}
