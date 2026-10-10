#pragma once
#include <cstdint>

extern double bc_scale;

inline bool bc_map_vehicle_sprite_eligible(uint8_t* rdram,uint32_t slot) {
    return bc_scale>1.0 && slot<64 && MEM_BU(int32_t(slot),S32(0x8036C1E0))==2 && MEM_BU(int32_t(slot),S32(0x8036C220))==1;
}

// Only the two map vehicle callers use this repair. Their 64x32 RGBA16
// strips fill 4 KiB TMEM, but use a mirrored 64-row render domain. Native
// 320px samples address rows 63..32, which wrap to 31..0 in TMEM. At higher
// resolutions the outer bilerp tap reaches row 31, wrapping to the opposite
// edge. An explicit mirrored 32-row domain preserves native samples and
// reflects that extra tap onto the correct border row. Coordinates, scale,
// texture loads and all unrelated display-list commands stay unchanged.
// Validate the entire expected two-strip layout before changing any word.
inline bool bc_map_vehicle_sprite_rewrite(uint8_t* rdram,uint32_t begin,uint32_t end,bool eligible) {
    const uint32_t first=begin&0x1FFFFFFF,last=end&0x1FFFFFFF;
    if(!eligible || (first&7) || (last&7) || last<=first || last>0x800000 || last-first>0x1000)return false;
    auto word=[&](uint32_t at)->int32_t& { return MEM_W(0,S32(0x80000000u|at)); };
    uint32_t tiles[2]={},coords[2]={};unsigned tileCount=0,rectCount=0;
    bool currentTile=false;
    for(uint32_t at=first;at<last;at+=8) {
        const uint32_t w0=uint32_t(word(at)),w1=uint32_t(word(at+4));
        if((w0>>24)==0xF5 && ((w1>>24)&7)==0) {
            if(tileCount==2 || w0!=0xF5102000 || w1!=0x00058200 || at+16>last ||
               uint32_t(word(at+8))!=0xF2000000 || uint32_t(word(at+12))!=0x000FC07C)return false;
            tiles[tileCount++]=at+4;currentTile=true;
        }
        if((w0>>24)==0xE4) {
            if(!currentTile || rectCount==2 || ((w1>>24)&7)!=0 || at+24>last ||
               uint32_t(word(at+8))!=0xB3000000 || uint32_t(word(at+16))!=0xB2000000 ||
               (uint32_t(word(at+12))&0xFFFF)!=0x0800)return false;
            coords[rectCount++]=at+12;currentTile=false;at+=16;
        }
    }
    if(tileCount!=2 || rectCount!=2)return false;
    for(unsigned i=0;i<2;i++) {
        word(tiles[i])=(uint32_t(word(tiles[i]))&~0x3C000u)|(5u<<14);
        word(coords[i])=(uint32_t(word(coords[i]))&0xFFFF0000u)|0x0400u;
    }
    return true;
}
