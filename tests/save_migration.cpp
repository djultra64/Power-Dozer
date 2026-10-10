#include "save_migration.h"
#include <algorithm>
#include <stdexcept>
static void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
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
}
