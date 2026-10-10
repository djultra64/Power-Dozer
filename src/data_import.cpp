#include "data_import.h"
#include "save_store.h"
#include "settings.h"
#include "controls_store.h"
#include "pak_store.h"
#include "pak_session.h"
#include "atomic_file.h"
#include <array>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
namespace bc {
void import_data(const std::filesystem::path& source,const std::filesystem::path& destination){
 namespace fs=std::filesystem;
 if(fs::is_symlink(source)||!fs::is_directory(source))throw std::runtime_error("Import source must be an existing data directory");
 if(fs::is_symlink(destination)||(fs::exists(destination)&&(!fs::is_directory(destination)||!fs::is_empty(destination))))throw std::runtime_error("Import destination must be new or empty; existing progress cannot be merged");
 const auto parent=destination.parent_path().empty()?fs::path("."):destination.parent_path();fs::create_directories(parent);
 std::string pattern=(parent/(destination.filename().string()+".importing-XXXXXX")).string();std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
 auto* created=mkdtemp(name.data());if(!created)throw std::runtime_error("Cannot create private import staging directory");fs::path stage=created;
 try{
  size_t copied=0;
  for(auto key:{"eeprom.bin","controller-pak.bin","settings.json","controls.json"}){
   auto input=source/key;
   if(std::string_view(key)=="controls.json"&&source.filename()==".local"&&!fs::exists(input)&&!fs::exists(input.string()+".bak"))input=source.parent_path()/"config/controls.json";
   for(auto suffix:{"",".bak"}){
    fs::path path=input.string()+suffix;
    if(fs::is_symlink(path))throw std::runtime_error("Symlinked import files are not supported");
    if(!fs::exists(path))continue;
    if(!fs::is_regular_file(path))throw std::runtime_error("Import input must be a regular file");
    auto size=fs::file_size(path);if(size>65536)throw std::runtime_error("Import file exceeds 64 KiB limit");
    std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Cannot read import file");std::vector<uint8_t> bytes(size);
    file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size));if(file.gcount()!=std::streamsize(size))throw std::runtime_error("Incomplete import file read");
    atomic_write_file(stage/(std::string(key)+suffix),bytes);++copied;
   }
  }
  if(!copied)throw std::runtime_error("Source contains no progress or preferences to import");
  // Validate the copied snapshot, including each store's recovery rules.
  // Preserve original bytes/metadata and backups instead of rewriting them.
  SaveStore(stage/"eeprom.bin").load();SettingsStore(stage/"settings.json").load();ControlsStore(stage/"controls.json").load();PakStore(stage/"controller-pak.bin",blast_pak_committed).load();
  // POSIX rename replaces an empty directory atomically; no partial snapshot
  // ever becomes the live destination. Nonempty competing destinations fail.
  fs::rename(stage,destination);
  int fd=open(parent.c_str(),O_RDONLY|O_DIRECTORY);if(fd<0)throw std::runtime_error("Cannot open import parent for synchronization");
  bool synced=fsync(fd)==0;int closed=close(fd);if(!synced||closed!=0)throw std::runtime_error("Cannot synchronize imported directory");
 }catch(...){std::error_code ignored;fs::remove_all(stage,ignored);throw;}
}
}
