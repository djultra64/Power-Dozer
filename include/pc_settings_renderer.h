#pragma once
#include "pc_settings_assets.h"
#include "pc_settings_view.h"
#include "input_glyph.h"
#include <memory>
namespace bc {
struct PcSettingsVertex {float x,y,u,v,r,g,b,a;};
enum class PcSettingsTexture {White,Font,Controls};
enum class PcSettingsPrimitive {BoxShadow,TextShadow,Text,Glyph};
struct PcSettingsBatch {
 PcSettingsTexture texture;
 PcSettingsPrimitive primitive;
 uint32_t first,count;
 std::optional<GlyphSprite> glyph;
};
struct PcSettingsDrawData {
 std::vector<PcSettingsVertex> vertices;
 std::vector<PcSettingsBatch> batches;
 PcSettingsRect scissor;
};
PcSettingsDrawData pc_settings_draw_data(const PcSettingsSnapshot&,const PcSettingsAssets&,int width,int height);
// Install before RT64 Application::setup; owns an asset copy through hook deinit.
void initialize_pc_settings_renderer(const PcSettingsAssets&);
// nullptr hides the overlay. The render hook retains one immutable snapshot/frame.
void publish_pc_settings_snapshot(std::shared_ptr<const PcSettingsSnapshot>);
}
