#include "pc_settings_input.h"
#include "pc_options.h"
#include <cstdio>
#include <stdexcept>
#include <filesystem>
#include <chrono>
static void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
static bc::InputModel model;
static void key(SDL_Scancode code,SDL_Keymod mod=KMOD_NONE){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.scancode=code;e.key.keysym.mod=mod;bc::pc_settings_event(e,model);}
static void mouse(int x,int y,bool click=false){SDL_Event e{};e.type=click?SDL_MOUSEBUTTONDOWN:SDL_MOUSEMOTION;if(click){e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;}else{e.motion.x=x;e.motion.y=y;}bc::pc_settings_event(e,model);}
static auto view(){return bc::pc_settings_current(model);}
int main(){
 bc::initialize_pc_options({});bc::pc_settings_resize(640,480);bc::pc_settings_begin();
 check(bc::pc_settings_open(),"existing entry must open complete host menu");
 key(SDL_SCANCODE_DOWN);check(view()->focus==1,"Down focuses aspect");
 mouse(80,190);check(view()->focus==1,"mouse reaches same aspect row");
 key(SDL_SCANCODE_TAB);check(view()->focus==2,"Tab advances focus");
 key(SDL_SCANCODE_TAB,KMOD_SHIFT);check(view()->focus==1,"Shift Tab reverses focus");
 key(SDL_SCANCODE_RIGHT);auto change=bc::take_pc_change();check(change&&change->field==bc::SettingsField::Aspect&&!change->settings.wide,"right adjusts focused field");
 bc::acknowledge_pc_change(change->settings,true);key(SDL_SCANCODE_LEFT);change=bc::take_pc_change();check(change&&change->settings.wide,"left reverses field");bc::acknowledge_pc_change(change->settings,true);
 mouse(350,75,true);check(view()->section==bc::SettingsMenu::Page::Audio,"mouse selects Audio tab");
 mouse(599,140,true);change=bc::take_pc_change();if(change)bc::acknowledge_pc_change(change->settings,true);
 SDL_Event drag{};drag.type=SDL_MOUSEMOTION;drag.motion.x=999;drag.motion.y=140;bc::pc_settings_event(drag,model);check(view()->rows[0].value=="100%","slider upper clamp");
 drag.motion.x=-50;bc::pc_settings_event(drag,model);change=bc::take_pc_change();check(change&&change->settings.volume==0,"slider lower clamp");bc::acknowledge_pc_change(change->settings,true);check(view()->status=="SAVED","volume drag remains live and never requests restart");
 SDL_Event up{};up.type=SDL_MOUSEBUTTONUP;up.button.button=SDL_BUTTON_LEFT;bc::pc_settings_event(up,model);
 mouse(500,75,true);mouse(45,160,true);check(view()->modal==bc::PcSettingsModal::Capture,"action click starts modal capture");
 auto request=bc::take_pc_remap_request();check(request&&request->action==bc::InputAction::Confirm,"capture logical action");key(SDL_SCANCODE_ESCAPE);check(view()->modal==bc::PcSettingsModal::None&&view()->focus==0,"cancel restores same action focus");check(bc::take_pc_controls_cancel(),"cancel reaches SDL capture");
 key(SDL_SCANCODE_ESCAPE);check(!bc::pc_settings_open(),"fixed Escape closes host menu");
 bc::InputSnapshot held;held.buttons[SDL_CONTROLLER_BUTTON_A]=true;bc::pc_settings_gate_input(model,held);check(model.sample(held).buttons==0,"held confirm after close is suppressed");model.sample({});check(model.sample(held).buttons==0x8000,"confirm returns after release");

 // Saving blocks conflicting edits, but tabs remain reachable. Failed writes
 // restore the durable value without moving the user's current focus.
 bc::Settings durable;durable.volume=60;bc::initialize_pc_options(durable);bc::pc_settings_begin();mouse(350,75,true);key(SDL_SCANCODE_LEFT);
 change=bc::take_pc_change();check(change&&change->settings.volume==50,"left decreases volume");
 mouse(510,75,true);check(view()->section==bc::SettingsMenu::Page::Controls&&view()->rows.size()==14,"section changes during saving and all actions remain visible");
 mouse(40,200);check(view()->focus==1,"focus moves while save pending");
 auto directory=std::filesystem::temp_directory_path()/("pc-settings-failure-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(directory);
 bool failed=false;try{bc::SettingsStore(directory).save(change->settings);}catch(const std::exception&){failed=true;}std::filesystem::remove_all(directory);
 check(failed,"test exercises a real durable write failure");bc::acknowledge_pc_change({},false);check(view()->focus==1&&view()->status=="SAVE FAILED","save failure preserves current section focus");mouse(350,75,true);check(view()->rows[0].value=="60%","failed setting rolls back to saved value");
 // Every action has a reachable capture target in the same table.
 mouse(510,75,true);
 for(int i=0;i<14;i++){mouse(40+(i/7)*296,160+(i%7)*32,true);request=bc::take_pc_remap_request();check(request&&int(request->action)==i,"all fourteen click targets capture their logical action");key(SDL_SCANCODE_ESCAPE);check(view()->focus==size_t(i)&&view()->modal==bc::PcSettingsModal::None,"modal restores its original row");bc::take_pc_controls_cancel();}
 mouse(40,120,true);check(view()->device==bc::InputSource::Controller,"device selector switches editing device");
 mouse(40,390,true);check(view()->modal==bc::PcSettingsModal::RestoreDefaults&&!bc::take_pc_remap_request(),"defaults requires confirmation");key(SDL_SCANCODE_ESCAPE);check(view()->focus_target==bc::PcSettingsFocusTarget::Control&&view()->focus==2,"defaults cancel restores focus");
 key(SDL_SCANCODE_RETURN);key(SDL_SCANCODE_RETURN);key(SDL_SCANCODE_E);check(view()->saving&&view()->status=="SAVING","defaults save feedback survives tab navigation");key(SDL_SCANCODE_Q);mouse(40,390);request=bc::take_pc_remap_request();check(request&&request->reset&&request->source==bc::InputSource::Controller,"confirmed defaults resets selected device only");bc::acknowledge_pc_remap({false,false,"SAVE FAILED"});check(view()->focus==2&&view()->status=="SAVE FAILED","failed controls save retains selected control");
 // Nintendo physical positions and optional remapping share InputModel's
 // prompt and the event that actually performs the action.
 model.manual_family(bc::ControllerFamily::Switch);model.activity(bc::InputSource::Controller);
 check(view()->prompts[0].label=="B"&&view()->prompts[1].label=="A","Switch south and east prompts are B and A");
 SDL_Event button{};button.type=SDL_CONTROLLERBUTTONDOWN;button.cbutton.button=SDL_CONTROLLER_BUTTON_A;bc::pc_settings_event(button,model);check(view()->modal==bc::PcSettingsModal::RestoreDefaults,"physical Switch B confirms");button.cbutton.button=SDL_CONTROLLER_BUTTON_B;bc::pc_settings_event(button,model);check(view()->modal==bc::PcSettingsModal::None,"physical Switch A cancels");
 model.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});model.replace(bc::InputAction::Back,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_Y}});
 check(view()->prompts[0].label=="Y"&&view()->prompts[1].label=="X","Switch remapped west and north prompts are Y and X");button.cbutton.button=SDL_CONTROLLER_BUTTON_X;bc::pc_settings_event(button,model);check(view()->modal==bc::PcSettingsModal::RestoreDefaults,"physical Switch Y remap confirms");button.cbutton.button=SDL_CONTROLLER_BUTTON_Y;bc::pc_settings_event(button,model);check(view()->modal==bc::PcSettingsModal::None,"physical Switch X remap cancels");
 button.cbutton.button=SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;bc::pc_settings_event(button,model);check(view()->section==bc::SettingsMenu::Page::Display,"shoulder advances tab with wrap");button.cbutton.button=SDL_CONTROLLER_BUTTON_LEFTSHOULDER;bc::pc_settings_event(button,model);check(view()->section==bc::SettingsMenu::Page::Controls,"opposite shoulder reverses tab");
 mouse(360,120,true);change=bc::take_pc_change();check(change&&change->field==bc::SettingsField::Family&&change->settings.family=="xbox","manual controller family remains available");auto confirm_bindings=model.bindings(bc::InputAction::Confirm),back_bindings=model.bindings(bc::InputAction::Back);
 bc::acknowledge_pc_change(change->settings,true);model.manual_family(bc::ControllerFamily::Xbox);
 check(model.bindings(bc::InputAction::Confirm)==confirm_bindings&&model.bindings(bc::InputAction::Back)==back_bindings,"applying manual family preserves physical bindings");
 check(view()->prompts[0].label=="X"&&view()->prompts[1].label=="Y","manual Xbox family relabels the same west and north bindings");
 mouse(40,390);button.cbutton.button=SDL_CONTROLLER_BUTTON_X;bc::pc_settings_event(button,model);check(view()->modal==bc::PcSettingsModal::RestoreDefaults,"same physical west binding confirms after changing family");button.cbutton.button=SDL_CONTROLLER_BUTTON_Y;bc::pc_settings_event(button,model);check(view()->modal==bc::PcSettingsModal::None,"same physical north binding cancels after changing family");
 mouse(510,75,true);key(SDL_SCANCODE_TAB,KMOD_SHIFT);check(view()->focus_target==bc::PcSettingsFocusTarget::Control&&view()->focus==1,"reverse tab reaches selector from first action");

 bc::initialize_pc_options({});bc::pc_settings_begin();
 SDL_Event resized{};resized.type=SDL_WINDOWEVENT;resized.window.event=SDL_WINDOWEVENT_SIZE_CHANGED;resized.window.data1=1280;resized.window.data2=720;bc::pc_settings_event(resized,model);
 mouse(250,295);check(view()->focus==1,"resize maps mouse into centered wide safe area");
 SDL_Event axis{};axis.type=SDL_CONTROLLERAXISMOTION;axis.caxis.axis=SDL_CONTROLLER_AXIS_LEFTY;axis.caxis.value=32767;bc::pc_settings_event(axis,model);check(view()->focus==2,"stick down navigates one row");bc::pc_settings_event(axis,model);check(view()->focus==2,"held stick does not repeatedly navigate on noise events");axis.caxis.value=0;bc::pc_settings_event(axis,model);axis.caxis.value=32767;bc::pc_settings_event(axis,model);check(view()->focus==3,"stick navigates again after neutral");

 bc::pc_settings_resize(640,480);mouse(510,75,true);mouse(40,390,true);key(SDL_SCANCODE_RETURN);key(SDL_SCANCODE_ESCAPE);bc::pc_settings_begin();key(SDL_SCANCODE_RIGHT);check(bc::take_pc_change().has_value(),"closing before defaults request is handled cannot leave next menu stuck saving");
 puts("PC settings input navigation and handoff passed");
}
