#include <vector>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <source_location>
bool bc_wide=false;
extern "C" void do_break(uint32_t){std::abort();}
static void check(bool value,const char* reason,std::source_location loc=std::source_location::current()){
 if(!value){std::fprintf(stderr,"FAIL line %u: %s\n",loc.line(),reason);std::exit(1);}
}
int main(){
 std::vector<uint32_t> memory(0x800000/4);auto* rdram=reinterpret_cast<uint8_t*>(memory.data());
 auto word=[&](uint32_t a)->int32_t&{return MEM_W(0,S32(a));};
 auto half=[&](uint32_t a)->int16_t&{return MEM_H(0,S32(a));};
 auto byte=[&](uint32_t a)->uint8_t&{return MEM_BU(0,S32(a));};
 auto real=[&](uint32_t a,float value){std::memcpy(&word(a),&value,4);};
 byte(0x8021592e)=0;half(0x802e8c94)=32;half(0x802e8c98)=0;word(0x80215458)=12;
 for(int i=0;i<128;++i)half(0x802155a0+i*2)=32;
 half(0x802154d2)=128;word(0x802154ec)=1;word(0x8021550c)=2;word(0x80215510)=3;word(0x80215514)=4;word(0x80215518)=5;
 byte(0x80364ae8)=0;byte(0x80364b81)=12;
 byte(0x80215915)=10;byte(0x80215916)=20;
 for(unsigned slot:{10,21,22,23,24}){byte(0x8036c1e0+slot)=1;byte(0x8036c220+slot)=slot==10?1:0;real(0x8036c260+4*slot,1);word(0x8036bfe0+4*slot)=0x80400000+slot*2048;}
 auto reset=[&](bool wide){bc_wide=wide;recomp_context ctx{};ctx.r29=S32(0x807ff000);func_801E8DCC(rdram,&ctx);word(0x802154dc)=0;};
 auto advance=[&](){recomp_context ctx{};ctx.r29=S32(0x807ff000);func_801E9528(rdram,&ctx);};
 auto draw=[&](float speed=0,int phase=0){recomp_context ctx{};ctx.r29=S32(0x807ff000);ctx.r4=S32(0x80500000);ctx.r5=S32(0x80480000);real(0x802154e4,speed);word(0x802154d8)=phase;half(0x8021593c)=255;func_801E9718(rdram,&ctx);std::vector<std::array<uint32_t,6>> rects;for(uint32_t p=0x80500000;p<uint32_t(ctx.r2);p+=8){const auto op=uint32_t(word(p))>>24;if(op==0xe4||op==0xd6){std::array<uint32_t,6> r{};for(int i=0;i<6;i++)r[i]=word(p+4*i);rects.push_back(r);p+=16;}}return rects;};
 reset(true);advance();
 const auto rects=draw();
 check(rects.size()==2,"new wide rank emits main and shadow at origin 364 instead of CPU-culling at 320");
 for(unsigned i=0;i<2;i++){
  const auto&r=rects[i];check(r[0]>>24==0xd6,"only ticker sprite headers carry signed marker");
  const int left=((r[1]>>12)&4095)-512,right=((r[0]>>12)&4095)-512;
  check(left==(i==0?361:364)*4&&right-left==124,"original 31 pixel rectangle span and shadow shift preserved");
  check(r[2]==0xb3000000&&r[4]==0xb2000000&&r[3]==0x400&&r[5]==0x04000400,"complete original rank UVs and derivatives survive bias");
 }

 // Every tested X runs the original producer and full sprite renderer.
 for(int x:{-86,-85,-54,-32,-31,0,319,320,373,374,376,377}){
  reset(true);advance();word(0x802154e8)=364-x;
  auto r=draw();check(r.size()==((x>=-85&&x<377)?2u:0u),"main/shadow union culling at both widescreen edges");
  for(size_t i=0;i<r.size();++i){
   check(int((r[i][1]>>12)&4095)-512==(x-(i==0?3:0))*4,"negative sprite X survives original clamps via bias");
   check(((r[i][0]>>12)&4095)-((r[i][1]>>12)&4095)==124,"no lost sprite columns at either wing");
   check(r[i][3]==0x400&&r[i][5]==0x04000400,"full flipped rank texture retained at either wing");
  }
 }
 reset(true);auto advancing=draw(12);check(advancing.size()==2&&word(0x802154e8)==12,"real FIFO shift insertion preserves same-frame original age update");
 check(bc::ticker::state.icons[0].index==35,"real advance executes exactly once before insertion");
 draw(0);check(bc::ticker::state.icons[0].index==35,"repeated producer call without a shift does not advance cohort");
 draw(3,10);check(bc::ticker::state.icons[0].index==34&&word(0x802154d8)==1,"partial phase wraps FIFO once with unchanged guest arithmetic");
 reset(true);for(int i=0;i<4;i++)check(draw().empty(),"sentinel ages and repeated frames cannot manufacture icons");
 advance();auto first=draw();check(draw()==first,"pause/repeated frame keeps same cohort and commands");
 // File focus rebuild changes future insertion indices/source only. Existing
 // FIFO/cohort stays alive until the exact next insertion store replaces it.
 word(0x802154ec)=99;word(0x8021550c)=100;word(0x80215510)=101;word(0x80215514)=102;word(0x80215518)=103;
 check(draw()==first,"source-only file focus change retains old FIFO icons");
 word(0x802154ec)=1;word(0x8021550c)=2;word(0x80215510)=3;word(0x80215514)=4;word(0x80215518)=5;
 reset(true);byte(0x80364b81)=11;for(int i=0;i<5;i++)advance();
 check(draw().size()==8,"absent fourth medal is not synthesized");
 byte(0x80364b81)=12;reset(true);for(int i=0;i<5;i++)advance();check(draw().size()==10,"rank and all four distinct logical medal cohorts insert");
 reset(false);advance();auto old43=draw();check(old43.size()==2&&old43[0][0]>>24==0xe4,"stable 4:3 retains ordinary E4 bytes");
 bc_wide=true;auto entering=draw();check(int((entering[1][1]>>12)&4095)-512==256*4,"retained narrow cohort shifts left with retained text");
 advance();check(draw().size()==4,"wide insertion adds a separate tail cohort");
 bc_wide=false;auto shrunk=draw();check(shrunk==old43,"shrink drops tail cohort and restores untouched original 4:3 commands");
 bc_wide=true;check(draw().size()==2,"truncated tail medal does not reappear when expanding");
 // A cohort inserted wide but advanced out of the trimmed suffix must remain
 // aligned with its surviving text after switching back to narrow.
 reset(true);advance();for(int i=0;i<9;i++)advance();word(0x802154e8)=120;
 bc_wide=false;auto survived=draw();bool found=false;
 for(const auto&r:survived)if((r[0]>>24)==0xd6&&int((r[1]>>12)&4095)-512==298*4)found=true;
 check(found,"surviving wide cohort keeps corrected origin after returning 4:3");
 // Unknown family/advance keeps original rendering; malformed FIFO cannot
 // mutate adjacent memory. A full NEW GAME reset removes all prior cohorts.
 reset(true);advance();word(0x80215458)=19;auto unknown=draw();check(!unknown.empty()&&unknown[0][0]>>24==0xe4,"unknown advance falls back to original sprite path");
 word(0x80215458)=12;reset(true);advance();byte(0x8021592e)=1;half(0x802e8c96)=32;half(0x802e8c9a)=0;unknown=draw();check(!unknown.empty()&&unknown[0][0]>>24==0xe4,"family1 is preserved without invented support");byte(0x8021592e)=0;
 reset(true);advance();for(int i=0;i<44;i++)half(0x802158a8+i*2)=65;const auto before=memory;bc_widescreen_ticker(rdram);check(memory==before,"missing terminator writes no FIFO or neighboring memory");
 reset(true);check(draw().empty(),"full reset/NEW GAME invalidates all old icons");
 // Bad spans are rejected atomically, including a malformed second texrect.
 for(int bad:{0,1,2,3}){
  auto memoryBefore=memory;const uint32_t at=0x80600000;
  for(int pass=0;pass<2;pass++)for(int j=0;j<6;j++)word(at+pass*24+j*4)=rects[pass][j];
  word(at)=(uint32_t(word(at))&0xffffff)|0xe4000000;word(at+24)=(uint32_t(word(at+24))&0xffffff)|0xe4000000;
  if(bad==0)word(at+32)=0xb4000000;
  if(bad==1)word(at+44)=0;
  auto unchanged=memory;
  const bool marked=bc::ticker::mark(rdram,at+(bad==2?1:0),at+(bad==3?40:48));
  check(!marked&&memory==unchanged,"malformed marking span fails atomically");memory=memoryBefore;
 }
 std::puts("actual generated ticker lifecycle, aspect transitions, fallback and full sprite commands passed");
}
