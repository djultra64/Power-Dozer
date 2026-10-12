#include "frontend_menu.h"
#include "pc_options.h"
#include "pc_settings_input.h"
#include "recomp.h"
#include <vector>
#include <stdexcept>
#include <cstdio>
static unsigned shown=0;
extern "C" void func_8026AF6C(uint8_t*,recomp_context* ctx){if(ctx->r4!=0x800A)throw std::runtime_error("file window demand");++shown;}
static void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){
 std::vector<uint8_t> memory(8*1024*1024);auto* rdram=memory.data();recomp_context ctx{};
 auto a=[](uint32_t p){return S32(p);};bc::InputModel input;bc::initialize_pc_options({});
 auto state=[&](uint32_t low,uint32_t high=0){MEM_W(0,a(0x80364A90))=high;MEM_W(0,a(0x80364A94))=low;};
 auto event=[&](SDL_Scancode code){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.scancode=code;bc::frontend_menu_event(e,input);e.type=SDL_KEYUP;bc::frontend_menu_event(e,input);};
 auto gate=[&](bc::InputSnapshot held={}){return bc::frontend_menu_gate_input(input,held);};
 MEM_H(0,a(0x8036BB18))=10;state(0x8000);bc_frontend_menu_entry(rdram);state(0x10000);gate();
 check(bc::frontend_menu_owns_window()&&bc::frontend_menu_current(input)->selected==0,"first file entry presents Start Game");
 MEM_H(0,a(0x8036BB16))=2;bc_frontend_menu_selection(rdram,&ctx);
 check(MEM_HU(0,a(0x8036BB16))==0,"hidden file cannot activate");
 constexpr uint32_t frame=0x80500000,list=frame+0x48B0;
 ctx.r29=a(0x801FFE00);ctx.r2=a(list+80);MEM_W(0,ctx.r29)=a(list);
 bc_frontend_menu_draw(rdram,&ctx,frame);check(uint32_t(ctx.r2)==list,"only file window commands discarded");
 MEM_H(0,a(0x8036BB18))=11;ctx.r2=a(list+80);bc_frontend_menu_draw(rdram,&ctx,frame);
 check(uint32_t(ctx.r2)==list+80,"name window draw untouched");MEM_H(0,a(0x8036BB18))=10;
 check(!bc_frontend_menu_freeze_focus(rdram),"initial original file focus must seed the live ticker");
 check(bc_frontend_menu_freeze_focus(rdram),"subsequent hidden focus cannot change ticker player");
 event(SDL_SCANCODE_X);check(bc::frontend_menu_owns_window(),"main Back inert");
 input.replace(bc::InputAction::Confirm,bc::InputSource::Keyboard,{{bc::BindingKind::Key,SDL_SCANCODE_T}});
 event(SDL_SCANCODE_Z);check(bc::frontend_menu_current(input)!=nullptr,"old Confirm does not activate");
 bc::InputSnapshot held;held.keys[SDL_SCANCODE_T]=true;gate(held);event(SDL_SCANCODE_T);
 check(bc::frontend_menu_owns_window(),"already held confirm suppressed");gate();event(SDL_SCANCODE_T);
 check(!bc::frontend_menu_owns_window()&&!bc::frontend_menu_current(input),"mapped Confirm hands off original files");
 check(gate(held)&&input.sample(held).buttons==0,"held opening confirm cannot select a file");
 MEM_H(0,a(0x8036BB16))=2;bc_frontend_menu_selection(rdram,&ctx);
 check(shown==1&&MEM_HU(0,a(0x8036BB16))==0,"Start reopens original window and clears stale selection");
 gate();input.sample({});check(input.sample(held).buttons==0x8000,"file input resumes after release");
 MEM_H(0,a(0x8036BB16))=3;bc_frontend_menu_selection(rdram,&ctx);check(MEM_HU(0,a(0x8036BB16))==3,"ordinary file dispatch retained");
 for(auto s:{0x400000,0x80000,0x40000}){state(s);MEM_H(0,a(0x8036BB16))=0xFFFF;bc_frontend_menu_selection(rdram,&ctx);check(MEM_HU(0,a(0x8036BB16))==0xFFFF&&!bc::frontend_menu_owns_window(),"erase/name Back dispatch untouched");}
 state(0x200000);bc_frontend_menu_entry(rdram);check(!bc::frontend_menu_owns_window(),"erase/name return does not reopen main");
 state(0x10000);MEM_H(0,a(0x8036BB16))=0xFFFF;bc_frontend_menu_selection(rdram,&ctx);gate();
 check(bc::frontend_menu_current(input)->selected==0&&MEM_HU(0,a(0x8036BB16))==0,"file Back returns main without title transition");
 // Original Back finishes closing window 0xA to -1 after publication.
 MEM_H(0,a(0x8036BB18))=-1;
 check(bc_frontend_menu_hides_files(rdram),"closed hidden file focus cannot change ticker player");
 event(SDL_SCANCODE_T);gate();bc_frontend_menu_selection(rdram,&ctx);
 check(shown==2,"Start reopens files even after original Back animation closes window");
 MEM_H(0,a(0x8036BB18))=10;MEM_H(0,a(0x8036BB16))=0xFFFF;bc_frontend_menu_selection(rdram,&ctx);gate();
 event(SDL_SCANCODE_DOWN);event(SDL_SCANCODE_T);check(bc::pc_settings_open()&&!bc::frontend_menu_current(input),"options opens full host settings");
 // An unrelated owner must not drain the queued pause return.
 bc::pc_settings_begin(bc::PcSettingsOrigin::Pause);bc::pc_settings_close();gate();
 check(bc::take_pc_settings_exit()==bc::PcSettingsExit::Pause,"frontend cannot consume another owner's exit");
 bc::pc_settings_begin(bc::PcSettingsOrigin::MainMenu);
 SDL_Event escape{};escape.type=SDL_KEYDOWN;escape.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc::pc_settings_event(escape,input);gate();
 check(bc::frontend_menu_current(input)->selected==1&&bc::take_pc_settings_exit()==bc::PcSettingsExit::None,"options return consumed by frontend once");
 bc::frontend_menu_resize(1280,720);SDL_Event mouse{};mouse.type=SDL_MOUSEBUTTONDOWN;mouse.button.button=SDL_BUTTON_LEFT;mouse.button.x=640;mouse.button.y=465;bc::frontend_menu_event(mouse,input);
 check(bc::take_frontend_menu_action()==bc::FrontendMenuAction::CloseGame,"resized hit target follows rendered Close Game row");
 event(SDL_SCANCODE_T);check(bc::take_frontend_menu_action()==bc::FrontendMenuAction::None,"Close Game exactly once even before process exits");
 bc::frontend_menu_configure(true);bc::frontend_menu_resize(960,720);bc::frontend_menu_begin();gate();
 check(bc::frontend_menu_current(input)->presentation_wide,"main snapshot owns startup presentation aspect");
 mouse.button.x=480;mouse.button.y=450;bc::frontend_menu_event(mouse,input);
 check(bc::take_frontend_menu_action()==bc::FrontendMenuAction::CloseGame,"letterboxed main mouse target follows guest viewport");
 puts("Frontend guest ownership, settings origin, remaps and release transitions passed");
}
