#pragma once
#include "input_model.h"

namespace bc {
// Reference coordinates shared by draw data and SDL hit testing.
inline constexpr std::array<int,3> main_menu_row_y={195,251,307};
inline constexpr int main_menu_row_x=96,main_menu_row_width=448,main_menu_row_height=38;
enum class FrontendMenuScreen {Main,Files,Options};
enum class FrontendMenuAction {None,StartGame,PCOptions,CloseGame,BackToMain};
// Value-owned data: safe to publish as an immutable render snapshot.
struct MainMenuSnapshot {
 size_t selected=0;
 std::array<std::string,3> labels;
 InputPrompt confirm;
 ControllerFamily controller_family=ControllerFamily::Unknown;
 bool presentation_wide=false;
};
// Ownership transitions are applied by the host, never by a render snapshot.
class MainMenuState {
 size_t selected_=0;
 FrontendMenuScreen screen_=FrontendMenuScreen::Main;
 FrontendMenuAction action_=FrontendMenuAction::None;
public:
 void enter();
 void move(int);
 void select(size_t);
 void confirm();
 void open_files();
 void open_options();
 void return_main();
 FrontendMenuScreen screen() const{return screen_;}
 FrontendMenuAction take_action();
 friend MainMenuSnapshot main_menu_snapshot(const MainMenuState&,const InputModel&);
};
MainMenuSnapshot main_menu_snapshot(const MainMenuState&,const InputModel&);
}
