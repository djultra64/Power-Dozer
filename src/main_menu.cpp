#include "main_menu.h"
namespace bc {
void MainMenuState::enter(){selected_=0;return_main();}
void MainMenuState::move(int direction){
 if(screen_!=FrontendMenuScreen::Main)return;
 const int next=int(selected_)+direction%3;
 selected_=size_t((next+3)%3);
}
void MainMenuState::select(size_t row){if(screen_==FrontendMenuScreen::Main&&row<3)selected_=row;}
void MainMenuState::confirm(){
 if(screen_!=FrontendMenuScreen::Main||action_!=FrontendMenuAction::None)return;
 constexpr std::array actions={FrontendMenuAction::StartGame,FrontendMenuAction::PCOptions,FrontendMenuAction::CloseGame};
 action_=actions[selected_];
}
void MainMenuState::open_files(){screen_=FrontendMenuScreen::Files;action_=FrontendMenuAction::None;}
void MainMenuState::open_options(){screen_=FrontendMenuScreen::Options;action_=FrontendMenuAction::None;}
void MainMenuState::return_main(){screen_=FrontendMenuScreen::Main;action_=FrontendMenuAction::None;}
FrontendMenuAction MainMenuState::take_action(){auto out=action_;action_=FrontendMenuAction::None;return out;}
MainMenuSnapshot main_menu_snapshot(const MainMenuState& state,const InputModel& input){
 return {state.selected_,{"Start Game","PC Options","Close Game"},input.prompt(InputAction::Confirm)};
}
}
