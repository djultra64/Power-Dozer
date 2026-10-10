#include "l3d.h"
#include "line_quad.h"
#include "prototype.h"
#include "hle/rt64_interpreter.h"
#include "gbi/rt64_gbi_f3d.h"
#include "gbi/rt64_gbi_rdp.h"
#include "contrib/xxHash/xxhash.h"
#include <cstdio>

namespace {
RT64::GBI l3d;
bool initialized=false;
// L3D uses the F3D vertex cache and matrix commands. Its primitives are lines,
// not F3D's triangles/quads. Expand their projected endpoints into thin quads.
void line(RT64::State* state,unsigned a,unsigned b,int width,unsigned shade) {
 auto& rsp=*state->rsp;
 auto& data=state->ext.workloadQueue->workloads[state->ext.workloadQueue->writeCursor].drawData;
 if(a>=16||b>=16||shade>=16)bc_fatal("l3d_vertex_index",a);
 const unsigned ia=rsp.indices[a],ib=rsp.indices[b],ic=rsp.indices[shade];
 if(ia>=data.vertexCount()||ib>=data.vertexCount()||ic>=data.vertexCount())bc_fatal("l3d_vertex_unloaded",a);
 const auto pa=data.posScreen[ia],pb=data.posScreen[ib];
 if(float(data.posTransformed[ia].w)<=0||float(data.posTransformed[ib].w)<=0)return;
 std::array<bc::LinePoint,4> quad;
 if(!bc::line_quad({float(pa.x),float(pa.y)},{float(pb.x),float(pb.y)},width,quad))return;
 const uint32_t rgba=uint32_t(data.normColBytes[ic*4])<<24 | uint32_t(data.normColBytes[ic*4+1])<<16 | uint32_t(data.normColBytes[ic*4+2])<<8 | data.normColBytes[ic*4+3];
 // Slots 32..35 are outside the guest L3D cache. modifyVertex clones each
 // endpoint's complete RT64 attributes, leaving guest vertices untouched.
 for(unsigned i=0;i<4;i++) {
  rsp.indices[32+i]=(i<2)?ia:ib;
  rsp.used[32+i]=true;
  rsp.modifyVertex(32+i,G_MWO_POINT_XYSCREEN,bc::line_xy(quad[i]));
  rsp.modifyVertex(32+i,G_MWO_POINT_RGBA,rgba);
 }
 rsp.pushGeometryMode();
 rsp.clearGeometryMode(rsp.cullBothMask | G_LIGHTING);
 rsp.drawIndexedTri(32,33,34);
 rsp.drawIndexedTri(32,34,35);
 rsp.popGeometryMode();
}
void line_command(RT64::State* state,RT64::DisplayList** dl) {
 const auto word=(*dl)->w1;
 const unsigned a=((word>>16)&255)/10,b=((word>>8)&255)/10;
 line(state,a,b,int8_t(word&255),(word>>24)?b:a);
}
void triangle_outline(RT64::State* state,RT64::DisplayList** dl) {
 const auto word=(*dl)->w1;
 const unsigned a=((word>>16)&255)/10,b=((word>>8)&255)/10,c=((*dl)->w0&255)/10;
 const unsigned flag=word>>24,shade=(flag==1)?b:(flag==2)?c:a;
 if(a!=b)line(state,a,b,0,shade);
 if(b!=c)line(state,b,c,0,shade);
 if(c!=a)line(state,c,a,0,shade);
}
}
void bc_load_l3d(RT64::Interpreter& interpreter,unsigned text,unsigned data) {
 if(text!=0x207090||data!=0x210690)return;
 // Match RT64's fingerprints of word-swapped RDRAM, not just addresses.
 auto* ram=interpreter.state->RDRAM;
 if(XXH3_64bits(ram+text,0xFA8)!=0xCD640EC520D9F1E0ULL||XXH3_64bits(ram+data,0x800)!=0x4B2D1A8C46895B0DULL)bc_fatal("l3d_fingerprint",text);
 if(!initialized){
  l3d.ucode=RT64::GBIUCode::F3D;
  RT64::GBI_RDP::setup(&l3d,true);
  RT64::GBI_F3D::setup(&l3d);
  l3d.map[0xB5]=line_command;
  l3d.map[0xBF]=triangle_outline;
  initialized=true;
  fprintf(stdout,"{\"event\":\"l3d_adapter_ready\"}\n");fflush(stdout);
 }
 interpreter.hleGBI=&l3d;
 interpreter.state->rsp->setGBI(&l3d);
 l3d.resetFromTask(interpreter.state);
}
