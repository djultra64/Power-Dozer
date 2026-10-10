#pragma once
#include <filesystem>
#include <string>
namespace bc {
struct Settings {
 double scale=3;
 bool wide=true,fullscreen=false,mute=false;
 int refresh=120,msaa=0,volume=100;
 std::string presentation="linear",texture="three_point",family="auto";
};
void validate(const Settings&);
class SettingsStore {
 std::filesystem::path path_;
public:
 explicit SettingsStore(std::filesystem::path path):path_(std::move(path)){}
 Settings load() const;
 void save(const Settings&) const;
};
extern Settings settings;
}
