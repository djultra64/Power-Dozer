#include "pc_settings_input.h"
#include "pc_options.h"
#include <algorithm>
#include <cmath>
#include <mutex>
namespace bc {
namespace {
std::mutex mutex;
Settings saved;
std::unique_ptr<SettingsMenu> menu;
PcSettingsViewState state;
int viewport_width=640,viewport_height=480;
bool presentation_wide=false;
PcSettingsLayout layout=pc_settings_layout(640,480);
std::optional<SettingsChange> pending;
std::optional<Settings> filters;
std::optional<RemapRequest> remap;
PcSettingsExit exit_request=PcSettingsExit::None;
bool cancel_capture=false,release_latch=false,dragging=false,remap_saving=false;
std::optional<int> drag_target;
std::string remap_status;
std::array<int,SDL_CONTROLLER_AXIS_MAX> axis_directions{};
constexpr std::array sections={SettingsMenu::Page::Display,SettingsMenu::Page::Graphics,SettingsMenu::Page::Audio,SettingsMenu::Page::Controls};
bool controls(){return menu->page()==SettingsMenu::Page::Controls;}
bool saving(){return menu->saving()||remap_saving;}
PcSettingsExit return_destination(){return state.origin==PcSettingsOrigin::MainMenu?PcSettingsExit::MainMenu:PcSettingsExit::Pause;}
void close(PcSettingsExit destination){
 menu.reset();remap.reset();remap_saving=false;cancel_capture=true;release_latch=true;dragging=false;drag_target.reset();exit_request=destination;
}
void section(size_t index){menu->section(sections[index]);state.focus_target=PcSettingsFocusTarget::Row;state.focus=0;dragging=false;drag_target.reset();remap_status.clear();}
void next_section(int direction){auto i=std::find(sections.begin(),sections.end(),menu->page())-sections.begin();section((i+4+direction)%4);}
void move(int direction){
 // Tab order follows the visible layout: tabs, selectors, action columns, footer.
 std::vector<std::pair<PcSettingsFocusTarget,size_t>> order;
 for(size_t i=0;i<4;i++)order.emplace_back(PcSettingsFocusTarget::Tab,i);
 if(controls())for(size_t i=0;i<2;i++)order.emplace_back(PcSettingsFocusTarget::Control,i);
 for(size_t i=0;i<(controls()?14:menu->fields().size());i++)order.emplace_back(PcSettingsFocusTarget::Row,i);
 if(controls())for(size_t i=2;i<4;i++)order.emplace_back(PcSettingsFocusTarget::Control,i);
 auto at=std::find(order.begin(),order.end(),std::pair{state.focus_target,state.focus})-order.begin();
 auto focus=order[(at+order.size()+direction)%order.size()];state.focus_target=focus.first;state.focus=focus.second;
}
void adjust(SettingsField field,int direction){
 if(saving())return;
 remap_status.clear();menu->adjust(field,direction);if(auto change=menu->take_change())pending=*change;
}
void volume(int target){
 target=std::clamp(target,0,100);
 if(saving()){drag_target=target;return;}
 remap_status.clear();menu->set_volume(target);if(auto change=menu->take_change())pending=*change;
}
void drag(float x){const auto& r=layout.rows[0];if(r.width>0)volume(int(std::round(std::clamp((x-r.x)/r.width,0.f,1.f)*100)));}
void select(int direction,bool activate){
 if(state.focus_target==PcSettingsFocusTarget::Tab){section(activate?state.focus:(state.focus+4+direction)%4);return;}
 if(saving())return;
 if(!controls()){auto fields=menu->fields();if(state.focus<fields.size())adjust(fields[state.focus],direction);return;}
 if(state.focus_target==PcSettingsFocusTarget::Row){
  if(!activate){state.focus=state.focus<7?state.focus+7:state.focus-7;return;}
  remap_status.clear();state.modal=PcSettingsModal::Capture;remap=RemapRequest{InputAction(state.focus),state.device};return;
 }
 switch(PcSettingsControl(state.focus)){
 case PcSettingsControl::Device:state.device=state.device==InputSource::Keyboard?InputSource::Controller:InputSource::Keyboard;break;
 case PcSettingsControl::Family:adjust(SettingsField::Family,direction);break;
 case PcSettingsControl::RestoreDefaults:if(activate)state.modal=PcSettingsModal::RestoreDefaults;break;
 case PcSettingsControl::GameOptions:if(activate)close(state.origin==PcSettingsOrigin::MainMenu?PcSettingsExit::MainMenu:PcSettingsExit::GameOptions);break;
 }
}
void cancel(){
 if(state.modal!=PcSettingsModal::None){state.modal=PcSettingsModal::None;remap.reset();cancel_capture=true;return;}
 close(return_destination());
}
bool hit(float x,float y){
 for(size_t i=0;i<4;i++)if(layout.tabs[i].contains(x,y)){state.focus_target=PcSettingsFocusTarget::Tab;state.focus=i;return true;}
 if(controls()){
  for(size_t i=0;i<4;i++)if(layout.controls[i].contains(x,y)){state.focus_target=PcSettingsFocusTarget::Control;state.focus=i;return true;}
  if(auto i=layout.hit_action(x,y)){state.focus_target=PcSettingsFocusTarget::Row;state.focus=*i;return true;}
 }else for(size_t i=0;i<menu->fields().size();i++)if(layout.rows[i].contains(x,y)){state.focus_target=PcSettingsFocusTarget::Row;state.focus=i;return true;}
 return false;
}
bool bound(const InputModel& model,InputAction action,InputBinding binding){const auto& list=model.bindings(action);return std::find(list.begin(),list.end(),binding)!=list.end();}
void confirm(){
 if(state.modal==PcSettingsModal::RestoreDefaults){remap=RemapRequest{InputAction::Confirm,state.device,true};state.modal=PcSettingsModal::None;remap_saving=true;remap_status="SAVING";}
 else select(1,true);
}
}
void initialize_pc_options(const Settings& settings,bool active_wide){
 std::lock_guard lock(mutex);presentation_wide=active_wide;saved=settings;menu.reset();state={};pending.reset();filters.reset();remap.reset();exit_request=PcSettingsExit::None;
 cancel_capture=release_latch=dragging=remap_saving=false;drag_target.reset();remap_status.clear();axis_directions.fill(0);
}
void pc_settings_begin(PcSettingsOrigin origin){std::lock_guard lock(mutex);menu=std::make_unique<SettingsMenu>(saved);state={};state.origin=origin;layout=pc_settings_layout(viewport_width,viewport_height,origin,presentation_wide);section(0);exit_request=PcSettingsExit::None;release_latch=true;axis_directions.fill(0);}
void pc_settings_close(){std::lock_guard lock(mutex);if(menu)close(return_destination());}
void pc_settings_close(PcSettingsOrigin origin){std::lock_guard lock(mutex);if(menu&&state.origin==origin)close(return_destination());}
bool pc_settings_open(){std::lock_guard lock(mutex);return bool(menu);}
void pc_settings_resize(int width,int height){std::lock_guard lock(mutex);viewport_width=width;viewport_height=height;layout=pc_settings_layout(width,height,state.origin,presentation_wide);}
PcSettingsExit take_pc_settings_exit(PcSettingsOrigin origin){
 std::lock_guard lock(mutex);if(state.origin!=origin)return PcSettingsExit::None;
 auto out=exit_request;exit_request=PcSettingsExit::None;return out;
}
PcSettingsExit take_pc_settings_exit(){std::lock_guard lock(mutex);auto out=exit_request;exit_request=PcSettingsExit::None;return out;}
std::shared_ptr<const PcSettingsSnapshot> pc_settings_current(const InputModel& input){
 std::lock_guard lock(mutex);if(!menu)return {};
 auto view=pc_settings_snapshot(*menu,input,state);view.presentation_wide=presentation_wide;
 if(!remap_status.empty())view.status=remap_status;
 view.saving|=remap_saving;
 if(remap_saving)view.status="SAVING";
 return std::make_shared<const PcSettingsSnapshot>(std::move(view));
}
void refresh_pc_settings(const Settings& settings){std::lock_guard lock(mutex);saved=settings;if(menu)menu->refresh(settings);}
std::optional<SettingsChange> take_pc_change(){std::lock_guard lock(mutex);auto out=pending;pending.reset();return out;}
void acknowledge_pc_change(const Settings& settings,bool success){
 std::lock_guard lock(mutex);if(success)saved=settings;if(menu)menu->acknowledge(saved,success);
 auto target=drag_target;drag_target.reset();if(success&&target&&menu)volume(*target);else if(!success)dragging=false;
}
void queue_pc_filter_change(const Settings& settings){std::lock_guard lock(mutex);filters=settings;}
std::optional<Settings> take_pc_filter_change(){std::lock_guard lock(mutex);auto out=filters;filters.reset();return out;}
std::optional<RemapRequest> take_pc_remap_request(){std::lock_guard lock(mutex);auto out=remap;remap.reset();return out;}
bool take_pc_controls_cancel(){std::lock_guard lock(mutex);auto out=cancel_capture;cancel_capture=false;return out;}
void acknowledge_pc_remap(const RemapResult& result){std::lock_guard lock(mutex);remap_saving=false;if(menu){state.modal=PcSettingsModal::None;remap_status=result.cancelled?"CANCELLED":result.status.empty()?(result.success?"SAVED":"SAVE FAILED"):result.status;}}
bool pc_settings_gate_input(InputModel& model,const InputSnapshot& held){
 std::lock_guard lock(mutex);const bool blocked=bool(menu)||release_latch;if(!blocked)return false;release_latch=false;
 for(size_t i=1;i<held.keys.size();i++)if(held.keys[i])model.consume({BindingKind::Key,int(i)});
 for(size_t i=0;i<held.buttons.size();i++)if(held.buttons[i])model.consume({BindingKind::Button,int(i)});
 for(size_t i=0;i<held.axes.size();i++)if(std::abs(held.axes[i])>.2f)model.consume({BindingKind::Axis,int(i)});
 return true;
}
bool pc_settings_event(const SDL_Event& event,const InputModel& input){
 std::lock_guard lock(mutex);if(!menu)return false;
 if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){viewport_width=event.window.data1;viewport_height=event.window.data2;layout=pc_settings_layout(viewport_width,viewport_height,state.origin,presentation_wide);return false;}
 if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST){dragging=false;if(state.modal==PcSettingsModal::Capture)cancel();return false;}
 if(event.type==SDL_CONTROLLERDEVICEREMOVED){axis_directions.fill(0);return false;}
 if(event.type==SDL_KEYDOWN&&event.key.keysym.scancode==SDL_SCANCODE_ESCAPE){if(!event.key.repeat)cancel();return true;}
 // Remap capture gets raw SDL events, including the device's dedicated cancel.
 if(state.modal==PcSettingsModal::Capture)return false;
 if(event.type==SDL_MOUSEBUTTONUP){dragging=false;return true;}
 if(event.type==SDL_MOUSEMOTION){if(dragging)drag(event.motion.x);else if(state.modal==PcSettingsModal::None)hit(event.motion.x,event.motion.y);return true;}
 if(event.type==SDL_MOUSEBUTTONDOWN){
  if(event.button.button!=SDL_BUTTON_LEFT)return true;
  if(state.modal==PcSettingsModal::RestoreDefaults){
   if(pc_settings_rect(layout,104,230,208,26).contains(event.button.x,event.button.y))confirm();
   else if(pc_settings_rect(layout,328,230,208,26).contains(event.button.x,event.button.y))cancel();
  }else if(hit(event.button.x,event.button.y)){
   if(menu->page()==SettingsMenu::Page::Audio&&state.focus_target==PcSettingsFocusTarget::Row&&state.focus==0){dragging=true;drag(event.button.x);}else select(1,true);
  }
  return true;
 }
 std::optional<InputBinding> binding;
 if(event.type==SDL_KEYDOWN){
  binding=InputBinding{BindingKind::Key,event.key.keysym.scancode};
 }else if(event.type==SDL_CONTROLLERBUTTONDOWN)binding=InputBinding{BindingKind::Button,event.cbutton.button};
 else if(event.type==SDL_CONTROLLERAXISMOTION){
  if(event.caxis.axis>=axis_directions.size())return true;
  int direction=event.caxis.value>21300?1:event.caxis.value<-21300?-1:0;
  auto& last=axis_directions[event.caxis.axis];if(direction==last)return true;last=direction;if(!direction)return true;
  binding=InputBinding{BindingKind::Axis,event.caxis.axis,direction};
 }
 if(!binding)return event.type==SDL_KEYUP||event.type==SDL_CONTROLLERBUTTONUP||event.type==SDL_MOUSEWHEEL;
 // Accepted Confirm/Back bindings own their keys, matching the published
 // prompts. Only the fixed Escape recovery above overrides those actions.
 const bool repeat=event.type==SDL_KEYDOWN&&event.key.repeat;
 if(bound(input,InputAction::Back,*binding)){if(!repeat)cancel();return true;}
 if(bound(input,InputAction::Confirm,*binding)){if(!repeat)confirm();return true;}
 if(event.type==SDL_KEYDOWN){
  const auto key=event.key.keysym.scancode;
  if(state.modal==PcSettingsModal::None){
   if(key==SDL_SCANCODE_TAB){move(event.key.keysym.mod&KMOD_SHIFT?-1:1);return true;}
   if(key==SDL_SCANCODE_UP||key==SDL_SCANCODE_DOWN){move(key==SDL_SCANCODE_UP?-1:1);return true;}
   if(key==SDL_SCANCODE_LEFT||key==SDL_SCANCODE_RIGHT){select(key==SDL_SCANCODE_LEFT?-1:1,false);return true;}
  }
  if(repeat)return true;
  if(key==SDL_SCANCODE_RETURN){confirm();return true;}
 }
 if(state.modal!=PcSettingsModal::None)return true;
 if(bound(input,InputAction::L,*binding))next_section(-1);
 else if(bound(input,InputAction::R,*binding))next_section(1);
 else if(bound(input,InputAction::MoveUp,*binding))move(-1);
 else if(bound(input,InputAction::MoveDown,*binding))move(1);
 else if(bound(input,InputAction::MoveLeft,*binding))select(-1,false);
 else if(bound(input,InputAction::MoveRight,*binding))select(1,false);
 return true;
}
}
