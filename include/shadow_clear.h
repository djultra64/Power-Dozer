#pragma once
#include <cstddef>
#include <cstdint>

// Returns the submitted entry count, unchanged if any guard fails. The whole
// original layout and its allocation are checked before changing guest memory.
uint32_t bc_shadow_clear_rewrite(uint8_t* rdram, size_t rdram_size,
                                 uint32_t address, uint32_t count);
