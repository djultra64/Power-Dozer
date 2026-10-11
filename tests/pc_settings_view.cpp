#include "pc_settings_view.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <set>

static void check(bool value,const char* message){
 if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}

int main(){
 using namespace bc;
 Settings initial;initial.volume=75;
 SettingsMenu menu(initial);
 InputModel input;
 PcSettingsViewState state;
 for(auto [section,count]:{
  std::pair{SettingsMenu::Page::Display,4},
  {SettingsMenu::Page::Graphics,3},
  {SettingsMenu::Page::Audio,2},
  {SettingsMenu::Page::Controls,14}
 }){
  menu.section(section);
  auto snapshot=pc_settings_snapshot(menu,input,state);
  check(snapshot.rows.size()==size_t(count),"all section rows visible");
  check(menu.rows().size()==size_t(count),"section has no pagination/back row");
  check(snapshot.title!="SAVING"&&snapshot.section==section,"title independent of status");
  if(section==SettingsMenu::Page::Controls){
   check(snapshot.controls.size()==4,
    "device, family, defaults and game options available without paging");
   check(snapshot.controls[1].field==SettingsField::Family,
    "family selector exposes adjustment field");
   std::set<InputAction> actions;
   for(const auto& row:snapshot.rows){
    check(row.action.has_value(),"control row identifies logical action");
    actions.insert(*row.action);
    check(!row.value.empty(),"action binding visible");
   }
   check(actions.size()==14,"fourteen unique actions reachable");
  }
 }
 for(auto [width,height]:{
  std::pair{960,720},{1280,720},{320,240},{400,720},{0,0},{-10,720}
 }){
  auto layout=pc_settings_layout(width,height);
  auto contains=[&](PcSettingsRect r){
   return r.width>=0&&r.height>=0&&r.x>=layout.safe_area.x&&r.y>=layout.safe_area.y&&
    r.x+r.width<=layout.safe_area.x+layout.safe_area.width+0.01f&&
    r.y+r.height<=layout.safe_area.y+layout.safe_area.height+0.01f;
  };
  for(auto r:layout.tabs)check(contains(r),"tab inside safe area");
  for(auto r:layout.rows)check(contains(r),"setting row inside safe area");
  for(auto r:layout.actions)check(contains(r),"action row inside safe area");
  for(auto r:layout.controls)check(contains(r),"control inside safe area");
  check(contains(layout.title)&&contains(layout.status)&&contains(layout.prompts)&&
   contains(layout.modal),"chrome inside safe area");
  if(width>0&&height>0){
   const float scale=std::min(width/640.0f,height/480.0f);
   check(layout.safe_area.width==640*scale&&layout.safe_area.height==480*scale,
    "safe area preserves aspect ratio");
   check(layout.safe_area.x==(width-layout.safe_area.width)/2&&
    layout.safe_area.y==(height-layout.safe_area.height)/2,"safe area centered");
   check(layout.actions[0].x<layout.actions[7].x,"two action columns");
   check(layout.actions[0].y==layout.actions[7].y,"seven rows per column");
   for(size_t i=0;i<14;i++){
    auto r=layout.actions[i];
    check(layout.hit_action(r.x+r.width/2,r.y+r.height/2)==i,
     "each action has distinct clickable target");
   }
   check(!layout.hit_action(-1,-1),"outside hit rejected");
  }
 }
 menu.section(SettingsMenu::Page::Display);
 state.focus=2;
 menu.adjust(SettingsField::Refresh,-1);
 auto change=menu.take_change();
 check(change&&change->settings.refresh==60,"left changes previous value");
 check(pc_settings_snapshot(menu,input,state).saving,"snapshot shows saving");
 menu.acknowledge(initial,false);
 auto failed=pc_settings_snapshot(menu,input,state);
 check(failed.focus==2&&failed.rows[2].value=="120"&&failed.status=="SAVE FAILED",
  "failure restores saved value and focus");
 menu.adjust(SettingsField::Refresh,1);
 change=menu.take_change();
 check(change&&change->settings.refresh==0,"right wraps supported values");
 menu.acknowledge(change->settings,true);
 check(pc_settings_snapshot(menu,input,state).rows[2].restart_required,
  "restart metadata available");
 menu.section(SettingsMenu::Page::Audio);
 menu.adjust(SettingsField::Volume,-1);
 change=menu.take_change();
 check(change&&change->settings.volume==70,"volume decreases to previous step");
 menu.acknowledge(change->settings,true);
 for(int i=0;i<20;i++){
  menu.adjust(SettingsField::Volume,-1);change=menu.take_change();
  if(change)menu.acknowledge(change->settings,true);
 }
 check(menu.value(SettingsField::Volume)=="0%","volume lower bound");
 menu.adjust(SettingsField::Volume,0);
 check(!menu.take_change()&&!menu.saving(),"zero direction is inert");
 for(int i=0;i<20;i++){
  menu.adjust(SettingsField::Volume,1);change=menu.take_change();
  if(change)menu.acknowledge(change->settings,true);
 }
 check(menu.value(SettingsField::Volume)=="100%","volume upper bound");
 menu.adjust(SettingsField::Volume,1);
 check(!menu.take_change()&&!menu.saving(),"endpoint does not issue redundant save");
 menu.section(SettingsMenu::Page::Controls);
 state.device=InputSource::Controller;
 state.modal=PcSettingsModal::Capture;
 state.focus=13;
 auto modal=pc_settings_snapshot(menu,input,state);
 check(modal.modal==PcSettingsModal::Capture&&modal.focus==13&&
  modal.device==InputSource::Controller,"modal preserves action and device");
 input.controller_family(ControllerFamily::Switch);
 input.activity(InputSource::Controller);
 auto controller=pc_settings_snapshot(menu,input,state);
 check(controller.prompts[0].label==input.prompt(InputAction::Confirm).label,
  "prompt follows logical actual binding");
}
