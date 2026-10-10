#include "input_glyph.h"
#include <stdexcept>
#include <cstdio>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 bc::InputModel model;model.activity(bc::InputSource::Controller);
 model.manual_family(bc::ControllerFamily::Xbox);
 auto xbox=bc::input_glyph(model.prompt(bc::InputAction::Confirm),model.family());
 check(!xbox.empty(),"Xbox face glyph exists");
 model.manual_family(bc::ControllerFamily::Switch);
 auto sw=bc::input_glyph(model.prompt(bc::InputAction::Confirm),model.family());
 check(sw!=xbox,"Switch south B differs from Xbox A");
 model.manual_family(bc::ControllerFamily::PlayStation);
 auto cross=bc::input_glyph(model.prompt(bc::InputAction::Confirm),model.family());
 model.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
 auto square=bc::input_glyph(model.prompt(bc::InputAction::Confirm),model.family());
 check(square!=cross,"remapped PlayStation square differs from cross");
 for(auto family:{bc::ControllerFamily::Xbox,bc::ControllerFamily::Switch,bc::ControllerFamily::PlayStation,bc::ControllerFamily::Unknown}){
  model.manual_family(family);
  std::vector<std::vector<bc::GlyphRectangle>> faces;
  for(int code=SDL_CONTROLLER_BUTTON_A;code<=SDL_CONTROLLER_BUTTON_Y;code++){
   model.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,code}});
   auto face=bc::input_glyph(model.prompt(bc::InputAction::Confirm),family);
   for(const auto& previous:faces)check(face!=previous,"all four face positions have distinct physical glyphs");faces.push_back(face);
  }
  for(unsigned i=0;i<unsigned(bc::InputAction::Count);++i){
   auto glyph=bc::input_glyph(model.prompt(bc::InputAction(i)),family);
   check(!glyph.empty()&&glyph.size()<=128,"bounded glyph for every default physical control");
   for(auto rectangle:glyph)check(rectangle.x>=0&&rectangle.y>=0&&rectangle.w>0&&rectangle.h>0&&rectangle.x+rectangle.w<=16&&rectangle.y+rectangle.h<=16,"glyph stays inside its cell");
  }
 }
 model.activity(bc::InputSource::Keyboard);
 check(!bc::input_glyph(model.prompt(bc::InputAction::Start),model.family()).empty(),"keyboard keycap");
 model.replace(bc::InputAction::Start,bc::InputSource::Keyboard,{});
 check(bc::input_glyph(model.prompt(bc::InputAction::Start),model.family()).empty(),"unbound has no promised physical glyph");
 auto press=bc::input_glyph(model.physical_prompt({bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_LEFTSTICK}),model.family());
 auto direction=bc::input_glyph(model.physical_prompt({bc::BindingKind::Axis,SDL_CONTROLLER_AXIS_LEFTX}),model.family());
 check(!press.empty()&&press!=direction,"stick click glyph cannot be mistaken for movement direction");
 auto commands=bc::glyph_commands({{10,20,2,3,0xFFFFFFFF}});
 check(commands.size()==11&&commands[6]==bc::GlyphCommand{0xF603005C,0x00028050},"rectangle encoding matches original SDK macros");
 check(commands[1][0]==0x6400001B&&commands[2][0]==0x64000027&&commands[8][0]==0x64000028&&commands[9][0]==0x6400001C,"combiner and colour paired restoration");
 check(bc::glyph_commands({{-1,0,2,2,0xFFFFFFFF},{319,239,2,2,0xFFFFFFFF}}).empty(),"out-of-screen cells never wrap in RDP coordinates");
 puts("Device glyph raster bounds and identities passed");
}
