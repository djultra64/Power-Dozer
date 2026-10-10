#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <source_location>
#include <cstdio>
// N64Recomp's guest memory convention: 32-bit words are host-endian;
// bytes/halfwords select XOR 3/XOR 2 lanes within each word.
#include "recomp.h"
bool bc_wide=false;
#include "widescreen_guest.h"
void check(bool value,std::source_location where=std::source_location::current()){if(!value){std::fprintf(stderr,"check failed at %u\n",where.line());std::abort();}}
int main(){
 std::vector<uint32_t> memory(0x800000/4,0xa5a5a5a5);auto* rdram=reinterpret_cast<uint8_t*>(memory.data());
 const auto original=memory;
 bc_widescreen_whiteout(rdram);
 for(int buffer=0;buffer<2;buffer++)for(int i=0;i<4;i++){
  const uint32_t at=0x802fa8b0+buffer*64+i*16;
  check(MEM_H(int32_t(0),S32(at))==(i==1||i==2?319:0));
 }
 const auto baseline43=memory;
 bc_widescreen_whiteout(rdram);check(memory==baseline43);
 bc_wide=true;bc_widescreen_whiteout(rdram);
 for(int buffer=0;buffer<2;buffer++)for(int i=0;i<4;i++){
  const uint32_t at=0x802fa8b0+buffer*64+i*16;
  check(MEM_H(int32_t(0),S32(at))==(i==1||i==2?373:-54));
  check(uint16_t(MEM_H(int32_t(2),S32(at)))==0xa5a5&&uint16_t(MEM_H(int32_t(12),S32(at)))==0xa5a5);
 }
 bc_wide=false;bc_widescreen_whiteout(rdram);check(memory==baseline43);
 // A fresh frame supplies original texture coordinates before the hook.
 MEM_BU(int32_t(0),S32(0x8036bfc5))=0;
 for(int i=0;i<4;i++){const uint32_t at=0x802fa820+i*16;MEM_H(int32_t(8),S32(at))=i%2?2552:0;}
 bc_wide=true;bc_widescreen_sky(rdram);
 for(int i=0;i<4;i++){const uint32_t at=0x802fa820+i*16;check(MEM_H(int32_t(0),S32(at))==(i%2?-214:1493));check(MEM_H(int32_t(8),S32(at))==(i%2?2978:-426));}
 // Inactive double buffer remains untouched, including its UVs.
 check(uint16_t(MEM_H(int32_t(0),S32(0x802fa860)))==0xa5a5);
 MEM_BU(int32_t(0),S32(0x8036bfc5))=1;
 for(int i=0;i<4;i++){const uint32_t at=0x802fa860+i*16;MEM_H(int32_t(8),S32(at))=i%2?4096:1544;}
 bc_widescreen_sky(rdram);check(MEM_H(int32_t(8),S32(0x802fa860))==1118);check(MEM_H(int32_t(8),S32(0x802fa870))==4522);
 bc_wide=false;
 for(int i=0;i<4;i++){const uint32_t at=0x802fa860+i*16;MEM_H(int32_t(8),S32(at))=i%2?4096:1544;}
 bc_widescreen_sky(rdram);check(MEM_H(int32_t(0),S32(0x802fa860))==1279&&MEM_H(int32_t(0),S32(0x802fa870))==0);check(MEM_H(int32_t(8),S32(0x802fa860))==1544);
 const auto sky43=memory;bc_widescreen_sky(rdram);check(memory==sky43);
 // FIFO switches aspect in-place, with ASCII/Japanese terminators and guards.
 MEM_BU(int32_t(0),S32(0x8021592e))=0;MEM_H(int32_t(0),S32(0x802e8c94))=32;MEM_H(int32_t(0),S32(0x802e8c98))=0;
 for(int i=0;i<27;i++)MEM_H(int32_t(i*2),S32(0x802158a8))=65+i;
 MEM_H(int32_t(27*2),S32(0x802158a8))=0;
 bc_wide=true;bc_widescreen_ticker(rdram);
 check(MEM_H(int32_t(0),S32(0x80208380))==-214&&MEM_H(int32_t(0),S32(0x80208390))==214);
 for(int i=0;i<27;i++)check(MEM_H(int32_t(i*2),S32(0x802158a8))==65+i);
 for(int i=27;i<36;i++)check(MEM_H(int32_t(i*2),S32(0x802158a8))==32);
 check(MEM_H(int32_t(72),S32(0x802158a8))==0&&uint16_t(MEM_H(int32_t(86),S32(0x802158a8)))==0xa5a5&&uint16_t(MEM_H(int32_t(0),S32(0x80215900)))==0xa5a5);
 bc_wide=false;bc_widescreen_ticker(rdram);check(MEM_H(int32_t(54),S32(0x802158a8))==0);check(MEM_H(int32_t(0),S32(0x80208380))==-160);
 const auto ticker43=memory;bc_widescreen_ticker(rdram);check(memory==ticker43);
 MEM_BU(int32_t(0),S32(0x8021592e))=1;MEM_H(int32_t(0),S32(0x802e8c96))=0x999;MEM_H(int32_t(0),S32(0x802e8c9a))=-1;
 for(int i=0;i<27;i++){MEM_H(int32_t(i*2),S32(0x802158a8))=0x100+i;}
 MEM_H(int32_t(54),S32(0x802158a8))=-1;
 bc_wide=true;bc_widescreen_ticker(rdram);check(MEM_H(int32_t(54),S32(0x802158a8))==0x999&&MEM_H(int32_t(72),S32(0x802158a8))==-1);
 recomp_context ctx{};ctx.r29=S32(0x80010000);MEM_W(int32_t(0x14),S32(ctx.r29))=11;
 bc_widescreen_ticker_text(rdram,&ctx);check(MEM_W(int32_t(0x14),S32(ctx.r29))==-43);
 bc_wide=false;MEM_W(int32_t(0x14),S32(ctx.r29))=11;bc_widescreen_ticker_text(rdram,&ctx);check(MEM_W(int32_t(0x14),S32(ctx.r29))==11);
 bc_widescreen_ticker(rdram);
 // Corrupt/missing terminators must not escape the allocated FIFO.
 for(int i=0;i<44;i++)MEM_H(int32_t(i*2),S32(0x802158a8))=123;
 const auto noTerminator=memory;bc_widescreen_ticker(rdram);check(memory==noTerminator);
 check(memory[0x100]==original[0x100]);
}
