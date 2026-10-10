#include "input_glyph.h"
#include "control_atlas_data.h"
#include <algorithm>
#include <cctype>
#include <map>
namespace bc {
namespace {
std::optional<GlyphSprite> sprite(std::string_view name){
 for(size_t i=0;i<control_atlas.size();++i)if(name==control_atlas[i].name)return GlyphSprite{uint16_t(i)};
 return std::nullopt;
}
std::string lower(std::string text){for(auto& c:text)c=char(std::tolower(static_cast<unsigned char>(c)));return text;}
bool valid(const GlyphDraw& d){return d.sprite.index<control_atlas.size()&&d.size>=8&&d.size<=32&&d.alpha>0&&d.alpha<=255&&d.x>=0&&d.y>=0&&d.x<=320-d.size&&d.y<=240-d.size;}
}
std::optional<GlyphSprite> input_glyph(const InputPrompt& p,ControllerFamily f){
 if(p.label=="UNBOUND")return std::nullopt;
 auto name=lower(p.label);
 if(p.source==InputSource::Keyboard){
  static const std::map<std::string,std::string> aliases={
   {"return","enter"},{"escape","esc"},{"capslock","caps"},{"left ctrl","ctrl"},{"right ctrl","ctrl"},{"left alt","alt"},{"right alt","altgr"},{"left gui","cmd"},{"right gui","cmd"},{"left shift","shift"},{"right shift","shift"},
   {"`","tilde"},{"-","minus"},{"=","equals"},{"[","lbracket"},{"]","rbracket"},{"\\","backslash"},{";","semicolon"},{"'","apostrophe"},{",","comma"},{".","period"},{"/","slash"},
   {"numlock","numlock"},{"numlockclear","numlock"},{"pageup","pageup"},{"pagedown","pagedown"},{"printscreen","printscreen"},{"scrolllock","scrolllock"},
   {"keypad 0","num0"},{"keypad 1","num1"},{"keypad 2","num2"},{"keypad 3","num3"},{"keypad 4","num4"},{"keypad 5","num5"},{"keypad 6","num6"},{"keypad 7","num7"},{"keypad 8","num8"},{"keypad 9","num9"},{"keypad .","numdecimal"},{"keypad /","numdivide"},{"keypad *","nummultiply"},{"keypad -","numminus"},{"keypad +","numplus"},{"keypad enter","numenter"}
  };
  if(auto it=aliases.find(name);it!=aliases.end())name=it->second;
  auto result=sprite("key_"+name);return result?result:sprite("key_blank");
 }
 std::string prefix=f==ControllerFamily::Xbox?"xbox_":f==ControllerFamily::PlayStation?"playstation_":f==ControllerFamily::Switch?"switch_":"generic_";
 switch(p.glyph){
 case InputGlyph::South:case InputGlyph::East:case InputGlyph::West:case InputGlyph::North:
  if(prefix=="generic_")name=p.glyph==InputGlyph::South?"1":p.glyph==InputGlyph::East?"2":p.glyph==InputGlyph::West?"3":"4";
  break;
 case InputGlyph::Stick:name=name.starts_with("ls ")?"l_stick":"r_stick";break;
 case InputGlyph::StickPress:name=(name=="l3"||name.starts_with("ls "))?"l3":"r3";break;
 case InputGlyph::Dpad:std::replace(name.begin(),name.end(),' ','_');break;
 case InputGlyph::Menu:
  if(name=="start"||name=="menu"||name=="options"||name=="plus")name=f==ControllerFamily::Switch?"plus":f==ControllerFamily::PlayStation?"options":f==ControllerFamily::Xbox?"menu":"start";
  else if(name=="back"||name=="view"||name=="select"||name=="share"||name=="create"||name=="-")name=f==ControllerFamily::Switch?"minus":f==ControllerFamily::PlayStation?"share":f==ControllerFamily::Xbox?"view":"select";
  else name="home";
  break;
 case InputGlyph::Shoulder:if(prefix=="generic_")name=(name=="lb"||name=="l")?"l":"r";break;
 case InputGlyph::Other:if(f==ControllerFamily::Switch&&name=="misc1")name="capture";break;
 default:break;
 }
 return sprite(prefix+name);
}
std::string_view glyph_name(GlyphSprite s){return s.index<control_atlas.size()?control_atlas[s.index].name:"";}
std::vector<GlyphCommand> glyph_commands(const std::vector<GlyphDraw>& draws){
 std::vector<GlyphCommand> commands;
 for(auto& d:draws)if(valid(d))commands.push_back({0xD5000000|uint32_t(d.size)<<16|uint32_t(d.alpha)<<8,uint32_t(d.sprite.index)<<20|uint32_t(d.x)<<10|uint32_t(d.y)});
 return commands;
}
std::optional<GlyphDraw> decode_glyph_command(GlyphCommand c){
 if((c[0]&0xFF0000FF)!=0xD5000000)return std::nullopt;
 GlyphDraw d{{uint16_t(c[1]>>20)},int(c[1]>>10&1023),int(c[1]&1023),int(c[0]>>16&255),int(c[0]>>8&255)};
 return valid(d)?std::optional{d}:std::nullopt;
}
GlyphMesh glyph_mesh(const GlyphDraw& d){
 GlyphMesh mesh;if(!valid(d))return mesh;
 const auto& pixels=control_atlas[d.sprite.index].pixels;
 const float scale=float(d.size)/32;
 for(int y=0;y<32;++y)for(int x=0;x<32;){
  const auto rgba=pixels[y*32+x];if(!(rgba&255)){++x;continue;}
  const int begin=x;while(x<32&&pixels[y*32+x]==rgba)++x;
  const float x0=d.x+begin*scale,x1=d.x+x*scale,y0=d.y+y*scale,y1=y0+scale;
  for(auto pos:std::array<std::array<float,2>,6>{{{x0,y0},{x1,y0},{x1,y1},{x0,y0},{x1,y1},{x0,y1}}}){
   mesh.positions.insert(mesh.positions.end(),{pos[0],pos[1],0,1});mesh.texcoords.insert(mesh.texcoords.end(),{0,0});
   mesh.colours.insert(mesh.colours.end(),{float(rgba>>24)/255,float(rgba>>16&255)/255,float(rgba>>8&255)/255,float(rgba&255)*d.alpha/(255*255)});
  }
 }
 return mesh;
}
}
