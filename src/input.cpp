#include "prototype.h"
#include <SDL.h>
#include <array>
#include <algorithm>
#include <string>
#include <mutex>
#include <fstream>
#include <cstdlib>
#include <json.hpp>

// SDL face-button identities represent physical Xbox positions. Labels below
// translate those positions to the active controller family's real layout.
enum class Action { Confirm, Back, Start, Trigger, CameraLeft, CameraDown, CameraUp, CameraRight, L, R, Count };
static std::array<SDL_GameControllerButton,10> bindings={SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_INVALID,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER};
static const uint16_t bits[]={0x8000,0x4000,0x1000,0x2000,2,4,8,1,0x20,0x10};
static const char* action_names[]={"confirm","back","start","trigger","camera_left","camera_down","camera_up","camera_right","l","r"};
static const SDL_Scancode keys[]={SDL_SCANCODE_Z,SDL_SCANCODE_X,SDL_SCANCODE_RETURN,SDL_SCANCODE_LSHIFT,SDL_SCANCODE_J,SDL_SCANCODE_K,SDL_SCANCODE_I,SDL_SCANCODE_L,SDL_SCANCODE_Q,SDL_SCANCODE_E};
static std::mutex input_mutex;
static uint16_t buttons=0;
static float stick_x=0,stick_y=0;
static SDL_GameController* pad=nullptr;
static std::string family="keyboard",manual_family="auto";
static bool suppress_keyboard_start=false;

static std::string detected_family(){
 if(manual_family!="auto")return manual_family;
 std::string name=pad?SDL_GameControllerName(pad):"";
 std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
 if(name.find("switch")!=std::string::npos || name.find("nintendo")!=std::string::npos)return "switch";
 if(name.find("playstation")!=std::string::npos || name.find("dual")!=std::string::npos || name.find("ps4")!=std::string::npos || name.find("ps5")!=std::string::npos)return "playstation";
 auto type=SDL_GameControllerGetType(pad);
 if(type==SDL_CONTROLLER_TYPE_PS3||type==SDL_CONTROLLER_TYPE_PS4||type==SDL_CONTROLLER_TYPE_PS5)return "playstation";
 if(type==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO)return "switch";
 if(type==SDL_CONTROLLER_TYPE_XBOX360||type==SDL_CONTROLLER_TYPE_XBOXONE)return "xbox";
 return "unknown";
}
static const char* face_label(SDL_GameControllerButton button) {
 if(button<SDL_CONTROLLER_BUTTON_A||button>SDL_CONTROLLER_BUTTON_Y)return SDL_GameControllerGetStringForButton(button);
 static const char* xbox[]={"A","B","X","Y"},*sw[]={"B","A","Y","X"},*ps[]={"Cross","Circle","Square","Triangle"},*unknown[]={"South","East","West","North"};
 return family=="switch"?sw[button]:family=="playstation"?ps[button]:family=="xbox"?xbox[button]:unknown[button];
}
void bc_input_set_family(const std::string& override_family){
 manual_family=override_family;
 if(family!="keyboard")family=detected_family();
}
void bc_input_init(const std::string& override_family){
 manual_family=override_family;
 auto mappings=bc_data_directory/"controls.json";
 if(bc_data_directory==std::filesystem::path(BC_ROOT "/.local") && !std::filesystem::exists(mappings))mappings=BC_ROOT "/config/controls.json";
 if(std::ifstream file{mappings};file){
  auto values=nlohmann::json::parse(file);
  for(size_t i=0;i<bindings.size();i++)if(values.contains(action_names[i])) {
   auto name=values[action_names[i]].get<std::string>();
   auto button=SDL_GameControllerGetButtonFromString(name.c_str());
   if(button==SDL_CONTROLLER_BUTTON_INVALID)bc_fatal("invalid_action_remap",i);
   bindings[i]=button;
  }
 }
 SDL_GameControllerAddMappingsFromFile(BC_ROOT "/.local/gamecontrollerdb.txt");
 for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){pad=SDL_GameControllerOpen(i);break;}
}
bool bc_input_event(const SDL_Event& event){
 std::string next=family;
 if(event.type==SDL_KEYDOWN||event.type==SDL_KEYUP)next="keyboard";
 else if(pad&&(event.type==SDL_CONTROLLERBUTTONDOWN||event.type==SDL_CONTROLLERBUTTONUP))next=detected_family();
 else if(pad&&event.type==SDL_CONTROLLERAXISMOTION&&std::abs(event.caxis.value)>6500)next=detected_family();
 if(next!=family){family=next;fprintf(stderr,"{\"event\":\"active_device\",\"family\":\"%s\"}\n",family.c_str());}
 if(event.type==SDL_KEYDOWN && event.key.keysym.scancode==SDL_SCANCODE_RETURN && (event.key.keysym.mod&KMOD_ALT)){
  suppress_keyboard_start=true;
  return event.key.repeat==0;
 }
 return false;
}
void bc_input_update(SDL_Window* window){
 if(!pad)for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){pad=SDL_GameControllerOpen(i);break;}
 if(pad&&!SDL_GameControllerGetAttached(pad)){SDL_GameControllerClose(pad);pad=nullptr;}
 auto keyboard=SDL_GetKeyboardState(nullptr);uint16_t value=0;float x=0,y=0;bool used_keys=false,used_pad=false;
 if(!keyboard[SDL_SCANCODE_RETURN])suppress_keyboard_start=false;
 for(size_t i=0;i<10;i++)if(keyboard[keys[i]] && !(i==size_t(Action::Start)&&suppress_keyboard_start)){value|=bits[i];used_keys=true;}
 x=keyboard[SDL_SCANCODE_RIGHT]-keyboard[SDL_SCANCODE_LEFT];y=keyboard[SDL_SCANCODE_UP]-keyboard[SDL_SCANCODE_DOWN];
 used_keys|=x||y;
 if(pad){
  for(size_t i=0;i<bindings.size();i++)if(bindings[i]!=SDL_CONTROLLER_BUTTON_INVALID&&SDL_GameControllerGetButton(pad,bindings[i])){value|=bits[i];used_pad=true;}
  auto ax=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX),ay=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);
  if(std::abs(ax)>6500||std::abs(ay)>6500){x=ax/32768.0f;y=-ay/32768.0f;used_pad=true;}
  if(SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>8000){value|=bits[3];used_pad=true;}
 }

 static std::string last_title;
 std::string confirm=family=="keyboard"?"Z":face_label(bindings[0]),back=family=="keyboard"?"X":face_label(bindings[1]);
 std::string title="Blast Corps - beta experimental | Confirm: "+confirm+" | Back: "+back+" | "+family+" | Alt+Enter: Fullscreen";
 if(title!=last_title){SDL_SetWindowTitle(window,title.c_str());last_title=title;}
 std::lock_guard lock(input_mutex);buttons=value;stick_x=x;stick_y=y;
 if(std::getenv("BC_TRACE_INPUT")){
  static uint16_t previous=0;static float previous_x=0,previous_y=0;
  if(value!=previous||x!=previous_x||y!=previous_y){fprintf(stderr,"{\"event\":\"input\",\"buttons\":%u,\"stick_x\":%.3f,\"stick_y\":%.3f}\n",value,x,y);previous=value;previous_x=x;previous_y=y;}
 }
}
void bc_poll_input(){++bc_polls;}
bool bc_get_input(int port,uint16_t* b,float* x,float* y){if(port!=0)return false;std::lock_guard lock(input_mutex);*b=buttons;*x=stick_x;*y=stick_y;return true;}
