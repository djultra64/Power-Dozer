#pragma once
#include <array>
#include <cstdint>

// Owned by the guest producer thread. Rendering consumes only marked display
// list commands; it never reads this mutable FIFO/cohort state.
namespace bc::ticker {
struct Cohort { bool valid=false, trimmed=false; int origin=310, index=0; };
struct State { std::array<Cohort,5> icons{}; bool supported=false; int padding=0; unsigned length=0; };
inline State state;
inline bool layout(uint8_t* rdram,unsigned& length) {
 if(MEM_BU(0,S32(0x8021592E))!=0||MEM_W(0,S32(0x80215458))!=12)return false;
 const int16_t end=MEM_H(0,S32(0x802E8C98));
 for(length=0;length<44;++length)if(MEM_H(int32_t(length*2),S32(0x802158A8))==end)return length==27||length==36;
 return false;
}
inline void reset(uint8_t* rdram) {
 state={};state.supported=layout(rdram,state.length);state.padding=bc_wide?54:0;
}
inline void resize(uint8_t* rdram,unsigned length,unsigned target) {
 unsigned checked=0;
 if(!layout(rdram,checked)||checked!=length){state={};return;}
 if(!state.supported){state={};state.supported=true;state.padding=bc_wide?54:0;}
 const int padding=bc_wide?54:0;
 for(auto& icon:state.icons)if(icon.valid){
  if(target<length&&icon.index>=int(target)){icon.valid=false;icon.trimmed=true;}
  else icon.origin+=state.padding-padding;
 }
 state.padding=padding;state.length=target;
}
inline void advance(uint8_t* rdram) {
 unsigned length=0;if(!layout(rdram,length)){state={};return;}
 for(auto& icon:state.icons)if(icon.valid&&icon.index>-128)--icon.index;
}
inline void insert(uint8_t* rdram,unsigned slot) {
 unsigned length=0;if(slot>=5||!state.supported||!layout(rdram,length))return;
 state.icons[slot]={true,false,310+(int(length)-27)*12-state.padding,int(length)-1};
}
inline bool corrected(unsigned slot) {
 return slot<5&&state.supported&&(bc_wide||state.icons[slot].trimmed||(state.icons[slot].valid&&state.icons[slot].origin!=310));
}
inline int origin(unsigned slot) {return corrected(slot)&&state.icons[slot].valid?state.icons[slot].origin:310;}
inline bool reject_left(unsigned slot,int x) {
 return corrected(slot)?(!state.icons[slot].valid||x < -31-state.padding):x < -31;
}
inline bool below_right(unsigned slot,int x) {return x < (corrected(slot)?323+state.padding:320);}

// Validate the full two-pass one-strip span before touching any word. Payloads
// remain byte-for-byte original (UVs, size, filtering, alpha and shadow).
inline bool mark(uint8_t* rdram,uint32_t begin,uint32_t end) {
 const uint32_t first=begin&0x1FFFFFFF,last=end&0x1FFFFFFF;
 if((first&7)||(last&7)||last<=first||last>0x800000||last-first>0x400)return false;
 auto word=[&](uint32_t at)->int32_t&{return MEM_W(0,S32(0x80000000u|at));};
 std::array<uint32_t,2> rects{};unsigned count=0;
 for(uint32_t at=first;at<last;at+=8){
  const uint32_t w0=word(at),w1=word(at+4);
  if((w0>>24)==0xD6)return false;
  if((w0>>24)!=0xE4)continue;
  if(count==2||at+24>last||(w1>>24)!=0||uint32_t(word(at+8))!=0xB3000000||uint32_t(word(at+16))!=0xB2000000||uint32_t(word(at+20))!=0x04000400)return false;
  const int left=(w1>>12)&4095,right=(w0>>12)&4095;
  if(right-left!=124||int(w0&4095)-int(w1&4095)!=128||(uint32_t(word(at+12))>>16)!=0)return false;
  rects[count++]=at;at+=16;
 }
 if(count!=2)return false;
 for(auto at:rects)word(at)=(uint32_t(word(at))&0x00FFFFFFu)|0xD6000000u;
 return true;
}
inline bool prepare(uint8_t* rdram,recomp_context* ctx,unsigned cohort) {
 const auto slot=uint32_t(ctx->r5);
 if(!corrected(cohort)||slot>=64||MEM_BU(int32_t(slot),S32(0x8036C1E0))!=1||MEM_BU(int32_t(slot),S32(0x8036C220))>1||uint32_t(MEM_W(int32_t(slot*4),S32(0x8036C260)))!=0x3F800000||SIGNED(ctx->r7)!=194||MEM_W(0X14,ctx->r29)!=1||uint32_t(MEM_W(0X18,ctx->r29))!=0x3F800000)return false;
 ctx->r6=ADD32(ctx->r6,128);return true;
}
}
