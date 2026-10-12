#pragma once
#include "input_model.h"

namespace bc {
enum class FrontendMenuScreen {Main,Files,Options};
enum class FrontendMenuAction {None,StartGame,PCOptions,CloseGame,BackToMain};
// Value-owned data: safe to publish as an immutable render snapshot.
struct MainMenuSnapshot {
 size_t selected=0;
 std::array<std::string,3> labels;
 InputPrompt confirm;
 ControllerFamily controller_family=ControllerFamily::Unknown;
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
