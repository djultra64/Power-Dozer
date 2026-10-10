#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>
namespace bc {
inline constexpr size_t rom_size=8*1024*1024;
struct RomImage {std::vector<uint8_t> rom,code,frontend;};
std::string sha1(std::span<const uint8_t>);
std::vector<uint8_t> normalize_rom(std::vector<uint8_t>);
std::vector<uint8_t> read_rom_input(const std::filesystem::path&);
void validate_rom(std::span<const uint8_t>);
RomImage load_rom_image(const std::filesystem::path&);
RomImage load_cached_rom(const std::filesystem::path& data,const std::filesystem::path& source={});
}
