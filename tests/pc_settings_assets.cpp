#include "pc_settings_assets.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <iterator>
using namespace bc;
static void check(bool v){if(!v)throw std::runtime_error("asset assertion");}
static void put16(std::vector<uint8_t>& b,size_t p,unsigned v){b[p]=v>>8;b[p+1]=v;}
static void put32(std::vector<uint8_t>& b,size_t p,unsigned v){put16(b,p,v>>16);put16(b,p+2,v);}
static std::vector<uint8_t> read(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
 if(argc==4){auto a=load_pc_settings_assets(read(argv[1]),read(argv[2]));auto ref=read(argv[3]);check(ref.size()==51*512);for(size_t g=0;g<51;++g)for(size_t p=0;p<1024;++p){auto b=ref[g*512+p/2];auto n=p%2?b&15:b>>4;size_t pixel=((g/8*32+31-p/32)*256+g%8*32+p%32)*4;check(a.font_rgba[pixel+3]==n*17);}std::cout<<"26112 original CPU bytes match\n";return 0;}
 std::vector<uint8_t> rom(0x1c000),code(0xb6000);const size_t start=0x15000;
 for(size_t g=0;g<51;++g){size_t t=0x4ce0+(0xf4c+g)*8;put32(rom,t,start+g*512-0x4ce0);put16(rom,t+4,512);put16(rom,t+6,6);for(size_t p=0;p<512;p+=2)put16(rom,start+g*512+p,((g%64)<<8)|((p/2)%64));}
 size_t pal=0x802f47b0-0x802447c0,vert=0x802f97b0-0x802447c0,idx=0x802f98b0-0x802447c0;
 code[pal]=200;code[pal+3]=255;put16(code,vert,65534);put16(code,vert+2,123);code[vert+15]=160;
 for(size_t i=0;i<14;++i){code[idx+8+i*8]=0xbf;code[idx+8+i*8+5]=0;code[idx+8+i*8+6]=50;code[idx+8+i*8+7]=140;}
 auto a=load_pc_settings_assets(rom,code);check(a.font_rgba.size()==256*224*4);check(a.glyph_advances[50]==.6f);check(a.palette[0][0]==200);check(a.shadow_vertices[0].x==-2&&a.shadow_vertices[0].y==123&&a.shadow_vertices[0].alpha==160);check(a.shadow_triangles[13][2]==14);
 for(size_t g=0;g<51;++g)for(size_t p=0;p<1024;++p){unsigned b=p%4<2?g%64:(p/4)%64;unsigned expanded=((b&0x38)<<2)|((b&7)<<1);unsigned n=p%2?expanded&15:expanded>>4;size_t q=((g/8*32+31-p/32)*256+g%8*32+p%32)*4;check(a.font_rgba[q]==255&&a.font_rgba[q+3]==n*17);}
 auto rejects=[&](auto r,auto c){try{load_pc_settings_assets(r,c);}catch(const std::runtime_error&){return;}throw std::runtime_error("accepted malformed asset");};
 rejects(std::span<const uint8_t>(rom).first(0x4ce0+0xf7e*8+7),std::span<const uint8_t>(code));rejects(std::span<const uint8_t>(rom),std::span<const uint8_t>(code).first(idx+119));
 auto bad=rom;put16(bad,0x4ce0+0xf4c*8+4,511);rejects(bad,code);bad=rom;put32(bad,0x4ce0+0xf4c*8,0xffffffff);rejects(bad,code);bad=rom;put16(bad,start,0x8001);rejects(bad,code);bad=rom;put16(bad,start,0x8041);rejects(bad,code);
 // Overlapping backreferences repeat a literal pair to exactly 512 bytes.
 bad=rom;size_t p=start;put16(bad,p,0x3f07);p+=2;for(int i=0;i<8;++i){put16(bad,p,0x805f);p+=2;}put16(bad,p,0x8047);p+=2;put16(bad,0x4ce0+0xf4c*8+4,p-start);a=load_pc_settings_assets(bad,code);check(a.font_rgba[31*256*4+3]==238);put16(bad,p-2,0x8048);rejects(bad,code);
 bad=rom;put16(bad,0x4ce0+0xf4c*8+6,5);rejects(bad,code);bad=rom;put16(bad,0x4ce0+0xf4c*8+4,2);rejects(bad,code);
 auto invalidcode=code;invalidcode[idx+13]=161;rejects(rom,invalidcode);
 std::cout<<"synthetic font, orientation, indexing, palette, shadow and bounds passed\n";
}
