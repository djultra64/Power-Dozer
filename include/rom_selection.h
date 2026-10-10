#pragma once
#include "rom_input.h"
#include <functional>
#include <optional>
namespace bc {
struct RomSelection {
 std::function<RomImage(const std::filesystem::path&)> load;
 std::function<std::optional<std::filesystem::path>()> choose;
 std::function<bool(const std::string&)> retry;
};
std::optional<RomImage> select_rom(bool try_cache,const RomSelection&);
std::optional<RomImage> select_rom_gui(const std::filesystem::path& data);
}
