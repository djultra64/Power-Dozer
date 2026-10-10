#include "save_migration.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
static void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
static std::vector<uint8_t> hex(const std::string& value){std::vector<uint8_t> bytes;for(size_t i=0;i<value.size();i+=2)bytes.push_back(std::stoul(value.substr(i,2),nullptr,16));return bytes;}
int main(){
 bc::SaveStore::Image source{};const char name[]="OLD TEST";std::copy_n(name,sizeof(name),source.begin());source[100]=0x55;
 // Independently generated bit-stream CRC fixture, not the implementation.
 source[252]=0x81;source[253]=0;source[254]=0;source[255]=0x60;
 const std::array<uint8_t,8> marker={0x87,0x56,0x9A,0xB6,0xCD,0x07,0x6A,0xEC};std::copy(marker.begin(),marker.end(),source.begin()+504);
 bc::PakImage pak;check(bc::import_eeprom_player(pak,source),"valid legacy player imported");
 const auto* f=pak.file(0);check(f&&f->occupied&&f->data.size()==3584,"first Pak file created");
 check(std::equal(source.begin(),source.begin()+256,f->data.begin()),"all basic progress preserved byte-for-byte");
 check(std::all_of(f->data.begin()+256,f->data.begin()+0xDE0,[](auto b){return b==0;}),"extra Pak level data starts empty");
 check(std::equal(marker.begin(),marker.end(),f->data.begin()+0xDE0),"original completion marker retained at Pak location");
 check(f->key.company==0x3031&&f->key.game==0x4E424345&&f->key.extension[0]==0x11,"original game identity");
 auto first=f->data;source[100]^=1;check(!bc::import_eeprom_player(pak,source),"existing Pak is never overwritten");check(pak.file(0)->data==first,"existing progress retained");
 bc::PakImage corrupt;bool refused=false;try{bc::import_eeprom_player(corrupt,source);}catch(const std::exception&){refused=true;}check(refused&&corrupt.free_bytes()==123*256,"invalid game checksum refuses import without mutation");
 bc::PakImage empty;bc::SaveStore::Image blank{};check(!bc::import_eeprom_player(empty,blank),"missing EEPROM has no player");
 std::copy_n("NEW GAME",9,blank.begin());check(!bc::import_eeprom_player(empty,blank),"unplayed template is skipped");
 std::ifstream fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/legacy-completed.hex");std::string encoded;fixture>>encoded;
 auto bytes=hex(encoded);check(bytes.size()==512,"completed legacy fixture read");std::copy(bytes.begin(),bytes.end(),source.begin());
 bc::PakImage completed;check(bc::import_eeprom_player(completed,source),"completed legacy player imported");f=completed.file(0);
 check(std::equal(source.begin(),source.begin()+256,f->data.begin()),"completed medals and unlocks remain intact");
 for(auto [level,expected]:{std::pair{0,"04d200000000000000000000000000000000000000000000000000008c000000"},std::pair{7,"000000000000000000000000000000000d80000000000000000000001b000000"},std::pair{57,"00000000000000000000000000000000000000000000000011d700001d000000"}}){
  auto time=hex(expected);check(std::equal(time.begin(),time.end(),f->data.begin()+0x100+32*level),"best time uses original vehicle slot and packet checksum");
 }
 auto demolition=hex("1234567887654321000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000005f000000");
 check(std::equal(demolition.begin(),demolition.end(),f->data.begin()+0x880),"completed first mission has original demolition reconstruction sentinel");
 check(std::equal(demolition.begin(),demolition.end(),f->data.begin()+0xD40),"later main mission uses compact demolition index excluding special levels");
 check(std::all_of(f->data.begin()+0x100+38*32,f->data.begin()+0x100+39*32,[](auto b){return b==0;}),"original excluded level does not import a time packet");
 source[0x103]^=1;bc::PakImage broken_time;refused=false;try{bc::import_eeprom_player(broken_time,source);}catch(const std::exception&){refused=true;}
 check(refused&&broken_time.free_bytes()==123*256,"bad EEPROM time pair refuses import without partially migrating");
 source[0x103]^=1;source[504]^=1;bc::PakImage unfinished;refused=false;try{bc::import_eeprom_player(unfinished,source);}catch(const std::exception&){refused=true;}
 check(refused&&unfinished.free_bytes()==123*256,"unfinished EEPROM save is preserved and refused");
}
