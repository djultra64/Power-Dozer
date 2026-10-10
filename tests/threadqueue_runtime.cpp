#include "ultramodern/ultramodern.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>

// Compile the actual SDK source (original for RED, prepared for GREEN).
// No scheduling implementation or queue operations are substituted.
static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
int main(int argc, char** argv) {
    try {
        alignas(OSThread) std::array<uint8_t, 1024> ram{};
        auto* rdram = ram.data();
        constexpr int32_t queue = int32_t(0x80000020);
        constexpr int32_t a = int32_t(0x80000080), b = int32_t(0x800000C0);
        constexpr int32_t c = int32_t(0x80000100), missing = int32_t(0x80000140);
        auto* ta = TO_PTR(OSThread, a); auto* tb = TO_PTR(OSThread, b);
        auto* tc = TO_PTR(OSThread, c);
        ta->priority = 30; tb->priority = 20; tc->priority = 10;
        const bool running = argc > 1 && std::strcmp(argv[1], "running") == 0;
        const int32_t destination = running ? ultramodern::running_queue : queue;
        if (argc > 1 && std::strcmp(argv[1], "empty") == 0) {
            require(!ultramodern::thread_queue_remove(rdram, destination, missing), "empty removal must return false");
        } else {
            ultramodern::thread_queue_insert(rdram, destination, c);
            ultramodern::thread_queue_insert(rdram, destination, a);
            ultramodern::thread_queue_insert(rdram, destination, b);
            require(ultramodern::thread_queue_peek(rdram, destination) == a && ta->next == b && tb->next == c,
                    "fixture must retain descending priority order");
            if (argc > 1 && std::strcmp(argv[1], "missing") == 0) {
                require(!ultramodern::thread_queue_remove(rdram, destination, missing), "absent thread removal must return false");
                require(ta->next == b && tb->next == c && tc->next == 0, "missing removal must preserve all links");
            } else {
                if (argc > 1 && std::strcmp(argv[1], "destroy") == 0) {
                    tb->state = OSThreadState::QUEUED;
                    // Use the actual SDK consumer used at original game PC
                    // 0x80256BDC when unloading the frontend thread.
                    osDestroyThread(rdram, b);
                } else {
                    require(ultramodern::thread_queue_remove(rdram, destination, b), "non-head thread removal must succeed");
                }
                require(ultramodern::thread_queue_peek(rdram, destination) == a && ta->next == c && tc->next == 0,
                        "non-head removal must retain head and tail order");
                require(ultramodern::thread_queue_remove(rdram, destination, c) && ta->next == 0, "tail removal must succeed");
                require(ultramodern::thread_queue_remove(rdram, destination, a), "head removal must succeed");
                require(ultramodern::thread_queue_empty(rdram, destination), "all removed threads leave empty queue");
                require(!ultramodern::thread_queue_remove(rdram, destination, missing), "empty queue removal must finish");
            }
        }
        std::puts("Actual SDK thread removal, ordering and absence checks passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
