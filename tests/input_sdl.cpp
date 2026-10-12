#include "input.h"
#include "frontend_menu.h"
extern "C" void func_8026AF6C(uint8_t*,recomp_context*){}
#include "controls_store.h"
#include "prototype.h"
#include "pc_settings_input.h"
#include "pc_options.h"
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
 auto reset_model=bc_input_view();reset_model.activity(bc::InputSource::Controller);check(reset_model.prompt(bc::InputAction::Confirm).label=="Y","keyboard reset preserves remapped controller action");
 bc::initialize_pc_options({});bc::pc_settings_resize(640,480);bc::pc_settings_begin();
 SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,0);sync();
 SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,1);sync();check(frame().buttons==0,"host settings selection cannot reach guest");
 key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc_input_event(key);sync();check(!bc::pc_settings_open()&&frame().buttons==0,"fixed Escape closes settings while held confirm stays suppressed");
 SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,0);sync();SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,1);sync();check(frame().buttons==0x8000,"held close latch releases only after physical release");
 SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,0);sync();
 bc::pc_settings_begin();
 SDL_Event mouse{};mouse.type=SDL_MOUSEMOTION;mouse.motion.x=50;mouse.motion.y=100;bc_input_event(mouse);check(bc_input_view().source()==bc::InputSource::Keyboard,"mouse restores keyboard prompts as last active device");
 mouse.type=SDL_MOUSEBUTTONDOWN;mouse.button.button=SDL_BUTTON_LEFT;mouse.button.x=510;mouse.button.y=75;bc_input_event(mouse);
 mouse.button.x=40;mouse.button.y=120;bc_input_event(mouse); // Controller device.
 mouse.button.x=40;mouse.button.y=160;bc_input_event(mouse);
 auto capture_request=bc::take_pc_remap_request();check(capture_request&&capture_request->source==bc::InputSource::Controller,"host capture uses selected controller");bc_input_begin_remap(*capture_request);sync();
 SDL_JoystickClose(joystick);check(SDL_JoystickDetachVirtual(index)==0,"detach while capturing");sync();outcome=bc_input_take_remap_result();check(outcome&&outcome->cancelled,"controller disconnect cancels capture");bc::acknowledge_pc_remap(*outcome);
 auto host=bc::pc_settings_current(bc_input_view());check(host->modal==bc::PcSettingsModal::None&&host->focus==0&&host->device==bc::InputSource::Controller,"disconnect restores same row and editing device");
 key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc_input_event(key);sync();

 // The global fullscreen chord must bypass ordinary selection, defaults
 // confirmation and raw remap capture, including repeated keydowns.
 for(int mode=0;mode<3;++mode){
  bc::pc_settings_close();bc::initialize_pc_options({});bc::pc_settings_begin();
  if(mode){
   mouse.button.x=510;mouse.button.y=75;bc_input_event(mouse);
   mouse.button.x=40;mouse.button.y=mode==1?390:160;bc_input_event(mouse);
  }
  if(mode==2){auto request=bc::take_pc_remap_request();check(request.has_value(),"keyboard capture requested");bc_input_begin_remap(*request);sync();}
  const auto before=bc::pc_settings_current(bc_input_view());
  SDL_Event chord{};chord.type=SDL_KEYDOWN;chord.key.keysym.scancode=SDL_SCANCODE_RETURN;chord.key.keysym.mod=KMOD_LALT;
  check(bc_input_event(chord),"Alt+Enter requests global fullscreen while settings is open");
  chord.key.repeat=1;check(!bc_input_event(chord),"held Alt+Enter never repeats fullscreen");
  auto consumed=bc_input_view();bc::InputSnapshot held;held.keys[SDL_SCANCODE_RETURN]=true;held.keys[SDL_SCANCODE_LALT]=true;held.keys[SDL_SCANCODE_RALT]=true;
  const auto blocked=consumed.sample(held);check(blocked.buttons==0&&blocked.x==0&&blocked.y==0,"fullscreen consumes held Return and both Alt keys");
  consumed.sample({});held={};held.keys[SDL_SCANCODE_RETURN]=true;check(consumed.sample(held).buttons==0x1000,"Return action recovers after chord release");sync();
  const auto after=bc::pc_settings_current(bc_input_view());
  check(after&&after->modal==before->modal&&after->focus==before->focus&&after->rows[0].value==before->rows[0].value,"fullscreen chord retains setting and modal state");
  check(!bc::take_pc_change()&&!bc::take_pc_remap_request(),"fullscreen chord queues no settings save or defaults reset");
  check(!bc_input_take_remap_result(),"fullscreen chord never captures Enter");
  check(frame().buttons==0&&frame().x==0&&frame().y==0,"fullscreen chord leaks no guest input");
  chord.type=SDL_KEYUP;chord.key.repeat=0;chord.key.keysym.mod=KMOD_NONE;bc_input_event(chord);
  if(mode==2){bc_input_cancel_remap();bc::acknowledge_pc_remap({false,true,"CANCELLED"});}
  bc::pc_settings_close();sync();check(frame().buttons==0,"fullscreen chord stays suppressed after settings close");
 }

 // Accept the remaps through the real capture/store path, then compare the
 // published instruction with both ordinary-page and confirmation behavior.
 auto assign_key=[&](bc::InputAction action,SDL_Scancode code){
  bc_input_begin_remap({action,bc::InputSource::Keyboard});key.key.keysym.scancode=code;bc_input_event(key);sync();
  auto result=bc_input_take_remap_result();check(result&&result->success,"navigation-key remap is accepted and saved");
 };
 struct RemapCase {bc::InputAction action;SDL_Scancode code;const char* label;};
 for(auto test:{RemapCase{bc::InputAction::Confirm,SDL_SCANCODE_TAB,"TAB"},RemapCase{bc::InputAction::Back,SDL_SCANCODE_TAB,"TAB"},RemapCase{bc::InputAction::Back,SDL_SCANCODE_RETURN,"ENTER"},RemapCase{bc::InputAction::Confirm,SDL_SCANCODE_UP,"UP"}}){
  bc::pc_settings_close();bc_input_begin_remap({bc::InputAction::Confirm,bc::InputSource::Keyboard,true});bc_input_take_remap_result();
  if(test.code==SDL_SCANCODE_RETURN)assign_key(bc::InputAction::Start,SDL_SCANCODE_F1);
  if(test.code==SDL_SCANCODE_UP)assign_key(bc::InputAction::MoveUp,SDL_SCANCODE_F2);
  assign_key(test.action,test.code);
  check(bc::ControlsStore(bc_data_directory/"controls.json").load().prompt(test.action).label==test.label,"accepted navigation-key remap persists its prompt");
  bc::initialize_pc_options({});bc::pc_settings_begin();
  const auto prompt_index=test.action==bc::InputAction::Confirm?0:1;
  check(bc::pc_settings_current(bc_input_view())->prompts[prompt_index].label==test.label,"normal settings prompt uses accepted remap");
  key.key.keysym.scancode=test.code;key.key.repeat=1;bc_input_event(key);
  check(bc::pc_settings_open()&&bc::pc_settings_current(bc_input_view())->focus==0&&!bc::take_pc_change(),"held logical key cannot fall through to navigation or re-activate");
  key.key.repeat=0;bc_input_event(key);
  if(test.action==bc::InputAction::Confirm){
   auto changed=bc::take_pc_change();check(changed&&changed->field==bc::SettingsField::Scale,"advertised remapped select activates row rather than navigation shortcut");bc::acknowledge_pc_change(changed->settings,true);
  }else check(!bc::pc_settings_open(),"advertised remapped back closes normal settings page");
  bc::initialize_pc_options({});bc::pc_settings_begin();
  mouse.button.x=510;mouse.button.y=75;bc_input_event(mouse);mouse.button.x=40;mouse.button.y=390;bc_input_event(mouse);
  host=bc::pc_settings_current(bc_input_view());check(host->modal==bc::PcSettingsModal::RestoreDefaults&&host->prompts[prompt_index].label==test.label,"defaults modal advertises accepted logical key");
  bc_input_event(key);host=bc::pc_settings_current(bc_input_view());auto reset=bc::take_pc_remap_request();
  check(host&&host->modal==bc::PcSettingsModal::None,"advertised logical key dismisses confirmation modal");
  if(test.action==bc::InputAction::Confirm)check(reset&&reset->reset,"remapped select confirms device defaults");
  else check(!reset&&host->focus==2,"remapped back cancels defaults without resetting bindings");
 }
 bc::pc_settings_close();

 // Frontend owns the original file window until Start, including live remaps.
 bc_input_begin_remap({bc::InputAction::Confirm,bc::InputSource::Keyboard,true});bc_input_take_remap_result();
 bc::initialize_pc_options({});bc::frontend_menu_resize(640,480);bc::frontend_menu_begin();sync();
 key.key.keysym.scancode=SDL_SCANCODE_DOWN;key.key.repeat=0;bc_input_event(key);sync();
 check(bc::frontend_menu_current(bc_input_view())->selected==1,"SDL mapped navigation reaches main menu");
 SDL_Event chord{};chord.type=SDL_KEYDOWN;chord.key.keysym.scancode=SDL_SCANCODE_RETURN;chord.key.keysym.mod=KMOD_LALT;
 check(bc_input_event(chord),"main menu preserves global fullscreen priority");
 check(!bc::pc_settings_open()&&bc::frontend_menu_current(bc_input_view())->selected==1,"fullscreen never activates selected PC Options");
 key.key.keysym.scancode=SDL_SCANCODE_Z;bc_input_event(key);sync();
 check(bc::pc_settings_open()&&!bc::frontend_menu_current(bc_input_view()),"SDL Confirm opens main-origin PC Options");
 key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc_input_event(key);sync();
 check(!bc::pc_settings_open()&&bc::frontend_menu_current(bc_input_view())->selected==1,"fixed Escape returns to main with options focus");
 for(auto family:{Family{"Main Xbox",0x045E,0x0B12,"X","",""},Family{"Main DualSense PS5",0x054C,0x0CE6,"SQUARE","",""},Family{"Main Nintendo Switch Pro",0x057E,0x2009,"Y","",""}}){
  bc_input_set_family("auto");
  SDL_VirtualJoystickDesc fd{};fd.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;fd.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;fd.naxes=6;fd.nbuttons=SDL_CONTROLLER_BUTTON_MAX;fd.vendor_id=family.vendor;fd.product_id=family.product;fd.name=family.name;fd.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;fd.axis_mask=63;
  auto fi=SDL_JoystickAttachVirtualEx(&fd);auto* fj=SDL_JoystickOpen(fi);check(fj,"main controller attached");
  SDL_JoystickSetVirtualAxis(fj,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(fj,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);
  bc::frontend_menu_begin();SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_X,1);sync();
  check(bc::frontend_menu_owns_window()&&frame().buttons==0,"held remapped Confirm on connect cannot open files");
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_X,0);sync();
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_X,1);sync();
  check(!bc::frontend_menu_owns_window()&&frame().buttons==0,"Main to Files consumes held physical Confirm");
  std::vector<uint8_t> guest_memory(8*1024*1024);auto* rdram=guest_memory.data();recomp_context guest_ctx{};
  MEM_W(0,S32(0x80364A94))=0x10000;MEM_H(0,S32(0x8036BB18))=10;
  bc_frontend_menu_selection(rdram,&guest_ctx);sync();check(frame().buttons==0,"guest handoff never leaks held Confirm");
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_X,0);sync();
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_B,1);sync();check(frame().buttons==0x4000,"mapped file Back reaches original controller logic");
  MEM_H(0,S32(0x8036BB16))=0xFFFF;bc_frontend_menu_selection(rdram,&guest_ctx);sync();
  check(bc::frontend_menu_current(bc_input_view())->selected==0&&frame().buttons==0,"Files to Main consumes held physical Back");
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_B,0);sync();
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_DPAD_DOWN,1);sync();
  check(bc::frontend_menu_current(bc_input_view())->confirm.label==family.south,"main published physical remap matches real family");
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_DPAD_DOWN,0);sync();
  SDL_JoystickSetVirtualButton(fj,SDL_CONTROLLER_BUTTON_X,1);sync();
  check(bc::pc_settings_open()&&frame().buttons==0,"remapped physical west opens options without guest action");
  key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc_input_event(key);sync();
  check(bc::frontend_menu_current(bc_input_view())->selected==1&&frame().buttons==0,"held controller stays consumed on options return");
  SDL_JoystickClose(fj);check(SDL_JoystickDetachVirtual(fi)==0,"main disconnect");sync();
  check(bc::frontend_menu_current(bc_input_view())->confirm.source==bc::InputSource::Keyboard,"disconnect restores keyboard menu prompt");
 }
 // Real SDL motion events interleaved with production frame sampling reproduce
 // gradual input, unlike a neutral-to-full synthetic event in a single frame.
 bc::InputModel analog;analog.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Axis,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,1}});
 bc::ControlsStore(bc_data_directory/"controls.json").save(analog);bc_input_init("switch");
 auto ai=SDL_JoystickAttachVirtualEx(&desc);auto* aj=SDL_JoystickOpen(ai);check(aj,"analog regression pad attached");
 SDL_JoystickSetVirtualAxis(aj,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(aj,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);sync();
 auto axis_frame=[&](int axis,int value){check(SDL_JoystickSetVirtualAxis(aj,axis,value)==0,"set virtual axis");sync();check(frame().buttons==0&&frame().x==0&&frame().y==0,"owned analog frame cannot reach hidden Files");};
 auto ramp=[&](int axis){for(int value:{11000,18000,28000})axis_frame(axis,value);};
 bc::initialize_pc_options({});bc::frontend_menu_begin();sync();
 mouse.button.x=320;mouse.button.y=270;bc_input_event(mouse);sync();
 // Controls capture also needs the first actionable event of a gradual ramp.
 mouse.button.x=510;mouse.button.y=60;bc_input_event(mouse);mouse.button.x=40;mouse.button.y=100;bc_input_event(mouse);mouse.button.x=40;mouse.button.y=130;bc_input_event(mouse);
 auto analog_request=bc::take_pc_remap_request();check(analog_request&&analog_request->source==bc::InputSource::Controller,"analog capture selects controller Confirm");bc_input_begin_remap(*analog_request);sync();
 for(int value:{-10000,0,10000,25000})axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,value);
 outcome=bc_input_take_remap_result();check(outcome&&outcome->success,"gradual trigger reaches raw remap capture");bc::acknowledge_pc_remap(*outcome);
 axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc_input_event(key);sync();
 bc::initialize_pc_options({});bc::frontend_menu_begin();sync();
 axis_frame(SDL_CONTROLLER_AXIS_LEFTY,11000);check(bc::frontend_menu_current(bc_input_view())->selected==0,"virtual subthreshold motion must not activate early");
 axis_frame(SDL_CONTROLLER_AXIS_LEFTY,28000);check(bc::frontend_menu_current(bc_input_view())->selected==1,"virtual gradual Main stick crosses threshold");
 axis_frame(SDL_CONTROLLER_AXIS_LEFTY,0);
 // Raw virtual trigger -32768..32767 maps to controller 0..32767.
 for(int value:{-10000,0,10000,25000})axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,value);
 check(bc::pc_settings_open(),"slow remapped trigger Confirm opens pregame Settings");
 check(bc_input_prompt(bc::InputAction::Confirm).label=="ZR","remapped trigger prompt matches Switch physical trigger");
 check(!bc::take_pc_change(),"held opening trigger cannot change Settings");
 axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,30000);check(!bc::take_pc_change(),"opening trigger remains blocked while held");
 axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);ramp(SDL_CONTROLLER_AXIS_LEFTY);
 check(bc::pc_settings_current(bc_input_view())->focus==1,"virtual gradual pregame Settings stick navigates");
 axis_frame(SDL_CONTROLLER_AXIS_LEFTY,0);
 for(int value:{-10000,0,10000,25000})axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,value);
 auto analog_change=bc::take_pc_change();check(analog_change.has_value(),"slow remapped trigger reactivates in Settings after neutral");bc::acknowledge_pc_change(analog_change->settings,true);
 key.key.keysym.scancode=SDL_SCANCODE_ESCAPE;bc_input_event(key);sync();
 axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,30000);check(bc::frontend_menu_current(bc_input_view())->selected==1,"Settings to Main held trigger cannot reopen Options");
 axis_frame(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);axis_frame(SDL_CONTROLLER_AXIS_LEFTY,11000);bc::frontend_menu_begin();sync();
 axis_frame(SDL_CONTROLLER_AXIS_LEFTY,28000);check(bc::frontend_menu_current(bc_input_view())->selected==0,"subthreshold stick held on Main entry stays blocked");
 axis_frame(SDL_CONTROLLER_AXIS_LEFTY,0);ramp(SDL_CONTROLLER_AXIS_LEFTY);check(bc::frontend_menu_current(bc_input_view())->selected==1,"held-opening stick works after neutral and gradual repress");
 SDL_JoystickClose(aj);check(SDL_JoystickDetachVirtual(ai)==0,"analog pad detach");sync();
 puts("Virtual SDL input families, focus, axes and reconnect passed");
 return 0;

}
