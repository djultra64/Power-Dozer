#pragma once
#include <cstdint>
#include "recomp.h"
#include "bindings.h"
extern "C" void bc_trace(uint32_t);

extern "C" void yield_self_1ms(uint8_t*);
extern "C" void bc_guest_window_trace(uint8_t*,recomp_context*,bool);
extern "C" void bc_original_fade_window(uint8_t*,recomp_context*,unsigned);
extern "C" void bc_guest_state(uint8_t*);
extern "C" void bc_guest_render_mode(uint8_t*,recomp_context*);
extern "C" void bc_guest_translucent_mode(uint8_t*,recomp_context*,unsigned);
extern "C" void bc_original_window_off(uint8_t*,unsigned);
extern "C" void bc_pc_options_draw(uint8_t*);
extern "C" bool bc_pc_options_selection(uint8_t*,recomp_context*);
extern "C" void bc_game_prompt_text(uint8_t*,recomp_context*);
extern "C" void bc_pc_options_glyphs(uint8_t*,recomp_context*,uint32_t);
