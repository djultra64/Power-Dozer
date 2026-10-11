#include "recomp.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "sound-heartbeat-declarations.inc"

namespace {
constexpr uint32_t player = 0x80366BD0, queue = player + 0x14;
constexpr uint32_t nodes = 0x80010000, event = 0x80020000;
constexpr uint32_t state = 0x80020100, sound = 0x80020200, key = 0x80020300;
constexpr uint32_t stack = 0x807fff00, player_global = 0x802E8CEC;
int invalid_visits = 0;
struct BoundedOriginalSpin {};

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
uint32_t word(uint8_t* ram, uint32_t address) {
    uint32_t value;
    std::memcpy(&value, ram + address - 0x80000000, 4);
    return value;
}
void put(uint8_t* ram, uint32_t address, uint32_t value) {
    std::memcpy(ram + address - 0x80000000, &value, 4);
}
uint16_t half(uint8_t* ram, uint32_t address) {
    uint16_t value;
    std::memcpy(&value, ram + ((address - 0x80000000) ^ 2), 2);
    return value;
}
void put_half(uint8_t* ram, uint32_t address, uint16_t value) {
    std::memcpy(ram + ((address - 0x80000000) ^ 2), &value, 2);
}
void call(uint8_t* ram, recomp_context& ctx, void (*function)(uint8_t*, recomp_context*),
          uint32_t a, uint32_t b = 0, uint32_t c = 0) {
    ctx.r4 = S32(a); ctx.r5 = S32(b); ctx.r6 = S32(c);
    function(ram, &ctx);
}
int count(uint8_t* ram, uint32_t head) {
    int size = 0;
    for (uint32_t item = word(ram, head); item; item = word(ram, item)) {
        check(++size <= 64, "Queue list cycle");
    }
    return size;
}
struct Fixture {
    std::vector<uint8_t> memory = std::vector<uint8_t>(8 * 1024 * 1024);
    uint8_t* ram = memory.data();
    recomp_context ctx{};
    Fixture() {
        invalid_visits = 0;
        ctx.r29 = S32(stack);
        put(ram, player_global, player);
        call(ram, ctx, func_802D6E3C, queue, nodes, 64);
        put(ram, player + 0x4c, 33000);
        put_half(ram, player + 0x28, 0x20);
        put(ram, state + 8, sound);
        put(ram, sound + 4, key);
        put_half(ram, event, 4); // Inactive PAN: original handler has no voice dependency.
        put(ram, event + 4, state);
        put(ram, event + 8, 64);
    }
    void post(uint32_t delta = 0, uint32_t target = queue) {
        call(ram, ctx, func_802D6C8C, target, event, delta);
    }
    void callback() {
        try {
            call(ram, ctx, func_8025F044, player);
        } catch (const BoundedOriginalSpin&) {
            check(count(ram, queue) == 64 && count(ram, queue + 8) == 0,
                  "Unexpected spin queue state");
            check(half(ram, player + 0x28) == 0xffff && word(ram, player + 0x50) == 0,
                  "Unexpected spin sentinel or delta");
            throw std::runtime_error("Original callback lost heartbeat: FFFF/zero endless loop (bounded at two visits)");
        }
        check(uint32_t(ctx.r29) == stack, "Hook must preserve caller stack");
    }
};
}

// The callback, queue, event handler and removal remain exact generated code.
// Only hardware/external boundaries are doubles; unused synthesis must not run.
void osSetIntMask_recomp(uint8_t*, recomp_context* ctx) { ctx->r2 = 1; }
void func_802DDDE0(uint8_t* ram, recomp_context* ctx) {
    std::memmove(ram + uint32_t(ctx->r5) - 0x80000000,
                 ram + uint32_t(ctx->r4) - 0x80000000, uint32_t(ctx->r6));
}
void bc_rmonPrintf(uint8_t* ram, recomp_context*) {
    check(half(ram, player + 0x28) == 0xffff, "Unexpected guest print");
    if (++invalid_visits == 2) throw BoundedOriginalSpin{};
}
extern "C" void switch_error(const char*, uint32_t, uint32_t) {
    throw std::runtime_error("Unexpected guest switch target");
}
#include "sound-heartbeat-unreachable.inc"
#include "sound-heartbeat-original.inc"
#ifdef BC_SOUND_HEARTBEAT_BASELINE
void func_802D6C8C(uint8_t* ram, recomp_context* ctx) {
    bc_original_sound_event_post(ram, ctx);
}
#endif

static void saturation() {
    Fixture f;
    // Removing the reserve causes the real callback to lose API before draining
    // all ordinary events and entering its original FFFF/zero loop.
    for (int cycle = 0; cycle < 100; ++cycle) {
        for (int n = 0; n < 64; ++n) f.post();
#ifndef BC_SOUND_HEARTBEAT_BASELINE
        check(count(f.ram, queue) == 1 && count(f.ram, queue + 8) == 63,
              "Ordinary posts must use every node except the heartbeat reserve");
#endif
        f.callback();
        check(invalid_visits == 0 && half(f.ram, player + 0x28) == 0x20,
              "Periodic API heartbeat must remain selected");
        check(word(f.ram, player + 0x50) == 33000 && uint32_t(f.ctx.r2) == 33000,
              "Original callback must return original positive frame delta");
        check(count(f.ram, queue) == 64, "All queue nodes must be returned");
    }
    std::puts("100 saturated producer/callback cycles retained original API heartbeat");
}
static void cancellation() {
    Fixture f;
    f.post(1);
    // Original callback initializes only type, leaving this stale state slot.
    put(f.ram, stack - 0x10, state);
    put(f.ram, stack - 0xc, 0xaaaaaaaa);
    put(f.ram, stack - 8, 0xbbbbbbbb);
    f.callback();
    check(half(f.ram, player + 0x28) == 4, "PAN should leave API queued");
    const uint32_t api = word(f.ram, queue + 8);
    check(api && half(f.ram, api + 0xc) == 0x20 && count(f.ram, queue) == 63,
          "Cancellation scenario must not depend on saturation");
    call(f.ram, f.ctx, func_80260148, queue, state, 0xffff);
    f.callback();
    check(invalid_visits == 0 && half(f.ram, player + 0x28) == 0x20,
          "Sound cancellation must preserve API heartbeat");
    check(word(f.ram, player + 0x50) == 32999, "Original remaining delta must survive");
    std::puts("Original sound removal preserved API despite stale callback-local state alias");
}
static void scope_and_payload() {
    Fixture f;
    // The same type and queue shape on a different live player must be untouched.
    put(f.ram, player_global, player + 0x100);
    for (int n = 0; n < 64; ++n) f.post();
    check(count(f.ram, queue) == 0 && count(f.ram, queue + 8) == 64,
          "Other queues retain their full original capacity");
    const uint32_t ordinary = word(f.ram, queue + 8);
    check(half(f.ram, ordinary + 0xc) == 4 && word(f.ram, ordinary + 0x10) == state &&
          word(f.ram, ordinary + 0x14) == 64, "Ordinary event payload must be preserved");
    for (uint32_t pointer : {0u, 0xfffffff0u, player + 1}) {
        put(f.ram, player_global, pointer);
        f.post(); // Full unrelated queue: original drop, no invalid pointer read.
    }
    Fixture api;
    put_half(api.ram, event, 0x20);
    put_half(api.ram, event + 2, 0xffff);
    put(api.ram, event + 4, state);
    put(api.ram, event + 8, 0xaaaaaaaa);
    put(api.ram, event + 12, 0xbbbbbbbb);
    std::array<uint8_t, 16> before{};
    std::memcpy(before.data(), api.ram + event - 0x80000000, 16);
    api.post(33000);
    const uint32_t item = word(api.ram, queue + 8);
    check(half(api.ram, item + 0xc) == 0x20 && half(api.ram, item + 0xe) == 0,
          "API event type and padding must be initialized");
    check(word(api.ram, item + 0x10) == 0 && word(api.ram, item + 0x14) == 0 &&
          word(api.ram, item + 0x18) == 0, "API payload must be initialized");
    check(std::memcmp(before.data(), api.ram + event - 0x80000000, 16) == 0,
          "Hook must not mutate caller event");
    check(uint32_t(api.ctx.r29) == stack, "API scratch frame must be restored");

    Fixture other_api;
    put(other_api.ram, player_global, player + 0x100);
    put_half(other_api.ram, event, 0x20);
    other_api.post(33000);
    check(word(other_api.ram, word(other_api.ram, queue + 8) + 0x10) == state,
          "API type on a different queue must retain its payload");

    Fixture shallow_stack;
    shallow_stack.ctx.r29 = S32(0x80000070);
    put_half(shallow_stack.ram, event, 0x20);
    shallow_stack.post(33000);
    check(word(shallow_stack.ram, word(shallow_stack.ram, queue + 8) + 0x10) == state &&
          uint32_t(shallow_stack.ctx.r29) == 0x80000070,
          "Insufficient scratch stack must preserve original post without out-of-bounds writes");
    std::puts("Queue scope and initialized API copy preserve caller event and stack");
}
int main(int argc, char** argv) {
    try {
        const std::string_view scenario = argc > 1 ? argv[1] : "saturation";
        if (scenario == "saturation") saturation();
        else if (scenario == "cancellation") cancellation();
        else if (scenario == "scope") scope_and_payload();
        else throw std::runtime_error("Unknown scenario");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
