#include "input_glyph.h"
#include <stdexcept>
#include <cstdio>
#include <cmath>
#include <algorithm>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static bc::GlyphSprite named(const bc::InputPrompt& prompt,bc::ControllerFamily family,const char* name){
 auto glyph=bc::input_glyph(prompt,family);check(glyph.has_value(),"physical control has an atlas sprite");
 check(bc::glyph_name(*glyph)==name,"atlas identity matches the actual physical control");return *glyph;
}
int main(){
 bc::InputModel model;model.activity(bc::InputSource::Controller);
 model.manual_family(bc::ControllerFamily::Xbox);
 auto xbox=named(model.prompt(bc::InputAction::Confirm),model.family(),"xbox_a");
 model.manual_family(bc::ControllerFamily::Switch);
 auto sw=named(model.prompt(bc::InputAction::Confirm),model.family(),"switch_b");
 check(sw!=xbox,"Switch south B differs from Xbox A");
 model.manual_family(bc::ControllerFamily::PlayStation);
 auto cross=named(model.prompt(bc::InputAction::Confirm),model.family(),"playstation_cross");
 model.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
 auto square=named(model.prompt(bc::InputAction::Confirm),model.family(),"playstation_square");
 check(square!=cross,"remapped PlayStation square differs from cross");
 for(auto family:{bc::ControllerFamily::Xbox,bc::ControllerFamily::Switch,bc::ControllerFamily::PlayStation,bc::ControllerFamily::Unknown}){
  model.manual_family(family);std::vector<bc::GlyphSprite> faces;
  for(int code=SDL_CONTROLLER_BUTTON_A;code<=SDL_CONTROLLER_BUTTON_Y;code++){
   model.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,code}});
   auto face=bc::input_glyph(model.prompt(bc::InputAction::Confirm),family);
   check(face.has_value(),"every face position has artwork");
   for(auto previous:faces)check(*face!=previous,"four physical face positions remain distinct");faces.push_back(*face);
  }
  for(unsigned i=0;i<unsigned(bc::InputAction::Count);++i){
   auto glyph=bc::input_glyph(model.prompt(bc::InputAction(i)),family);
   check(glyph&&!bc::glyph_name(*glyph).empty(),"atlas covers every default physical control");
  }
 }
 model.activity(bc::InputSource::Keyboard);
 auto enter=named(model.prompt(bc::InputAction::Start),model.family(),"key_enter");
 auto shift=named(model.prompt(bc::InputAction::Trigger),model.family(),"key_shift");
 auto z=named(model.prompt(bc::InputAction::Confirm),model.family(),"key_z");
 check(shift!=z&&z!=enter&&enter!=shift,"keyboard keycaps identify each key instead of sharing a generic image");
 model.replace(bc::InputAction::Start,bc::InputSource::Keyboard,{});
 check(!bc::input_glyph(model.prompt(bc::InputAction::Start),model.family()),"unbound action promises no physical glyph");
 auto press=bc::input_glyph(model.physical_prompt({bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_LEFTSTICK}),model.family());
 auto direction=bc::input_glyph(model.physical_prompt({bc::BindingKind::Axis,SDL_CONTROLLER_AXIS_LEFTX}),model.family());
 check(press&&direction&&press!=direction,"stick click cannot be mistaken for movement direction");
 // Hand-derived private command: sprite0, size16, alpha127, position(10,20).
 auto commands=bc::glyph_commands({{{0},10,20,16,127}});
 check(commands==std::vector<bc::GlyphCommand>{{0xD5107F00,0x00002814}},"one sprite emits one exact D5 command without changing RDP state");
 auto decoded=bc::decode_glyph_command({0xD5107F00,0x00002814});
 check(decoded&&decoded->sprite==bc::GlyphSprite{0}&&decoded->x==10&&decoded->y==20&&decoded->size==16&&decoded->alpha==127,"decoder preserves sprite, location, size and fade");
 check(bc::glyph_commands({{{0},304,224,16,255}}).size()==1,"glyph can touch the exact bottom-right screen edge");
 check(bc::glyph_commands({{{0},-1,0,16,255},{{0},305,224,16,255},{{0},304,225,16,255},{{0},0,0,0,255},{{0},0,0,16,0},{{0},0,0,16,256},{{65535},0,0,16,255}}).empty(),"invalid bounds, opacity and sprite IDs cannot wrap into commands");
 check(!bc::decode_glyph_command({0xF5107F00,0x00002814})&&!bc::decode_glyph_command({0xD5107F01,0x00002814})&&!bc::decode_glyph_command({0xD5100000,0x00002814}),"unrelated opcodes, reserved bits and invisible draws are rejected");
 auto mesh=bc::glyph_mesh({xbox,10,20,16,127});
 check(!mesh.positions.empty()&&mesh.positions.size()%24==0,"each nontransparent horizontal run yields two triangles");
 auto vertices=mesh.positions.size()/4;
 check(mesh.texcoords.size()==vertices*2&&mesh.colours.size()==vertices*4,"mesh attributes agree on vertex count");
 float maximum_alpha=0;
 for(size_t i=0;i<vertices;++i){
  check(mesh.positions[i*4]>=10&&mesh.positions[i*4]<=26&&mesh.positions[i*4+1]>=20&&mesh.positions[i*4+1]<=36&&mesh.positions[i*4+2]==0&&mesh.positions[i*4+3]==1,"pixel-space geometry stays in its sprite cell");
  for(unsigned c=0;c<4;++c)check(std::isfinite(mesh.colours[i*4+c])&&mesh.colours[i*4+c]>=0&&mesh.colours[i*4+c]<=1,"atlas RGBA is normalized");
  auto alpha=mesh.colours[i*4+3];check(alpha<=127.f/255+.00001f,"atlas alpha cannot exceed the UI fade");maximum_alpha=std::max(maximum_alpha,alpha);
 }
 check(std::abs(maximum_alpha-127.f/255)<.00001f,"opaque source artwork receives requested fade exactly");
 for(size_t i=0;i<vertices;i+=6){
  check(mesh.positions[(i+1)*4]>mesh.positions[i*4]&&mesh.positions[(i+2)*4+1]>mesh.positions[i*4+1],"runs have positive area");
  for(size_t v=i+1;v<i+6;++v)for(unsigned c=0;c<4;++c)check(mesh.colours[v*4+c]==mesh.colours[i*4+c],"a run never combines differently colored source pixels");
 }
 check(bc::glyph_mesh({xbox,-1,20,16,127}).positions.empty()&&bc::glyph_mesh({{65535},0,0,16,255}).positions.empty(),"invalid draw data cannot reach mesh geometry");
 puts("Atlas glyph identities, commands, mesh bounds and fade passed");
}
