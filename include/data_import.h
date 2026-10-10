#pragma once
#include <filesystem>
namespace bc {
// Copy a validated snapshot into a new/empty destination; never merge progress.
void import_data(const std::filesystem::path& source,const std::filesystem::path& destination);
}
