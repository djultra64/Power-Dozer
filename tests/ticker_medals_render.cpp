#include "hle/rt64_state.h"
#include "gbi/rt64_gbi_rdp.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <memory>
#ifdef BC_TICKER_HANDLER
#include "ticker_medals_render.h"
#endif
namespace RT64 { RenderRect convertFixedRect(FixedRect,hlslpp::float2,int32_t,float,float,int32_t,uint16_t,uint16_t); }
static void check(bool v,const char* why){if(!v){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
 std::vector<uint8_t> ram(0x800000);uint32_t intr=0;
 auto state=std::make_unique<RT64::State>(ram.data(),&intr,+[]{});
 auto queue=std::make_unique<RT64::WorkloadQueue>();
 RT64::EnhancementConfiguration enhancement;RT64::EmulatorConfiguration emulator;
 state->ext={};state->ext.workloadQueue=queue.get();state->ext.enhancementConfig=&enhancement;state->ext.emulatorConfig=&emulator;
 // Execute the exact startup assignments extracted from renderer.cpp, then
 // use RT64's real configuration pipeline and raster scissor conversion.
 auto shared=std::make_unique<RT64::SharedQueueResources>();
 queue->ext.sharedResources=shared.get();
 shared->fbConfigChanged=false;shared->swapChainSizeChanged=false;
 for(bool bc_wide:{false,true}){
  struct App { RT64::UserConfiguration userConfig; };auto app_=std::make_unique<App>();
#include "ticker-renderer-aspect.inc"
  shared->userConfig=app_->userConfig;
  for(auto size:{std::pair{1280,720},std::pair{960,720},std::pair{2560,1080}}){
   shared->swapChainWidth=size.first;shared->swapChainHeight=size.second;
   RT64::WorkloadQueue::WorkloadConfiguration config;
   queue->threadConfigurationUpdate({320,240},config);
   check(config.extAspectPercentage==(bc_wide?1.f:0.f),"actual renderer startup enables extended edges only in wide mode");
   const auto clip=RT64::convertFixedRect(RT64::FixedRect(0,0,1280,960),{bc_wide?4.f:3.f,3.f},320,1.f/config.aspectRatioScale,config.extAspectPercentage,0,G_EX_ORIGIN_LEFT,G_EX_ORIGIN_RIGHT);
   const int target=bc_wide?1280:960;
   check(clip.left==0&&clip.right>=target-3&&clip.right<=target,"actual RT64 raster scissor reaches output edges within its native pixel-grid snapping");
  }
 }
 auto& rdp=*state->rdp;auto& work=queue->workloads[queue->writeCursor];
 work.addFramebufferPair(0x300000,G_IM_FMT_RGBA,G_IM_SIZ_16b,320,0);
 rdp.colorImage.width=320;rdp.colorImage.changed=false;rdp.depthImage.changed=false;
 // Texture state matches the original one-strip RGBA16 medal. Load draw state
 // through RT64 itself; texture cache upload is deliberately outside this CPU test.
 rdp.setTile(0,G_IM_FMT_RGBA,G_IM_SIZ_16b,8,0,0,G_TX_MIRROR,G_TX_CLAMP,5,0,0,0);
 rdp.setTileSize(0,0,0,124,124);
 rdp.setCombine(uint64_t(0xFF2FFFFF)<<32|0x00127E24);
 state->rsp->setOtherMode(0,0x00504240);
 rdp.setScissor(0,0,0,1280,960);
 rdp.setRectAspect(G_EX_ASPECT_STRETCH);
 RT64::ExtendedAlignment untouched;untouched.leftOffset=17;untouched.rightOffset=19;rdp.setRectAlign(untouched);
 for(int x:{-54,-32,-1,0,200,310,320,364}){
  RT64::DisplayList commands[3];
  commands[0].w0=0xD6000000u|uint32_t((x+128+31)*4)<<12|904;commands[0].w1=uint32_t((x+128)*4)<<12|776;
  commands[1].w0=0xB3000000;commands[1].w1=0;commands[2].w0=0xB2000000;commands[2].w1=0x04000400;
  state->updateDrawStatusAttribute(RT64::DrawAttribute::Texture);
  auto* dl=commands;
#ifdef BC_TICKER_HANDLER
  bc::ticker_medal_rectangle(state.get(),&dl);
#else
  RT64::GBI_RDP::texrect(state.get(),&dl);state->flush();
#endif
  auto& pair=work.fbPairs[work.currentFramebufferPairIndex()];auto& proj=pair.projections[pair.projectionCount-1];auto& call=proj.gameCalls[proj.gameCallCount-1].callDesc;
  check(call.rect.ulx==x*4&&call.rect.lrx==(x+31)*4,"official RT64 decoder receives signed unbiased X");
  check(call.rectAspect==G_EX_ASPECT_ADJUST,"proportional aspect metadata");
  check(call.scissorRect.ulx==0&&call.scissorRect.lrx==1280,"guest framebuffer scissor stays 320 pixels");
  check(call.scissorLeftOrigin==G_EX_ORIGIN_LEFT&&call.scissorRightOrigin==G_EX_ORIGIN_RIGHT,"raster scissor anchored to output edges");
  check(pair.scissorRect.ulx==0&&pair.scissorRect.lrx==1280,"framebuffer extent not inflated by signed wings");
  check(rdp.scissorStackSize==1&&rdp.extended.scissorLeftOriginStack[0]==G_EX_ORIGIN_NONE&&rdp.extended.scissorRightOriginStack[0]==G_EX_ORIGIN_NONE,"scissor stack and origins restored");
  check(rdp.extended.global.rectAspect==G_EX_ASPECT_STRETCH&&rdp.extended.global.rect.leftOffset==17&&rdp.extended.global.rect.rightOffset==19,"global aspect and rectangle alignment restored");
  check(dl==commands+2,"official three-word E4 decoding advanced correctly");
  check(call.tileCount==1,"original combiner selects one texture");
  const auto& tile=work.drawData.callTiles[call.tileIndex];
  check(tile.valid&&tile.sampleWidth==32&&tile.sampleHeight==32,"full sprite sample extent retained even wholly in wings");
  const auto n=work.drawData.triTcFloats.size();check(n>=12&&work.drawData.triTcFloats[n-12]==0&&work.drawData.triTcFloats[n-10]==31,"signed alignment preserves full original UV span");
  std::printf("x=%d tracked=(%d,%d)-(%d,%d) sample=%ux%u\n",x,tile.minTexcoord.x,tile.minTexcoord.y,tile.maxTexcoord.x,tile.maxTexcoord.y,tile.sampleWidth,tile.sampleHeight);
 }

 // Custom scissors/origins and an exhausted stack are preserved. In
 // particular, failed push must never pop another caller's stack entry.
 for(int variant=0;variant<3;variant++){
  while(rdp.scissorStackSize>1)rdp.popScissor();
  RT64::ExtendedAlignment align;
  if(variant==1){align.leftOrigin=G_EX_ORIGIN_LEFT;align.rightOrigin=G_EX_ORIGIN_RIGHT;}
  rdp.setScissor(2,variant==0?4:0,0,variant==1?0:variant==0?1276:1280,960,align);
  if(variant==2)while(rdp.scissorStackSize<RDP_EXTENDED_STACK_SIZE)rdp.pushScissor();
  const int depth=rdp.scissorStackSize;const auto old=rdp.scissorRectStack[depth-1];
  const auto left=rdp.extended.scissorLeftOriginStack[depth-1],right=rdp.extended.scissorRightOriginStack[depth-1];
  RT64::DisplayList commands[3];commands[0].w0=0xD609E388;commands[0].w1=0x00080308;commands[1].w0=0xB3000000;commands[1].w1=0;commands[2].w0=0xB2000000;commands[2].w1=0x04000400;
  auto* dl=commands;bc::ticker_medal_rectangle(state.get(),&dl);
  auto& pair=work.fbPairs[work.currentFramebufferPairIndex()];auto& proj=pair.projections[pair.projectionCount-1];const auto& call=proj.gameCalls[proj.gameCallCount-1].callDesc;
  check(rdp.scissorStackSize==depth&&rdp.scissorRectStack[depth-1].ulx==old.ulx&&rdp.scissorRectStack[depth-1].lrx==old.lrx,"custom/exhausted scissor stack unchanged");
  check(call.scissorRect.ulx==old.ulx&&call.scissorRect.lrx==old.lrx&&call.scissorLeftOrigin==left&&call.scissorRightOrigin==right&&call.scissorMode==2,"custom scissor metadata preserved on actual draw");
 }
 std::puts("official RT64 signed ticker decode, texture extent and restored state passed");
}
