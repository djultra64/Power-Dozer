#include "save_store.h"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
namespace fs=std::filesystem;
void check(bool value,const char* what){if(!value)throw std::runtime_error(what);}
void write(const fs::path& path,const std::string& bytes){std::ofstream f(path,std::ios::binary);f.write(bytes.data(),bytes.size());}
bool rejects(bc::SaveStore& store){try{store.load();return false;}catch(const std::runtime_error&){return true;}}
int main(){
 char directory[]="/tmp/blastcorps-save-test-XXXXXX";
 auto* created=mkdtemp(directory);check(created,"temporary directory");
 fs::path root=created;
 try{
  std::array<uint8_t,512> zero{},a{},b{};a[17]=123;b[511]=45;
  bc::SaveStore fresh(root/"fresh/eeprom.bin");
  check(fresh.load()==zero,"new game starts empty");
  fresh.save(a);check(fresh.load()==a,"restart reloads complete progress");
  fresh.save(b);check(fresh.load()==b,"latest progress wins");
  fs::resize_file(root/"fresh/eeprom.bin",512);
  check(fresh.load()==a,"a checked image truncated to legacy length is not imported as raw");
  fresh.save(b);
  write(root/"fresh/eeprom.bin","truncated");
  check(fresh.load()==a,"damaged primary recovers previous progress");
  fresh.save(b);check(fresh.load()==b,"saving after recovery succeeds");
  write(root/"fresh/eeprom.bin","broken");write(root/"fresh/eeprom.bin.bak","broken too");
  check(rejects(fresh),"damaged primary and backup never reset progress");
  bool refused=false;try{fresh.save(zero);}catch(const std::runtime_error&){refused=true;}
  check(refused&&fs::file_size(root/"fresh/eeprom.bin")==6,"cannot overwrite unrecoverable progress");
  bc::SaveStore legacy(root/"legacy.bin");
  write(root/"legacy.bin",std::string(reinterpret_cast<const char*>(a.data()),a.size()));
  check(legacy.load()==a,"existing raw progress remains loadable");
  legacy.save(b);check(legacy.load()==b&&fs::file_size(root/"legacy.bin")==524,"legacy migrates on write");
  {std::ofstream f(root/"legacy.bin",std::ios::binary|std::ios::app);f.put('x');}
  check(legacy.load()==a,"extra bytes rejected, valid backup restored");
  fs::remove(root/"legacy.bin.bak");check(rejects(legacy),"extra bytes without backup are an error");
  bc::SaveStore one(root/"one/eeprom.bin"),two(root/"two/eeprom.bin");
  one.save(a);two.save(b);check(one.load()==a&&two.load()==b,"independent installations remain isolated");
  // Interrupted temporary writes cannot replace the last committed primary.
  write(root/"one/eeprom.bin.tmp.abandoned","partial");
  check(one.load()==a,"interrupted temporary write is ignored");
  auto image=root/"two/eeprom.bin";
  {std::fstream f(image,std::ios::binary|std::ios::in|std::ios::out);f.seekp(12+511);f.put(char(46));}
  check(rejects(two),"checksum rejects modified payload");
  fs::remove_all(root);
  std::cout<<"Save persistence, recovery, migration and isolation passed\n";
 }catch(...){fs::remove_all(root);throw;}
}
