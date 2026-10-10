#pragma once
#include "input_model.h"
#include "input_capture.h"
void bc_input_init(const std::string& family);
void bc_input_set_family(const std::string& family);
void bc_input_update(SDL_Window*);
bool bc_input_event(const SDL_Event&);
void bc_poll_input();
bool bc_get_input(int,uint16_t*,float*,float*);
bc::InputPrompt bc_input_prompt(bc::InputAction);
bc::InputModel bc_input_view();
void bc_input_begin_remap(bc::RemapRequest);
void bc_input_cancel_remap();
std::optional<bc::RemapResult> bc_input_take_remap_result();
