#pragma once
#include "input_model.h"
void bc_input_init(const std::string& family);
void bc_input_set_family(const std::string& family);
void bc_input_update(SDL_Window*);
bool bc_input_event(const SDL_Event&);
void bc_poll_input();
bool bc_get_input(int,uint16_t*,float*,float*);
bc::InputPrompt bc_input_prompt(bc::InputAction);
