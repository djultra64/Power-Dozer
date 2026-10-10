#include "input_glyph.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace bc {
namespace {
constexpr uint32_t white=0xEEEEEEFF;
using Pixels=std::array<std::array<uint32_t,16>,16>;
void dot(Pixels& p,int x,int y,uint32_t colour){if(x>=0&&x<16&&y>=0&&y<16)p[y][x]=colour;}
void line(Pixels& p,int x0,int y0,int x1,int y1,uint32_t colour){
 int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1,error=dx+dy;
 for(;;){dot(p,x0,y0,colour);if(x0==x1&&y0==y1)break;auto twice=2*error;if(twice>=dy){error+=dy;x0+=sx;}if(twice<=dx){error+=dx;y0+=sy;}}
}
void circle(Pixels& p,uint32_t colour){for(int y=0;y<16;y++)for(int x=0;x<16;x++){auto distance=std::hypot(x-7.5,y-7.5);if(distance>=6&&distance<=7.5)dot(p,x,y,colour);}}
void box(Pixels& p,int x,int y,int w,int h,uint32_t colour){line(p,x,y,x+w-1,y,colour);line(p,x,y+h-1,x+w-1,y+h-1,colour);line(p,x,y,x,y+h-1,colour);line(p,x+w-1,y,x+w-1,y+h-1,colour);}
void letter(Pixels& p,char c,int x,int y,uint32_t colour){
 // Three by five letter forms preserve distinct face-button names at N64 size.
 const char* bits=c=='A'?"010101111101101":c=='B'?"110101110101110":c=='X'?"101101010101101":c=='Y'?"101101010010010":nullptr;
 if(bits)for(int row=0;row<5;row++)for(int col=0;col<3;col++)if(bits[row*3+col]=='1')dot(p,x+col,y+row,colour);
}
}
std::vector<GlyphRectangle> input_glyph(const InputPrompt& prompt,ControllerFamily family){
 if(prompt.label=="UNBOUND")return {};
 Pixels p{};auto colour=white;
 switch(prompt.glyph){
 case InputGlyph::South:case InputGlyph::East:case InputGlyph::West:case InputGlyph::North:
  if(family==ControllerFamily::Xbox)colour=prompt.label=="A"?0x75DB65FF:prompt.label=="B"?0xFA6464FF:prompt.label=="X"?0x71B7FFFF:0xF9DF5FFF;
  circle(p,colour);
  if(prompt.label.size()==1)letter(p,prompt.label[0],6,5,colour);
  else {
   for(auto position:std::array<std::array<int,2>,4>{{{7,11},{11,7},{3,7},{7,3}}})dot(p,position[0],position[1],white);
   auto position=prompt.glyph==InputGlyph::South?std::array<int,2>{7,11}:prompt.glyph==InputGlyph::East?std::array<int,2>{11,7}:prompt.glyph==InputGlyph::West?std::array<int,2>{3,7}:std::array<int,2>{7,3};
   for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)dot(p,position[0]+dx,position[1]+dy,white);
  }
  break;
 case InputGlyph::Cross:line(p,3,3,12,12,0x71B7FFFF);line(p,3,12,12,3,0x71B7FFFF);break;
 case InputGlyph::Circle:circle(p,0xFA6464FF);break;
 case InputGlyph::Square:box(p,3,3,10,10,0xFBA7DFFF);break;
 case InputGlyph::Triangle:line(p,7,2,2,12,0x75DBB4FF);line(p,7,2,13,12,0x75DBB4FF);line(p,2,12,13,12,0x75DBB4FF);break;
 case InputGlyph::Key:box(p,1,3,14,10,white);line(p,3,10,12,10,white);break;
 case InputGlyph::Shoulder:box(p,1,5,14,7,white);line(p,4,3,11,3,white);break;
 case InputGlyph::Trigger:box(p,3,2,10,12,white);line(p,5,4,10,4,white);break;
 case InputGlyph::Menu:
  if(prompt.label=="PLUS"){line(p,7,3,7,12,white);line(p,3,7,12,7,white);}
  else if(prompt.label=="-")line(p,3,7,12,7,white);
  else {box(p,2,3,12,10,white);line(p,5,6,10,6,white);line(p,5,9,10,9,white);}
  break;
 case InputGlyph::Stick:circle(p,white);[[fallthrough]];
 case InputGlyph::Dpad:{
  const bool right=prompt.label.find("RIGHT")!=std::string::npos,left=prompt.label.find("LEFT")!=std::string::npos,up=prompt.label.find("UP")!=std::string::npos;
  int dx=right?1:left?-1:0,dy=dx?0:up?-1:1;
  line(p,7-dx*3,7-dy*3,7+dx*3,7+dy*3,white);
  line(p,7+dx*3,7+dy*3,7+dx-dy*2,7+dy+dx*2,white);
  line(p,7+dx*3,7+dy*3,7+dx+dy*2,7+dy-dx*2,white);break;
 }
 case InputGlyph::StickPress:circle(p,white);box(p,6,6,4,4,white);break;
 case InputGlyph::Other:box(p,3,3,10,10,white);break;
 }
 std::vector<GlyphRectangle> rectangles;
 for(int y=0;y<16;y++)for(int x=0;x<16;){auto colour=p[y][x];if(!colour){++x;continue;}int start=x;while(x<16&&p[y][x]==colour)++x;rectangles.push_back({start,y,x-start,1,colour});}
 return rectangles;
}
std::vector<GlyphCommand> glyph_commands(const std::vector<GlyphRectangle>& rectangles){
 if(rectangles.empty())return {};
 if(rectangles.size()>384)throw std::invalid_argument("Too many UI glyph rectangles");
 std::vector<GlyphCommand> result={
  {0x00525464,0x10000064}, // gEXEnable, default extended opcode 0x64
  {0x6400001B,0},{0x64000027,0}, // push combine and primitive colour
  {0xE7000000,0},{0xFCFFFFFF,0xFFFDF6FB} // sync; G_CC_PRIMITIVE
 };
 uint32_t colour=0;bool have_colour=false;
 for(const auto& rect:rectangles){
  if(rect.w<=0||rect.h<=0||rect.w>320||rect.h>240||rect.x<0||rect.y<0||rect.x>320-rect.w||rect.y>240-rect.h)continue;
  if(!have_colour||colour!=rect.rgba){colour=rect.rgba;have_colour=true;result.push_back({0xFA000000,colour});}
  // RDP 1-cycle rectangles have exclusive lower bounds (quarter pixels).
  result.push_back({0xF6000000|uint32_t(rect.x+rect.w)<<14|uint32_t(rect.y+rect.h)<<2,uint32_t(rect.x)<<14|uint32_t(rect.y)<<2});
 }
 if(!have_colour)return {};
 result.insert(result.end(),{{0xE7000000,0},{0x64000028,0},{0x6400001C,0},{0x00525464,0x20000000}});
 return result;
}
}
