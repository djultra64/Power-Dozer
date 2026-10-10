#include "input_model.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <stdexcept>
namespace bc {
namespace {
constexpr float deadzone=.2f;
constexpr std::array<uint16_t,14> bits={0x8000,0x4000,0x1000,0x2000,2,4,8,1,0x20,0x10,0,0,0,0};
bool keyboard(InputBinding binding){return binding.kind==BindingKind::Key;}
float axis(float value){if(!std::isfinite(value))return 0;auto magnitude=std::abs(std::clamp(value,-1.f,1.f));return magnitude<=deadzone?0:std::copysign((magnitude-deadzone)/(1-deadzone),value);}
std::string key_name(int code){
 if(code==SDL_SCANCODE_RETURN)return "ENTER";if(code==SDL_SCANCODE_LSHIFT||code==SDL_SCANCODE_RSHIFT)return "SHIFT";
 std::string name=SDL_GetScancodeName(SDL_Scancode(code));if(name.empty())name="KEY "+std::to_string(code);
 std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::toupper(c);});return name;
}
}
InputModel::InputModel(){
 auto key=[&](InputAction action,int code){bindings_[size_t(action)].push_back({BindingKind::Key,code});};
 auto button=[&](InputAction action,int code){bindings_[size_t(action)].push_back({BindingKind::Button,code});};
 auto analog=[&](InputAction action,int code,int sign=1){bindings_[size_t(action)].push_back({BindingKind::Axis,code,sign});};
 constexpr std::array<int,14> keys={SDL_SCANCODE_Z,SDL_SCANCODE_X,SDL_SCANCODE_RETURN,SDL_SCANCODE_LSHIFT,SDL_SCANCODE_J,SDL_SCANCODE_K,SDL_SCANCODE_I,SDL_SCANCODE_L,SDL_SCANCODE_Q,SDL_SCANCODE_E,SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP,SDL_SCANCODE_DOWN};
 for(size_t i=0;i<keys.size();i++)key(InputAction(i),keys[i]);
 button(InputAction::Confirm,SDL_CONTROLLER_BUTTON_A);analog(InputAction::Confirm,SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
 button(InputAction::Back,SDL_CONTROLLER_BUTTON_B);button(InputAction::Start,SDL_CONTROLLER_BUTTON_START);
 analog(InputAction::Trigger,SDL_CONTROLLER_AXIS_TRIGGERLEFT);button(InputAction::Trigger,SDL_CONTROLLER_BUTTON_Y);
 button(InputAction::L,SDL_CONTROLLER_BUTTON_LEFTSHOULDER);button(InputAction::R,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
 analog(InputAction::CameraLeft,SDL_CONTROLLER_AXIS_RIGHTX,-1);analog(InputAction::CameraRight,SDL_CONTROLLER_AXIS_RIGHTX);
 analog(InputAction::CameraUp,SDL_CONTROLLER_AXIS_RIGHTY,-1);analog(InputAction::CameraDown,SDL_CONTROLLER_AXIS_RIGHTY);
 analog(InputAction::MoveLeft,SDL_CONTROLLER_AXIS_LEFTX,-1);button(InputAction::MoveLeft,SDL_CONTROLLER_BUTTON_DPAD_LEFT);
 analog(InputAction::MoveRight,SDL_CONTROLLER_AXIS_LEFTX);button(InputAction::MoveRight,SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
 analog(InputAction::MoveUp,SDL_CONTROLLER_AXIS_LEFTY,-1);button(InputAction::MoveUp,SDL_CONTROLLER_BUTTON_DPAD_UP);
 analog(InputAction::MoveDown,SDL_CONTROLLER_AXIS_LEFTY);button(InputAction::MoveDown,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
}
void InputModel::validate(InputBinding binding){
 const bool valid=binding.kind==BindingKind::Key?binding.code>SDL_SCANCODE_UNKNOWN&&binding.code<SDL_NUM_SCANCODES:
  binding.kind==BindingKind::Button?binding.code>=0&&binding.code<SDL_CONTROLLER_BUTTON_MAX:
  binding.kind==BindingKind::Axis&&binding.code>=0&&binding.code<SDL_CONTROLLER_AXIS_MAX;
 if(!valid||(binding.direction!=1&&binding.direction!=-1)||(binding.kind!=BindingKind::Axis&&binding.direction!=1))throw std::invalid_argument("Invalid input binding");
}
void InputModel::replace(InputAction action,InputSource source,const std::vector<InputBinding>& values){
 if(values.size()>4)throw std::invalid_argument("Too many input bindings");
 for(auto binding:values){validate(binding);if(keyboard(binding)!=(source==InputSource::Keyboard))throw std::invalid_argument("Binding does not match input device");}
 auto next=bindings_.at(size_t(action));std::erase_if(next,[&](auto binding){return keyboard(binding)==(source==InputSource::Keyboard);});
 next.insert(next.end(),values.begin(),values.end());bindings_.at(size_t(action))=std::move(next);
}
void InputModel::block_held(const InputSnapshot& snapshot){
 for(size_t i=0;i<blocked_.keys.size();i++)blocked_.keys[i]|=snapshot.keys[i];
 for(size_t i=0;i<blocked_.buttons.size();i++)blocked_.buttons[i]|=snapshot.buttons[i];
 for(size_t i=0;i<blocked_.axes.size();i++)if(axis(snapshot.axes[i])!=0)blocked_.axes[i]=1;
}
void InputModel::focus(bool focused,const InputSnapshot& snapshot){focused_=focused;block_held(snapshot);}
void InputModel::disconnect(const InputSnapshot& snapshot){block_held(snapshot);connected_=false;source_=InputSource::Keyboard;}
void InputModel::consume(InputBinding binding){validate(binding);if(binding.kind==BindingKind::Key)blocked_.keys[binding.code]=true;else if(binding.kind==BindingKind::Button)blocked_.buttons[binding.code]=true;else blocked_.axes[binding.code]=1;}
InputFrame InputModel::sample(const InputSnapshot& snapshot){
 for(size_t i=0;i<blocked_.keys.size();i++)if(!snapshot.keys[i])blocked_.keys[i]=false;
 for(size_t i=0;i<blocked_.buttons.size();i++)if(!snapshot.buttons[i])blocked_.buttons[i]=false;
 for(size_t i=0;i<blocked_.axes.size();i++)if(axis(snapshot.axes[i])==0)blocked_.axes[i]=0;
 if(!focused_){block_held(snapshot);return {};}
 std::array<float,size_t(InputAction::Count)> amounts{};
 for(size_t action=0;action<amounts.size();action++)for(auto binding:bindings_[action]){
  float value=0;
  if(binding.kind==BindingKind::Key&&!blocked_.keys[binding.code])value=snapshot.keys[binding.code];
  if(connected_&&binding.kind==BindingKind::Button&&!blocked_.buttons[binding.code])value=snapshot.buttons[binding.code];
  if(connected_&&binding.kind==BindingKind::Axis&&!blocked_.axes[binding.code])value=std::max(0.f,axis(snapshot.axes[binding.code])*binding.direction);
  amounts[action]=std::max(amounts[action],value);
 }
 InputFrame result;for(size_t i=0;i<bits.size();i++)if(amounts[i]>.25f)result.buttons|=bits[i];
 result.x=amounts[size_t(InputAction::MoveRight)]-amounts[size_t(InputAction::MoveLeft)];
 result.y=amounts[size_t(InputAction::MoveUp)]-amounts[size_t(InputAction::MoveDown)];return result;
}
InputPrompt InputModel::prompt(InputAction action) const{
 auto& list=bindings_.at(size_t(action));auto found=std::find_if(list.begin(),list.end(),[&](auto b){return keyboard(b)==(source_==InputSource::Keyboard);});
 if(found==list.end())return {"UNBOUND",InputGlyph::Other,source_};auto binding=*found;
 if(binding.kind==BindingKind::Key)return {key_name(binding.code),InputGlyph::Key,source_};
 auto f=family();
 if(binding.kind==BindingKind::Axis){
  auto code=binding.code;
  if(code>=SDL_CONTROLLER_AXIS_TRIGGERLEFT)return {f==ControllerFamily::PlayStation?(code==SDL_CONTROLLER_AXIS_TRIGGERLEFT?"L2":"R2"):f==ControllerFamily::Switch?(code==SDL_CONTROLLER_AXIS_TRIGGERLEFT?"ZL":"ZR"):(code==SDL_CONTROLLER_AXIS_TRIGGERLEFT?"LT":"RT"),InputGlyph::Trigger,source_};
  std::string label=code<=SDL_CONTROLLER_AXIS_LEFTY?"LS ":"RS ";label+=(code==SDL_CONTROLLER_AXIS_LEFTX||code==SDL_CONTROLLER_AXIS_RIGHTX)?(binding.direction>0?"RIGHT":"LEFT"):(binding.direction>0?"DOWN":"UP");return {label,InputGlyph::Stick,source_};
 }
 auto code=binding.code;
 if(code>=SDL_CONTROLLER_BUTTON_A&&code<=SDL_CONTROLLER_BUTTON_Y){
  constexpr std::array<const char*,4> xbox={"A","B","X","Y"},sw={"B","A","Y","X"},ps={"CROSS","CIRCLE","SQUARE","TRIANGLE"},unknown={"SOUTH","EAST","WEST","NORTH"};
  constexpr std::array<InputGlyph,4> positions={InputGlyph::South,InputGlyph::East,InputGlyph::West,InputGlyph::North},shapes={InputGlyph::Cross,InputGlyph::Circle,InputGlyph::Square,InputGlyph::Triangle};
  return {(f==ControllerFamily::Xbox?xbox:f==ControllerFamily::Switch?sw:f==ControllerFamily::PlayStation?ps:unknown)[code],f==ControllerFamily::PlayStation?shapes[code]:positions[code],source_};
 }
 if(code==SDL_CONTROLLER_BUTTON_START)return {f==ControllerFamily::Switch?"+":f==ControllerFamily::PlayStation?"OPTIONS":"MENU",InputGlyph::Menu,source_};
 if(code==SDL_CONTROLLER_BUTTON_BACK)return {f==ControllerFamily::Switch?"-":f==ControllerFamily::PlayStation?"SHARE":"VIEW",InputGlyph::Menu,source_};
 if(code==SDL_CONTROLLER_BUTTON_LEFTSHOULDER||code==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER){bool left=code==SDL_CONTROLLER_BUTTON_LEFTSHOULDER;return {f==ControllerFamily::Switch?(left?"L":"R"):f==ControllerFamily::PlayStation?(left?"L1":"R1"):(left?"LB":"RB"),InputGlyph::Shoulder,source_};}
 if(code>=SDL_CONTROLLER_BUTTON_DPAD_UP&&code<=SDL_CONTROLLER_BUTTON_DPAD_RIGHT){constexpr std::array<const char*,4> labels={"DPAD UP","DPAD DOWN","DPAD LEFT","DPAD RIGHT"};return {labels[code-SDL_CONTROLLER_BUTTON_DPAD_UP],InputGlyph::Dpad,source_};}
 std::string name=SDL_GameControllerGetStringForButton(SDL_GameControllerButton(code));std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::toupper(c);});return {name,InputGlyph::Other,source_};
}
}
