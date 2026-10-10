#include "game_prompts.h"
#include "input.h"
#include "support.h"
#include "input_glyph.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <json.hpp>
#include <mutex>
#include <algorithm>
#include <optional>
namespace {
struct Annotation {
 uint32_t frame;size_t index;int x,y,size,alpha;bc::ControllerFamily family;
 std::vector<bc::InputPrompt> prompts;
};
std::array<std::optional<Annotation>,bc::original_game_prompts.size()> annotations;
std::mutex annotation_mutex;
void annotate(uint8_t* rdram,recomp_context* ctx,size_t index,const bc::InputModel& view,std::string_view replacement){
 const auto frame=uint32_t(ctx->r4),stack=uint32_t(ctx->r29);
 if(frame<0x80000000||frame>0x807DEB68||(frame&7)||stack<0x80000000||stack>0x807FFF98||(stack&3))return;
 // These verified instruction sites use explicit positions and forward layout.
 if(MEM_W(0x10,S32(stack))!=0||MEM_W(0x24,S32(stack))!=1)return;
 const auto local_x=std::bit_cast<float>(uint32_t(MEM_W(0x14,S32(stack))));
 auto x=local_x;
 int y=MEM_W(0x18,S32(stack)),height=MEM_W(0x20,S32(stack));
 if(!std::isfinite(x)||x< -4096||x>4096||y< -4096||y>4096||height<8||height>64)return;
 const bool hud=index==0||index==11||index==12;
 if(!hud){
  if(MEM_H(0,S32(0x8036BB1C))!=2)return;
  int window=MEM_H(0,S32(0x8036BB18));if(window<0||window>=108)return;
  const auto pointer=S32(0x802F8BDC+uint32_t(window)*28);
  x+=MEM_H(4,pointer)+MEM_HU(0,pointer)/2;
  y+=MEM_H(6,pointer)+MEM_HU(2,pointer)/2;
 }
 auto actions=bc::game_prompt_actions(bc::original_game_prompts[index].text);
 if(actions.empty()||x<0||x>320||y<0||y>240-height)return;
 int alpha=0;for(auto offset:{0x34,0x44,0x54,0x64})alpha=std::max(alpha,std::clamp<int>(MEM_W(offset,S32(stack)),0,255));
 if(!alpha)return;
 // Window shadows are a separate black-text pass, including during some
 // selected-row blinks. Follow the foreground pass instead of retaining a
 // colored control icon over an otherwise hidden instruction.
 bool foreground=false;
 for(auto offset:{0x28,0x2C,0x30,0x38,0x3C,0x40,0x48,0x4C,0x50,0x58,0x5C,0x60})foreground|=MEM_W(offset,S32(stack))!=0;
 if(!foreground)return;
 // Map/page hints begin at x=24. Smaller cells keep both controls before
 // the original text instead of clipping one of the two action glyphs.
 int size=std::min(std::clamp(height-2,8,16),(int(x)-2)/int(actions.size())-2);
 if(size<8)return;
 int origin=int(x)-int(actions.size())*(size+2);
 if(!hud){
  // Original tutorial sprites occupy the decorative left column. Reserve
  // an inline prefix at the existing text anchor instead of covering them.
  // All verified window instruction sites use the normal 0.6 advance mode.
  if(ctx->r7!=0)return;
  auto advance=std::bit_cast<float>(uint32_t(MEM_W(0,S32(0x802E8C84))));
  auto width=std::bit_cast<float>(uint32_t(MEM_W(0x1C,S32(stack))));
  if(!std::isfinite(advance)||advance<=0||advance>1||!std::isfinite(width)||width<=0)return;
  float factor=1+float(replacement.size()-1)*advance,span=width*factor;
  int prefix=int(actions.size())*(size+2);
  if(span<=prefix)return;
  MEM_W(0x14,S32(stack))=std::bit_cast<uint32_t>(local_x+prefix);
  MEM_W(0x1C,S32(stack))=std::bit_cast<uint32_t>((span-prefix)/factor);
  origin=int(x);
 }
 Annotation value{frame,index,origin,y+(height-size)/2,size,alpha,view.family(),{}};
 for(auto action:actions)value.prompts.push_back(view.prompt(action));
 std::lock_guard lock(annotation_mutex);annotations[index]=std::move(value);
}
}
extern "C" void bc_game_prompt_text(uint8_t* rdram,recomp_context* ctx){
 const auto pointer=uint32_t(ctx->r5);
 for(size_t index=0;index<bc::original_game_prompts.size();++index){
  const auto& original=bc::original_game_prompts[index];
  if(pointer!=original.address)continue;
  const auto length=std::strlen(original.text);
  for(size_t i=0;i<=length;++i)if(MEM_BU(i,S32(pointer))!=uint8_t(original.text[i]))return;
  auto view=bc_input_view();
  auto replacement=bc::game_prompt(original.text,view);
  if(!replacement||replacement->size()>=128)return;
  // CPU-only text slots follow the existing PC menu storage. The guest font
  // consumes each string synchronously into ordinary RSP-visible geometry.
  const uint32_t destination=0x80803000+uint32_t(index)*128;
  for(size_t i=0;i<=replacement->size();++i)MEM_B(i,S32(destination))=i<replacement->size()?(*replacement)[i]:0;
  ctx->r5=S32(destination);ctx->r6=0;
  // Preserve each original line's space when a device name is longer. arg7
  // of the core font layout is the float glyph width at entry SP+0x1C.
  const auto stack=uint32_t(ctx->r29);
  if(replacement->size()>length&&stack>=0x80000000&&stack<=0x807FFFE0){
   auto width=std::bit_cast<float>(uint32_t(MEM_W(0x1C,S32(stack))));
   if(std::isfinite(width)&&width>0)MEM_W(0x1C,S32(stack))=std::bit_cast<uint32_t>(width*float(length)/float(replacement->size()));
  }
  annotate(rdram,ctx,index,view,*replacement);
  if(std::getenv("BC_TRACE_INPUT")){
   static std::array<std::string,bc::original_game_prompts.size()> last;
   if(last[index]!=*replacement){
    auto event=nlohmann::json{{"event","game_action_prompt"},{"original",original.text},{"text",*replacement},{"address",pointer}}.dump();
    fprintf(stderr,"%s\n",event.c_str());last[index]=*replacement;
   }
  }
  return;
 }
}

extern "C" void bc_game_prompt_reset(){std::lock_guard lock(annotation_mutex);for(auto& value:annotations)value.reset();}
extern "C" void bc_game_prompt_flush(uint8_t* rdram,uint32_t reference,uint32_t frame){
 std::vector<Annotation> pending;
 {std::lock_guard lock(annotation_mutex);for(auto& value:annotations)if(value&&value->frame==frame){pending.push_back(std::move(*value));value.reset();}}
 if(pending.empty()||frame<0x80000000||frame>0x807DEB68||(frame&7)||reference<0x80000000||reference>0x807FFFFC||(reference&3))return;
 auto pointer=uint32_t(MEM_W(0,S32(reference)));const auto begin=frame+0x48B0,end=frame+0xA3A0;
 if(pointer<begin||pointer>end||(pointer&7))return;
 for(auto& site:pending){
  std::vector<bc::GlyphRectangle> rectangles;
  for(size_t i=0;i<site.prompts.size();++i){
   auto glyph=bc::input_glyph(site.prompts[i],site.family);
   std::array<std::array<uint32_t,16>,16> pixels{};
   for(auto rect:glyph){
    int x0=rect.x*site.size/16,y0=rect.y*site.size/16;
    int x1=((rect.x+rect.w)*site.size+15)/16,y1=((rect.y+rect.h)*site.size+15)/16;
    auto colour=(rect.rgba&0xFFFFFF00)|uint32_t((rect.rgba&255)*site.alpha/255);
    for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)pixels[y][x]=colour;
   }
   // Preserve thin borders while reducing cells, and merge overlaps so a
   // partially faded pixel blends once rather than becoming more opaque.
   for(int y=0;y<site.size;++y)for(int x=0;x<site.size;){
    auto colour=pixels[y][x];if(!colour){++x;continue;}
    int begin=x;while(x<site.size&&pixels[y][x]==colour)++x;
    rectangles.push_back({site.x+int(i)*(site.size+2)+begin,site.y+y,x-begin,1,colour});
   }
  }
  auto commands=bc::glyph_commands(rectangles);
  if(commands.empty()||commands.size()*8+256>end-pointer)continue;
  for(auto command:commands){MEM_W(0,S32(pointer))=command[0];MEM_W(4,S32(pointer))=command[1];pointer+=8;}
  if(std::getenv("BC_TRACE_INPUT")){
   static std::array<std::string,bc::original_game_prompts.size()> last;
   std::vector<std::string> labels;for(auto& prompt:site.prompts)labels.push_back(prompt.label);
   auto event=nlohmann::json{{"event","game_action_glyphs"},{"original",bc::original_game_prompts[site.index].text},{"labels",labels},{"x",site.x},{"y",site.y},{"size",site.size},{"alpha",site.alpha},{"commands",commands.size()}}.dump();
   if(last[site.index]!=event){fprintf(stderr,"%s\n",event.c_str());last[site.index]=std::move(event);}
  }
 }
 MEM_W(0,S32(reference))=pointer;
}
