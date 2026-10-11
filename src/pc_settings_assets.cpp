#include "pc_settings_assets.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace bc {
namespace {
[[noreturn]] void invalid(){throw std::runtime_error("Invalid original PC settings assets");}
std::span<const uint8_t> slice(std::span<const uint8_t> b,size_t p,size_t n){if(p>b.size()||n>b.size()-p)invalid();return b.subspan(p,n);}
unsigned be16(std::span<const uint8_t> b,size_t p){auto s=slice(b,p,2);return unsigned(s[0])<<8|s[1];}
uint32_t be32(std::span<const uint8_t> b,size_t p){return uint32_t(be16(b,p))<<16|be16(b,p+2);}
std::array<uint8_t,512> decode(std::span<const uint8_t> in){
 if(in.size()%2)invalid();
 std::array<uint8_t,512> out{};size_t used=0;
 for(size_t p=0;p<in.size();p+=2){unsigned token=be16(in,p);
  if(token&0x8000){size_t offset=(token&0x7fff)>>5,count=(token&31)*2;if(!offset||offset%2||offset>used||count>out.size()-used)invalid();for(size_t n=0;n<count;++n){out[used]=out[used-offset];++used;}}
  else {if(out.size()-used<2)invalid();for(unsigned b:{token>>8,token&255})out[used++]=uint8_t(((b&0x38)<<2)|((b&7)<<1));}
 }
 if(used!=out.size())invalid();
 return out;
}
}
PcSettingsAssets load_pc_settings_assets(std::span<const uint8_t> rom,std::span<const uint8_t> code){
 constexpr size_t table=0x4ce0,base=0x802447c0;
 PcSettingsAssets assets;assets.font_rgba.resize(assets.font_width*assets.font_height*4);assets.glyph_advances.fill(.6f);
 for(size_t g=0;g<assets.glyph_count;++g){size_t entry=table+(0xf4c+g)*8;slice(rom,entry,8);if(be16(rom,entry+6)!=6)invalid();uint32_t offset=be32(rom,entry);if(offset>rom.size()||table>rom.size()-offset)invalid();auto raw=decode(slice(rom,table+offset,be16(rom,entry+4)));
  for(size_t p=0;p<1024;++p){unsigned n=p%2?raw[p/2]&15:raw[p/2]>>4;size_t q=((g/8*32+31-p/32)*assets.font_width+g%8*32+p%32)*4;std::fill_n(assets.font_rgba.begin()+q,3,255);assets.font_rgba[q+3]=uint8_t(n*17);}
 }
 for(size_t i=0;i<assets.palette.size();++i){auto s=slice(code,0x802f47b0-base+i*8,4);std::copy(s.begin(),s.end(),assets.palette[i].begin());}
 for(size_t i=0;i<assets.shadow_vertices.size();++i){auto s=slice(code,0x802f97b0-base+i*16,16);assets.shadow_vertices[i]={std::bit_cast<int16_t>(uint16_t(be16(s,0))),std::bit_cast<int16_t>(uint16_t(be16(s,2))),s[15]};}
 // Original F3D list starts with a vertex load, then fourteen G_TRI1 commands.
 for(size_t i=0;i<assets.shadow_triangles.size();++i){auto s=slice(code,0x802f98b0-base+8+i*8,8);if(s[0]!=0xbf)invalid();for(size_t j=0;j<3;++j){if(s[5+j]%10||s[5+j]/10>=16)invalid();assets.shadow_triangles[i][j]=s[5+j]/10;}}
 return assets;
}
}
