#include "save_migration.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace bc {
namespace {
uint8_t controller_crc(const uint8_t* data){
 uint8_t crc=0;
 // The ROM's __osContDataCrc: 32 input bytes followed by eight zero bits.
 for(unsigned byte=0;byte<=32;byte++)for(int bit=7;bit>=0;bit--){
  const auto feedback=(crc&0x80)?0x85:0;
  crc=uint8_t((crc<<1)|(byte<32?((data[byte]>>bit)&1):0));crc^=feedback;
 }
 return crc;
}
}
bool import_eeprom_player(PakImage& image,const SaveStore::Image& source){
 if(image.free_bytes()!=123*256||source[0]==0||std::memcmp(source.data(),"NEW GAME",9)==0)return false;
 auto end=std::find(source.begin(),source.begin()+16,0);
 if(end==source.begin()+16||!std::all_of(source.begin(),end,[](uint8_t b){return b>=32&&b<=126;}))throw std::runtime_error("Legacy EEPROM player name is invalid; restore a valid save before importing");
 std::array<uint8_t,256> packet{};std::copy_n(source.begin(),256,packet.begin());std::fill(packet.end()-4,packet.end(),0);
 std::array<uint8_t,4> checksum{};
 for(unsigned block=0;block<8;block++)checksum[block%4]+=controller_crc(packet.data()+block*32);
 if(!std::equal(checksum.begin(),checksum.end(),source.begin()+252))throw std::runtime_error("Legacy EEPROM game checksum is invalid; restore a valid save before importing");
 constexpr std::array<uint8_t,8> marker={0x87,0x56,0x9A,0xB6,0xCD,0x07,0x6A,0xEC};
 if(!std::equal(marker.begin(),marker.end(),source.begin()+504))throw std::runtime_error("Legacy EEPROM completion marker is invalid; restore a valid save before importing");
 PakKey key;key.company=0x3031;key.game=0x4E424345;
 key.name={0x1B,0x25,0x1A,0x2C,0x2D,0x1C,0x28,0x2B,0x29,0x2C,0x0F,0x20,0x1A,0x26,0x1E,0};key.extension[0]=0x11;
 auto candidate=image;int index;
 if(candidate.allocate(key,3584,index)!=0||candidate.write(index,0,std::span(source.data(),256))!=0)throw std::runtime_error("Cannot import EEPROM player into Controller Pak");
 std::array<uint8_t,32> tail{};std::copy(marker.begin(),marker.end(),tail.begin());
 if(candidate.write(index,0xDE0,tail)!=0)throw std::runtime_error("Cannot import EEPROM completion marker");
 image=std::move(candidate);return true;
}
}
