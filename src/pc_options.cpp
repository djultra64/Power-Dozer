#include "pc_options.h"
#include "pc_settings_input.h"
#include "support.h"
#include "funcs.h"
namespace {
// Only guest-thread hooks touch these transition variables or RDRAM.
bool guest_active=false;
bc::PcSettingsExit returning=bc::PcSettingsExit::None;
constexpr uint32_t entries=0x802F5804,more=0x80800000,selection=0x8036BB16;
gpr address(uint32_t value){return S32(value);}
bool in_pause(uint8_t* rdram){
 const auto state=uint32_t(MEM_W(0,address(0x80364A94)));
 return MEM_W(0,address(0x80364A90))==0&&(state==4||state==256);
}
void show(uint8_t* rdram,recomp_context* ctx){ctx->r4=0x8000;func_8026AF6C(rdram,ctx);}
}
extern "C" void bc_pc_options_draw(uint8_t* rdram){
 constexpr char label[]="PC OPTIONS";
 for(size_t i=0;i<sizeof(label);i++)MEM_B(i,address(more))=label[i];
 MEM_W(0xC,address(entries+28))=S32(more);
 if(guest_active&&!in_pause(rdram)){
  bc::pc_settings_close();bc::take_pc_settings_exit();guest_active=false;returning=bc::PcSettingsExit::None;
 }
}
extern "C" void bc_pc_options_glyphs(uint8_t* rdram,recomp_context* ctx,uint32_t frame){
 if(!guest_active)return;
 // func_8026BCE0 saves its incoming display-list pointer at old SP+0x150,
 // then restores SP before this hook. Discard that window's commands only;
 // its animation/selection bookkeeping and the game's backdrop are retained.
 const auto sp=uint32_t(ctx->r29);
 if(sp>=0x80000000&&sp<=0x807FFFFC&&(sp&3)==0&&frame>=0x80000000&&frame<=0x807DEB68){
  const auto begin=uint32_t(MEM_W(0,address(sp))),end=uint32_t(ctx->r2);
  if(begin>=frame+0x48B0&&begin<=end&&end<=frame+0xA3A0&&(begin&7)==0)ctx->r2=address(begin);
 }
 if(auto request=bc::take_pc_settings_exit();request!=bc::PcSettingsExit::None)returning=request;
 // The original dispatcher is called only for a nonzero selection after this
 // draw. Queue an explicit handoff here, never a synthetic gameplay button.
 if(returning!=bc::PcSettingsExit::None)MEM_H(0,address(selection))=returning==bc::PcSettingsExit::GameOptions?1:0xFFFF;
 else MEM_H(0,address(selection))=0;
}
extern "C" bool bc_pc_options_selection(uint8_t* rdram,recomp_context* ctx){
 if(!in_pause(rdram))return false;
 const auto selection_address=ctx->r4;
 if(returning!=bc::PcSettingsExit::None){
  const bool game_options=returning==bc::PcSettingsExit::GameOptions;
  returning=bc::PcSettingsExit::None;guest_active=false;
  if(game_options){MEM_H(0,selection_address)=1;return false;}
  MEM_B(0,address(0x802E8BD4))=0;MEM_H(0,selection_address)=0;show(rdram,ctx);return true;
 }
 if(guest_active){MEM_H(0,selection_address)=0;MEM_B(0,address(0x802E8BD4))=0;return true;}
 // Start retains its original pause-exit behavior. Only the existing entry's
 // confirm selection opens the host screen; no custom four-row table exists.
 if(MEM_HU(0,address(0x80370C28))&0x1000)return false;
 if(MEM_HU(0,selection_address)!=1||MEM_H(0,address(0x8036BB18))!=0)return false;
 bc::pc_settings_begin();guest_active=true;MEM_H(0,selection_address)=0;show(rdram,ctx);return true;
}
