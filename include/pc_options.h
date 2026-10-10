#pragma once
#include "settings_menu.h"
namespace bc {
void initialize_pc_options(const Settings&);
void refresh_pc_settings(const Settings&);
std::optional<SettingsChange> take_pc_change();
void acknowledge_pc_change(const Settings&,bool);
void queue_pc_filter_change(const Settings&);
std::optional<Settings> take_pc_filter_change();
}
