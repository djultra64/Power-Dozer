#include "input_capture.h"
#include <algorithm>
#include <cmath>
namespace bc {
void InputCapture::begin(RemapRequest request,const InputSnapshot& snapshot){request_=request;active_=true;ready_=cancelled_=false;binding_.reset();update(snapshot);}
void InputCapture::update(const InputSnapshot& snapshot){
 if(!active_||ready_)return;
 if(request_.source==InputSource::Keyboard)ready_=std::none_of(snapshot.keys.begin(),snapshot.keys.end(),[](bool down){return down;});
 else ready_=std::none_of(snapshot.buttons.begin(),snapshot.buttons.end(),[](bool down){return down;})&&std::none_of(snapshot.axes.begin(),snapshot.axes.end(),[](float axis){return std::isfinite(axis)&&std::abs(axis)>.2f;});
}
void InputCapture::press(InputBinding binding){
 if(!active_)return;
 if((binding.kind==BindingKind::Key&&binding.code==SDL_SCANCODE_ESCAPE)||(request_.source==InputSource::Controller&&binding.kind==BindingKind::Button&&binding.code==SDL_CONTROLLER_BUTTON_BACK)){
  active_=ready_=false;cancelled_=true;binding_.reset();return;
 }
 if(!ready_||(binding.kind==BindingKind::Key)!=(request_.source==InputSource::Keyboard))return;
 InputModel::validate(binding);binding_=binding;active_=ready_=false;
}
}
