#include "event_delivery.h"
#include "ultramodern/ultramodern.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

// Real SDK message and concurrent queues. No fixture threads block on guest
// queues; reaching a scheduler callback is a test failure, not a mocked wakeup.
namespace ultramodern {
bool thread_queue_empty(uint8_t* rdram, int32_t queue) { return *TO_PTR(int32_t, queue) == 0; }
void thread_queue_insert(uint8_t*, int32_t, int32_t) { throw std::runtime_error("unexpected blocked send"); }
int32_t thread_queue_pop(uint8_t*, int32_t) { throw std::runtime_error("unexpected guest receiver"); }
void schedule_running_thread(uint8_t*, int32_t) { throw std::runtime_error("unexpected guest scheduling"); }
void run_next_thread_and_wait(uint8_t*) { throw std::runtime_error("unexpected guest switch"); }
int32_t this_thread() { throw std::runtime_error("unexpected guest thread lookup"); }
}
extern void dequeue_external_messages(uint8_t*);
extern bool do_recv(uint8_t*, int32_t, int32_t, bool);
extern bool do_send(uint8_t*, int32_t, int32_t, bool, bool);

namespace {
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
struct Producers {
    std::atomic<unsigned> ready{0};
    std::atomic<bool> done{false};
    std::vector<std::thread> threads;
    template<class F> void add(F f) {
        unsigned target = unsigned(threads.size()) + 1;
        threads.emplace_back([this, f] { f(); ready.fetch_add(1); ready.notify_all(); done.wait(false); });
        for (unsigned n = ready.load(); n != target; n = ready.load()) ready.wait(n);
    }
    ~Producers() { done.store(true); done.notify_all(); for (auto& thread : threads) thread.join(); }
};
}

int main(int argc, char** argv) try {
    bool timed = argc > 1 && std::strcmp(argv[1], "timed") == 0;
    std::array<uint8_t, 2048> ram{}; auto* rdram = ram.data();
    constexpr int32_t good = int32_t(0x80000040), good_buffer = int32_t(0x80000080);
    constexpr int32_t full = int32_t(0x80000100), full_buffer = int32_t(0x80000140);
    constexpr int32_t out = int32_t(0x80000200);
    bc::initialize_event_delivery();
    osCreateMesgQueue(rdram, good, good_buffer, 1);
    osCreateMesgQueue(rdram, full, full_buffer, 8);
    for (int i = 0; i < 8; ++i) require(do_send(rdram, full, 100 + i, false, false), "seed full queue");
    auto wait = [&] {
        if (timed) ultramodern::wait_for_external_message_timed(rdram, 1);
        else ultramodern::wait_for_external_message(rdram);
    };
    {
        Producers producers;
        // Older VI stream is behind three non-empty critical producer streams.
        // Keep all producers alive to prevent implicit TLS stream recycling.
        producers.add([=] { ultramodern::enqueue_external_message_src(good, 77, false, ultramodern::EventMessageSource::Vi); });
        producers.add([=] { ultramodern::enqueue_external_message_src(full, 22, false, ultramodern::EventMessageSource::Dp); });
        producers.add([=] { ultramodern::enqueue_external_message_src(full, 23, false, ultramodern::EventMessageSource::Timer); });
        producers.add([=] {
            ultramodern::enqueue_external_message_src(full, 41, false, ultramodern::EventMessageSource::Sp);
            ultramodern::enqueue_external_message_src(full, 42, false, ultramodern::EventMessageSource::Si);
        });
        unsigned attempts = 0;
        while (TO_PTR(OSMesgQueue, good)->validCount == 0 && attempts++ < 10000) wait();
        require(TO_PTR(OSMesgQueue, good)->validCount == 1, "ready VI starved behind retained critical retries");
        require(do_recv(rdram, good, out, false) && *TO_PTR(OSMesg, out) == 77, "VI payload changed");
        for (int i = 0; i < 8; ++i)
            require(do_recv(rdram, full, out, false) && *TO_PTR(OSMesg, out) == 100 + i, "retry overwrote occupied guest queue");
        dequeue_external_messages(rdram);
        require(TO_PTR(OSMesgQueue, full)->validCount == 4, "critical completion lost or duplicated");
        std::array<int, 4> actual{};
        for (int& value : actual) { require(do_recv(rdram, full, out, false), "missing retained completion"); value = *TO_PTR(OSMesg, out); }
        int first = -1, second = -1; std::array<unsigned, 4> counts{};
        for (unsigned i = 0; i < actual.size(); ++i) {
            int value = actual[i];
            if (value == 22) counts[0]++;
            else if (value == 23) counts[1]++;
            else if (value == 41) { counts[2]++; first = int(i); }
            else if (value == 42) { counts[3]++; second = int(i); }
            else require(false, "unknown completion payload");
        }
        for (unsigned count : counts) require(count == 1, "retained completion not delivered exactly once");
        require(first < second, "same-producer retained completion order reversed");
        dequeue_external_messages(rdram);
        require(TO_PTR(OSMesgQueue, full)->validCount == 0, "completion repeated after recovery");
    }
    // SDK defaults discard saturated VI/AI/PI ticks; only critical retries persist.
    osCreateMesgQueue(rdram, full, full_buffer, 1);
    require(do_send(rdram, full, 9, false, false), "seed nonretry queue");
    for (auto source : {ultramodern::EventMessageSource::Vi, ultramodern::EventMessageSource::Ai, ultramodern::EventMessageSource::Pi}) {
        ultramodern::enqueue_external_message_src(full, 88, false, source); wait();
    }
    require(do_recv(rdram, full, out, false) && *TO_PTR(OSMesg, out) == 9, "nonretry tick overwrote full queue");
    dequeue_external_messages(rdram);
    require(TO_PTR(OSMesgQueue, full)->validCount == 0, "nonretry tick retained");
    // A jammed critical completion retains head insertion after being deferred.
    require(do_send(rdram, full, 11, false, false), "seed jam queue");
    ultramodern::enqueue_external_message_src(full, 55, true, ultramodern::EventMessageSource::Sp); wait();
    require(do_recv(rdram, full, out, false) && *TO_PTR(OSMesg, out) == 11, "jam retry overwrote occupied queue");
    TO_PTR(OSMesgQueue, full)->msgCount = 2;
    require(do_send(rdram, full, 66, false, false), "seed jam recovery tail");
    dequeue_external_messages(rdram);
    require(do_recv(rdram, full, out, false) && *TO_PTR(OSMesg, out) == 55, "jam flag lost after retry");
    require(do_recv(rdram, full, out, false) && *TO_PTR(OSMesg, out) == 66, "jam damaged existing tail");
    dequeue_external_messages(rdram);
    require(TO_PTR(OSMesgQueue, full)->validCount == 0, "jam completion delivered twice");
    std::puts("Actual SDK producer fairness, critical retention/order, tick dropping and jam passed");
    return 0;
} catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
