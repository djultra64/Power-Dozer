#pragma once
#include "input_model.h"
#include <cstdint>
namespace bc {
struct GlyphRectangle {int x,y,w,h;uint32_t rgba;bool operator==(const GlyphRectangle&) const=default;};
// Vector UI rectangles in a 16x16 cell. No ROM texture or guest allocation.
std::vector<GlyphRectangle> input_glyph(const InputPrompt&,ControllerFamily);
using GlyphCommand=std::array<uint32_t,2>;
// Screen-space rectangles; preserve the RDP combiner/primitive colour using
// RT64's paired state stacks. Geometry, textures, scissor and cycle stay intact.
std::vector<GlyphCommand> glyph_commands(const std::vector<GlyphRectangle>&);
}
