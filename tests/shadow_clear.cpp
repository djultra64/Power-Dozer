#include "shadow_clear.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
constexpr uint32_t address = 0x803650B0, offset = 0x3650B0;
using Command = std::array<uint32_t, 2>;
// Synthetic producer output: variable physical pointers and normalization
// are test values; fixed command words follow the verified F3D ABI.
constexpr std::array<Command, 26> original = {{
    {0x03800010, 0x002E8C60}, {0xB6000000, 0xFFFFFFFF},
    {0xBB000000, 0xFFFFFFFF}, {0xBC000006, 0},
    {0xBC001806, 0x00100000}, {0xBC001C06, 0x00200000},
    {0xE7000000, 0}, {0xED000000, 0x000FC0FC},
    {0xFA000000, 0xFFFFFFFF}, {0xBA000602, 0x000000C0},
    {0xBA001402, 0x00300000}, {0xFF48003F, 0x00340000},
    {0xF7000000, 0x00010001}, {0xF60FC0FC, 0},
    {0xE7000000, 0}, {0xBA001402, 0},
    {0xB900031D, 0x0F0A0000}, {0xFCFFFFFF, 0xFFFDF6FB},
    {0x01030040, 0x003651F0}, {0xB4000000, 0x00000082},
    {0x01020040, 0x003652F0}, {0x01000040, 0x00365270},
    {0x01000040, 0x003652B0}, {0x01000040, 0x00365230},
    {0x06000000, 0x00180000}, {0xB8000000, 0},
}};
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
Command command(const std::vector<uint8_t>& ram, size_t index) {
    Command result;
    std::memcpy(result.data(), ram.data() + offset + index * 8, 8);
    return result;
}
void put(std::vector<uint8_t>& ram, size_t index, Command value) {
    std::memcpy(ram.data() + offset + index * 8, value.data(), 8);
}
std::vector<uint8_t> fresh() {
    std::vector<uint8_t> ram(offset + 40 * 8 + 16, 0xA5);
    std::memcpy(ram.data() + offset, original.data(), sizeof(original));
    return ram;
}
void unchanged(std::vector<uint8_t> ram, size_t size, uint32_t at, uint32_t count) {
    auto before = ram;
    require(bc_shadow_clear_rewrite(ram.data(), size, at, count) == count,
            "Invalid list changed submission count");
    require(ram == before, "Invalid list was partially rewritten");
}
}

int main() {
    auto ram = fresh();
    auto before = ram;
    require(bc_shadow_clear_rewrite(ram.data(), ram.size(), address, 26) == 27,
            "Shadow clear must submit 27 commands including restored silhouette scissor");
    for (size_t i = 0; i < 26; ++i) {
        const auto expected = i == 7 ? Command{0xED000000, 0x00100100} : original[i];
        require(command(ram, i < 15 ? i : i + 1) == expected,
                "Original shadow commands changed beyond the clear scissor and insertion");
    }
    require(command(ram, 15) == Command{0xED000000, 0x000FC0FC},
            "Original silhouette scissor must follow the clear pipe sync");
    require(std::equal(ram.begin(), ram.begin() + offset, before.begin()) &&
            std::equal(ram.begin() + offset + 27 * 8, ram.end(), before.begin() + offset + 27 * 8),
            "Rewrite touched memory outside its 27 commands");

    // Decode the actual clear/fill commands: fill mode's inclusive lower right
    // intersects scissor's exclusive boundary, so every sampled texel is defined.
    const auto clip = command(ram, 7)[1];
    const auto fill = command(ram, 13)[0];
    const unsigned width = std::min(((clip >> 12) & 0xFFF) / 4, ((fill >> 12) & 0xFFF) / 4 + 1);
    const unsigned height = std::min((clip & 0xFFF) / 4, (fill & 0xFFF) / 4 + 1);
    require(width == 64 && height == 64, "Consumer's 64x64 sample domain has undefined border texels");
    unchanged(ram, ram.size(), address, 27); // Already rewritten: no duplicate insertion.
    unchanged(ram, ram.size(), address, 26); // Stale count cannot patch a different layout.
    for (unsigned repeat = 0; repeat < 4; ++repeat) {
        std::memcpy(ram.data() + offset, original.data(), sizeof(original));
        require(bc_shadow_clear_rewrite(ram.data(), ram.size(), address, 26) == 27,
                "Regenerating original commands must remain stable");
        require(command(ram, 26) == Command{0xB8000000, 0}, "Submission lost its end command");
    }

    // Mutations in every fixed word must reject atomically. Dynamic producer
    // words accept their entire original physical-pointer / u16 domain only.
    for (size_t i = 0; i < original.size(); ++i) {
        for (size_t w = 0; w < 2; ++w) {
            ram = fresh();
            auto changed = original[i];
            const bool physical = w == 1 && (i == 4 || i == 5 || i == 11 || i == 24);
            const bool normalization = w == 1 && i == 19;
            changed[w] ^= physical ? 0x80000000 : normalization ? 0x10000 : 1;
            put(ram, i, changed);
            unchanged(ram, ram.size(), address, 26);
        }
    }
    ram = fresh();
    for (size_t i : {4, 5, 11, 24}) put(ram, i, {original[i][0], 0x00123458});
    put(ram, 19, {0xB4000000, 0xFFFF});
    require(bc_shadow_clear_rewrite(ram.data(), ram.size(), address, 26) == 27,
            "Valid changing model/image pointers or perspective normalization were rejected");
    ram = fresh();
    for (size_t size : {size_t(0), size_t(offset - 1), size_t(offset + 26 * 8), size_t(offset + 40 * 8 - 1)})
        unchanged(ram, size, address, 26);
    for (uint32_t at : {uint32_t(0), offset, address + 1, address + 8, uint32_t(0xA03650B0), uint32_t(0xFFFFFFF8)})
        unchanged(ram, ram.size(), at, 26);
    for (uint32_t count : {uint32_t(0), uint32_t(25), uint32_t(27), uint32_t(40), uint32_t(0xFFFFFFFF)})
        unchanged(ram, ram.size(), address, count);
    require(bc_shadow_clear_rewrite(nullptr, 0x800000, address, 26) == 26, "Null memory must be rejected");
    // Exact declared allocation, no trailing writable padding required.
    ram = fresh(); ram.resize(offset + 40 * 8);
    require(bc_shadow_clear_rewrite(ram.data(), ram.size(), address, 26) == 27, "Exact allocation rejected");
}
