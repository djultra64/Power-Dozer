#include "pc_settings_renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

static void check(bool value,const char* message){
 if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
struct TextRun {std::string label;float left,right,top,bottom;bool red;};
static std::vector<TextRun> text_runs(const bc::PcSettingsDrawData& draw){
 constexpr std::string_view tiles="0123456789ABCD\x7f" "EFGHIJKLMNOPQRSTUVWXYZ\x7f'):,$!.-(%?#/";
 std::vector<TextRun> out;
 for(const auto& batch:draw.batches)if(batch.primitive==bc::PcSettingsPrimitive::Text){
  TextRun run{"",std::numeric_limits<float>::max(),0,std::numeric_limits<float>::max(),0,false};
  for(size_t i=batch.first;i<batch.first+batch.count;i+=6){
   const auto& v=draw.vertices[i];const size_t tile=size_t(std::lround(v.v*1024/32))*8+size_t(std::lround(v.u*512/32));
   check(tile<tiles.size(),"main text uses original font tiles");run.label+=tiles[tile];
  }
  for(size_t i=batch.first;i<batch.first+batch.count;++i){
   const auto& v=draw.vertices[i];run.left=std::min(run.left,v.x);run.right=std::max(run.right,v.x);run.top=std::min(run.top,v.y);run.bottom=std::max(run.bottom,v.y);
  }
  const auto& v=draw.vertices[batch.first];run.red=v.r==1&&v.g==0&&v.b==0;out.push_back(run);
 }
 return out;
}
int main(){
 using namespace bc;
 PcSettingsAssets assets;assets.glyph_advances.fill(.6f);
 assets.palette[4]={255,0,0,255};assets.palette[7]={255,255,255,255};
 for(size_t i=0;i<16;++i)assets.shadow_vertices[i]={int16_t((i%4)*100-150),int16_t((i/4)*100-150),uint8_t(i%3?160:0)};
 for(auto& triangle:assets.shadow_triangles)triangle={0,5,15};
 MainMenuState state;InputModel input;
 // Published screen replacement must release its predecessor, including
 // replacement across types and hide through the existing options API.
 auto published_main=std::make_shared<const MainMenuSnapshot>(main_menu_snapshot(state,input));
 std::weak_ptr<const MainMenuSnapshot> old_main=published_main;
 publish_main_menu_snapshot(published_main);published_main.reset();
 check(!old_main.expired(),"publication retains immutable main screen");
 auto published_options=std::make_shared<const PcSettingsSnapshot>();
 std::weak_ptr<const PcSettingsSnapshot> old_options=published_options;
 publish_pc_settings_snapshot(published_options);published_options.reset();
 check(old_main.expired()&&!old_options.expired(),"options atomically replaces main screen ownership");
 publish_main_menu_snapshot(std::make_shared<const MainMenuSnapshot>(main_menu_snapshot(state,input)));
 check(old_options.expired(),"main atomically replaces options screen ownership");
 auto hidden_main=std::make_shared<const MainMenuSnapshot>(main_menu_snapshot(state,input));
 old_main=hidden_main;publish_main_menu_snapshot(hidden_main);hidden_main.reset();
 publish_pc_settings_snapshot(nullptr);check(old_main.expired(),"null options publication hides either screen");
 publish_main_menu_snapshot(nullptr);
 // Missing entries, horizontal rows, file content, wrong focus tint, a low
 // shadow over the ticker or unscaled geometry must fail these assertions.
 for(size_t selected=0;selected<3;++selected){
  state.select(selected);const auto snapshot=main_menu_snapshot(state,input);
  for(auto [width,height]:{std::pair{960,720},{1280,720},{3840,2160}}){
   const auto draw=main_menu_draw_data(snapshot,assets,width,height);
   check(!draw.vertices.empty(),"main menu emits visible draw data");
   check(draw.scissor.x==0&&draw.scissor.y==0&&draw.scissor.width==width&&draw.scissor.height==height,"main scissor follows framebuffer extent");
   const float scale=std::min(width/640.f,height/480.f),safe_y=(height-480*scale)/2;
   size_t boxes=0,texts=0,shadows=0;
   for(const auto& batch:draw.batches){
    check(batch.count&&batch.first+batch.count<=draw.vertices.size(),"main draw range valid");
    if(batch.primitive==PcSettingsPrimitive::Text)++texts;
    if(batch.primitive==PcSettingsPrimitive::TextShadow)++shadows;
    if(batch.primitive==PcSettingsPrimitive::BoxShadow){
     ++boxes;check(batch.count==42,"main reuses original fourteen shadow triangles");
     for(size_t i=batch.first;i<batch.first+batch.count;++i)check(draw.vertices[i].y<safe_y+400*scale,"main shadow clears original bottom ticker region");
    }
   }
   check(boxes==1&&texts==shadows,"main has original box and shadow for every text run");
   for(const auto& v:draw.vertices)check(std::isfinite(v.x)&&std::isfinite(v.y)&&v.x>=0&&v.y>=0&&v.x<=width&&v.y<=height,"main geometry stays inside framebuffer");
   const auto runs=text_runs(draw);std::vector<TextRun> entries;
   check(runs.size()==6,"main draws only heading, three entries and physical select prompt");
   for(const auto& run:runs){
    check(run.label!="BACK"&&run.label!="ERASEGAME"&&!run.label.starts_with("FILE"),"main omits unavailable back and original file labels");
    if(run.label=="STARTGAME"||run.label=="PCOPTIONS"||run.label=="CLOSEGAME")entries.push_back(run);
   }
   check(runs[0].label=="POWERDOZER","main heading uses original POWER DOZER typography");
   check(runs[0].top>=safe_y+146*scale,"main heading clears retained original logo");
   for(const auto& v:draw.vertices)check(v.y<safe_y+388*scale,"all main geometry clears actual original ticker");
   check(entries.size()==3&&entries[0].label=="STARTGAME"&&entries[1].label=="PCOPTIONS"&&entries[2].label=="CLOSEGAME","exactly three ordered main entries");
   for(size_t i=0;i<3;++i){
    check(std::abs((entries[i].left+entries[i].right)/2-width/2.f)<10*scale,"each main entry centered horizontally");
    check(entries[i].red==(i==selected),"only selected entry has original focus red");
    if(i)check(entries[i].top-entries[i-1].bottom>20*scale,"vertical entries clearly separated");
   }
  }
 }
 auto wide_snapshot=main_menu_snapshot(state,input);wide_snapshot.presentation_wide=true;
 const auto letterboxed=main_menu_draw_data(wide_snapshot,assets,960,720);
 const auto letterbox_runs=text_runs(letterboxed);
 check(letterbox_runs[0].top>=90+146*1.125f,"wide presentation letterbox keeps heading below original logo");
 for(const auto& v:letterboxed.vertices)check(v.y>=90&&v.y<90+388*1.125f,"wide letterbox keeps all main content above actual ticker");
 const auto reference=main_menu_draw_data(main_menu_snapshot(state,input),assets,960,720);
 for(auto [width,height]:{std::pair{1280,720},{3840,2160}}){
  const auto draw=main_menu_draw_data(main_menu_snapshot(state,input),assets,width,height);
  const float factor=height/720.f,offset=(width-960*factor)/2;
  check(draw.vertices.size()==reference.vertices.size(),"resize preserves original main mesh topology");
  for(size_t i=0;i<draw.vertices.size();++i){
   check(std::abs(draw.vertices[i].x-(reference.vertices[i].x*factor+offset))<.002f&&std::abs(draw.vertices[i].y-reference.vertices[i].y*factor)<.002f,"4:3, 16:9 and 4K preserve proportional centered main geometry");
   check(draw.vertices[i].u==reference.vertices[i].u&&draw.vertices[i].v==reference.vertices[i].v,"resize preserves original atlas UVs");
  }
 }
 // Wrong family, hardcoded south button or stale keyboard prompts must fail.
 input.activity(InputSource::Controller);
 for(auto family:{ControllerFamily::Xbox,ControllerFamily::PlayStation,ControllerFamily::Switch}){
  input.controller_family(family);
  for(auto button:{SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_Y}){
   input.replace(InputAction::Confirm,InputSource::Controller,{{BindingKind::Button,button}});
   const auto snapshot=main_menu_snapshot(state,input);
   // A published snapshot must retain family as well as its owned prompt.
   const auto saved_label=snapshot.confirm.label;
   input.controller_family(ControllerFamily::Unknown);
   check(snapshot.controller_family==family&&snapshot.confirm.label==saved_label,"main snapshot owns family and prompt across live input changes");
   input.controller_family(family);
   const auto draw=main_menu_draw_data(snapshot,assets,960,720);
   const char* want=family==ControllerFamily::Xbox?(button==SDL_CONTROLLER_BUTTON_A?"xbox_a":"xbox_y"):family==ControllerFamily::PlayStation?(button==SDL_CONTROLLER_BUTTON_A?"playstation_cross":"playstation_triangle"):(button==SDL_CONTROLLER_BUTTON_A?"switch_b":"switch_x");
   std::vector<std::string> glyphs;for(const auto& batch:draw.batches)if(batch.glyph)glyphs.emplace_back(glyph_name(*batch.glyph));
   check(glyphs==std::vector<std::string>{want},"main Confirm glyph matches active family and actual physical remap");
   auto label=snapshot.confirm.label;label.erase(std::remove(label.begin(),label.end(),' '),label.end());
   const auto runs=text_runs(draw);check(std::any_of(runs.begin(),runs.end(),[&](const auto& r){return r.label==label;}),"main prompt names physical binding beside glyph");
  }
 }
 input.activity(InputSource::Keyboard);input.replace(InputAction::Confirm,InputSource::Keyboard,{{BindingKind::Key,SDL_SCANCODE_K}});
 const auto keyboard=main_menu_draw_data(main_menu_snapshot(state,input),assets,1280,720);
 check(std::any_of(keyboard.batches.begin(),keyboard.batches.end(),[](const auto& b){return b.glyph&&glyph_name(*b.glyph)=="key_k";}),"main immediately follows remapped last active keyboard");
 input.activity(InputSource::Controller);input.controller_family(ControllerFamily::Switch);
 input.replace(InputAction::Confirm,InputSource::Controller,{{BindingKind::Axis,SDL_CONTROLLER_AXIS_RIGHTY,-1}});
 const auto stick=main_menu_draw_data(main_menu_snapshot(state,input),assets,1280,720);
 bool arrow=false;
 for(size_t i=1;i<stick.batches.size();++i){
  const auto& b=stick.batches[i];if(b.primitive!=PcSettingsPrimitive::Glyph||b.texture!=PcSettingsTexture::White)continue;
  const auto& icon=stick.batches[i-1];check(icon.glyph&&glyph_name(*icon.glyph)=="switch_r_stick","main remapped axis retains correct physical stick sprite");
  const auto& a=stick.vertices[icon.first];const auto& z=stick.vertices[icon.first+2];const auto& tip=stick.vertices[b.first];
  check(std::abs(tip.x-(a.x+z.x)/2)<.001f&&std::abs(tip.y+5.25f-(a.y+z.y)/2)<.001f,"main retains centered directional stick arrow");
  for(size_t j=b.first;j<b.first+b.count;++j){const auto& v=stick.vertices[j];check(v.r==1&&v.g==1&&v.b==1&&v.a==1&&v.x>=a.x&&v.x<=z.x&&v.y>=a.y&&v.y<=z.y,"main stick arrow is white and inside sprite");}arrow=true;
 }
 check(arrow,"main renders physical stick direction");
 for(auto [width,height]:{std::pair{0,720},{960,0},{-1,720},{960,-1}})check(main_menu_draw_data(main_menu_snapshot(state,input),assets,width,height).vertices.empty(),"invalid main extent emits no geometry");
 std::puts("main menu original drawing and physical prompts passed");
}
