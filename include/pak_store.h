#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>
namespace bc {
struct PakKey {
 uint16_t company=0;
 uint32_t game=0;
 std::array<uint8_t,16> name{};
 std::array<uint8_t,4> extension{};
 bool operator==(const PakKey&) const=default;
};
struct PakFile {PakKey key;bool occupied=false;std::vector<uint8_t> data;};
class PakImage {
 std::array<std::optional<PakFile>,16> files_;
 friend class PakStore;
public:
 int free_bytes() const;
 int find(const PakKey&,int& index) const;
 int allocate(const PakKey&,int bytes,int& index);
 int remove(const PakKey&);
 int read(int index,int offset,std::span<uint8_t>) const;
 int write(int index,int offset,std::span<const uint8_t>);
 const PakFile* file(int index) const;
};
class PakStore {
 std::filesystem::path path_;
 static std::vector<uint8_t> encode(const PakImage&);
 static PakImage decode(std::span<const uint8_t>);
 static bool read_valid(const std::filesystem::path&,PakImage&);
public:
 explicit PakStore(std::filesystem::path path):path_(std::move(path)){}
 PakImage load() const;
 void save(const PakImage&) const;
};
}
