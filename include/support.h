#pragma once
#include <cstdint>
#include "recomp.h"
#include "bindings.h"
extern "C" void bc_trace(uint32_t);

extern "C" void yield_self_1ms(uint8_t*);
