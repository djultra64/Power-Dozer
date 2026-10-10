#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
namespace bc {
// Sync a complete temporary image, atomically rename, then sync its directory.
void atomic_write_file(const std::filesystem::path&,std::span<const uint8_t>);
}
