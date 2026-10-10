#pragma once
#include <SDL.h>
#include <array>
#include <string>
#include <vector>
#include <string_view>
namespace bc {
enum class InputAction {Confirm,Back,Start,Trigger,CameraLeft,CameraDown,CameraUp,CameraRight,L,R,MoveLeft,MoveRight,MoveUp,MoveDown,Count};
inline constexpr std::array<const char*,14> input_action_keys={"confirm","back","start","trigger","camera_left","camera_down","camera_up","camera_right","l","r","move_left","move_right","move_up","move_down"};
inline constexpr std::array<const char*,14> input_action_labels={"ACCEL/OK","BACK/REVERSE","PAUSE","BRAKE/EXIT","CAM LEFT","CAM DOWN","CAM UP","CAM RIGHT","SPECIAL L","SPECIAL R","MOVE LEFT","MOVE RIGHT","MOVE UP","MOVE DOWN"};
enum class InputSource {Keyboard,Controller};
enum class ControllerFamily {Auto,Unknown,Xbox,PlayStation,Switch};
struct ControllerIdentity {ControllerFamily family=ControllerFamily::Unknown;SDL_GameControllerType type=SDL_CONTROLLER_TYPE_UNKNOWN;};
ControllerIdentity controller_identity(SDL_GameControllerType,std::string_view);
enum class BindingKind {Key,Button,Axis};
enum class InputGlyph {Key,South,East,West,North,Cross,Circle,Square,Triangle,Shoulder,Trigger,Stick,StickPress,Dpad,Menu,Other};
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
 SDL_GameControllerType type_=SDL_CONTROLLER_TYPE_UNKNOWN;
 void block_held(const InputSnapshot&);
public:
 InputModel();
 InputFrame sample(const InputSnapshot&);
 void activity(InputSource source){source_=source;}
 void controller_family(ControllerFamily family){controller_identity({family,SDL_CONTROLLER_TYPE_UNKNOWN});}
 void controller_identity(ControllerIdentity identity){detected_=identity.family;type_=identity.type;connected_=true;}
 void manual_family(ControllerFamily family){manual_=family;}
 ControllerFamily family() const{return manual_==ControllerFamily::Auto?detected_:manual_;}
 SDL_GameControllerType controller_type() const{return manual_==ControllerFamily::Auto||manual_==detected_?type_:SDL_CONTROLLER_TYPE_UNKNOWN;}
 InputSource source() const{return source_;}
 void focus(bool,const InputSnapshot&);
 void disconnect(const InputSnapshot&);
 void consume(InputBinding);
 void replace(InputAction,InputSource,const std::vector<InputBinding>&);
 const std::vector<InputBinding>& bindings(InputAction action) const{return bindings_.at(size_t(action));}
 InputPrompt prompt(InputAction) const;
 InputPrompt physical_prompt(InputBinding) const;
 static void validate(InputBinding);
};
}
