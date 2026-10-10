#define SDL_MAIN_HANDLED
#include "atomic_file.h"
#include "data_import.h"
#include "save_store.h"
#include "settings.h"
#include "controls_store.h"
#include "pak_store.h"
#include <chrono>
#include <fstream>
#include <cstdio>
#include <stdexcept>
namespace fs=std::filesystem;
static void check(bool value,const char* what){if(!value)throw std::runtime_error(what);}
static std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 const auto root=fs::temp_directory_path()/("Power Dozer files "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directory(root);
 try{
  const auto image=root/fs::u8path(u8"carpeta con espacios/avance ñ.bin");
  const std::array<uint8_t,3> first{1,2,3},second{4,5,6};
  bc::atomic_write_file(image,first);bc::atomic_write_file(image,second);
  check(read(image)==std::string("\4\5\6",3),"complete replacement with spaces and Unicode");
  bc::SaveStore::Image progress{};progress[511]=87;
  const auto source=root/fs::u8path(u8"previous data 漢字");bc::SaveStore(source/"eeprom.bin").save(progress);
  bc::SaveStore(source/"eeprom.bin").save(progress);
  bc::Settings settings;settings.volume=37;bc::SettingsStore(source/"settings.json").save(settings);bc::SettingsStore(source/"settings.json").save(settings);
  bc::ControlsStore(source/"controls.json").save(bc::InputModel{});bc::ControlsStore(source/"controls.json").save(bc::InputModel{});
  bc::PakImage pak;bc::PakStore(source/"controller-pak.bin").save(pak);bc::PakStore(source/"controller-pak.bin").save(pak);
  const auto original=read(source/"eeprom.bin");const auto dest=root/fs::u8path(u8"new data 漢字");
  bc::import_data(source,dest);
  check(bc::SaveStore(dest/"eeprom.bin").load()==progress,"import retains EEPROM");
  check(bc::SettingsStore(dest/"settings.json").load().volume==37,"import retains settings");
  check(read(source/"eeprom.bin")==original,"source remains unchanged");
  check(read(dest/"controls.json")==read(source/"controls.json"),"remap bytes retained");
  check(read(dest/"controller-pak.bin")==read(source/"controller-pak.bin"),"Pak bytes retained");
  bool refused=false;try{bc::import_data(source,dest);}catch(const std::exception&){refused=true;}
  check(refused&&read(dest/"eeprom.bin")==original,"nonempty destination preserved");
  const auto empty=root/"empty data";fs::create_directory(empty);bc::import_data(source,empty);
  check(bc::SaveStore(empty/"eeprom.bin").load()==progress,"empty destination supported");
  for(const auto& item:fs::recursive_directory_iterator(root))check(item.path().filename().u8string().find(u8".tmp.")==std::u8string::npos,"no incomplete temporary files");
  fs::remove_all(root);
  std::puts("Windows Unicode atomic files, EEPROM/settings/Pak/remaps import passed");
 }catch(const std::exception& error){std::fprintf(stderr,"Windows file test: %s\n",error.what());fs::remove_all(root);return 1;}
}
