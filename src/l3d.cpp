#include "l3d.h"
#include "line_quad.h"
#include "carrier_arrow.h"
#include "input_glyph.h"
#include "prototype.h"
#include "hle/rt64_interpreter.h"
#include "gbi/rt64_gbi_f3d.h"
#include "gbi/rt64_gbi_rdp.h"
#include "contrib/xxHash/xxhash.h"
#include <cstdio>
#include <cstdlib>
#include <set>

namespace {
RT64::GBI l3d;
bool initialized=false;
RT64::GBI hud_gbi;
void menu_background_rectangle(RT64::State* state,RT64::DisplayList** dl) {
 // Private opcode D4 is emitted only by the verified frontend backdrop
 // producer. Its 64x16 tiles cannot trigger RT64's full-width heuristic.
 // Explicit stretch preserves their common texture coordinates and edges.
 auto& rdp=*state->rdp;
 const auto previous=rdp.extended.global.rectAspect;
 if(bc_wide)rdp.setRectAspect(G_EX_ASPECT_STRETCH);
 RT64::GBI_RDP::texrect(state,dl);
 rdp.setRectAspect(previous);
}
void atlas_glyph(RT64::State* state,RT64::DisplayList** dl) {
 auto descriptor=bc::decode_glyph_command({(*dl)->w0,(*dl)->w1});
 if(!descriptor)return;
 auto mesh=bc::glyph_mesh(*descriptor);
 if(mesh.positions.empty())return;
 if(std::getenv("BC_TRACE_INPUT")){
  static std::set<uint16_t> seen;
  if(seen.insert(descriptor->sprite.index).second){fprintf(stderr,"{\"event\":\"atlas_glyph_rendered\",\"sprite\":\"%.*s\",\"x\":%d,\"y\":%d,\"size\":%d,\"triangles\":%zu}\n",int(bc::glyph_name(descriptor->sprite).size()),bc::glyph_name(descriptor->sprite).data(),descriptor->x,descriptor->y,descriptor->size,mesh.positions.size()/12);}
 }
 // Raw triangles bypass RSP matrices. Match the centered, proportional
 // original UI in the renderer's fixed 16:9 mode rather than stretching keys.
 if(bc_wide)for(size_t i=0;i<mesh.positions.size();i+=4)
  mesh.positions[i]=160+(mesh.positions[i]-160)*.75f;
 auto& rdp=*state->rdp;auto& rsp=*state->rsp;
 state->flush();
 const auto textureOn=state->drawCall.textureOn;
 const auto textureTile=state->drawCall.textureTile;
 const auto textureLevels=state->drawCall.textureLevels;
 rsp.pushOtherMode();rdp.pushCombine();rsp.pushGeometryMode();
 rsp.clearGeometryMode(~uint32_t(0));
 rsp.setGeometryMode(G_SHADE|rsp.shadingSmoothMask);
 // SHADE, one cycle, straight alpha, no depth/fog/texture sampling.
 // RT64 packs N64 w1 above w0 (GBI_RDP::setCombine), not the
 // display-list word order. Reversing these words produces zero alpha.
 rdp.setCombine(uint64_t(0xFFFE793C)<<32|0x00FFFFFF);
 rsp.setOtherMode(0xF0,0x00504240);
 rdp.drawTris(uint32_t(mesh.positions.size()/12),mesh.positions.data(),mesh.texcoords.data(),mesh.colours.data(),0,0);
 // Capture glyph state before restoring guest state for following draws.
 state->flush();
 rsp.popGeometryMode();rdp.popCombine();rsp.popOtherMode();
 state->drawCall.textureOn=textureOn;
 state->drawCall.textureTile=textureTile;
 state->drawCall.textureLevels=textureLevels;
 state->updateDrawStatusAttribute(RT64::DrawAttribute::Texture);
}
void hud_vertex(RT64::State* state,RT64::DisplayList** dl) {
 auto& rsp=*state->rsp;
 const auto address=rsp.fromSegmentedMasked((*dl)->w1);
 const auto count=(*dl)->p0(20,4)+1,first=(*dl)->p0(16,4);
 RT64::GBI_F3D::vertex(state,dl);
 if(!bc::carrier_arrow_vertices(address,count,first))return;
 auto& data=state->ext.workloadQueue->workloads[state->ext.workloadQueue->writeCursor].drawData;
 if(rsp.curTransformIndex>=data.worldTransformGroups.size()||rsp.curViewProjIndex>=data.viewProjTransformGroups.size())
  bc_fatal("carrier_arrow_transform");
 bc::annotate_carrier_arrow(data.transformGroups,
  data.worldTransformGroups[rsp.curTransformIndex],data.viewProjTransformGroups[rsp.curViewProjIndex]);
}
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
void bc_load_hud_annotations(RT64::Interpreter& interpreter,unsigned text) {
 // The verified USA Rev 1 main F3D program. L3D and other GBIs retain their
 // own vertex handlers. Only the carrier's exact ten-vertex mesh is tagged.
 if(text!=0x2E53F0||!interpreter.hleGBI||interpreter.hleGBI->ucode!=RT64::GBIUCode::F3D)return;
 hud_gbi=*interpreter.hleGBI;
 hud_gbi.map[F3D_G_VTX]=hud_vertex;
 hud_gbi.map[0xD4]=menu_background_rectangle;
 hud_gbi.map[0xD5]=atlas_glyph;
 interpreter.hleGBI=&hud_gbi;
 interpreter.state->rsp->setGBI(&hud_gbi);
}
