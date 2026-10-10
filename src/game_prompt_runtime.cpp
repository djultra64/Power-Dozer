#include "game_prompts.h"
#include "input.h"
#include "support.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <json.hpp>
extern "C" void bc_game_prompt_text(uint8_t* rdram,recomp_context* ctx){
 const auto pointer=uint32_t(ctx->r5);
 for(size_t index=0;index<bc::original_game_prompts.size();++index){
  const auto& original=bc::original_game_prompts[index];
  if(pointer!=original.address)continue;
  const auto length=std::strlen(original.text);
  for(size_t i=0;i<=length;++i)if(MEM_BU(i,S32(pointer))!=uint8_t(original.text[i]))return;
  auto replacement=bc::game_prompt(original.text,bc_input_view());
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
