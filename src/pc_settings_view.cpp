#include "pc_settings_view.h"
#include <algorithm>
namespace bc {
PcSettingsLayout pc_settings_layout(int width,int height){
 PcSettingsLayout out;
 const float scale=std::min(std::max(width,0)/640.0f,std::max(height,0)/480.0f);
 const float x=(std::max(width,0)-640*scale)/2,y=(std::max(height,0)-480*scale)/2;
 auto rect=[&](float rx,float ry,float rw,float rh){return PcSettingsRect{x+rx*scale,y+ry*scale,rw*scale,rh*scale};};
 out.safe_area=rect(0,0,640,480);
 out.title=rect(32,20,576,32);out.status=rect(32,410,576,24);
 out.prompts=rect(32,444,576,24);out.modal=rect(80,136,480,208);
 for(size_t i=0;i<4;i++){
  out.tabs[i]=rect(32+i*144,60,140,32);
  out.rows[i]=rect(40,124+i*58,560,44);
 }
 for(size_t i=0;i<14;i++)out.actions[i]=rect(32+(i/7)*296,154+(i%7)*32,280,28);
 out.controls[0]=rect(32,108,280,32);out.controls[1]=rect(328,108,280,32);
 out.controls[2]=rect(32,382,280,24);out.controls[3]=rect(328,382,280,24);
 return out;
}
std::optional<size_t> PcSettingsLayout::hit_action(float x,float y) const{
 for(size_t i=0;i<actions.size();i++)if(actions[i].contains(x,y))return i;
 return {};
}
PcSettingsSnapshot pc_settings_snapshot(const SettingsMenu& menu,const InputModel& input,const PcSettingsViewState& state){
 PcSettingsSnapshot out;
 out.section=menu.page();out.focus=state.focus;out.focus_target=state.focus_target;out.device=state.device;out.modal=state.modal;
 out.controller_family=input.family();
 out.saving=menu.saving();out.status=menu.status();
 out.device_label=state.device==InputSource::Keyboard?"KEYBOARD":"CONTROLLER";
 out.family_label=menu.value(SettingsField::Family);
 out.prompts={input.prompt(InputAction::Confirm),input.prompt(InputAction::Back)};
 switch(menu.page()){
 case SettingsMenu::Page::Display:out.title="DISPLAY";break;
 case SettingsMenu::Page::Graphics:out.title="GRAPHICS";break;
 case SettingsMenu::Page::Audio:out.title="AUDIO";break;
 case SettingsMenu::Page::Controls:out.title="CONTROLS";break;
 default:return out;
 }
 if(menu.page()==SettingsMenu::Page::Controls){
  out.controls={
   {"DEVICE",out.device_label,std::nullopt,std::nullopt,false,std::nullopt},
   {"CONTROLLER FAMILY",out.family_label,SettingsField::Family,std::nullopt,false,std::nullopt},
   {"RESTORE DEFAULTS","",std::nullopt,std::nullopt,false,std::nullopt},
   {state.origin==PcSettingsOrigin::MainMenu?"MAIN MENU":"GAME OPTIONS","",std::nullopt,std::nullopt,false,std::nullopt}
  };
  auto device_input=input;device_input.activity(state.device);
  for(size_t i=0;i<size_t(InputAction::Count);i++){
   const auto action=static_cast<InputAction>(i);const auto binding=device_input.prompt(action);
   out.rows.push_back({input_action_labels[i],binding.label,std::nullopt,action,false,binding});
  }
 }else{
  for(auto field:menu.fields()){
   const char* label="";
   switch(field){
   case SettingsField::Scale:label="INTERNAL RESOLUTION";break;
   case SettingsField::Aspect:label="ASPECT RATIO";break;
   case SettingsField::Refresh:label="FRAME RATE";break;
   case SettingsField::Fullscreen:label="WINDOW MODE";break;
   case SettingsField::MSAA:label="ANTI-ALIASING";break;
   case SettingsField::Presentation:label="SCREEN FILTER";break;
   case SettingsField::Texture:label="TEXTURE FILTER";break;
   case SettingsField::Volume:label="MASTER VOLUME";break;
   case SettingsField::Mute:label="MUTE";break;
   case SettingsField::Family:label="CONTROLLER FAMILY";break;
   }
   const bool restart=field==SettingsField::Scale||field==SettingsField::Aspect||field==SettingsField::Refresh||field==SettingsField::MSAA;
   out.rows.push_back({label,menu.value(field),field,std::nullopt,restart,std::nullopt});
  }
 }
 return out;
}
}
