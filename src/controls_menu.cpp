#include "controls_menu.h"
#include <algorithm>
namespace bc {
bool ControlsMenu::interactive(unsigned row) const{
 if(page_==Page::Help)return row>=std::min<unsigned>(3,input_action_labels.size()-part_*3);
 if(page_==Page::Capture)return row==unsigned(source_==InputSource::Keyboard?1:2);
 return true;
}
std::string ControlsMenu::title(const InputModel& view) const{
 if(!status_.empty())return status_;
 switch(page_){
 case Page::Devices:return "REMAP CONTROLS";
 case Page::Actions:return std::string(source_==InputSource::Keyboard?"KEYBOARD ":"PAD ")+std::to_string(part_+1)+"/5";
 case Page::Capture:return std::string("REMAP ")+input_action_labels[size_t(action_)];
 case Page::Reset:return "RESET ALL CONTROLS?";
 case Page::Saving:return "SAVING CONTROLS";
 case Page::Help:
  if(view.source()==InputSource::Keyboard)return "KEYBOARD HELP";
  switch(view.family()){case ControllerFamily::Xbox:return "XBOX HELP";case ControllerFamily::PlayStation:return "PLAYSTATION HELP";case ControllerFamily::Switch:return "SWITCH HELP";default:return "PAD HELP";}
 case Page::Closed:return {};
 }
 return {};
}
std::vector<std::string> ControlsMenu::rows(const InputModel& view) const{
 switch(page_){
 case Page::Devices:return {"KEYBOARD","CONTROLLER","RESET ALL","BACK"};
 case Page::Actions:case Page::Help:{
  auto model=view;if(page_==Page::Actions)model.activity(source_);std::vector<std::string> rows;
  for(unsigned i=part_*3;i<std::min<unsigned>(part_*3+3,input_action_labels.size());i++)rows.push_back(std::string(input_action_labels[i])+": "+model.prompt(InputAction(i)).label);
  if(part_==4){rows.push_back("FIRST PAGE");rows.push_back("BACK");}else rows.push_back("NEXT PAGE");return rows;
 }
 case Page::Capture:
  if(source_==InputSource::Keyboard)return {"RELEASE THEN PRESS KEY","ESC: CANCEL"};
  return {"RELEASE THEN PRESS INPUT","MOVE STICKS FULLY",std::string("ESC / ")+(view.family()==ControllerFamily::PlayStation?"SHARE":view.family()==ControllerFamily::Switch?"-":"VIEW")+": CANCEL"};
 case Page::Reset:return {"NO","YES RESET ALL"};
 case Page::Saving:return {"PLEASE WAIT"};
 case Page::Closed:return {};
 }
 return {};
}
void ControlsMenu::select(int row){
 if(row<0||row>3||page_==Page::Saving||page_==Page::Closed)return;status_.clear();++revision_;
 switch(page_){
 case Page::Devices:if(row<2){source_=row==0?InputSource::Keyboard:InputSource::Controller;part_=0;page_=Page::Actions;}else page_=row==2?Page::Reset:Page::Closed;break;
 case Page::Actions:case Page::Help:{
  const auto count=std::min<unsigned>(3,input_action_labels.size()-part_*3);
  if(unsigned(row)<count){
   if(page_==Page::Help)return;action_=InputAction(part_*3+row);request_=RemapRequest{action_,source_};page_=Page::Capture;
  }else if(unsigned(row)==count)part_=part_==4?0:part_+1;
  else if(part_==4&&row==3)page_=page_==Page::Help?Page::Closed:Page::Devices;
  break;
 }
 case Page::Capture:if(row==(source_==InputSource::Keyboard?1:2))back();break;
 case Page::Reset:if(row==0)page_=Page::Devices;else if(row==1){request_=RemapRequest{InputAction::Confirm,InputSource::Keyboard,true};page_=Page::Saving;}break;
 default:break;
 }
}
void ControlsMenu::back(){
 if(page_==Page::Saving)return;status_.clear();++revision_;
 if(page_==Page::Capture){cancel_=true;request_.reset();page_=Page::Actions;}
 else if(page_==Page::Actions||page_==Page::Reset)page_=Page::Devices;
 else page_=Page::Closed;
}
void ControlsMenu::acknowledge(const RemapResult& result){
 if(page_!=Page::Capture&&page_!=Page::Saving)return;request_.reset();++revision_;
 const bool reset=page_==Page::Saving;page_=reset?Page::Devices:Page::Actions;
 status_=result.cancelled?"CANCELLED":result.status.empty()?(result.success?"SAVED":"SAVE FAILED"):result.status;
}
}
