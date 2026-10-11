#pragma once
#include "settings_menu.h"
#include "input_model.h"
#include <array>
#include <optional>
namespace bc {
struct PcSettingsRect {
 float x=0,y=0,width=0,height=0;
 bool contains(float px,float py) const{return width>0&&height>0&&px>=x&&py>=y&&px<x+width&&py<y+height;}
};
// Pixel bounds transformed from a centered, proportionally scaled 640x480 area.
struct PcSettingsLayout {
 PcSettingsRect safe_area,title,status,prompts,modal;
 std::array<PcSettingsRect,4> tabs;
 std::array<PcSettingsRect,4> rows;
 std::array<PcSettingsRect,14> actions;
 // Controls device selector, family selector, defaults, original Game Options.
 std::array<PcSettingsRect,4> controls;
 std::optional<size_t> hit_action(float x,float y) const;
};
PcSettingsLayout pc_settings_layout(int width,int height);
enum class PcSettingsModal {None,Capture,RestoreDefaults};
enum class PcSettingsFocusTarget {Row,Control,Tab};
enum class PcSettingsControl {Device,Family,RestoreDefaults,GameOptions};
struct PcSettingsViewState {
 size_t focus=0;
 PcSettingsFocusTarget focus_target=PcSettingsFocusTarget::Row;
 InputSource device=InputSource::Keyboard;
 PcSettingsModal modal=PcSettingsModal::None;
};
struct PcSettingsRow {
 std::string label,value;
 std::optional<SettingsField> field;
 std::optional<InputAction> action;
 bool restart_required=false;
 std::optional<InputPrompt> binding;
};
// Value-owned data: safe to publish as shared_ptr<const PcSettingsSnapshot>.
struct PcSettingsSnapshot {
 SettingsMenu::Page section=SettingsMenu::Page::Display;
 size_t focus=0;
 PcSettingsFocusTarget focus_target=PcSettingsFocusTarget::Row;
 std::string title,status;
 std::vector<PcSettingsRow> rows;
 std::array<InputPrompt,2> prompts;
 // Indexed by PcSettingsControl, matching layout.controls; present on Controls.
 std::vector<PcSettingsRow> controls;
 InputSource device=InputSource::Keyboard;
 ControllerFamily controller_family=ControllerFamily::Unknown;
 std::string device_label,family_label;
 PcSettingsModal modal=PcSettingsModal::None;
 bool saving=false;
};
PcSettingsSnapshot pc_settings_snapshot(const SettingsMenu&,const InputModel&,const PcSettingsViewState&);
}
