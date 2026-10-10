#pragma once
#include <cstdint>

extern bool bc_wide;

// Extend only the original full-screen failure quad. Keep y/z/color/alpha
// intact, and restore exact original positions when returning to 4:3.
inline void bc_widescreen_whiteout(uint8_t* rdram) {
    for (unsigned buffer=0;buffer<2;buffer++) for(unsigned i=0;i<4;i++) {
        const bool right=i==1||i==2;
        MEM_H(0,S32(0x802FA8B0+buffer*64+i*16))=right?(bc_wide?373:319):(bc_wide?-54:0);
    }
}

// DrawSky supplies fresh UVs each frame. Extend its geometry and UVs together
// so the newly exposed sides show more tiled sky rather than wider texels.
// Called immediately before the original double-buffer selector flips.
inline void bc_widescreen_sky(uint8_t* rdram) {
    const uint32_t base=0x802FA820+(MEM_BU(S32(0x8036BFC5),0)&1)*64;
    const int rightU=MEM_H(8,S32(base)),leftU=MEM_H(8,S32(base+16));
    const int padding=bc_wide?((leftU-rightU)*214)/1279:0;
    for(unsigned i=0;i<4;i++) {
        const bool right=(i&1)==0;
        MEM_H(0,S32(base+i*16))=right?(bc_wide?1493:1279):(bc_wide?-214:0);
        MEM_H(8,S32(base+i*16))=right?rightU-padding:leftU+padding;
    }
}

// Player/menu ticker owns this 44-halfword FIFO. Increase the visible length
// from 27 to 36 without changing the original scroll cadence or font size.
// Preserve existing text on an aspect change, padding only newly exposed slots.
inline void bc_widescreen_ticker(uint8_t* rdram) {
    constexpr bool right[8]={false,true,true,false,false,false,true,true};
    for(unsigned i=0;i<8;i++) MEM_H(0,S32(0x80208380+i*16))=right[i]?(bc_wide?214:160):(bc_wide?-214:-160);
    const unsigned family=MEM_BU(S32(0x8021592E),0);
    if(family>1)return;
    const int16_t end=MEM_H(0,S32(0x802E8C98+family*2));
    unsigned count=0;
    while(count<44&&MEM_H(int32_t(count*2),S32(0x802158A8))!=end)count++;
    if(count==44)return; // Refuse to overwrite neighboring frontend state.
    const unsigned target=bc_wide?36:27;
    const int16_t blank=MEM_H(0,S32(0x802E8C94+family*2));
    for(unsigned i=count;i<target;i++)MEM_H(int32_t(i*2),S32(0x802158A8))=blank;
    MEM_H(int32_t(target*2),S32(0x802158A8))=end;
}

inline void bc_widescreen_ticker_text(uint8_t* rdram,recomp_context* ctx) {
    // Sixth o32 argument is the text origin x; all other text callers remain
    // untouched. Both the main text and its shadow receive the same offset.
    if(bc_wide)MEM_W(0x14,ctx->r29)-=54;
}
