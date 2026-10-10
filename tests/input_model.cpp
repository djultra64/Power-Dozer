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
}
