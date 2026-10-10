#include "hle/rt64_rigid_body.h"
#include "../include/rt64_extended_gbi.h"
#include "pivot_interpolation.h"
#include "vehicle_pivots.h"
#include <vector>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdio>

static auto spin(float a) {
 return hlslpp::float4x4(1,0,0,0,0,std::cos(a),std::sin(a),0,0,-std::sin(a),std::cos(a),0,0,0,0,1);
}
static auto translate(hlslpp::float3 p) {
 auto m=hlslpp::float4x4::identity();m[3]=hlslpp::float4(p,1);return m;
}
static auto point(hlslpp::float3 p,const hlslpp::float4x4& m) {
 return hlslpp::mul(hlslpp::float4(p,1),m).xyz;
}
static void near(hlslpp::float3 a,hlslpp::float3 b) {
 const float error=hlslpp::length(a-b);
 if(error>0.001f)fprintf(stderr,"Pivot moved by %.6f world units\n",error);
 assert(error<0.001f);
}
static void skin_bindings() {
 std::vector<uint8_t> ram(0x800000);
 auto byte=[&](uint32_t a,uint8_t v){ram[a^3]=v;};
 auto word=[&](uint32_t a,uint32_t v){std::memcpy(ram.data()+a,&v,4);};
 const uint32_t source=0x10000,output=0x20000,channels=0x30000,blob=0x40000;
 word(channels,0x80000000|blob);byte(channels+0x10,1);
 byte(blob,2);byte(blob+3,2); // two frames, two parts
 word(blob+4,0x80);word(blob+8,0x100);
 word(blob+52,0xC0);word(blob+56,0x180);
 auto bind=translate(hlslpp::float3(-26,-22,40));
 // A rotated bind must recover the same pivot, not merely negate translation.
 bind=hlslpp::mul(bind,spin(0.5f));
 for(unsigned row=0;row<4;++row){
  float values[4];hlslpp::store(bind[row],values);
  for(unsigned col=0;col<4;++col)word(source+0x100+row*16+col*4,uint32_t(int32_t(std::lround(values[col]*65536))));
 }
 auto pivots=bc::read_vehicle_pivots(ram.data(),0x80000000|source,0x80000000|output,0x80000000|channels);
 assert(pivots.size()==1); // the second bind is singular and must be skipped
 assert(pivots.count(0x80)==1);
 near(pivots.at(0x80),hlslpp::float3(26,22,-40));
 bc_register_vehicle_pivots(ram.data(),source,output,channels);
 auto snapshot=bc::snapshot_vehicle_pivots();
 assert(snapshot.at(source).count(0x80)==1 && snapshot.at(output).count(0x80)==1);
 byte(channels+0x10,0);
 bc_register_vehicle_pivots(ram.data(),source,output,channels);
 assert(bc::snapshot_vehicle_pivots().at(source).count(0x80)==1);
 assert(bc::snapshot_vehicle_pivots().at(output).count(0x80)==1);
 bc_register_vehicle_pivots(ram.data(),0,output,0);
 assert(bc::snapshot_vehicle_pivots().count(output)==0);
 assert(snapshot.at(output).count(0x80)==1); // an in-flight task keeps its snapshot
 bc_register_vehicle_pivots(ram.data(),0,source,0);
 assert(bc::snapshot_vehicle_pivots().empty());
 assert(bc::read_vehicle_pivots(ram.data(),source,output,channels).empty());
 assert(bc::read_vehicle_pivots(ram.data(),source,output,0x807FFFF0).empty());
}
int main() {
 skin_bindings();
 // The guest builds T(-pivot) * spin * T(pivot), then multiplies by
 // the vehicle transform. Its vertices are in vehicle, not axle, space.
 const hlslpp::float3 pivot(26,22,-40);
 for(float angle:{0.1f,1.0f,2.8f,-2.8f}) {
  auto prev=hlslpp::mul(translate(-pivot),spin(0.3f));
  prev=hlslpp::mul(prev,translate(pivot));
  prev=hlslpp::mul(prev,translate(hlslpp::float3(100,0,200)));
  auto cur=hlslpp::mul(translate(-pivot),spin(0.3f+angle));
  cur=hlslpp::mul(cur,translate(pivot));
  cur=hlslpp::mul(cur,translate(hlslpp::float3(120,3,230)));
  const auto a=bc::rebase_pivot_transform(prev,pivot), b=bc::rebase_pivot_transform(cur,pivot);
  const auto centered=bc::rebase_pivot_vertex(pivot,pivot);
  const hlslpp::float3 rim=pivot+hlslpp::float3(0,12,0);
  near(point(bc::rebase_pivot_vertex(rim,pivot),a),point(rim,prev));
  near(point(bc::rebase_pivot_vertex(rim,pivot),b),point(rim,cur));
  RT64::RigidBody body;
  body.updateDecomposition(a,true);body.updateDecomposition(b,true);
  body.updateLinear(a,b,G_EX_COMPONENT_INTERPOLATE);
  body.updateAngular(a,b,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE);
  for(float weight:{0.0f,0.25f,0.5f,0.75f,1.0f}) {
   auto interpolated=body.lerp(weight,a,b,true);
   near(point(centered,interpolated),hlslpp::lerp(point(pivot,prev),point(pivot,cur),weight));
   // A fast spin must retain the tire's radius as well as its axle.
   auto radius=hlslpp::length(point(bc::rebase_pivot_vertex(rim,pivot),interpolated)-point(centered,interpolated));
   assert(std::abs(float(radius)-12)<0.001f);
  }
 }
}
