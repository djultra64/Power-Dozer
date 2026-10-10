#include <array>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "recomp.h"
double bc_scale=3;
#include "map_vehicle_sprite.h"
void check(bool value){if(!value)std::abort();}
// Reference integer RDP mirror address followed by 4 KiB TMEM row wrap.
int row(int t,int bits){const int period=1<<bits;const int wrapped=t&((period*2)-1);return (wrapped<period?wrapped:(period*2-1-wrapped))&31;}
int main(){
 // Native 0.8125x strips have pixels 0..25 and derivative floor(1024/.8125).
 for(int pixel=0;pixel<26;pixel++){const int q=(pixel*1260)/1024;check(row(64+q,6)==row(32+q,5));check(row(64+q+1,6)==row(32+q+1,5));}
 // Every 1/32 texel position inside the strip has the same primary row.
 for(int q=0;q<32*32;q++)check(row(64+q/32,6)==row(32+q/32,5));
 // Fractional bilerp neighbors also match except the last texel's outside tap.
 for(int q=0;q<31*32;q++)check(row(64+q/32+1,6)==row(32+q/32+1,5));
 check(row(96,6)==31&&row(64,5)==0); // Higher resolution outer tap now reflects the border, avoiding opposite-edge TMEM wrap.
 std::vector<uint32_t> memory(0x800000/4,0xa5a5a5a5);auto* rdram=reinterpret_cast<uint8_t*>(memory.data());
 constexpr uint32_t begin=0x80010000;
 // Original 2.0D F3D gDPLoadTextureBlock render tile + size and texture rectangle.
 const std::array<uint32_t,10> strip={0xf5102000,0x00058200,0xf2000000,0x000fc07c,0xe4123456,0x006789ab,0xb3000000,0x12340800,0xb2000000,0x04ec04ec};
 for(unsigned i=0;i<2;i++)for(unsigned j=0;j<strip.size();j++)MEM_W(int32_t(i*40+j*4),S32(begin))=strip[j];
 const auto original=memory;
 check(!bc_map_vehicle_sprite_rewrite(rdram,begin,begin+80,false));check(memory==original);
 check(bc_map_vehicle_sprite_rewrite(rdram,begin,begin+80,true));
 for(unsigned i=0;i<2;i++)for(unsigned j=0;j<strip.size();j++)check(uint32_t(MEM_W(int32_t(i*40+j*4),S32(begin)))==(j==1?0x00054200:j==7?0x12340400:strip[j]));
 check(memory[0x10000/4-1]==0xa5a5a5a5&&memory[(0x10000+80)/4]==0xa5a5a5a5);
 memory=original;MEM_W(int32_t(44),S32(begin))=0x00054200;const auto changed=memory;
 check(!bc_map_vehicle_sprite_rewrite(rdram,begin,begin+80,true));check(memory==changed); // All-or-nothing layout validation.
 check(!bc_map_vehicle_sprite_rewrite(rdram,begin+1,begin+80,true));
 check(!bc_map_vehicle_sprite_rewrite(rdram,begin,begin+0x2000,true));
 memory=original;MEM_BU(int32_t(2),S32(0x8036c1e0))=2;MEM_BU(int32_t(2),S32(0x8036c220))=1;
 check(bc_map_vehicle_sprite_eligible(rdram,2));bc_scale=1;check(!bc_map_vehicle_sprite_eligible(rdram,2));bc_scale=3;MEM_BU(int32_t(2),S32(0x8036c220))=5;check(!bc_map_vehicle_sprite_eligible(rdram,2));
}
