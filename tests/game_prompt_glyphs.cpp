#include "game_prompts.h"
#include "input_glyph.h"
#include "support.h"
#include <vector>
#include <bit>
#include <limits>
#include <cmath>
#include <stdexcept>
#include <cstdio>
static bc::InputModel model;
static bool settings_open=false;
namespace bc {bool pc_settings_open(){return settings_open;}}
bc::InputModel bc_input_view(){return model;}
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Fixture {
 std::vector<uint8_t> memory=std::vector<uint8_t>(9*1024*1024);
 uint8_t* rdram=memory.data();recomp_context ctx{};
 static constexpr uint32_t frame=0x80200000,stack=0x80400000,reference=0x80500000,begin=frame+0x48B0,end=frame+0xA3A0;
 Fixture(){settings_open=false;MEM_W(0,S32(0x802E8C84))=std::bit_cast<uint32_t>(0.6f);model=bc::InputModel{};ctx.r29=S32(stack);ctx.r4=S32(frame);MEM_W(0,S32(reference))=begin;bc_game_prompt_reset();}
 void text(uint32_t address,const char* value){for(size_t i=0;;++i){MEM_B(i,S32(address))=value[i];if(!value[i])break;}}
 void layout(uint32_t address,const char* value,float x,int y,int height=20,int alpha=255,bool shadow=false){
  text(address,value);ctx.r4=S32(frame);ctx.r5=S32(address);ctx.r6=0;ctx.r29=S32(stack);
  MEM_W(0x10,ctx.r29)=0;MEM_W(0x14,ctx.r29)=std::bit_cast<uint32_t>(x);MEM_W(0x18,ctx.r29)=y;
  MEM_W(0x1C,ctx.r29)=std::bit_cast<uint32_t>(float(height));MEM_W(0x20,ctx.r29)=height;MEM_W(0x24,ctx.r29)=1;
  for(auto offset:{0x28,0x2C,0x30,0x38,0x3C,0x40,0x48,0x4C,0x50,0x58,0x5C,0x60})MEM_W(offset,ctx.r29)=shadow?0:255;
  for(auto offset:{0x34,0x44,0x54,0x64})MEM_W(offset,ctx.r29)=alpha;
  bc_game_prompt_text(rdram,&ctx);
 }
 void set_pointer(uint32_t value){MEM_W(0,S32(reference))=value;}
 uint32_t pointer(){return uint32_t(MEM_W(0,S32(reference)));}
 void flush(){bc_game_prompt_flush(rdram,reference,frame);}
 std::vector<std::array<uint32_t,2>> commands(){std::vector<std::array<uint32_t,2>> values;for(uint32_t p=begin;p<pointer();p+=8)values.push_back({uint32_t(MEM_W(0,S32(p))),uint32_t(MEM_W(4,S32(p)))});return values;}
 std::vector<bc::GlyphDraw> glyphs(){std::vector<bc::GlyphDraw> values;for(auto command:commands()){auto glyph=bc::decode_glyph_command(command);check(glyph.has_value(),"font annotations emit only valid private atlas commands");values.push_back(*glyph);}return values;}
 void window(int phase=2){
  MEM_H(0,S32(0x8036BB18))=0;MEM_H(0,S32(0x8036BB1C))=phase;
  MEM_H(0,S32(0x802F8BDC))=256;MEM_H(2,S32(0x802F8BDC))=208;
  MEM_H(4,S32(0x802F8BDC))=32;MEM_H(6,S32(0x802F8BDC))=16;
 }
};
int main(){
 {
  Fixture f;f.layout(0x8030821C,"PRESS START",92,196,26);f.flush();
  check(f.pointer()>f.begin,"HUD instruction must append actual glyph commands at font flush");
  auto glyphs=f.glyphs();check(glyphs.size()==1&&bc::glyph_name(glyphs.front().sprite)=="key_enter","HUD instruction emits one sprite for its physical Start key");
  auto end=f.pointer();f.flush();check(f.pointer()==end,"a consumed annotation cannot draw twice");
 }
 {
  Fixture f;f.layout(0x8030821C,"PRESS START",92,196);bc_game_prompt_reset();f.flush();check(f.pointer()==f.begin,"hidden or blinking text cannot retain last frame's glyph");
  f.layout(0x8030821C,"MODIFIED",92,196);f.flush();check(f.pointer()==f.begin,"modified original strings never gain control icons");
  f.layout(0x80001000,"PRESS START",92,196);f.flush();check(f.pointer()==f.begin,"ordinary copies of matching prose are not treated as original prompts");
 }
 {
  Fixture f;f.layout(0x8030821C,"PRESS START",92,196,20,0);f.flush();check(f.pointer()==f.begin,"invisible text has no visible icon");
  f.layout(0x8030821C,"PRESS START",92,196,20,127);f.flush();auto glyphs=f.glyphs();
  check(glyphs.size()==1&&glyphs.front().alpha==127,"sprite alpha follows original text fade");
 }
 {
  Fixture f;f.layout(0x80308228,"USE Z/R TO TURN PAGES",24,20,15);f.flush();auto glyphs=f.glyphs();
  check(glyphs.size()==2&&glyphs[0].x==2&&glyphs[1].x==17&&glyphs[0].size==13&&glyphs[1].size==13&&glyphs[0].y==21&&glyphs[1].y==21,"height15 page controls use readable 13-pixel sprites at the reserved origin");
  check(bc::glyph_name(glyphs[0].sprite)=="key_shift"&&bc::glyph_name(glyphs[1].sprite)=="key_e","both logical page controls identify the actual keyboard keys");
  auto* rdram=f.rdram;auto x=std::bit_cast<float>(uint32_t(MEM_W(0x14,f.ctx.r29))),width=std::bit_cast<float>(uint32_t(MEM_W(0x1C,f.ctx.r29)));
  check(x==32&&std::abs(x+width*15.4f-219.f)<.001f,"longer key names plus page prefix preserve the original right edge");
 }
 {
  Fixture f;model.activity(bc::InputSource::Controller);model.manual_family(bc::ControllerFamily::PlayStation);
  f.layout(0x80308240,"USE Z/R TO MOVE MAP",24,20,16);f.flush();auto glyphs=f.glyphs();
  check(glyphs.size()==2&&glyphs[0].x==2&&glyphs[1].x==18&&glyphs[0].size==14&&glyphs[1].size==14&&glyphs[0].y==21&&glyphs[1].y==21,"height16 map controls retain 14-pixel sprites instead of shrinking to fit x24");
  check(bc::glyph_name(glyphs[0].sprite)=="playstation_l2"&&bc::glyph_name(glyphs[1].sprite)=="playstation_r1","map trigger and shoulder match the physical PlayStation controls");
  auto* rdram=f.rdram;auto x=std::bit_cast<float>(uint32_t(MEM_W(0x14,f.ctx.r29))),width=std::bit_cast<float>(uint32_t(MEM_W(0x1C,f.ctx.r29)));
  constexpr auto original=std::string_view("USE Z/R TO MOVE MAP"),replacement=std::string_view("USE L2/R1 TO MOVE MAP");
  const auto original_right=24.f+16.f*(1.f+float(original.size()-1)*.6f);
  check(x==34&&std::abs(x+width*(1.f+float(replacement.size()-1)*.6f)-original_right)<.001f,"two map icons shift text to34 and preserve its original right edge with L2/R1 names");
 }
 {
  Fixture f;f.window();f.layout(0x8030AECC,"USE A TO THRUST",-88,-64,20,127,true);f.flush();check(f.pointer()==f.begin,"black shadow pass cannot leave a colored icon while foreground blinks off");
 }
 {
  Fixture f;f.window(0);f.layout(0x8030AECC,"USE A TO THRUST",-88,-64);f.flush();check(f.pointer()==f.begin,"animated window opening hides new icons");
  f.window();f.layout(0x8030AECC,"USE A TO THRUST",-88,-64);f.flush();check(f.pointer()>f.begin,"stable tutorial window gains a glyph in screen coordinates");
  auto* rdram=f.rdram;
  auto text_x=std::bit_cast<float>(uint32_t(MEM_W(0x14,f.ctx.r29)));
  auto text_width=std::bit_cast<float>(uint32_t(MEM_W(0x1C,f.ctx.r29)));
  check(text_x== -70,"tutorial text reserves its 18-pixel inline prefix");
  check(std::abs(text_x+text_width*(1+float(std::string_view("USE A TO THRUST").size()-1)*.6f)-(-88+20*(1+float(std::string_view("USE A TO THRUST").size()-1)*.6f)))<.001f,"inline glyph preserves the original instruction's right edge");
  auto glyphs=f.glyphs();check(glyphs.size()==1&&glyphs[0].x==72&&glyphs[0].y==58&&glyphs[0].size==16,"tutorial icon keeps the original screen anchor clear of the decorative column");
 }
 {
  Fixture f;f.layout(0x8030821C,"PRESS START",92,196);f.set_pointer(f.end-128);f.flush();check(f.pointer()==f.end-128,"font flush never writes beyond the main display list or caller reserve");
  f.set_pointer(f.begin);f.flush();check(f.pointer()==f.begin,"a rejected append cannot leak into a later flush");
 }
 {
  Fixture f;f.layout(0x8030821C,"PRESS START",std::numeric_limits<float>::infinity(),196);f.flush();check(f.pointer()==f.begin,"nonfinite guest positions do not create graphic commands");
  f.layout(0x8030821C,"PRESS START",92,196);bc_game_prompt_flush(f.rdram,f.reference,0x807FFFF0);check(f.pointer()==f.begin,"invalid full frame context is rejected");
 }
 {
  Fixture f;model.activity(bc::InputSource::Controller);model.manual_family(bc::ControllerFamily::PlayStation);f.window();
  f.layout(0x8030AECC,"USE A TO THRUST",-88,-64);f.flush();auto cross=f.commands();f.set_pointer(f.begin);
  model.replace(bc::InputAction::Confirm,bc::InputSource::Controller,{{bc::BindingKind::Button,SDL_CONTROLLER_BUTTON_X}});
  f.layout(0x8030AECC,"USE A TO THRUST",-88,-64);f.flush();check(f.commands()!=cross,"action remapping changes the actual tutorial glyph raster");
  auto glyphs=f.glyphs();check(glyphs.size()==1&&bc::glyph_name(glyphs.front().sprite)=="playstation_square","remapped PlayStation instruction draws supplied Square artwork instead of Cross");
 }

 {
  Fixture f;f.layout(0x80308240,"USE Z/R TO MOVE MAP",24,20,16);
  settings_open=true;f.flush();check(f.pointer()==f.begin,"opening settings drops map glyphs queued before overlay opened");
  settings_open=false;f.flush();check(f.pointer()==f.begin,"suppressed annotations cannot reappear after close");
  settings_open=true;f.layout(0x80308240,"USE Z/R TO MOVE MAP",24,20,16);f.flush();check(f.pointer()==f.begin,"map instruction cannot queue glyphs while settings is open");
  settings_open=false;f.layout(0x80308240,"USE Z/R TO MOVE MAP",24,20,16);f.flush();auto glyphs=f.glyphs();
  check(glyphs.size()==2&&bc::glyph_name(glyphs[0].sprite)=="key_shift"&&bc::glyph_name(glyphs[1].sprite)=="key_e","closed settings restore map glyphs from current bindings");
 }
 {
  Fixture f;f.layout(0x80308240,"USE Z/R TO MOVE MAP",24,20,16);
  settings_open=true;f.window();f.layout(0x8030AECC,"USE A TO THRUST",-88,-64);
  settings_open=false;f.flush();check(f.pointer()==f.begin,"suppressed text also clears pending annotations before a rapid close");
 }
 puts("Original instruction glyph queue and display-list bounds passed");
}
