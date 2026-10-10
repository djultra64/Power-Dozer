#pragma once
#include <SDL.h>
#include <array>
#include <string>
#include <vector>
namespace bc {
enum class InputAction {Confirm,Back,Start,Trigger,CameraLeft,CameraDown,CameraUp,CameraRight,L,R,MoveLeft,MoveRight,MoveUp,MoveDown,Count};
enum class InputSource {Keyboard,Controller};
enum class ControllerFamily {Auto,Unknown,Xbox,PlayStation,Switch};
enum class BindingKind {Key,Button,Axis};
enum class InputGlyph {Key,South,East,West,North,Cross,Circle,Square,Triangle,Shoulder,Trigger,Stick,Dpad,Menu,Other};
struct InputBinding {BindingKind kind;int code;int direction=1;bool operator==(const InputBinding&) const=default;};
struct InputPrompt {std::string label;InputGlyph glyph=InputGlyph::Other;InputSource source;};
struct InputSnapshot {
 std::array<bool,SDL_NUM_SCANCODES> keys{};
 std::array<bool,SDL_CONTROLLER_BUTTON_MAX> buttons{};
 std::array<float,SDL_CONTROLLER_AXIS_MAX> axes{};
};
struct InputFrame {uint16_t buttons=0;float x=0,y=0;};
class InputModel {
 std::array<std::vector<InputBinding>,size_t(InputAction::Count)> bindings_;
 InputSnapshot blocked_;
 bool focused_=true,connected_=true;
 InputSource source_=InputSource::Keyboard;
 ControllerFamily detected_=ControllerFamily::Unknown,manual_=ControllerFamily::Auto;
 void block_held(const InputSnapshot&);
public:
 InputModel();
 InputFrame sample(const InputSnapshot&);
 void activity(InputSource source){source_=source;}
 void controller_family(ControllerFamily family){detected_=family;connected_=true;}
 void manual_family(ControllerFamily family){manual_=family;}
 ControllerFamily family() const{return manual_==ControllerFamily::Auto?detected_:manual_;}
 InputSource source() const{return source_;}
 void focus(bool,const InputSnapshot&);
 void disconnect(const InputSnapshot&);
 void consume(InputBinding);
 void replace(InputAction,InputSource,const std::vector<InputBinding>&);
 const std::vector<InputBinding>& bindings(InputAction action) const{return bindings_.at(size_t(action));}
 InputPrompt prompt(InputAction) const;
 static void validate(InputBinding);
};
}
