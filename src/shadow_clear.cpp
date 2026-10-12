#include "shadow_clear.h"
#include <array>
#include <cstring>

uint32_t bc_shadow_clear_rewrite(uint8_t* rdram, size_t rdram_size,
                                 uint32_t address, uint32_t count) {
    // The original shadow producer owns 40 Gfx entries at this address.
    // Restrict the repair to its complete, unmodified 26-command submission.
    constexpr size_t offset = 0x3650B0, capacity = 40 * 8;
    if (!rdram || address != 0x803650B0 || count != 26 ||
        rdram_size < offset + capacity) return count;

    using Command = std::array<uint32_t, 2>;
    constexpr std::array<Command, 26> expected = {{
        {0x03800010, 0x002E8C60}, {0xB6000000, 0xFFFFFFFF},
        {0xBB000000, 0xFFFFFFFF}, {0xBC000006, 0},
        {0xBC001806, 0}, {0xBC001C06, 0},
        {0xE7000000, 0}, {0xED000000, 0x000FC0FC},
        {0xFA000000, 0xFFFFFFFF}, {0xBA000602, 0x000000C0},
        {0xBA001402, 0x00300000}, {0xFF48003F, 0},
        {0xF7000000, 0x00010001}, {0xF60FC0FC, 0},
        {0xE7000000, 0}, {0xBA001402, 0},
        {0xB900031D, 0x0F0A0000}, {0xFCFFFFFF, 0xFFFDF6FB},
        {0x01030040, 0x003651F0}, {0xB4000000, 0},
        {0x01020040, 0x003652F0}, {0x01000040, 0x00365270},
        {0x01000040, 0x003652B0}, {0x01000040, 0x00365230},
        {0x06000000, 0}, {0xB8000000, 0},
    }};
    // Recompiled RDRAM stores 32-bit guest words in host word order.
    // memcpy avoids requiring pointer alignment or aliasing a byte buffer.
    auto* commands = rdram + offset;
    for (size_t i = 0; i < expected.size(); ++i) {
        Command actual;
        std::memcpy(actual.data(), commands + i * 8, 8);
        if (actual[0] != expected[i][0]) return count;
        // Four words are original VIRTUAL_TO_PHYSICAL arguments; the
        // perspective normalization is the producer's unsigned 16-bit value.
        const uint32_t mask = (i == 4 || i == 5 || i == 11 || i == 24) ? 0xE0000000u :
                              i == 19 ? 0xFFFF0000u : 0xFFFFFFFFu;
        if ((actual[1] & mask) != expected[i][1]) return count;
    }

    // Fill mode includes 0..63, while scissor excludes its lower/right edge.
    // Clear all 64 rows/columns consumed by the IA8 shadow, then restore the
    // original 63x63 model clip after the clear's pipe sync. This also makes
    // RT64 track a full 64-row source for framebuffer reinterpretation.
    std::memmove(commands + 16 * 8, commands + 15 * 8, 11 * 8);
    const Command silhouette_scissor = {0xED000000, 0x000FC0FC};
    std::memcpy(commands + 15 * 8, silhouette_scissor.data(), 8);
    const uint32_t clear_scissor = 0x00100100;
    std::memcpy(commands + 7 * 8 + 4, &clear_scissor, 4);
    return 27;
}
