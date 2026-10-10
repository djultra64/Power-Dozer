#include "data_import.h"
#include "save_store.h"
#include "settings.h"
#include "controls_store.h"
#include "pak_store.h"
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <cstdio>
namespace fs=std::filesystem;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>static void rejects(F f,const char* message){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,message);}
static void write(const fs::path& p,const std::string& value){fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f<<value;}
static std::string read(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 char name[]="/tmp/blastcorps-data-import-XXXXXX";auto* made=mkdtemp(name);check(made,"temporary directory");fs::path root=made;
 try{
  auto source=root/"old/.local",dest=root/"portable data";fs::create_directories(source);
  bc::SaveStore::Image progress{};progress[511]=87;bc::SaveStore(source/"eeprom.bin").save(progress);
  bc::Settings settings;settings.volume=37;bc::SettingsStore(source/"settings.json").save(settings);
  bc::ControlsStore(source/"controls.json").save(bc::InputModel{});
  bc::PakImage pak;bc::PakStore(source/"controller-pak.bin").save(pak);
  auto original=read(source/"eeprom.bin");
  write(source/"rom.z64","private ROM must not be copied by progress importer");write(source/"unrelated","ignore");
  bc::import_data(source,dest);
  check(bc::SaveStore(dest/"eeprom.bin").load()==progress,"import retains progress");
  check(bc::SettingsStore(dest/"settings.json").load().volume==37,"import retains settings");
  check(read(dest/"controls.json")==read(source/"controls.json"),"import preserves remap metadata exactly");
  check(read(dest/"controller-pak.bin")==read(source/"controller-pak.bin"),"import preserves Pak exactly");
  check(read(source/"eeprom.bin")==original,"source remains untouched");
  check(!fs::exists(dest/"rom.z64")&&!fs::exists(dest/"unrelated"),"import copies only progress/preferences");
  rejects([&]{bc::import_data(source,dest);},"existing nonempty destination never overwritten");check(read(dest/"eeprom.bin")==original,"refused merge preserves destination");
  write(source/"settings.json","broken");auto bad=root/"bad import";rejects([&]{bc::import_data(source,bad);},"invalid snapshot rejected");check(!fs::exists(bad),"invalid import never exposes partial destination");
  bc::SettingsStore(source/"settings.json.bak").save(settings);
  bc::import_data(source,root/"backup recovery");check(bc::SettingsStore(root/"backup recovery/settings.json").load().volume==37,"valid backup recovery preserved");
  fs::remove(source/"controls.json");write(root/"old/config/controls.json","{}");
  auto empty=root/"empty target";fs::create_directory(empty);bc::import_data(source,empty);check(fs::exists(empty/"controls.json"),"legacy remap imported into empty destination");
  rejects([&]{bc::import_data(root/"missing",root/"absent");},"missing source rejected");
  auto nofiles=root/"no saves";fs::create_directory(nofiles);rejects([&]{bc::import_data(nofiles,root/"none");},"empty source cannot look like a successful migration");
  auto interrupted=root/"new.importing-interrupted";write(interrupted/"eeprom.bin","partial");
  bc::import_data(source,root/"new");check(bc::SaveStore(root/"new/eeprom.bin").load()==progress,"retry ignores partial staging directory");check(read(interrupted/"eeprom.bin")=="partial","retry does not delete unrelated staging evidence");
  fs::create_symlink(source/"eeprom.bin",source/"controls.json");rejects([&]{bc::import_data(source,root/"symlink");},"symlinked input rejected");
  fs::remove_all(root);
 }catch(...){fs::remove_all(root);throw;}
 puts("Transactional portable data import and progress preservation passed");
}
