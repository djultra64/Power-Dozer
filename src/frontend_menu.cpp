#include "frontend_menu.h"
#include "pc_settings_input.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
extern "C" void func_8026AF6C(uint8_t*,recomp_context*);
namespace {
// Ordering: input -> frontend -> settings. Guest hooks take only frontend;
// rendering takes none of these locks and receives value-owned snapshots.
std::mutex mutex;
// Guest-thread-only one-shot: seed the original player summary before freezing focus.
bool initialize_focus=false;
bc::MainMenuState menu;
bool active=false,release_latch=false,start_request=false,quit_requested=false;
bc::FrontendMenuAction action=bc::FrontendMenuAction::None;
bc::InputSnapshot blocked;
std::array<int,SDL_CONTROLLER_AXIS_MAX> axes{};
int width=640,height=480;
bool presentation_wide=false;
bool owns(){return active&&menu.screen()!=bc::FrontendMenuScreen::Files;}
void trace(const char* screen){if(std::getenv("BC_TRACE_FRONTEND_MENU"))fprintf(stderr,"{\"event\":\"frontend\",\"screen\":\"%s\"}\n",screen);}
void begin(){trace("main");active=true;menu.enter();action=bc::FrontendMenuAction::None;start_request=false;quit_requested=false;release_latch=true;axes.fill(0);}
void settings_return(){
 if(active&&menu.screen()==bc::FrontendMenuScreen::Options&&bc::take_pc_settings_exit(bc::PcSettingsOrigin::MainMenu)==bc::PcSettingsExit::MainMenu){menu.return_main();trace("main-options-return");release_latch=true;axes.fill(0);}
}
bool bound(const bc::InputModel& model,bc::InputAction action,bc::InputBinding binding){
 const auto& bindings=model.bindings(action);return std::find(bindings.begin(),bindings.end(),binding)!=bindings.end();
}
void confirm(){
 if(quit_requested)return;
 menu.confirm();switch(menu.take_action()){
 case bc::FrontendMenuAction::StartGame:menu.open_files();trace("files");start_request=true;release_latch=true;break;
 case bc::FrontendMenuAction::PCOptions:menu.open_options();trace("options");release_latch=true;bc::pc_settings_begin(bc::PcSettingsOrigin::MainMenu);break;
 case bc::FrontendMenuAction::CloseGame:quit_requested=true;trace("close");action=bc::FrontendMenuAction::CloseGame;break;
 default:break;
 }
}
bool hit(int x,int y){
 const auto area=bc::frontend_safe_area(width,height,presentation_wide);const float scale=area.width/640;if(scale<=0)return false;
 const float px=(x-area.x)/scale,py=(y-area.y)/scale;
 for(size_t i=0;i<3;++i)if(px>=bc::main_menu_row_x&&px<bc::main_menu_row_x+bc::main_menu_row_width&&py>=bc::main_menu_row_y[i]&&py<bc::main_menu_row_y[i]+bc::main_menu_row_height){menu.select(i);return true;}
 return false;
}
gpr address(uint32_t value){return S32(value);}
bool files(uint8_t* rdram){return MEM_W(0,address(0x80364A90))==0&&uint32_t(MEM_W(0,address(0x80364A94)))==0x10000;}
bool file_window(uint8_t* rdram){return MEM_H(0,address(0x8036BB18))==0xA;}
}
namespace bc {
void frontend_menu_configure(bool wide){std::lock_guard lock(mutex);presentation_wide=wide;}
void frontend_menu_begin(){std::lock_guard lock(mutex);begin();}
bool frontend_menu_owns_window(){std::lock_guard lock(mutex);return owns();}
void frontend_menu_resize(int w,int h){std::lock_guard lock(mutex);width=w;height=h;}
std::shared_ptr<const MainMenuSnapshot> frontend_menu_current(const InputModel& input){
 std::lock_guard lock(mutex);settings_return();if(!active||menu.screen()!=FrontendMenuScreen::Main)return {};
 auto snapshot=main_menu_snapshot(menu,input);snapshot.presentation_wide=presentation_wide;
 return std::make_shared<const MainMenuSnapshot>(std::move(snapshot));
}
FrontendMenuAction take_frontend_menu_action(){std::lock_guard lock(mutex);auto out=action;action=FrontendMenuAction::None;return out;}
bool frontend_menu_gate_input(InputModel& input,const InputSnapshot& held){
 std::lock_guard lock(mutex);settings_return();const bool transition=release_latch,gate=owns()||transition;release_latch=false;
 for(size_t i=1;i<held.keys.size();++i){blocked.keys[i]=held.keys[i]&&(gate||blocked.keys[i]);if(gate&&held.keys[i])input.consume({BindingKind::Key,int(i)});}
 for(size_t i=0;i<held.buttons.size();++i){blocked.buttons[i]=held.buttons[i]&&(gate||blocked.buttons[i]);if(gate&&held.buttons[i])input.consume({BindingKind::Button,int(i)});}
 // Owned frames suppress guest input continuously. Host axes are latched only
 // at transitions; otherwise a fresh slow ramp would be blocked before the
 // event adapter's activation threshold. Activated Main axes latch in event().
 for(size_t i=0;i<held.axes.size();++i){
  if(std::abs(held.axes[i])<=.2f)blocked.axes[i]=0;
  else{if(transition)blocked.axes[i]=held.axes[i];if(gate)input.consume({BindingKind::Axis,int(i)});}
 }
 return gate;
}
bool frontend_menu_event(const SDL_Event& event,const InputModel& input){
 std::lock_guard lock(mutex);settings_return();
 if(event.type==SDL_KEYUP&&unsigned(event.key.keysym.scancode)<blocked.keys.size())blocked.keys[event.key.keysym.scancode]=false;
 if(event.type==SDL_CONTROLLERBUTTONUP&&event.cbutton.button<blocked.buttons.size())blocked.buttons[event.cbutton.button]=false;
 if(event.type==SDL_CONTROLLERDEVICEREMOVED){blocked.buttons.fill(false);blocked.axes.fill(0);axes.fill(0);}
 if(event.type==SDL_WINDOWEVENT){
  if(event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){width=event.window.data1;height=event.window.data2;}
  if(event.window.event==SDL_WINDOWEVENT_FOCUS_LOST){axes.fill(0);release_latch=owns();}
 }
 if(!owns())return false;
 std::optional<InputBinding> binding;
 bool held=false;
 if(event.type==SDL_KEYDOWN&&unsigned(event.key.keysym.scancode)<blocked.keys.size()){
  binding=InputBinding{BindingKind::Key,event.key.keysym.scancode};held=blocked.keys[binding->code]||event.key.repeat;
 }else if(event.type==SDL_CONTROLLERBUTTONDOWN&&event.cbutton.button<blocked.buttons.size()){
  binding=InputBinding{BindingKind::Button,event.cbutton.button};held=blocked.buttons[binding->code];
 }else if(event.type==SDL_CONTROLLERAXISMOTION&&event.caxis.axis<axes.size()){
  const int direction=event.caxis.value>21300?1:event.caxis.value<-21300?-1:0;
  auto& last=axes[event.caxis.axis];held=direction==last||blocked.axes[event.caxis.axis]!=0;last=direction;
  if(std::abs(int(event.caxis.value))<=6500)blocked.axes[event.caxis.axis]=0;
  if(direction)binding=InputBinding{BindingKind::Axis,event.caxis.axis,direction};
 }
 // The opening control is consumed before settings can interpret it. Other
 // settings events (including raw remap capture) remain settings-owned.
 if(binding&&held)return true;
 if(menu.screen()==FrontendMenuScreen::Options)return false;
 if(release_latch&&binding)return true;
 if(event.type==SDL_MOUSEMOTION){hit(event.motion.x,event.motion.y);return true;}
 if(event.type==SDL_MOUSEBUTTONDOWN){if(event.button.button==SDL_BUTTON_LEFT&&hit(event.button.x,event.button.y))confirm();return true;}
 if(binding){
  if(bound(input,InputAction::Back,*binding))return true;
  if(bound(input,InputAction::Confirm,*binding))confirm();
  else if(bound(input,InputAction::MoveUp,*binding))menu.move(-1);
  else if(bound(input,InputAction::MoveDown,*binding))menu.move(1);
  if(binding->kind==BindingKind::Key)blocked.keys[binding->code]=true;
  if(binding->kind==BindingKind::Button)blocked.buttons[binding->code]=true;
  if(binding->kind==BindingKind::Axis)blocked.axes[binding->code]=binding->direction;
 }
 return true;
}
}
extern "C" void bc_frontend_menu_entry(uint8_t* rdram){
 // Original setup has just run, while currentGameState still identifies the
 // preceding screen. 0x200000 is the original erase/name return transition.
 if(MEM_W(0,address(0x80364A90))==0&&uint32_t(MEM_W(0,address(0x80364A94)))==0x200000)return;
 bc::frontend_menu_begin();initialize_focus=true;
}
extern "C" void bc_frontend_menu_selection(uint8_t* rdram,recomp_context* ctx){
 if(!files(rdram))return;
 const auto window=MEM_H(0,address(0x8036BB18));if(window!=0xA&&window!=-1)return;
 bool show=false;
 {
  std::lock_guard lock(mutex);
  if(start_request){start_request=false;show=true;MEM_H(0,address(0x8036BB16))=0;}
  else if(owns())MEM_H(0,address(0x8036BB16))=0;
  else if(MEM_HU(0,address(0x8036BB16))==0xFFFF){MEM_H(0,address(0x8036BB16))=0;begin();}
 }
 if(show){const auto saved=*ctx;ctx->r4=0x800A;func_8026AF6C(rdram,ctx);*ctx=saved;}
}
extern "C" bool bc_frontend_menu_hides_files(uint8_t* rdram){return files(rdram)&&bc::frontend_menu_owns_window();}
extern "C" void bc_frontend_menu_draw(uint8_t* rdram,recomp_context* ctx,uint32_t frame){
 if(!file_window(rdram)||!bc_frontend_menu_hides_files(rdram))return;
 // Retain original window animation/bookkeeping, discard only its commands.
 const auto sp=uint32_t(ctx->r29);
 if(sp>=0x80000000&&sp<=0x807FFFFC&&(sp&3)==0&&frame>=0x80000000&&frame<=0x807DEB68){
  const auto first=uint32_t(MEM_W(0,address(sp))),last=uint32_t(ctx->r2);
  if(first>=frame+0x48B0&&first<=last&&last<=frame+0xA3A0&&(first&7)==0)ctx->r2=address(first);
 }
 MEM_H(0,address(0x8036BB16))=0;
}

extern "C" bool bc_frontend_menu_freeze_focus(uint8_t* rdram){
 if(initialize_focus&&files(rdram)&&file_window(rdram)){initialize_focus=false;return false;}
 return bc_frontend_menu_hides_files(rdram);
}
