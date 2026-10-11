#include "game_prompts.h"
#include "support.h"
#include <vector>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <cstdio>
static bc::InputModel input_view;
static bool settings_open=false;
namespace bc {bool pc_settings_open(){return settings_open;}}
bc::InputModel bc_input_view(){return input_view;}
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 check(bc::game_prompt_actions("USE Z/R TO MOVE MAP").size()==2,"two glyphs use the two actual map actions");
 check(bc::game_prompt_actions("PRESS START").size()==1,"pause instruction has one glyph action");
 check(bc::game_prompt_actions("FOR A HOLIDAY.").empty(),"narrative prose has no control glyphs");
 auto& model=input_view;
 auto prompt=[&](const char* text){return bc::game_prompt(text,model);};
 check(prompt("PRESS START")=="PRESS ENTER","keyboard pause tutorial");
 check(prompt("USE A TO THRUST")=="USE Z TO THRUST","keyboard thrust action");
 check(prompt("USE Z/R TO TURN PAGES")=="USE SHIFT/E TO TURN PAGES","two independent actions");
 check(!prompt("FOR A HOLIDAY."),"ordinary A prose unchanged");
 check(!prompt("RESTART"),"menu Restart unchanged");
 model.activity(bc::InputSource::Controller);model.manual_family(bc::ControllerFamily::Switch);
 check(prompt("USE A TO THRUST")=="USE B TO THRUST","Switch south physical B");
 check(prompt("THEN HIT B TO")=="THEN HIT A TO","Switch east physical A");
 check(prompt("PRESS START")=="PRESS PLUS","Switch plus must use a name supported by the original font");
 model.manual_family(bc::ControllerFamily::Xbox);
 check(prompt("THEN HIT B TO")=="THEN HIT B TO","Xbox east action");
 check(prompt("USE Z/R TO MOVE MAP")=="USE LT/RB TO MOVE MAP","Xbox trigger and shoulder");
 model.manual_family(bc::ControllerFamily::PlayStation);
 check(prompt("USE A TO THRUST")=="USE CROSS TO THRUST","PlayStation cross identity");
 check(prompt("USE R TO SKID")=="USE R1 TO SKID","right shoulder action");
 model.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
 check(prompt("USE A TO THRUST")=="USE SQUARE TO THRUST","remap shared with actual action");
 model.activity(bc::InputSource::Keyboard);
 model.replace(bc::InputAction::Trigger,bc::InputSource::Keyboard,{});
 check(prompt("USE Z TO GET OUT")=="USE UNBOUND TO GET OUT","unbound must not promise old key");
 std::vector<uint8_t> ram(9*1024*1024);auto* rdram=ram.data();recomp_context ctx{};
 auto put=[&](uint32_t address,const char* text){for(size_t i=0;;++i){MEM_B(i,S32(address))=text[i];if(!text[i])break;}};
 auto get=[&](uint32_t address){std::string text;for(size_t i=0;i<128&&MEM_BU(i,S32(address));++i)text+=char(MEM_BU(i,S32(address)));return text;};
 ctx.r29=S32(0x80400000);ctx.r5=S32(0x80001000);ctx.r6=123;
 put(0x80001000,"PRESS START");bc_game_prompt_text(rdram,&ctx);
 check(uint32_t(ctx.r5)==0x80001000&&ctx.r6==123,"identical unknown text must not be rewritten");
 ctx.r5=S32(0x8030821C);put(0x8030821C,"MODIFIED");bc_game_prompt_text(rdram,&ctx);
 check(uint32_t(ctx.r5)==0x8030821C,"modified known text stays intact");
 model.activity(bc::InputSource::Controller);model.manual_family(bc::ControllerFamily::PlayStation);
 for(const auto& original:bc::original_game_prompts){
  put(original.address,original.text);ctx.r5=S32(original.address);ctx.r6=123;
  MEM_W(0x1C,ctx.r29)=std::bit_cast<uint32_t>(20.f);
  bc_game_prompt_text(rdram,&ctx);
  auto expected=bc::game_prompt(original.text,model);
  check(expected&&get(uint32_t(ctx.r5))==*expected,"all original guest strings follow bindings");
  check(uint32_t(ctx.r5)>=0x80803000&&uint32_t(ctx.r5)<0x80804000&&ctx.r6==0,"host text slots and English pointer bound");
  check(get(original.address)==original.text,"original ROM text remains unchanged");
  auto width=std::bit_cast<float>(uint32_t(MEM_W(0x1C,ctx.r29)));
  check(std::isfinite(width)&&width>0&&width<=20,"longer device labels fit original line space");
 }

 settings_open=true;
 for(const auto& original:bc::original_game_prompts){
  put(original.address,original.text);ctx.r5=S32(original.address);ctx.r6=123;ctx.r7=456;
  for(int i=0;i<0x68;i++)MEM_B(i,ctx.r29)=uint8_t(i+1);
  std::vector<uint8_t> layout(0x68);for(size_t i=0;i<layout.size();i++)layout[i]=MEM_BU(i,ctx.r29);
  bc_game_prompt_text(rdram,&ctx);
  check(get(uint32_t(ctx.r5)).empty(),"open settings suppress verified original action instruction text");
  check(uint32_t(ctx.r5)>=0x80803000&&uint32_t(ctx.r5)<0x80804000,"suppressed text uses bounded private CPU storage");
  check(get(original.address)==original.text&&ctx.r6==123&&ctx.r7==456,"suppression preserves caller text and font arguments");
  for(size_t i=0;i<layout.size();i++)check(MEM_BU(i,ctx.r29)==layout[i],"suppression leaves caller layout untouched");
 }
 ctx.r5=S32(0x80001000);put(0x80001000,"FOR A HOLIDAY.");bc_game_prompt_text(rdram,&ctx);
 check(uint32_t(ctx.r5)==0x80001000&&get(0x80001000)=="FOR A HOLIDAY.","narrative remains intact under settings");
 ctx.r5=S32(0x80001000);put(0x80001000,"PRESS START");bc_game_prompt_text(rdram,&ctx);
 check(uint32_t(ctx.r5)==0x80001000,"unverified copies remain intact under settings");
 ctx.r5=S32(0x80308240);put(0x80308240,"MODIFIED");bc_game_prompt_text(rdram,&ctx);
 check(uint32_t(ctx.r5)==0x80308240&&get(0x80308240)=="MODIFIED","changed text at known address remains intact under settings");
 settings_open=false;model=bc::InputModel{};ctx.r5=S32(0x80308240);put(0x80308240,"USE Z/R TO MOVE MAP");bc_game_prompt_text(rdram,&ctx);
 check(get(uint32_t(ctx.r5))=="USE SHIFT/E TO MOVE MAP","closing settings restores current device map instruction");
 puts("Original game prompt actions passed");
}
