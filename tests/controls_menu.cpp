#include "controls_menu.h"
#include <stdexcept>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 bc::InputModel view;bc::ControlsMenu editor(false);check(editor.rows(view).size()==4,"device selector fits original layout");
 editor.select(1);check(editor.page()==bc::ControlsMenu::Page::Actions,"pad action list");check(editor.rows(view)[0].find("SOUTH")!=std::string::npos,"editor displays selected device even when keyboard is active");
 check(editor.prompts(view).size()==3&&editor.prompts(view)[0].source==bc::InputSource::Controller,"glyphs use the editor device with matching rows");
 editor.select(0);auto request=editor.take_request();check(request&&request->action==bc::InputAction::Confirm&&request->source==bc::InputSource::Controller,"selected logical action reaches capture request");check(editor.page()==bc::ControlsMenu::Page::Capture,"capture modal");
 view.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});view.manual_family(bc::ControllerFamily::Switch);
 editor.acknowledge({true,false,"SAVED"});check(editor.page()==bc::ControlsMenu::Page::Actions&&editor.rows(view)[0].find("Y")!=std::string::npos,"save acknowledgement returns to updated family-correct binding");
 editor.select(1);editor.acknowledge({false,false,"SAVE FAILED"});check(editor.status()=="SAVE FAILED"&&editor.page()==bc::ControlsMenu::Page::Actions,"failure preserves current action view");
 for(int i=0;i<4;i++){check(editor.rows(view).size()<=4,"action page row bound");editor.select(3);}check(editor.rows(view).size()==4,"last action page");editor.select(3);check(editor.page()==bc::ControlsMenu::Page::Devices,"last-page Back");
 editor.select(2);check(editor.page()==bc::ControlsMenu::Page::Reset,"reset requires confirmation");check(!editor.take_request(),"opening confirmation does not reset controls");editor.select(0);check(editor.page()==bc::ControlsMenu::Page::Devices,"No cancels reset");
 editor.select(2);editor.select(1);request=editor.take_request();check(request&&request->reset,"explicit Yes requests reset");editor.acknowledge({true,false,"RESET SAVED"});check(editor.page()==bc::ControlsMenu::Page::Devices,"reset acknowledgement returns devices");
 editor.select(0);editor.select(0);editor.back();check(editor.take_cancel_request()&&editor.page()==bc::ControlsMenu::Page::Actions,"Back cancels active capture");editor.back();editor.back();check(editor.closed(),"Back returns to parent Controls");
 bc::ControlsMenu help(true);view.activity(bc::InputSource::Controller);check(help.title(view)=="SWITCH HELP"&&help.rows(view)[0].find("Y")!=std::string::npos,"help follows remapped controller binding");view.activity(bc::InputSource::Keyboard);check(help.title(view)=="KEYBOARD HELP"&&help.rows(view)[0].find("Z")!=std::string::npos,"help follows last active keyboard");
 check(help.prompts(view)[0].source==bc::InputSource::Keyboard&&help.prompts(view)[0].glyph==bc::InputGlyph::Key,"help glyph and label follow the same last-active source");
 for(int i=0;i<4;i++)help.select(3);check(help.rows(view).size()==4,"help paginates all logical actions");help.select(3);check(help.closed(),"help Back returns to Controls");
}
