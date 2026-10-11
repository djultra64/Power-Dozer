#include "pc_options.h"
#include "pc_settings_input.h"
#include "support.h"
#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstdio>
static unsigned shown=0;
extern "C" void func_8026AF6C(uint8_t*,recomp_context* ctx){if(ctx->r4!=0x8000)throw std::runtime_error("wrong window demand");++shown;}
static void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(){
 std::vector<uint8_t> ram(9*1024*1024);auto* rdram=ram.data();recomp_context ctx{};
 auto a=[](uint32_t value){return S32(value);};
 bc::initialize_pc_options({});bc::pc_settings_resize(640,480);bc::InputModel model;
 MEM_W(0,a(0x80364A94))=4;MEM_H(0,a(0x8036BB18))=0;
 MEM_W(8,a(0x802F8BDC))=0xF1000039;MEM_H(0x10,a(0x802F8BDC))=5;MEM_H(0x18,a(0x802F8BDC))=1;
 MEM_W(0,a(0x8036BB24))=0x12345678;
 bc_pc_options_draw(rdram); // Existing More label becomes PC OPTIONS once.
 std::vector<uint8_t> original(28),entries(5*28);
 std::memcpy(original.data(),rdram+0x2F8BDC,28);std::memcpy(entries.data(),rdram+0x2F5804,entries.size());
 auto intact=[&](){check(std::memcmp(original.data(),rdram+0x2F8BDC,28)==0,"original window structure and flags unchanged");check(std::memcmp(entries.data(),rdram+0x2F5804,entries.size())==0,"original four-row table unchanged");check(MEM_W(0,a(0x8036BB24))==0x12345678,"original row pointer unchanged");};
 auto choose=[&](uint16_t row){ctx.r4=a(0x8036BB16);MEM_H(0,ctx.r4)=row;return bc_pc_options_selection(rdram,&ctx);};
 auto click=[&](int x,int y){SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_LEFT;event.button.x=x;event.button.y=y;bc::pc_settings_event(event,model);};
 constexpr uint32_t frame=0x80500000,list=frame+0x48B0;
 auto draw_end=[&](){ctx.r29=a(0x801FFE00);MEM_W(0,ctx.r29)=a(list);ctx.r2=a(list+80);bc_pc_options_glyphs(rdram,&ctx,frame);};
 check(!choose(0)&&!choose(3),"ordinary pause entries stay original");
 MEM_H(0,a(0x80370C28))=0x1000;check(!choose(1),"Start retains original pause exit");MEM_H(0,a(0x80370C28))=0;
 check(choose(1)&&bc::pc_settings_open(),"existing PC OPTIONS entry opens host overlay");intact();
 check(bc::pc_settings_current(model)->rows.size()==4,"host opens full Display section");
 bc_pc_options_draw(rdram);draw_end();check(uint32_t(ctx.r2)==list,"hide only original window display-list commands");intact();
 check(MEM_HU(0,a(0x8036BB16))==0,"stale guest selection suppressed while open");
 SDL_Event escape{};escape.type=SDL_KEYDOWN;escape.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc::pc_settings_event(escape,model);
 draw_end();check(MEM_HU(0,a(0x8036BB16))==0xFFFF,"close queues guest transition after draw");
 MEM_B(0,a(0x802E8BD4))=1;check(choose(0xFFFF),"close returns to pause window");check(MEM_BU(0,a(0x802E8BD4))==0,"return does not unpause");intact();
 check(shown==2,"open and close demand the original pause window");
 check(choose(1),"reopen PC settings");click(510,75);click(420,390);check(!bc::pc_settings_open(),"Game Options hides host UI");
 draw_end();check(MEM_HU(0,a(0x8036BB16))==1,"handoff injects original selection only");
 check(!choose(1),"Game Options reaches unmodified original dispatch");intact();
 check(choose(1),"PC settings opens again after handoff");MEM_W(0,a(0x80364A94))=1;bc_pc_options_draw(rdram);check(!bc::pc_settings_open(),"leaving pause closes overlay");check(bc::take_pc_controls_cancel(),"external close cancels capture");intact();check(!choose(1),"outside pause unchanged");
 MEM_W(0,a(0x80364A94))=256;check(choose(1),"alternate pause state opens same host view");
 // Invalid draw pointer must never become a replacement guest display list.
 ctx.r29=a(0x80900000);ctx.r2=a(list+80);bc_pc_options_glyphs(rdram,&ctx,frame);check(uint32_t(ctx.r2)==list+80,"out-of-bounds saved pointer is ignored");
 puts("PC options host overlay and original guest handoff passed");
}
