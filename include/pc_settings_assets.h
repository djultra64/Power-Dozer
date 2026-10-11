#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>
namespace bc {
struct PcSettingsShadowVertex {int16_t x=0,y=0;uint8_t alpha=0;};
struct PcSettingsAssets {
 static constexpr unsigned font_width=256,font_height=224,glyph_size=32,glyph_count=51;
 // Top-down white, tintable RGBA; tiles flipped vertically to match original UVs.
 std::vector<uint8_t> font_rgba;
 std::array<float,glyph_count> glyph_advances{}; // In units of rendered glyph size.
 std::array<std::array<uint8_t,4>,23> palette{};
 std::array<PcSettingsShadowVertex,16> shadow_vertices{};
 std::array<std::array<uint8_t,3>,14> shadow_triangles{};
};
// Canonical big-endian ROM and decompressed code image based at 0x802447C0.
// Rejects missing/malformed assets with std::runtime_error; never uses guest cache.
PcSettingsAssets load_pc_settings_assets(std::span<const uint8_t> rom,std::span<const uint8_t> code);
}
