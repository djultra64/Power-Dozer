#include "sound_heartbeat.h"

extern "C" void osSetIntMask_recomp(uint8_t*, recomp_context*);

namespace {
constexpr uint32_t player_pointer = 0x802E8CEC;
constexpr uint16_t api_event = 0x20;

bool guest_words(uint32_t address, uint32_t size) {
    return (address & 3) == 0 && address >= 0x80000000 &&
           address <= 0x80800000 - size;
}
}

extern "C" void func_802D6C8C(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = MEM_W(0, S32(player_pointer));
    const uint32_t queue = uint32_t(ctx->r4);
    const uint32_t event = uint32_t(ctx->r5);
    if (!guest_words(player, 0x58) || queue != player + 0x14 ||
        !guest_words(event, 16)) {
        bc_original_sound_event_post(rdram, ctx);
        return;
    }

    // Keep the capacity decision and original post in one guest critical
    // section. The SDK currently serializes guest execution cooperatively;
    // its mask translation is a no-op. There is no scheduling boundary in
    // this section. Nested mask restoration also supports a real translation.
    recomp_context mask_context = *ctx;
    mask_context.r4 = 1; // OS_IM_NONE, as used by the original queue function.
    osSetIntMask_recomp(rdram, &mask_context);
    const auto previous_mask = mask_context.r2;
    const uint16_t type = MEM_HU(0, S32(event));
    const uint32_t free_head = MEM_W(0, S32(queue));
    const bool reserve_last = type != api_event && guest_words(free_head, 8) &&
                              MEM_W(0, S32(free_head)) == 0;
    if (!reserve_last) {
        const auto original_stack = ctx->r29;
        const uint32_t stack = uint32_t(original_stack);
        const bool initialize_api = type == api_event && stack >= 0x80000080 &&
                                    guest_words(stack - 0x20, 0x20);
        if (initialize_api) {
            // Reserve outgoing argument slots followed by a local event.
            // The original function writes its arguments at SP+0,+4,+8.
            // API has no sound state or data: clear every otherwise stale
            // byte so sndRemoveEvents cannot mistake it for a sound event.
            ctx->r29 = S32(stack - 0x20);
            ctx->r5 = S32(stack - 0x10);
            MEM_W(0, ctx->r5) = 0;
            MEM_W(4, ctx->r5) = 0;
            MEM_W(8, ctx->r5) = 0;
            MEM_W(12, ctx->r5) = 0;
            MEM_H(0, ctx->r5) = api_event;
        }
        bc_original_sound_event_post(rdram, ctx);
        ctx->r29 = original_stack;
    }
    mask_context.r4 = previous_mask;
    osSetIntMask_recomp(rdram, &mask_context);
}
