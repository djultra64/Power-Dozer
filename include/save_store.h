#pragma once
#include <array>
#include <cstdint>
#include <filesystem>

namespace bc {
// Loads legacy 512-byte saves and checked images; errors never reset progress.
class SaveStore {
 std::filesystem::path path_;
public:
 using Image=std::array<uint8_t,512>;
 explicit SaveStore(std::filesystem::path path):path_(std::move(path)){}
 Image load() const;
 void save(const Image& image) const;
};
}
