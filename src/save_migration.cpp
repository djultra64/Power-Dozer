#include "save_migration.h"
#include "blast_save_format.h"
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
void packet_checksum(std::span<uint8_t> packet){
 std::fill(packet.end()-4,packet.end(),0);
 std::array<uint8_t,4> lanes{};
 for(size_t block=0;block<packet.size()/32;block++)lanes[block%4]+=controller_crc(packet.data()+block*32);
 std::copy(lanes.begin(),lanes.end(),packet.end()-4);
}
bool excluded(unsigned level){return level==38||level==47||level==49;}
}
bool import_eeprom_player(PakImage& image,const SaveStore::Image& source){
 if(image.free_bytes()!=123*256||source[0]==0||std::memcmp(source.data(),"NEW GAME",9)==0)return false;
 auto end=std::find(source.begin(),source.begin()+16,0);
 if(end==source.begin()+16||!std::all_of(source.begin(),end,[](uint8_t b){return b>=32&&b<=126;}))throw std::runtime_error("Legacy EEPROM player name is invalid; restore a valid save before importing");
 std::array<uint8_t,256> packet{};std::copy_n(source.begin(),256,packet.begin());std::fill(packet.end()-4,packet.end(),0);
 std::array<uint8_t,4> checksum{};
 for(unsigned block=0;block<8;block++)checksum[block%4]+=controller_crc(packet.data()+block*32);
 if(!std::equal(checksum.begin(),checksum.end(),source.begin()+252))throw std::runtime_error("Legacy EEPROM game checksum is invalid; restore a valid save before importing");
 if(!std::equal(blast_save_complete.begin(),blast_save_complete.end(),source.begin()+504))throw std::runtime_error("Legacy EEPROM completion marker is invalid; restore a valid save before importing");
 auto candidate=image;int index;
 if(candidate.allocate(blast_pak_key,3584,index)!=0||candidate.write(index,0,std::span(source.data(),256))!=0)throw std::runtime_error("Cannot import EEPROM player into Controller Pak");
 // EEPROM has one time per level and no building-state records. Reconstruct
 // the same Pak records as the ROM's EEPROM restore path (80201E80), including
 // its sentinel used by stats_perm to recreate a completed demolition mission.
 unsigned demolition_index=0;
 for(unsigned level=0;level<60;level++){
  if(excluded(level))continue;
  const bool demolition=blast_level_types[level]==1;
  const auto medal=source[0x18+level];
  if(medal>0&&medal<6){
   auto vehicle=source[0x92+level];
   if(vehicle>=blast_vehicle_time_slots.size()||blast_vehicle_time_slots[vehicle]>=14)throw std::runtime_error("Legacy EEPROM completed-level vehicle is invalid; restore a valid save before importing");
   const auto offset=0x100+4*level;auto time=uint16_t(source[offset]<<8|source[offset+1]),inverse=uint16_t(source[offset+2]<<8|source[offset+3]);
   if((time^0x55AA)!=inverse)throw std::runtime_error("Legacy EEPROM best-time checksum is invalid; restore a valid save before importing");
   std::array<uint8_t,32> times{};auto slot=blast_vehicle_time_slots[vehicle]*2;
   times[slot]=uint8_t(time>>8);times[slot+1]=uint8_t(time);packet_checksum(times);
   if(candidate.write(index,0x100+32*level,times)!=0)throw std::runtime_error("Cannot import EEPROM best time");
   if(demolition){
    std::array<uint8_t,64> status{};constexpr std::array<uint8_t,8> sentinel={0x12,0x34,0x56,0x78,0x87,0x65,0x43,0x21};
    std::copy(sentinel.begin(),sentinel.end(),status.begin());packet_checksum(status);
    if(candidate.write(index,0x880+64*demolition_index,status)!=0)throw std::runtime_error("Cannot import EEPROM demolition state");
   }
  }
  if(demolition)demolition_index++;
 }
 std::array<uint8_t,32> tail{};std::copy(blast_save_complete.begin(),blast_save_complete.end(),tail.begin());
 if(candidate.write(index,0xDE0,tail)!=0)throw std::runtime_error("Cannot import EEPROM completion marker");
 image=std::move(candidate);return true;
}
}
