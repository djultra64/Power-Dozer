#pragma once
#include "pc_settings_view.h"
#include <memory>
namespace bc {
// Guest hooks request transitions; SDL owns events and publishes value snapshots.
enum class PcSettingsExit {None,Pause,GameOptions};
void pc_settings_begin();
void pc_settings_close();
bool pc_settings_open();
bool pc_settings_event(const SDL_Event&,const InputModel&);
void pc_settings_resize(int width,int height);
std::shared_ptr<const PcSettingsSnapshot> pc_settings_current(const InputModel&);
PcSettingsExit take_pc_settings_exit();
// Consume held inputs on both opening and closing, until physical release.
bool pc_settings_gate_input(InputModel&,const InputSnapshot&);
}
