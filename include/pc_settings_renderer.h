#pragma once
#include "pc_settings_assets.h"
#include "pc_settings_view.h"
#include "input_glyph.h"
#include "main_menu.h"
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
PcSettingsDrawData main_menu_draw_data(const MainMenuSnapshot&,const PcSettingsAssets&,int width,int height);
// Install before RT64 Application::setup; owns an asset copy through hook deinit.
void initialize_pc_settings_renderer(const PcSettingsAssets&);
// Both APIs replace one tagged screen atomically; nullptr in either hides it.
// The render hook retains one immutable screen value for the whole frame.
void publish_pc_settings_snapshot(std::shared_ptr<const PcSettingsSnapshot>);
void publish_main_menu_snapshot(std::shared_ptr<const MainMenuSnapshot>);
}
