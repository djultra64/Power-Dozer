#include "main_menu.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <tuple>

static void check(bool value,const char* message){
 if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}

int main(){
 using namespace bc;
 InputModel input;
 MainMenuState menu;
 auto snapshot=main_menu_snapshot(menu,input);
 check(snapshot.selected==0&&snapshot.labels==std::array<std::string,3>{"Start Game","PC Options","Close Game"},"fresh main selects Start Game among exactly three entries");
 check(menu.screen()==FrontendMenuScreen::Main&&menu.take_action()==FrontendMenuAction::None,"fresh main has no transition request");
 for(auto [row,action]:{std::pair{size_t(0),FrontendMenuAction::StartGame},{1,FrontendMenuAction::PCOptions},{2,FrontendMenuAction::CloseGame}}){
  menu.select(row);menu.confirm();
  check(menu.screen()==FrontendMenuScreen::Main,"confirm only requests a transition for its owner");
  check(menu.take_action()==action&&menu.take_action()==FrontendMenuAction::None,"selected row requests its exact action once");
 }
 menu.return_main();
 check(menu.screen()==FrontendMenuScreen::Main&&menu.take_action()==FrontendMenuAction::None,"Back on main never requests exit or another screen");
 menu.confirm();menu.return_main();check(menu.take_action()==FrontendMenuAction::None,"main return clears a queued close request");
 menu.select(0);menu.confirm();menu.select(2);menu.confirm();
 check(menu.take_action()==FrontendMenuAction::StartGame&&menu.take_action()==FrontendMenuAction::None,"one queued request cannot be overwritten by repeated confirm");
 menu.enter();menu.move(-1);check(main_menu_snapshot(menu,input).selected==2,"up wraps to Close Game");
 menu.move(1);check(main_menu_snapshot(menu,input).selected==0,"down wraps to Start Game");
 menu.move(0);menu.select(99);check(main_menu_snapshot(menu,input).selected==0,"zero movement and invalid mouse row are inert");
 menu.move(std::numeric_limits<int>::min());check(main_menu_snapshot(menu,input).selected==1,"large negative movement remains bounded");
 menu.select(1);menu.confirm();menu.open_options();
 check(menu.screen()==FrontendMenuScreen::Options&&menu.take_action()==FrontendMenuAction::None,"opening options consumes stale transition request");
 menu.confirm();menu.move(1);menu.select(2);
 check(menu.take_action()==FrontendMenuAction::None,"hidden main cannot issue actions in options");
 menu.return_main();check(main_menu_snapshot(menu,input).selected==1,"options return keeps PC Options selected");
 menu.select(0);menu.confirm();menu.open_files();menu.confirm();
 check(menu.screen()==FrontendMenuScreen::Files&&menu.take_action()==FrontendMenuAction::None,"hidden main cannot confirm a file");
 menu.return_main();check(main_menu_snapshot(menu,input).selected==0,"files return preserves Start Game selection");
 menu.select(2);menu.confirm();menu.enter();
 check(main_menu_snapshot(menu,input).selected==0&&menu.take_action()==FrontendMenuAction::None,"fresh entry resets selection and stale exit request");
 check(main_menu_snapshot(menu,input).confirm.label=="Z","keyboard confirm prompt matches default action binding");
 input.activity(InputSource::Controller);
 for(auto [family,label,glyph]:{std::tuple{ControllerFamily::Xbox,"A",InputGlyph::South},{ControllerFamily::PlayStation,"CROSS",InputGlyph::Cross},{ControllerFamily::Switch,"B",InputGlyph::South}}){
  input.manual_family(family);auto prompt=main_menu_snapshot(menu,input).confirm;
  check(prompt.label==label&&prompt.glyph==glyph&&prompt.source==InputSource::Controller,"family prompt names the physical south Confirm binding");
 }
 input.replace(InputAction::Confirm,InputSource::Controller,{{BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
 auto retained=main_menu_snapshot(menu,input);
 check(retained.confirm.label=="Y"&&retained.confirm.glyph==InputGlyph::West,"Switch remap exposes the actual west Y button");
 input.manual_family(ControllerFamily::Xbox);menu.move(1);
 check(retained.selected==0&&retained.confirm.label=="Y"&&main_menu_snapshot(menu,input).confirm.label=="X","published snapshot owns selection labels and prompt");
 puts("Main menu state, request routing and prompts passed");
}
