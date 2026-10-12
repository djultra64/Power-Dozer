#pragma once
#include "main_menu.h"
#include <memory>
#include "recomp.h"
namespace bc {
// Called once before guest startup; changing saved Aspect does not alter it.
void frontend_menu_configure(bool presentation_wide);
void frontend_menu_begin();
bool frontend_menu_owns_window();
bool frontend_menu_event(const SDL_Event&,const InputModel&);
void frontend_menu_resize(int,int);
std::shared_ptr<const MainMenuSnapshot> frontend_menu_current(const InputModel&);
bool frontend_menu_gate_input(InputModel&,const InputSnapshot&);
FrontendMenuAction take_frontend_menu_action();
}
// Called exclusively on the guest thread. No renderer or SDL access.
extern "C" void bc_frontend_menu_entry(uint8_t*);
extern "C" void bc_frontend_menu_selection(uint8_t*,recomp_context*);
extern "C" bool bc_frontend_menu_hides_files(uint8_t*);
extern "C" bool bc_frontend_menu_freeze_focus(uint8_t*);
extern "C" void bc_frontend_menu_draw(uint8_t*,recomp_context*,uint32_t);
