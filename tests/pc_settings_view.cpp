#include "pc_settings_view.h"
#include "pc_settings_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <set>

static void check(bool value,const char* message){
 if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}

int main(){
 using namespace bc;
 PcSettingsAssets assets;assets.glyph_advances.fill(.6f);
 assets.palette[4]={255,0,0,255};assets.palette[7]={255,255,255,255};assets.palette[8]={128,128,128,255};
 // Synthetic original-mesh-shaped fixture; no ROM pixels in tests.
 for(size_t i=0;i<16;++i)assets.shadow_vertices[i]={int16_t((i%4)*100-150),int16_t((i/4)*100-150),uint8_t(i%3?160:0)};
 for(auto& triangle:assets.shadow_triangles)triangle={0,5,15};
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
  auto main_state=state;main_state.origin=PcSettingsOrigin::MainMenu;
  const auto main_snapshot=pc_settings_snapshot(menu,input,main_state);
  check(main_snapshot.rows.size()==snapshot.rows.size(),"main entry retains every settings row");
  for(size_t i=0;i<snapshot.rows.size();++i){
   const auto& a=snapshot.rows[i];const auto& b=main_snapshot.rows[i];
   check(a.label==b.label&&a.value==b.value&&a.field==b.field&&a.action==b.action&&a.restart_required==b.restart_required,"main entry preserves original settings and actions");
   check(a.binding.has_value()==b.binding.has_value()&&(!a.binding||(a.binding->label==b.binding->label&&a.binding->glyph==b.binding->glyph&&a.binding->source==b.binding->source)),"main entry preserves physical binding prompts");
  }
  for(size_t i=0;i<2;++i)check(main_snapshot.prompts[i].label==snapshot.prompts[i].label&&main_snapshot.prompts[i].glyph==snapshot.prompts[i].glyph&&main_snapshot.prompts[i].source==snapshot.prompts[i].source,"main entry retains Confirm and Back prompts");
  if(section==SettingsMenu::Page::Controls){
   check(snapshot.controls.size()==4&&main_snapshot.controls.size()==4,"both origins preserve all controls");
   check(snapshot.controls[size_t(PcSettingsControl::GameOptions)].label=="GAME OPTIONS","default pause origin preserves Game Options");
   check(main_snapshot.controls[size_t(PcSettingsControl::GameOptions)].label=="MAIN MENU","main origin names the same return-control position MAIN MENU");
   for(size_t i=0;i<3;++i)check(snapshot.controls[i].label==main_snapshot.controls[i].label&&snapshot.controls[i].value==main_snapshot.controls[i].value&&snapshot.controls[i].field==main_snapshot.controls[i].field,"main origin preserves device family and defaults controls");
  }
  check(snapshot.rows.size()==size_t(count),"all section rows visible");
  check(menu.rows().size()==size_t(count),"section has no pagination/back row");
  check(snapshot.title!="SAVING"&&snapshot.section==section,"title independent of status");
  if(section!=SettingsMenu::Page::Controls){
   const std::vector<SettingsField> expected=section==SettingsMenu::Page::Display?
    std::vector{SettingsField::Scale,SettingsField::Aspect,SettingsField::Refresh,SettingsField::Fullscreen}:
    section==SettingsMenu::Page::Graphics?std::vector{SettingsField::MSAA,SettingsField::Presentation,SettingsField::Texture}:
    std::vector{SettingsField::Volume,SettingsField::Mute};
   for(size_t i=0;i<expected.size();++i)check(snapshot.rows[i].field==expected[i],"section field order matches editable rows");
  }
  for(auto width:{960,1280}){
   auto draw=pc_settings_draw_data(snapshot,assets,width,720);
   check(!draw.vertices.empty(),"visible section emits triangle draw data");
   check(draw.scissor.x==0&&draw.scissor.y==0&&draw.scissor.width==width&&draw.scissor.height==720,"full framebuffer scissor never clips Controls");
   const auto layout=pc_settings_layout(width,720);
   const int selected=section==SettingsMenu::Page::Display?0:section==SettingsMenu::Page::Graphics?1:section==SettingsMenu::Page::Audio?2:3;
   bool active_tab_red=false;
   for(const auto& v:draw.vertices)if(layout.tabs[selected].contains(v.x,v.y)&&v.r==1&&v.g==0&&v.b==0)active_tab_red=true;
   check(active_tab_red,"active section tab is red independently of legacy page enum values");
   size_t texts=0,shadows=0,boxes=0;
   for(const auto& batch:draw.batches){
    check(batch.count&&batch.first+batch.count<=draw.vertices.size(),"valid draw range");
    if(batch.primitive==PcSettingsPrimitive::Text)++texts;
    if(batch.primitive==PcSettingsPrimitive::TextShadow)++shadows;
    if(batch.primitive==PcSettingsPrimitive::BoxShadow){++boxes;check(batch.count==42,"original fourteen shadow triangles");}
   }
   check(boxes==1&&texts==shadows,"original box plus shadow for every text run");
   for(const auto& v:draw.vertices)check(std::isfinite(v.x)&&std::isfinite(v.y)&&v.x>=0&&v.y>=0&&v.x<=width&&v.y<=720,"geometry visible in framebuffer");
   for(size_t i=0;i<snapshot.rows.size();++i){
    auto hidden=snapshot;hidden.rows[i].label.clear();hidden.rows[i].value.clear();hidden.rows[i].binding.reset();
    check(pc_settings_draw_data(hidden,assets,width,720).vertices.size()<draw.vertices.size(),"every visible row emits geometry");
   }
  }
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
 for(auto family:{ControllerFamily::Xbox,ControllerFamily::PlayStation,ControllerFamily::Switch}){
  input.controller_family(family);
  auto snap=pc_settings_snapshot(menu,input,state);
  auto draw=pc_settings_draw_data(snap,assets,960,720);
  const char* want=family==ControllerFamily::Xbox?"xbox_a":family==ControllerFamily::PlayStation?"playstation_cross":"switch_b";
  bool found=false;
  for(const auto& batch:draw.batches)if(batch.glyph&&glyph_name(*batch.glyph)==want)found=true;
  check(!found,"raw capture hides ordinary logical select/back glyphs");
  std::vector<std::string> capture_labels;
  constexpr std::string_view capture_tiles="0123456789ABCD\x7f" "EFGHIJKLMNOPQRSTUVWXYZ\x7f'):,$!.-(%?#/";
  for(const auto& batch:draw.batches)if(batch.primitive==PcSettingsPrimitive::Text){
   std::string label;
   for(size_t i=batch.first;i<batch.first+batch.count;i+=6){const auto& v=draw.vertices[i];const size_t tile=size_t(std::lround(v.v*1024/32))*8+size_t(std::lround(v.u*512/32));check(tile<capture_tiles.size(),"capture uses original font tiles");label+=capture_tiles[tile];}
   capture_labels.push_back(label);
  }
  for(const auto* expected:{"PRESSANINPUT","ESC:CANCEL"})check(std::find(capture_labels.begin(),capture_labels.end(),expected)!=capture_labels.end(),"capture retains accurate instruction and Escape cancellation");
  for(const auto* misleading:{"SELECT","BACK"})check(std::find(capture_labels.begin(),capture_labels.end(),misleading)==capture_labels.end(),"capture omits ordinary action footer labels");
  auto ordinary=snap;ordinary.modal=PcSettingsModal::None;
  const auto ordinary_draw=pc_settings_draw_data(ordinary,assets,960,720);
  check(std::any_of(ordinary_draw.batches.begin(),ordinary_draw.batches.end(),[&](const auto& batch){return batch.glyph&&glyph_name(*batch.glyph)==want;}),"ordinary settings keeps active family select glyph");
  const auto prompt_area=pc_settings_layout(960,720).prompts;
  for(const auto& vertex:draw.vertices)check(!prompt_area.contains(vertex.x,vertex.y),"capture hides misleading ordinary footer geometry");
  check(std::count_if(draw.batches.begin(),draw.batches.end(),[](const auto& b){return b.primitive==PcSettingsPrimitive::BoxShadow;})==2,"capture modal adds original shadow mesh over same rows");
 }
 // Inspect only geometry after the modal's shadow; the dynamic footer must
 // not make a hardcoded modal pass this test.
 auto defaults_prompts=[&](const char* confirm_glyph,const char* back_glyph,const char* confirm_label,const char* back_label){
  state.modal=PcSettingsModal::RestoreDefaults;
  const auto snapshot=pc_settings_snapshot(menu,input,state);
  const auto draw=pc_settings_draw_data(snapshot,assets,960,720);
  auto no_options=snapshot;no_options.rows.clear();no_options.controls.clear();
  check(pc_settings_draw_data(no_options,assets,960,720).vertices.size()==draw.vertices.size(),
   "modal hides underlying option text while retaining its snapshot and focus");
  std::vector<std::string> labels,glyphs;bool in_modal=false;int boxes=0;
  // Original font tile order, decoded from the emitted texture coordinates.
  constexpr std::string_view tiles="0123456789ABCD\x7f" "EFGHIJKLMNOPQRSTUVWXYZ\x7f'):,$!.-(%?#/";
  for(const auto& batch:draw.batches){
   if(batch.primitive==PcSettingsPrimitive::BoxShadow){in_modal=++boxes==2;continue;}
   if(!in_modal)continue;
   if(batch.glyph)glyphs.emplace_back(glyph_name(*batch.glyph));
   if(batch.primitive==PcSettingsPrimitive::Text){
    std::string text;
    for(size_t i=batch.first;i<batch.first+batch.count;i+=6){
     const auto& v=draw.vertices[i];const size_t tile=size_t(std::lround(v.v*1024/32))*8+size_t(std::lround(v.u*512/32));
     check(tile<tiles.size(),"modal text addresses original font tile");text+=tiles[tile];
    }
    labels.push_back(text);
   }
  }
  check(glyphs==std::vector<std::string>{confirm_glyph,back_glyph},"defaults modal uses remapped active-device confirm and back glyphs");
  for(const auto* expected:{confirm_label,back_label,"CONFIRM","CANCEL","ESC:CANCEL"})
   check(std::find(labels.begin(),labels.end(),expected)!=labels.end(),"defaults modal names actual bindings and retains fixed Escape recovery");
 };
 input.replace(InputAction::Confirm,InputSource::Controller,{{BindingKind::Button,SDL_CONTROLLER_BUTTON_Y}});
 input.replace(InputAction::Back,InputSource::Controller,{{BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
 input.controller_family(ControllerFamily::Switch);
 defaults_prompts("switch_x","switch_y","X","Y");
 input.controller_family(ControllerFamily::Xbox);
 defaults_prompts("xbox_y","xbox_x","Y","X");
 input.controller_family(ControllerFamily::PlayStation);
 defaults_prompts("playstation_triangle","playstation_square","TRIANGLE","SQUARE");
 input.activity(InputSource::Keyboard);
 input.replace(InputAction::Confirm,InputSource::Keyboard,{{BindingKind::Key,SDL_SCANCODE_K}});
 input.replace(InputAction::Back,InputSource::Keyboard,{{BindingKind::Key,SDL_SCANCODE_H}});
 defaults_prompts("key_k","key_h","K","H");
 // Direction belongs to the displayed physical binding, including remapped
 // logical Confirm, rather than the action's label or the controller family.
 state.modal=PcSettingsModal::None;state.focus_target=PcSettingsFocusTarget::Row;state.focus=0;
 for(auto family:{ControllerFamily::Xbox,ControllerFamily::PlayStation,ControllerFamily::Switch}){
  input.controller_family(family);input.activity(InputSource::Controller);
  for(auto axis:{SDL_CONTROLLER_AXIS_LEFTX,SDL_CONTROLLER_AXIS_LEFTY,SDL_CONTROLLER_AXIS_RIGHTX,SDL_CONTROLLER_AXIS_RIGHTY})for(int sign:{-1,1}){
   input.replace(InputAction::Confirm,InputSource::Controller,{{BindingKind::Axis,axis,sign}});
   auto snap=pc_settings_snapshot(menu,input,state);snap.rows.resize(1);snap.rows[0].label="ACCEL/OK";snap.rows[0].binding=input.prompt(InputAction::Confirm);
   for(auto [width,height]:{std::pair{960,720},{1280,720},{320,240}}){
    const auto draw=pc_settings_draw_data(snap,assets,width,height);size_t arrows=0;
    for(size_t i=0;i<draw.batches.size();++i){
     const auto& arrow=draw.batches[i];if(arrow.texture!=PcSettingsTexture::White||arrow.primitive!=PcSettingsPrimitive::Glyph)continue;
     ++arrows;check(i>0&&draw.batches[i-1].glyph.has_value(),"direction overlay follows original stick sprite");
     const auto& sprite=draw.batches[i-1];check(glyph_name(*sprite.glyph).ends_with(axis<=SDL_CONTROLLER_AXIS_LEFTY?"l_stick":"r_stick"),"direction retains correct physical stick identity");
     const auto& origin=draw.vertices[sprite.first];const auto& extent=draw.vertices[sprite.first+2];
     const auto& tip=draw.vertices[arrow.first];bool direction_visible=false;
     const auto& head_base_a=draw.vertices[arrow.first+1];const auto& head_base_b=draw.vertices[arrow.first+2];
     const float scale=pc_settings_layout(width,height).safe_area.width/640;
     const bool horizontal=axis==SDL_CONTROLLER_AXIS_LEFTX||axis==SDL_CONTROLLER_AXIS_RIGHTX;
     const float arrow_center_x=tip.x-(horizontal?sign*3.5f*scale:0);
     const float arrow_center_y=tip.y-(horizontal?0:sign*3.5f*scale);
     check(std::abs(arrow_center_x-(origin.x+extent.x)/2)<.001f&&std::abs(arrow_center_y-(origin.y+extent.y)/2)<.001f,"direction arrow is centered inside original stick circle");
     check(std::abs((horizontal?head_base_a.y-head_base_b.y:head_base_a.x-head_base_b.x))==5*scale,"centering preserves tiny five-unit arrowhead width");
     for(size_t j=arrow.first;j<arrow.first+arrow.count;++j){
      const auto& v=draw.vertices[j];check(v.r==1&&v.g==1&&v.b==1&&v.a==1,"direction arrow is opaque white even on red focused row");
      check(v.x>=origin.x&&v.x<=extent.x&&v.y>=origin.y&&v.y<=extent.y,"small arrow stays within existing stick icon at all aspect ratios");
      const bool horizontal=axis==SDL_CONTROLLER_AXIS_LEFTX||axis==SDL_CONTROLLER_AXIS_RIGHTX;
      const float offset=horizontal?(tip.x-v.x)*sign:(tip.y-v.y)*sign;
      check(offset>=-.001f,"arrow tip points in displayed physical axis direction");if(offset>2*pc_settings_layout(width,height).safe_area.width/640)direction_visible=true;
     }
     check(direction_visible,"direction arrow has a visible shaft behind its tip");
    }
    check(arrows==2,"remapped Confirm has directional overlay on row and footer");
   }
  }
 }
 for(auto prompt:{input.physical_prompt({BindingKind::Button,SDL_CONTROLLER_BUTTON_A}),input.physical_prompt({BindingKind::Axis,SDL_CONTROLLER_AXIS_TRIGGERLEFT}),input.physical_prompt({BindingKind::Key,SDL_SCANCODE_LEFT}),InputPrompt{"LS ",InputGlyph::Stick,InputSource::Controller}}){
  auto snap=pc_settings_snapshot(menu,input,state);snap.rows.resize(1);snap.rows[0].binding=prompt;snap.prompts[0]=prompt;
  const auto draw=pc_settings_draw_data(snap,assets,960,720);
  check(std::none_of(draw.batches.begin(),draw.batches.end(),[](const auto& b){return b.texture==PcSettingsTexture::White&&b.primitive==PcSettingsPrimitive::Glyph;}),"face, trigger, keyboard and neutral stick glyphs get no directional arrow");
 }
 check(pc_settings_draw_data(controller,assets,0,0).vertices.empty(),"minimized framebuffer emits nothing");
}
