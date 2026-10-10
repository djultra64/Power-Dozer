#include "input.h"
#include "controls_store.h"
#include "prototype.h"
#include <chrono>
#include <stdexcept>
#include <cstdio>
std::filesystem::path bc_data_directory;
std::atomic<uint64_t> bc_polls{0};
[[noreturn]] void bc_fatal(const char* name,uint32_t){throw std::runtime_error(name);}
static void check(bool valid,const char* message){if(!valid)throw std::runtime_error(std::string(message)+": "+SDL_GetError());}
int main(){
 SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS,"0");SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
 check(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)==0,"SDL init");
 auto* window=SDL_CreateWindow("Input test",0,0,320,240,0);check(window,"window");
 bc_data_directory=std::filesystem::temp_directory_path()/("bc-input-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(bc_data_directory);
 struct Cleanup{SDL_Window* window;~Cleanup(){SDL_DestroyWindow(window);SDL_Quit();std::filesystem::remove_all(bc_data_directory);}} cleanup{window};
 bc_input_init("auto");
 auto sync=[&](){SDL_JoystickUpdate();SDL_Event event;while(SDL_PollEvent(&event))bc_input_event(event);bc_input_update(window);};
 auto frame=[](){bc::InputFrame value;check(bc_get_input(0,&value.buttons,&value.x,&value.y),"controller zero");return value;};
 auto event=[](uint8_t type){SDL_Event value{};value.type=SDL_WINDOWEVENT;value.window.event=type;bc_input_event(value);};
 struct Family {const char* name;uint16_t vendor,product;const char* south;const char* start;const char* cancel;};
 for(auto family:{Family{"QA Xbox One",0x045E,0x0B12,"A","MENU","VIEW"},Family{"QA Microsoft X-Box 360 pad",0x045E,0x028E,"A","START","BACK"},Family{"QA DualShock 3",0x054C,0x0268,"CROSS","START","SELECT"},Family{"QA DualShock 4",0x054C,0x09CC,"CROSS","OPTIONS","SHARE"},Family{"QA DualSense PS5",0x054C,0x0CE6,"CROSS","OPTIONS","CREATE"},Family{"QA Nintendo Switch Pro",0x057E,0x2009,"B","PLUS","-"},Family{"QA Ambiguous",0x1234,0xBEEF,"SOUTH","MENU","BACK"}}){
  SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=6;desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;desc.vendor_id=family.vendor;desc.product_id=family.product;desc.name=family.name;desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;desc.axis_mask=63;
  auto index=SDL_JoystickAttachVirtualEx(&desc);check(index>=0,"attach virtual controller");auto* joystick=SDL_JoystickOpen(index);check(joystick,"open virtual joystick");
  // Virtual trigger axes use the full raw range; the controller mapping maps
  // -32768 to its neutral zero and +32767 to a fully depressed trigger.
  SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1);sync();check(frame().buttons==0,"a button already held when connecting must not fire an action");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);sync();
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1);sync();
  check(frame().buttons==0x8000,"physical south invokes confirm only");check(bc_input_prompt(bc::InputAction::Confirm).label==family.south,"detected family and real action agree");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_START,1);sync();check(frame().buttons==0x1000&&bc_input_prompt(bc::InputAction::Start).label==family.start,"actual Start event and generation-specific prompt agree");
  check(bc_input_view().physical_prompt({bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_BACK}).label==family.cancel,"physical capture-cancel name follows SDL controller identity");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_START,0);SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1);sync();
  if(family.vendor==0x1234){
   bc_input_set_family("playstation");check(bc_input_prompt(bc::InputAction::Confirm).label=="CROSS","manual family applies to an ambiguous SDL device");
   bc_input_set_family("switch");check(bc_input_prompt(bc::InputAction::Confirm).label=="B","manual Switch south is B");bc_input_set_family("auto");
  }
  event(SDL_WINDOWEVENT_FOCUS_LOST);check(frame().buttons==0,"focus loss immediately clears guest buttons");sync();
  event(SDL_WINDOWEVENT_FOCUS_GAINED);sync();check(frame().buttons==0,"held controller suppressed on regain");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);sync();SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1);sync();check(frame().buttons==0x8000,"controller works after release/repress");
  SDL_Event keyboard{};keyboard.type=SDL_KEYDOWN;keyboard.key.keysym.scancode=SDL_SCANCODE_Z;bc_input_event(keyboard);
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);sync();check(bc_input_prompt(bc::InputAction::Confirm).label=="Z","button release does not steal keyboard prompts");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_DPAD_UP,1);sync();check(frame().buttons==0&&frame().y==1,"virtual D-pad maps movement without camera bit");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_DPAD_UP,0);SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_RIGHTX,-32768);sync();check(frame().buttons==2&&frame().x==0,"virtual right stick maps C-left camera");
  SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_RIGHTX,0);SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,32767);sync();check(frame().buttons==0x2000,"virtual left trigger brakes/exits");
  SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,32767);sync();check(frame().buttons==0x8000,"virtual right trigger accelerates");
  SDL_JoystickClose(joystick);check(SDL_JoystickDetachVirtual(index)==0,"detach virtual controller");sync();check(frame().buttons==0&&frame().x==0&&frame().y==0,"disconnect releases all guest actions");
  check(bc_input_prompt(bc::InputAction::Confirm).label=="Z","disconnect restores keyboard prompt");
 }
 bc::InputModel remapped;remapped.replace(bc::InputAction::Confirm,bc::InputSource::Keyboard,{{bc::BindingKind::Key,SDL_SCANCODE_T}});remapped.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
 bc::ControlsStore(bc_data_directory/"controls.json").save(remapped);bc_input_init("switch");check(bc_input_prompt(bc::InputAction::Confirm).label=="T","SDL adapter loads versioned keyboard mapping");
 SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=6;desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;desc.name="QA Remap";desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;desc.axis_mask=63;
 auto index=SDL_JoystickAttachVirtualEx(&desc);check(index>=0,"attach remapped virtual pad");auto* joystick=SDL_JoystickOpen(index);
 SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);sync();
 SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,1);sync();check(frame().buttons==0x8000&&bc_input_prompt(bc::InputAction::Confirm).label=="Y","saved physical west remap invokes confirm with correct Switch Y prompt");
  SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,0);SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,1);sync();check(frame().buttons==0,"saved remap removes previous physical south assignment");
 bc_input_begin_remap({bc::InputAction::Back,bc::InputSource::Controller});sync();check(frame().buttons==0,"editor excludes guest actions while capture is active");
 SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);sync();SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,1);sync();
 auto outcome=bc_input_take_remap_result();check(outcome&&!outcome->success&&outcome->status.find("IN USE")!=std::string::npos,"editor refuses a binding assigned to another logical action");
 check(frame().buttons==0,"conflicting capture does not leak its original confirm action");SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,0);sync();
 bc_input_begin_remap({bc::InputAction::Back,bc::InputSource::Keyboard});SDL_Event key{};key.type=SDL_KEYDOWN;key.key.keysym.scancode=SDL_SCANCODE_W;bc_input_event(key);sync();outcome=bc_input_take_remap_result();
 check(outcome&&outcome->success&&bc_input_prompt(bc::InputAction::Back).label=="W","live keyboard remap applies only after durable save");
 check(bc::ControlsStore(bc_data_directory/"controls.json").load().prompt(bc::InputAction::Back).label=="W","live remap survives reload");
 auto backup=bc_data_directory/"controls.json.bak";std::filesystem::remove(backup);std::filesystem::create_directory(backup);
 bc_input_begin_remap({bc::InputAction::Back,bc::InputSource::Keyboard});key.key.keysym.scancode=SDL_SCANCODE_U;bc_input_event(key);sync();outcome=bc_input_take_remap_result();
 check(outcome&&!outcome->success&&bc_input_prompt(bc::InputAction::Back).label=="W","failed remap save retains actual live binding");std::filesystem::remove(backup);
 bc_input_begin_remap({bc::InputAction::Back,bc::InputSource::Keyboard});key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc_input_event(key);sync();outcome=bc_input_take_remap_result();check(outcome&&outcome->cancelled,"Escape cancels live capture");
 bc_input_begin_remap({bc::InputAction::Confirm,bc::InputSource::Keyboard,true});outcome=bc_input_take_remap_result();check(outcome&&outcome->success&&bc_input_prompt(bc::InputAction::Confirm).label=="Z","explicit reset restores defaults and saves them");
 check(bc::ControlsStore(bc_data_directory/"controls.json").load().prompt(bc::InputAction::Back).label=="X","reset persists default reverse/back");
  SDL_JoystickClose(joystick);SDL_JoystickDetachVirtual(index);sync();
 puts("Virtual SDL input families, focus, axes and reconnect passed");
}
