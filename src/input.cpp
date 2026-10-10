#include "prototype.h"
#include "input.h"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <fstream>
#include <cstdlib>
#include <json.hpp>
namespace {
std::mutex input_mutex;
bc::InputModel model;
bc::InputFrame frame;
bc::InputSnapshot previous;
std::array<bool,SDL_NUM_SCANCODES> focus_keys{};
SDL_GameController* pad=nullptr;
const char* action_names[]={"confirm","back","start","trigger","camera_left","camera_down","camera_up","camera_right","l","r","move_left","move_right","move_up","move_down"};
bc::ControllerFamily parse_family(const std::string& family){
 if(family=="auto")return bc::ControllerFamily::Auto;if(family=="xbox")return bc::ControllerFamily::Xbox;
 if(family=="playstation")return bc::ControllerFamily::PlayStation;if(family=="switch")return bc::ControllerFamily::Switch;
 throw std::invalid_argument("Invalid controller family");
}
bc::ControllerFamily detected_family(){
 if(!pad)return bc::ControllerFamily::Unknown;
 auto type=SDL_GameControllerGetType(pad);
 if(type==SDL_CONTROLLER_TYPE_PS3||type==SDL_CONTROLLER_TYPE_PS4||type==SDL_CONTROLLER_TYPE_PS5)return bc::ControllerFamily::PlayStation;
 if(type==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO)return bc::ControllerFamily::Switch;
 if(type==SDL_CONTROLLER_TYPE_XBOX360||type==SDL_CONTROLLER_TYPE_XBOXONE)return bc::ControllerFamily::Xbox;
 const char* raw=SDL_GameControllerName(pad);std::string name=raw?raw:"";
 std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
 if(name.find("switch")!=std::string::npos||name.find("nintendo")!=std::string::npos)return bc::ControllerFamily::Switch;
 if(name.find("playstation")!=std::string::npos||name.find("dual")!=std::string::npos||name.find("ps4")!=std::string::npos||name.find("ps5")!=std::string::npos)return bc::ControllerFamily::PlayStation;
 if(name.find("xbox")!=std::string::npos)return bc::ControllerFamily::Xbox;
 return bc::ControllerFamily::Unknown;
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
void connect(){
 if(pad&&!SDL_GameControllerGetAttached(pad)){model.disconnect(previous);SDL_GameControllerClose(pad);pad=nullptr;frame={};}
 if(!pad)for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){
  pad=SDL_GameControllerOpen(i);if(pad){
   model.controller_family(detected_family());auto held=snapshot();
   for(size_t button=0;button<held.buttons.size();button++)if(held.buttons[button])model.consume({bc::BindingKind::Button,int(button)});
   for(size_t axis=0;axis<held.axes.size();axis++)if(std::abs(held.axes[axis])>.2f)model.consume({bc::BindingKind::Axis,int(axis)});
   break;
  }
 }
}
}
void bc_input_set_family(const std::string& family){std::lock_guard lock(input_mutex);model.manual_family(parse_family(family));log_device();}
void bc_input_init(const std::string& family){
 std::lock_guard lock(input_mutex);model=bc::InputModel();model.manual_family(parse_family(family));
 auto mappings=bc_data_directory/"controls.json";
 if(bc_data_directory==std::filesystem::path(BC_ROOT "/.local")&&!std::filesystem::exists(mappings))mappings=BC_ROOT "/config/controls.json";
 try{
  if(std::ifstream file{mappings};file){
   if(std::filesystem::file_size(mappings)>65536)throw std::runtime_error("Controls file is too large");
   auto values=nlohmann::json::parse(file);if(!values.is_object())throw std::runtime_error("Controls must be an object");
   for(size_t i=0;i<size_t(bc::InputAction::Count);i++)if(values.contains(action_names[i])){
    auto name=values[action_names[i]].get<std::string>();auto button=SDL_GameControllerGetButtonFromString(name.c_str());
    model.replace(bc::InputAction(i),bc::InputSource::Controller,{{bc::BindingKind::Button,button}});
   }
  }
 }catch(const std::exception& error){fprintf(stderr,"Controls: %s\n",error.what());bc_fatal("invalid_action_remap");}
 SDL_GameControllerAddMappingsFromFile(BC_ROOT "/.local/gamecontrollerdb.txt");connect();
}
bc::InputPrompt bc_input_prompt(bc::InputAction action){std::lock_guard lock(input_mutex);return model.prompt(action);}
bool bc_input_event(const SDL_Event& event){
 std::lock_guard lock(input_mutex);
 if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST){
  for(size_t i=0;i<focus_keys.size();i++)focus_keys[i]|=previous.keys[i];model.focus(false,previous);frame={};
 }
 if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)model.focus(true,snapshot());
 if(event.type==SDL_KEYUP&&unsigned(event.key.keysym.scancode)<focus_keys.size())focus_keys[event.key.keysym.scancode]=false;
 if(event.type==SDL_KEYDOWN&&event.key.repeat==0)model.activity(bc::InputSource::Keyboard);
 if(event.type==SDL_CONTROLLERBUTTONDOWN&&active_pad(event.cbutton.which))model.activity(bc::InputSource::Controller);
 if(event.type==SDL_CONTROLLERAXISMOTION&&active_pad(event.caxis.which)&&std::abs(event.caxis.value)>6500)model.activity(bc::InputSource::Controller);
 log_device();
 if(event.type==SDL_KEYDOWN&&event.key.keysym.scancode==SDL_SCANCODE_RETURN&&(event.key.keysym.mod&KMOD_ALT)){
  model.consume({bc::BindingKind::Key,SDL_SCANCODE_RETURN});model.consume({bc::BindingKind::Key,SDL_SCANCODE_LALT});model.consume({bc::BindingKind::Key,SDL_SCANCODE_RALT});
  return event.key.repeat==0;
 }
 return false;
}
void bc_input_update(SDL_Window* window){
 std::string title;
 {
  std::lock_guard lock(input_mutex);connect();auto current=snapshot();
  for(size_t i=1;i<focus_keys.size();i++)if(focus_keys[i]&&current.keys[i])model.consume({bc::BindingKind::Key,int(i)});
  frame=model.sample(current);previous=current;log_device();
  title="Blast Corps - beta experimental | Confirm: "+model.prompt(bc::InputAction::Confirm).label+" | Back: "+model.prompt(bc::InputAction::Back).label+" | "+active_name()+" | Alt+Enter: Fullscreen";
  if(std::getenv("BC_TRACE_INPUT")){
   static bc::InputFrame before;
   if(frame.buttons!=before.buttons||frame.x!=before.x||frame.y!=before.y){fprintf(stderr,"{\"event\":\"input\",\"buttons\":%u,\"stick_x\":%.3f,\"stick_y\":%.3f}\n",frame.buttons,frame.x,frame.y);before=frame;}
  }
 }
 static std::string last_title;if(title!=last_title){SDL_SetWindowTitle(window,title.c_str());last_title=title;}
}
void bc_poll_input(){++bc_polls;}
bool bc_get_input(int port,uint16_t* buttons,float* x,float* y){
 if(port!=0)return false;std::lock_guard lock(input_mutex);*buttons=frame.buttons;*x=frame.x;*y=frame.y;return true;
}
