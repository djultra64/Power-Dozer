#pragma once
#include "input_model.h"
#include <cstdint>
#include <optional>
#include <string_view>
namespace bc {
struct GlyphSprite { uint16_t index; bool operator==(const GlyphSprite&) const=default; };
std::optional<GlyphSprite> input_glyph(const InputPrompt&,ControllerFamily);
std::string_view glyph_name(GlyphSprite);
struct GlyphDraw { GlyphSprite sprite; int x,y,size,alpha; };
using GlyphCommand=std::array<uint32_t,2>;
// One private, verified F3D command per atlas sprite. No guest texture/TMEM writes.
std::vector<GlyphCommand> glyph_commands(const std::vector<GlyphDraw>&);
std::optional<GlyphDraw> decode_glyph_command(GlyphCommand);
struct GlyphMesh { std::vector<float> positions,texcoords,colours; };
GlyphMesh glyph_mesh(const GlyphDraw&);
}
