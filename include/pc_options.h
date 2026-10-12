#pragma once
#include "settings_menu.h"
#include "input_capture.h"
namespace bc {
// Presentation aspect is fixed at startup; persisted Aspect edits require restart.
void initialize_pc_options(const Settings&,bool presentation_wide=false);
void refresh_pc_settings(const Settings&);
std::optional<SettingsChange> take_pc_change();
void acknowledge_pc_change(const Settings&,bool);
void queue_pc_filter_change(const Settings&);
std::optional<Settings> take_pc_filter_change();
std::optional<RemapRequest> take_pc_remap_request();
bool take_pc_controls_cancel();
void acknowledge_pc_remap(const RemapResult&);
}
