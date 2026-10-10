// Private SDL virtual-controller driver for native UI acceptance.
// Only test processes on an isolated display may use it. No OS device writes.
#include <SDL.h>
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <string>
namespace {
SDL_Joystick* joystick=nullptr;
uint64_t serial=0;
bool enabled(){const char* display=std::getenv("DISPLAY");return display&&std::strcmp(display,":0")&&std::strcmp(display,":0.0")&&std::getenv("BC_QA_CONTROLLER")&&std::getenv("BC_QA_INPUT_FILE");}
void attach(bool held=false){
 if(joystick)return;
 const char* family=std::getenv("BC_QA_CONTROLLER");
 SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
 desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=6;desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;
 desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;desc.axis_mask=63;
 if(!std::strcmp(family,"xbox360")){desc.name="QA Microsoft X-Box 360 pad";desc.vendor_id=0x045e;desc.product_id=0x028e;}
 else if(!std::strcmp(family,"ps5")){desc.name="QA DualSense PS5";desc.vendor_id=0x054c;desc.product_id=0x0ce6;}
 else if(!std::strcmp(family,"switch")){desc.name="QA Nintendo Switch Pro";desc.vendor_id=0x057e;desc.product_id=0x2009;}
 else{fprintf(stderr,"Invalid private QA controller family\n");return;}
 auto index=SDL_JoystickAttachVirtualEx(&desc);
 if(index>=0)joystick=SDL_JoystickOpen(index);
 if(!joystick){fprintf(stderr,"Private QA controller attach failed: %s\n",SDL_GetError());return;}
 SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);
 SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);
 SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,held);
 SDL_JoystickUpdate();
 fprintf(stderr,"{\"event\":\"qa_controller\",\"phase\":\"attached\",\"family\":\"%s\",\"held\":%s}\n",family,held?"true":"false");
}
void detach(){
 if(!joystick)return;
 auto instance=SDL_JoystickInstanceID(joystick);int index=-1;
 for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_JoystickGetDeviceInstanceID(i)==instance){index=i;break;}
 SDL_JoystickClose(joystick);joystick=nullptr;
 if(index>=0)SDL_JoystickDetachVirtual(index);
 SDL_JoystickUpdate();fprintf(stderr,"{\"event\":\"qa_controller\",\"phase\":\"detached\"}\n");
}
void requests(){
 std::ifstream file(std::getenv("BC_QA_INPUT_FILE"));uint64_t next=0;std::string kind,name;int value=0;
 if(!(file>>next>>kind>>name>>value)||next<=serial)return;serial=next;
 bool ok=true;
 if(kind=="attach")attach(value!=0);
 else if(kind=="detach")detach();
 else if(kind=="button"&&joystick){auto button=SDL_GameControllerGetButtonFromString(name.c_str());ok=button!=SDL_CONTROLLER_BUTTON_INVALID&&(value==0||value==1)&&SDL_JoystickSetVirtualButton(joystick,button,value)==0;}
 else if(kind=="axis"&&joystick){auto axis=SDL_GameControllerGetAxisFromString(name.c_str());ok=axis!=SDL_CONTROLLER_AXIS_INVALID&&value>=-32768&&value<=32767&&SDL_JoystickSetVirtualAxis(joystick,axis,value)==0;}
 else ok=false;
 SDL_JoystickUpdate();
 fprintf(stderr,"{\"event\":\"qa_controller_request\",\"serial\":%llu,\"ok\":%s}\n",(unsigned long long)serial,ok?"true":"false");
}
}
extern "C" int SDL_Init(Uint32 flags){
 static auto original=reinterpret_cast<decltype(&SDL_Init)>(dlsym(RTLD_NEXT,"SDL_Init"));
 auto result=original(flags);if(!result&&enabled()&&(flags&SDL_INIT_GAMECONTROLLER))attach();return result;
}
extern "C" SDL_bool SDL_IsGameController(int index){
 static auto original=reinterpret_cast<decltype(&SDL_IsGameController)>(dlsym(RTLD_NEXT,"SDL_IsGameController"));
 if(enabled()&&!SDL_JoystickIsVirtual(index))return SDL_FALSE;
 return original(index);
}
extern "C" int SDL_PollEvent(SDL_Event* event){
 static auto original=reinterpret_cast<decltype(&SDL_PollEvent)>(dlsym(RTLD_NEXT,"SDL_PollEvent"));
 if(enabled())requests();return original(event);
}
