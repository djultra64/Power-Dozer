#include "input_model.h"
#include <cmath>
#include <stdexcept>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 bc::InputModel input;bc::InputSnapshot snapshot;
 snapshot.keys[SDL_SCANCODE_Z]=true;auto frame=input.sample(snapshot);check(frame.buttons==0x8000,"keyboard accelerate/confirm");
 snapshot={};snapshot.buttons[SDL_CONTROLLER_BUTTON_A]=true;input.activity(bc::InputSource::Controller);
 for(auto family:{bc::ControllerFamily::Xbox,bc::ControllerFamily::PlayStation,bc::ControllerFamily::Switch}){
  input.controller_family(family);frame=input.sample(snapshot);auto prompt=input.prompt(bc::InputAction::Confirm);
  check(frame.buttons==0x8000,"same physical south action on all families");
  check(prompt.label==(family==bc::ControllerFamily::Xbox?"A":family==bc::ControllerFamily::PlayStation?"CROSS":"B"),"family label matches actual south binding");
 }
 input.controller_family(bc::ControllerFamily::Switch);check(input.prompt(bc::InputAction::Back).label=="A","Switch east is A");
 input.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
 snapshot={};snapshot.buttons[SDL_CONTROLLER_BUTTON_X]=true;check(input.sample(snapshot).buttons==0x8000&&input.prompt(bc::InputAction::Confirm).label=="Y","remapped Switch west is Y and triggers confirm");
 snapshot={};snapshot.buttons[SDL_CONTROLLER_BUTTON_A]=true;check(input.sample(snapshot).buttons==0,"old pad binding is removed");
 input.controller_family(bc::ControllerFamily::Unknown);check(input.prompt(bc::InputAction::Confirm).label=="WEST","ambiguous controller uses physical position");
 input.manual_family(bc::ControllerFamily::PlayStation);check(input.prompt(bc::InputAction::Confirm).label=="SQUARE"&&input.prompt(bc::InputAction::Confirm).glyph==bc::InputGlyph::Square,"manual fallback updates name and glyph");
 input.activity(bc::InputSource::Keyboard);check(input.prompt(bc::InputAction::Confirm).label=="Z","last keyboard activity restores keyboard prompt without changing bindings");
 snapshot={};snapshot.buttons[SDL_CONTROLLER_BUTTON_DPAD_UP]=true;frame=input.sample(snapshot);check(frame.y==1&&frame.buttons==0,"D-pad navigates and moves without camera leakage");
 snapshot={};snapshot.axes[SDL_CONTROLLER_AXIS_RIGHTX]=-1;frame=input.sample(snapshot);check(frame.buttons==2&&frame.x==0,"right stick drives C-left camera only");
 snapshot={};snapshot.axes[SDL_CONTROLLER_AXIS_LEFTX]=.1;check(input.sample(snapshot).x==0,"analog dead zone");
 snapshot.axes[SDL_CONTROLLER_AXIS_LEFTX]=1;check(input.sample(snapshot).x==1,"full analog range");
 snapshot.axes[SDL_CONTROLLER_AXIS_LEFTX]=NAN;check(input.sample(snapshot).x==0,"nonfinite axis cannot enter guest state");
 snapshot={};snapshot.keys[SDL_SCANCODE_Z]=true;snapshot.axes[SDL_CONTROLLER_AXIS_LEFTX]=1;input.focus(false,snapshot);frame=input.sample(snapshot);check(frame.buttons==0&&frame.x==0,"focus loss clears all input");
 input.focus(true,snapshot);frame=input.sample(snapshot);check(frame.buttons==0&&frame.x==0,"held controls stay blocked on focus regain");
 snapshot={};input.sample(snapshot);snapshot.keys[SDL_SCANCODE_Z]=true;snapshot.axes[SDL_CONTROLLER_AXIS_LEFTX]=1;frame=input.sample(snapshot);check(frame.buttons==0x8000&&frame.x==1,"released controls work on next press");
 snapshot={};snapshot.keys[SDL_SCANCODE_RETURN]=true;input.consume({bc::BindingKind::Key,SDL_SCANCODE_RETURN});check(input.sample(snapshot).buttons==0,"fullscreen shortcut does not leak Start");
 input.sample(snapshot);check(input.sample(snapshot).buttons==0,"held shortcut remains suppressed");snapshot={};input.sample(snapshot);snapshot.keys[SDL_SCANCODE_RETURN]=true;check(input.sample(snapshot).buttons==0x1000,"Start works after shortcut release");
 snapshot={};snapshot.buttons[SDL_CONTROLLER_BUTTON_X]=true;input.disconnect(snapshot);check(input.sample(snapshot).buttons==0&&input.prompt(bc::InputAction::Confirm).label=="Z","disconnect clears pad state and restores keyboard prompts");
 input.controller_family(bc::ControllerFamily::Xbox);
 bool refused=false;try{input.replace(bc::InputAction::Back,bc::InputSource::Controller,{{bc::BindingKind::Button,999}});}catch(const std::exception&){refused=true;}check(refused,"invalid remap is rejected");
 snapshot={};snapshot.buttons[SDL_CONTROLLER_BUTTON_B]=true;check(input.sample(snapshot).buttons==0x4000,"failed remap keeps old binding");
 bc::InputModel names;names.activity(bc::InputSource::Controller);
 struct Layout {SDL_GameControllerType type;const char* start;const char* back;};
 for(auto layout:{Layout{SDL_CONTROLLER_TYPE_XBOX360,"START","BACK"},Layout{SDL_CONTROLLER_TYPE_XBOXONE,"MENU","VIEW"},Layout{SDL_CONTROLLER_TYPE_PS3,"START","SELECT"},Layout{SDL_CONTROLLER_TYPE_PS4,"OPTIONS","SHARE"},Layout{SDL_CONTROLLER_TYPE_PS5,"OPTIONS","CREATE"},Layout{SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO,"PLUS","-"}}){
  auto identity=bc::controller_identity(layout.type,"unreliable name");names.controller_identity(identity);
  names.replace(bc::InputAction::Back,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_BACK}});
  check(names.prompt(bc::InputAction::Start).label==layout.start&&names.prompt(bc::InputAction::Back).label==layout.back,"generation-specific menu names");
  names.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_LEFTSTICK}});
  auto click=names.prompt(bc::InputAction::Confirm);check(click.glyph==bc::InputGlyph::StickPress,"stick press is distinct from stick direction");
  check(click.label==(identity.family==bc::ControllerFamily::PlayStation?"L3":"LS CLICK"),"stick click uses physical family name");
 }
 names.controller_identity(bc::controller_identity(SDL_CONTROLLER_TYPE_UNKNOWN,"QA DualSense PS5"));
 names.manual_family(bc::ControllerFamily::PlayStation);check(names.prompt(bc::InputAction::Back).label=="CREATE","same-family manual override preserves reliable generation");
 names.manual_family(bc::ControllerFamily::Xbox);check(names.prompt(bc::InputAction::Back).label=="VIEW","cross-family override discards the foreign generation");
 names.manual_family(bc::ControllerFamily::Auto);
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_UNKNOWN,"Microsoft X-Box 360 pad").type==SDL_CONTROLLER_TYPE_XBOX360,"Linux hyphenated Xbox name fallback");
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_UNKNOWN,"DualShock 3").type==SDL_CONTROLLER_TYPE_PS3,"PS3 name fallback");
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_UNKNOWN,"Wireless PS4 controller").type==SDL_CONTROLLER_TYPE_PS4,"PS4 name fallback");
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_PS5,"Xbox").family==bc::ControllerFamily::PlayStation,"reliable SDL type takes priority over misleading name");
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_UNKNOWN,"PLAYSTATION(R)3 Controller").type==SDL_CONTROLLER_TYPE_PS3,"registered-mark Linux PS3 name fallback");
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR,"").family==bc::ControllerFamily::Switch,"paired Joy-Con has normal Switch layout");
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT,"Nintendo Switch").family==bc::ControllerFamily::Unknown,"single Joy-Con must not promise Pro letters with unknown orientation");
 check(bc::controller_identity(SDL_CONTROLLER_TYPE_UNKNOWN,"Nintendo Switch Joy-Con (L)").family==bc::ControllerFamily::Unknown,"single Joy-Con name also keeps position prompts");
 names.activity(bc::InputSource::Keyboard);
 check(names.physical_prompt({bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_BACK}).label=="CREATE","reserved physical control lookup is independent of last-active source and action remaps");
}
