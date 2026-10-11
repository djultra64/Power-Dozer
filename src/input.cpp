#include "prototype.h"
#include "input.h"
#include "controls_store.h"
#include "pc_settings_input.h"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <cstdlib>
namespace {
std::mutex input_mutex;
bc::InputModel model;
bc::InputFrame frame;
bc::InputSnapshot previous;
std::array<bool,SDL_NUM_SCANCODES> focus_keys{};
SDL_GameController* pad=nullptr;
bool input_focused=true;
bc::InputCapture capture;
std::optional<bc::RemapResult> remap_result;
bc::ControllerFamily parse_family(const std::string& family){
 if(family=="auto")return bc::ControllerFamily::Auto;if(family=="xbox")return bc::ControllerFamily::Xbox;
 if(family=="playstation")return bc::ControllerFamily::PlayStation;if(family=="switch")return bc::ControllerFamily::Switch;
 throw std::invalid_argument("Invalid controller family");
}
bc::ControllerIdentity detected_identity(){
 if(!pad)return {};
 const char* name=SDL_GameControllerName(pad);
 return bc::controller_identity(SDL_GameControllerGetType(pad),name?name:"");
}
std::string active_name(){
 if(model.source()==bc::InputSource::Keyboard)return "keyboard";
 switch(model.family()){
 case bc::ControllerFamily::Xbox:return "xbox";case bc::ControllerFamily::PlayStation:return "playstation";
 case bc::ControllerFamily::Switch:return "switch";default:return "unknown";
 }
}
void log_device(){static std::string before="keyboard";auto now=active_name();if(now!=before){fprintf(stderr,"{\"event\":\"active_device\",\"family\":\"%s\"}\n",now.c_str());before=now;}}
bc::InputSnapshot snapshot(){
 bc::InputSnapshot value;auto keys=SDL_GetKeyboardState(nullptr);for(size_t i=0;i<value.keys.size();i++)value.keys[i]=keys[i];
 if(pad&&SDL_GameControllerGetAttached(pad)){
  for(size_t i=0;i<value.buttons.size();i++)value.buttons[i]=SDL_GameControllerGetButton(pad,SDL_GameControllerButton(i));
  for(size_t i=0;i<value.axes.size();i++)value.axes[i]=SDL_GameControllerGetAxis(pad,SDL_GameControllerAxis(i))/32768.f;
 }
 return value;
}
bool active_pad(SDL_JoystickID instance){return pad&&SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==instance;}
void consume_held(const bc::InputSnapshot& held,bool keyboard=true,bool controller=true){
 if(keyboard)for(size_t key=1;key<held.keys.size();key++)if(held.keys[key])model.consume({bc::BindingKind::Key,int(key)});
 if(controller){
  for(size_t button=0;button<held.buttons.size();button++)if(held.buttons[button])model.consume({bc::BindingKind::Button,int(button)});
  for(size_t axis=0;axis<held.axes.size();axis++)if(std::abs(held.axes[axis])>.2f)model.consume({bc::BindingKind::Axis,int(axis)});
 }
}
void save_remap(bc::InputModel candidate,const bc::InputSnapshot& held){
 try{bc::ControlsStore(bc_data_directory/"controls.json").save(candidate);model=std::move(candidate);remap_result=bc::RemapResult{true,false,"SAVED"};}
 catch(const std::exception& error){fprintf(stderr,"Controls save: %s\n",error.what());remap_result=bc::RemapResult{false,false,"SAVE FAILED"};}
 consume_held(held);
}
void finish_capture(const bc::InputSnapshot& held){
 if(auto binding=capture.take_binding()){
  auto request=capture.request();
  for(size_t i=0;i<size_t(bc::InputAction::Count);i++)if(i!=size_t(request.action))for(auto assigned:model.bindings(bc::InputAction(i)))if(assigned==*binding){
   remap_result=bc::RemapResult{false,false,std::string("IN USE: ")+bc::input_action_labels[i]};consume_held(held);model.consume(*binding);return;
  }
  auto candidate=model;candidate.replace(request.action,request.source,{*binding});save_remap(std::move(candidate),held);model.consume(*binding);
 }
 if(capture.take_cancelled()){remap_result=bc::RemapResult{false,true,"CANCELLED"};consume_held(held);}
}
void connect(){
 if(pad&&!SDL_GameControllerGetAttached(pad)){
  if(capture.active()&&capture.request().source==bc::InputSource::Controller){capture.cancel();remap_result=bc::RemapResult{false,true,"CANCELLED"};}
  model.disconnect(previous);SDL_GameControllerClose(pad);pad=nullptr;frame={};
 }
 if(!pad)for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){
  pad=SDL_GameControllerOpen(i);if(pad){
   model.controller_identity(detected_identity());auto held=snapshot();
   consume_held(held,false);
   break;
  }
 }
}
}
void bc_input_set_family(const std::string& family){std::lock_guard lock(input_mutex);model.manual_family(parse_family(family));log_device();}
void bc_input_init(const std::string& family){
 std::lock_guard lock(input_mutex);if(pad){SDL_GameControllerClose(pad);pad=nullptr;}frame={};previous={};focus_keys.fill(false);capture.cancel();remap_result.reset();input_focused=true;
 auto mappings=bc_data_directory/"controls.json";
 try{
  model=bc::ControlsStore(mappings).load();model.manual_family(parse_family(family));
 }catch(const std::exception& error){fprintf(stderr,"Controls: %s\n",error.what());bc_fatal("invalid_action_remap");}
 auto* base=SDL_GetBasePath();
 if(base){auto database=std::filesystem::u8path(base)/"gamecontrollerdb.txt";SDL_free(base);const auto utf8=database.u8string();SDL_GameControllerAddMappingsFromFile(reinterpret_cast<const char*>(utf8.c_str()));}
 connect();
}
bc::InputPrompt bc_input_prompt(bc::InputAction action){std::lock_guard lock(input_mutex);return model.prompt(action);}
bc::InputModel bc_input_view(){std::lock_guard lock(input_mutex);return model;}
std::optional<bc::RemapResult> bc_input_take_remap_result(){std::lock_guard lock(input_mutex);auto out=remap_result;remap_result.reset();return out;}
void bc_input_cancel_remap(){std::lock_guard lock(input_mutex);capture.cancel();remap_result.reset();consume_held(snapshot());frame={};}
void bc_input_begin_remap(bc::RemapRequest request){
 std::lock_guard lock(input_mutex);capture.cancel();remap_result.reset();frame={};auto held=snapshot();
 if(!request.reset){capture.begin(request,held);return;}
 auto candidate=model;bc::InputModel defaults;
 for(size_t i=0;i<size_t(bc::InputAction::Count);i++){
  auto source=request.source;
  std::vector<bc::InputBinding> bindings;for(auto binding:defaults.bindings(bc::InputAction(i)))if((binding.kind==bc::BindingKind::Key)==(source==bc::InputSource::Keyboard))bindings.push_back(binding);
  candidate.replace(bc::InputAction(i),source,bindings);
 }
 save_remap(std::move(candidate),held);
}
bool bc_input_event(const SDL_Event& event){
 std::lock_guard lock(input_mutex);
 if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST){
  for(size_t i=0;i<focus_keys.size();i++)focus_keys[i]|=previous.keys[i];model.focus(false,previous);frame={};input_focused=false;
  if(capture.active()){capture.cancel();remap_result=bc::RemapResult{false,true,"CANCELLED"};}
 }
 if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED){input_focused=true;model.focus(true,snapshot());}
 if(event.type==SDL_KEYUP&&unsigned(event.key.keysym.scancode)<focus_keys.size())focus_keys[event.key.keysym.scancode]=false;
 if(input_focused&&event.type==SDL_KEYDOWN&&event.key.repeat==0)model.activity(bc::InputSource::Keyboard);
 if(input_focused&&event.type==SDL_CONTROLLERBUTTONDOWN&&active_pad(event.cbutton.which))model.activity(bc::InputSource::Controller);
 if(input_focused&&event.type==SDL_CONTROLLERAXISMOTION&&active_pad(event.caxis.which)&&std::abs(event.caxis.value)>6500)model.activity(bc::InputSource::Controller);
 if(input_focused&&(event.type==SDL_MOUSEMOTION||event.type==SDL_MOUSEBUTTONDOWN))model.activity(bc::InputSource::Keyboard);
 log_device();
 const bool controller_event=event.type==SDL_CONTROLLERBUTTONDOWN||event.type==SDL_CONTROLLERBUTTONUP||event.type==SDL_CONTROLLERAXISMOTION;
 const bool ours=!controller_event||active_pad(event.type==SDL_CONTROLLERAXISMOTION?event.caxis.which:event.cbutton.which);
 if(ours&&(input_focused||event.type==SDL_WINDOWEVENT||event.type==SDL_CONTROLLERDEVICEREMOVED)&&bc::pc_settings_event(event,model)){
  if(event.type==SDL_KEYDOWN&&event.key.keysym.scancode!=SDL_SCANCODE_UNKNOWN)model.consume({bc::BindingKind::Key,event.key.keysym.scancode});
  if(event.type==SDL_CONTROLLERBUTTONDOWN)model.consume({bc::BindingKind::Button,event.cbutton.button});
  if(event.type==SDL_CONTROLLERAXISMOTION)model.consume({bc::BindingKind::Axis,event.caxis.axis});
  frame={};return false;
 }
 if(event.type==SDL_KEYDOWN&&event.key.keysym.scancode==SDL_SCANCODE_RETURN&&(event.key.keysym.mod&KMOD_ALT)){
  model.consume({bc::BindingKind::Key,SDL_SCANCODE_RETURN});model.consume({bc::BindingKind::Key,SDL_SCANCODE_LALT});model.consume({bc::BindingKind::Key,SDL_SCANCODE_RALT});
  return event.key.repeat==0;
 }
 if(input_focused&&capture.active()){
  try{
   if(event.type==SDL_KEYDOWN&&event.key.repeat==0)capture.press({bc::BindingKind::Key,event.key.keysym.scancode});
   if(event.type==SDL_CONTROLLERBUTTONDOWN&&active_pad(event.cbutton.which))capture.press({bc::BindingKind::Button,event.cbutton.button});
   if(event.type==SDL_CONTROLLERAXISMOTION&&active_pad(event.caxis.which)&&std::abs(event.caxis.value)>21300)capture.press({bc::BindingKind::Axis,event.caxis.axis,event.caxis.value>0?1:-1});
  }catch(const std::invalid_argument&){/* Ignore unassignable SDL events and keep waiting. */}
 }
 return false;
}
void bc_input_update(SDL_Window* window){
 std::string title;
 {
  std::lock_guard lock(input_mutex);connect();auto current=snapshot();
  capture.update(current);finish_capture(current);
  for(size_t i=1;i<focus_keys.size();i++)if(focus_keys[i]&&current.keys[i])model.consume({bc::BindingKind::Key,int(i)});
  const bool settings_gate=bc::pc_settings_gate_input(model,current);
  frame=model.sample(current);if(capture.active()||settings_gate)frame={};previous=current;log_device();
  title="Power Dozer v0.83 | Confirm: "+model.prompt(bc::InputAction::Confirm).label+" | Back: "+model.prompt(bc::InputAction::Back).label+" | "+active_name()+" | Alt+Enter: Fullscreen";
  if(std::getenv("BC_TRACE_INPUT")){
   static bc::InputFrame before;
   if(frame.buttons!=before.buttons||frame.x!=before.x||frame.y!=before.y){fprintf(stderr,"{\"event\":\"input\",\"buttons\":%u,\"stick_x\":%.3f,\"stick_y\":%.3f}\n",frame.buttons,frame.x,frame.y);before=frame;}
  }
 }
 static std::string last_title;if(title!=last_title){SDL_SetWindowTitle(window,title.c_str());last_title=title;}
}
void bc_poll_input(){++bc_polls;}
bool bc_get_input(int port,uint16_t* buttons,float* x,float* y){
 if(port!=0)return false;std::lock_guard lock(input_mutex);const auto value=bc::pc_settings_open()?bc::InputFrame{}:frame;*buttons=value.buttons;*x=value.x;*y=value.y;return true;
}
