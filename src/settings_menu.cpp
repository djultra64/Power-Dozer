#include "settings_menu.h"
#include "input_model.h"
#include <array>
#include <algorithm>
namespace bc {
Settings merge_setting(const Settings& saved,const SettingsChange& change){
 auto out=saved;auto& s=change.settings;
 switch(change.field){
 case SettingsField::Scale:out.scale=s.scale;break;
 case SettingsField::Aspect:out.wide=s.wide;break;
 case SettingsField::Refresh:out.refresh=s.refresh;break;
 case SettingsField::Fullscreen:out.fullscreen=s.fullscreen;break;
 case SettingsField::MSAA:out.msaa=s.msaa;break;
 case SettingsField::Presentation:out.presentation=s.presentation;break;
 case SettingsField::Texture:out.texture=s.texture;break;
 case SettingsField::Volume:out.volume=s.volume;break;
 case SettingsField::Mute:out.mute=s.mute;break;
 case SettingsField::Family:out.family=s.family;break;
 }
 validate(out);return out;
}
std::string SettingsMenu::value(SettingsField field) const{
 switch(field){
 case SettingsField::Scale:
  if(settings_.scale==1)return "240P";if(settings_.scale==3)return "720P";if(settings_.scale==4.5)return "1080P";if(settings_.scale==6)return "1440P";if(settings_.scale==9)return "2160P";return "CUSTOM";
 case SettingsField::Aspect:return settings_.wide?"16:9":"4:3";
 case SettingsField::Refresh:return settings_.refresh?std::to_string(settings_.refresh):"ORIGINAL";
 case SettingsField::Fullscreen:return settings_.fullscreen?"FULL":"WINDOW";
 case SettingsField::MSAA:return settings_.msaa?std::to_string(settings_.msaa)+"X":"OFF";
 case SettingsField::Presentation:return settings_.presentation=="nearest"?"NEAREST":settings_.presentation=="pixel"?"PIXEL":"LINEAR";
 case SettingsField::Texture:return settings_.texture=="three_point"?"3-POINT":"LINEAR";
 case SettingsField::Volume:return std::to_string(settings_.volume)+"%";
 case SettingsField::Mute:return settings_.mute?"ON":"OFF";
 case SettingsField::Family:return settings_.family=="auto"?"AUTO":settings_.family=="xbox"?"XBOX":settings_.family=="switch"?"SWITCH":"PLAYSTATION";
 }
 return {};
}
std::vector<std::string> SettingsMenu::rows() const{
 auto label=[&](const char* name,SettingsField field){return std::string(name)+": "+value(field);};
 if(section_mode_){
  if(page_==Page::Controls)return {input_action_labels.begin(),input_action_labels.end()};
  std::vector<std::string> out;
  for(auto field:fields()){
   const char* name=field==SettingsField::Scale?"RES":field==SettingsField::Aspect?"ASPECT":field==SettingsField::Refresh?"FPS":field==SettingsField::Fullscreen?"WINDOW":field==SettingsField::MSAA?"MSAA":field==SettingsField::Presentation?"FILTER":field==SettingsField::Texture?"TEXTURE":field==SettingsField::Volume?"VOLUME":"MUTE";
   out.push_back(label(name,field));
  }
  return out;
 }
 switch(page_){
 case Page::Display:case Page::Graphics:return {};
 case Page::Root:return {"GRAPHICS","AUDIO","CONTROLS","BACK"};
 case Page::Graphics1:return {label("RES",SettingsField::Scale),label("ASPECT",SettingsField::Aspect),label("FPS",SettingsField::Refresh),"MORE"};
 case Page::Graphics2:return {label("WINDOW",SettingsField::Fullscreen),label("MSAA",SettingsField::MSAA),label("FILTER",SettingsField::Presentation),"MORE"};
 case Page::Graphics3:return {label("TEXTURE",SettingsField::Texture),"PREVIOUS","BACK"};
 case Page::Audio:return {label("VOLUME",SettingsField::Volume),label("MUTE",SettingsField::Mute),"BACK"};
 case Page::Controls:return {label("FAMILY",SettingsField::Family),"REMAP","HELP","GAME OPTIONS"};
 case Page::Closed:return {};
 }
 return {};
}
template<class T,size_t N>void cycle(T& current,const std::array<T,N>& values,int direction){
 auto i=std::find(values.begin(),values.end(),current);
 if(i==values.end()){current=direction>0?values.front():values.back();return;}
 const auto index=static_cast<int>(i-values.begin());
 current=values[(index+(direction>0?1:int(N)-1))%N];
}
void SettingsMenu::section(Page page){
 if(page!=Page::Display&&page!=Page::Graphics&&page!=Page::Audio&&page!=Page::Controls)return;
 page_=page;section_mode_=true;
}
std::vector<SettingsField> SettingsMenu::fields() const{
 switch(page_){
 case Page::Display:return {SettingsField::Scale,SettingsField::Aspect,SettingsField::Refresh,SettingsField::Fullscreen};
 case Page::Graphics:return {SettingsField::MSAA,SettingsField::Presentation,SettingsField::Texture};
 case Page::Audio:return {SettingsField::Volume,SettingsField::Mute};
 default:return {};
 }
}
void SettingsMenu::set_volume(int value){
 value=std::clamp(value,0,100);
 if(saving_||closed()||settings_.volume==value)return;
 settings_.volume=value;last_field_=SettingsField::Volume;
 change_=SettingsChange{settings_,last_field_};saving_=true;status_="SAVING";
}
void SettingsMenu::adjust(SettingsField field,int direction){
 if(saving_||!direction||closed())return;
 const auto before=value(field);
 switch(field){
 case SettingsField::Scale:cycle(settings_.scale,std::array<double,5>{1,3,4.5,6,9},direction);break;
 case SettingsField::Aspect:settings_.wide=!settings_.wide;break;
 case SettingsField::Refresh:cycle(settings_.refresh,std::array<int,4>{0,30,60,120},direction);break;
 case SettingsField::Fullscreen:settings_.fullscreen=!settings_.fullscreen;break;
 case SettingsField::MSAA:cycle(settings_.msaa,std::array<int,4>{0,2,4,8},direction);break;
 case SettingsField::Presentation:cycle(settings_.presentation,std::array<std::string,3>{"nearest","linear","pixel"},direction);break;
 case SettingsField::Texture:settings_.texture=settings_.texture=="linear"?"three_point":"linear";break;
 case SettingsField::Volume:set_volume(direction>0?(settings_.volume/10+1)*10:((settings_.volume+9)/10-1)*10);return;
 case SettingsField::Mute:settings_.mute=!settings_.mute;break;
 case SettingsField::Family:cycle(settings_.family,std::array<std::string,4>{"auto","xbox","playstation","switch"},direction);break;
 }
 if(value(field)==before)return;
 last_field_=field;change_=SettingsChange{settings_,field};saving_=true;status_="SAVING";
}
void SettingsMenu::select(int row){
 if(saving_||row<0||size_t(row)>=rows().size())return;
 status_.clear();
 if(section_mode_){auto list=fields();if(size_t(row)<list.size())adjust(list[row],1);return;}
 switch(page_){
 case Page::Display:case Page::Graphics:break;
 case Page::Root:page_=row==0?Page::Graphics1:row==1?Page::Audio:row==2?Page::Controls:Page::Closed;break;
 case Page::Graphics1:if(row==3)page_=Page::Graphics2;else adjust(static_cast<SettingsField>(row));break;
 case Page::Graphics2:if(row==3)page_=Page::Graphics3;else adjust(static_cast<SettingsField>(row+3));break;
 case Page::Graphics3:if(row==0)adjust(SettingsField::Texture);else page_=row==1?Page::Graphics2:Page::Root;break;
 case Page::Audio:if(row==2)page_=Page::Root;else adjust(row==0?SettingsField::Volume:SettingsField::Mute);break;
 case Page::Controls:if(row==0)adjust(SettingsField::Family);else if(row==1)controls_=true;else if(row==2)help_=true;else {game_options_=true;page_=Page::Closed;}break;
 case Page::Closed:break;
 }
}
void SettingsMenu::back(){if(saving_)return;status_.clear();if(section_mode_){page_=Page::Closed;return;}page_=page_==Page::Root?Page::Closed:page_==Page::Graphics3?Page::Graphics2:page_==Page::Graphics2?Page::Graphics1:Page::Root;}
void SettingsMenu::acknowledge(const Settings& saved,bool success){settings_=saved;saving_=false;change_.reset();const bool restart=last_field_==SettingsField::Scale||last_field_==SettingsField::Aspect||last_field_==SettingsField::Refresh||last_field_==SettingsField::MSAA;status_=success?(restart?"SAVED RESTART":"SAVED"):"SAVE FAILED";}
}
