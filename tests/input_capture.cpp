#include "input_capture.h"
#include <stdexcept>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 bc::InputCapture capture;bc::InputSnapshot snapshot;snapshot.keys[SDL_SCANCODE_Z]=true;
 capture.begin({bc::InputAction::Confirm,bc::InputSource::Keyboard},snapshot);
 check(capture.active()&&!capture.ready(),"opening button must release before capture");
 capture.press({bc::BindingKind::Key,SDL_SCANCODE_Z});check(!capture.take_binding(),"opening key is not remapped immediately");
 snapshot={};capture.update(snapshot);check(capture.ready(),"release arms capture");
 capture.press({bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X});check(!capture.take_binding(),"pad does not bind keyboard action");
 capture.press({bc::BindingKind::Key,SDL_SCANCODE_T});auto result=capture.take_binding();check(result&&result->code==SDL_SCANCODE_T&&!capture.active(),"fresh keyboard press completes capture");
 snapshot.buttons[SDL_CONTROLLER_BUTTON_A]=true;capture.begin({bc::InputAction::Back,bc::InputSource::Controller},snapshot);capture.update(snapshot);check(!capture.ready(),"held pad opening button must release");
 snapshot={};capture.update(snapshot);capture.press({bc::BindingKind::Key,SDL_SCANCODE_Q});check(!capture.take_binding(),"keyboard does not bind pad action");
 capture.press({bc::BindingKind::Axis,SDL_CONTROLLER_AXIS_RIGHTX,-1});result=capture.take_binding();check(result&&result->direction==-1,"signed axis capture");
 capture.begin({bc::InputAction::Back,bc::InputSource::Controller},snapshot);capture.press({bc::BindingKind::Key,SDL_SCANCODE_ESCAPE});check(capture.take_cancelled()&&!capture.active(),"Escape cancels either device without binding");
 capture.begin({bc::InputAction::Start,bc::InputSource::Keyboard},snapshot);capture.cancel();check(!capture.active()&&!capture.take_binding(),"closing the parent editor ends capture");
 capture.begin({bc::InputAction::Back,bc::InputSource::Controller},snapshot);capture.press({bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_BACK});check(capture.take_cancelled(),"physical View/Share/minus cancels pad capture");
}
