#pragma once
#include "input_model.h"
#include <filesystem>
namespace bc {
class ControlsStore {
 std::filesystem::path path_;
public:
 explicit ControlsStore(std::filesystem::path path):path_(std::move(path)){}
 InputModel load() const;
 void save(const InputModel&) const;
};
}
