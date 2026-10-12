#include "ticker_medals_render.h"
#include "hle/rt64_state.h"

void bc::ticker_medal_rectangle(RT64::State* state,RT64::DisplayList** dl) {
 // D6 is published solely by the checked ticker callers. Decode the complete
 // original E4 payload, including flipped rank UVs and unmodified derivatives.
 const auto* command=*dl;
 const int ulx=command[0].p1(12,12),uly=command[0].p1(0,12);
 const int lrx=command[0].p0(12,12),lry=command[0].p0(0,12);
 const uint8_t tile=command[0].p1(24,3);
 const int16_t s=command[1].p1(16,16),t=command[1].p1(0,16);
 const int16_t dsdx=command[2].p1(16,16),dtdy=command[2].p1(0,16);
 *dl+=2;
 auto& rdp=*state->rdp;
 state->flush();
 const auto oldAspect=rdp.extended.global.rectAspect;
 const auto index=rdp.scissorStackSize-1;
 const auto scissor=rdp.scissorRectStack[index];
 const auto mode=rdp.scissorModeStack[index];
 // Only an ordinary full 320x240 guest scissor can become output anchored.
 // Keep its guest extent exactly intact: framebuffer aspect heuristics still
 // need 320 pixels. Guard capacity because pushScissor silently fails at max.
 const bool anchor=rdp.colorImage.width==320&&scissor.ulx==0&&scissor.uly==0&&scissor.lrx==1280&&scissor.lry==960&&
  rdp.extended.scissorLeftOriginStack[index]==G_EX_ORIGIN_NONE&&rdp.extended.scissorRightOriginStack[index]==G_EX_ORIGIN_NONE&&
  rdp.scissorStackSize<RDP_EXTENDED_STACK_SIZE;
 if(anchor){
  rdp.pushScissor();
  RT64::ExtendedAlignment edges;edges.leftOrigin=G_EX_ORIGIN_LEFT;edges.rightOrigin=G_EX_ORIGIN_RIGHT;
  rdp.setScissor(mode,scissor.ulx,scissor.uly,scissor.lrx-4*rdp.colorImage.width,scissor.lry,edges);
 }
 RT64::ExtendedAlignment alignment;alignment.leftOffset=alignment.rightOffset=-512;
 rdp.setRectAspect(G_EX_ASPECT_ADJUST);
 rdp.drawTexRect(ulx,uly,lrx,lry,tile,s,t,dsdx,dtdy,false,alignment);
 state->flush();
 rdp.setRectAspect(oldAspect);
 if(anchor)rdp.popScissor();
}
