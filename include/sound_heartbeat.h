#pragma once
#include "recomp.h"

// The generated definition is renamed in a private build copy. Every original
// guest caller uses this public entry point on both ELF and COFF platforms.
extern "C" void func_802D6C8C(uint8_t* rdram, recomp_context* ctx);
extern "C" void bc_original_sound_event_post(uint8_t* rdram, recomp_context* ctx);
