#include "ultramodern/ultramodern.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Compile the real SDK thread, queue, scheduling and message implementations.
// Only guest entrypoints are supplied by this CPU fixture, instead of ROM code.
std::atomic_bool exited{false};
namespace {
constexpr int32_t parent = int32_t(0x80000040), child = int32_t(0x800000C0);
constexpr int32_t work_queue = int32_t(0x80000300), buffer = int32_t(0x80000340);
constexpr int32_t message = int32_t(0x80000380);
enum class Mode { Unstarted, Blocked, Self, Return };
Mode mode = Mode::Blocked;
std::atomic<unsigned> entered{0};
void require(bool value, const char* text) {
    if (!value) { std::fprintf(stderr, "%s\n", text); std::fflush(stderr); std::_Exit(1); }
}
}

void run_thread_function(uint8_t* ram, uint64_t entry, uint64_t, uint64_t) {
    auto* rdram = ram; // SDK TO_PTR uses this parameter spelling.
    if (uint32_t(entry) == 0x80000100) {
        osCreateMesgQueue(ram, work_queue, buffer, 1);
        for (unsigned i = 0; i < 1500; i++) {
            osCreateThread(ram, child, 2, int32_t(0x80000104), 0, int32_t(0x80000800), 11);
            if (mode != Mode::Unstarted) {
                osStartThread(ram, child);
                require(entered.load() == i + 1, "started child did not execute exactly once");
            }
            if (mode == Mode::Unstarted || mode == Mode::Blocked) osDestroyThread(ram, child);
            require(TO_PTR(OSThread, child)->context == nullptr, "destroyed child retains its context");
            require(ultramodern::thread_queue_empty(ram, GET_MEMBER(OSMesgQueue, work_queue, blocked_on_recv)),
                    "destroyed child remains in its receiver queue");
        }
        require(mode != Mode::Unstarted || entered.load() == 0, "never-started child executed guest code");
        std::puts("Actual SDK 1500 thread lifecycle cycles passed");
        std::fflush(stdout);
        std::_Exit(0); // SDK has no shutdown for its guest and cleanup workers.
    }
    entered.fetch_add(1);
    if (mode == Mode::Self) osDestroyThread(ram, 0);
    if (mode == Mode::Return) return;
    osRecvMesg(ram, work_queue, message, OS_MESG_BLOCK);
    require(false, "destroyed blocked child resumed guest code");
}

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "unstarted") == 0) mode = Mode::Unstarted;
    else if (argc > 1 && std::strcmp(argv[1], "self") == 0) mode = Mode::Self;
    else if (argc > 1 && std::strcmp(argv[1], "return") == 0) mode = Mode::Return;
    auto* ram = new uint8_t[4096]{};
    ultramodern::init_thread_cleanup();
    osCreateThread(ram, parent, 1, int32_t(0x80000100), 0, int32_t(0x80000C00), 0);
    osStartThread(ram, parent);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    require(false, "SDK lifecycle fixture timed out");
}
