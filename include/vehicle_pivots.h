#pragma once
#include "pivot_interpolation.h"
#include <map>
#include <optional>
#include <cmath>
#include <cstring>

namespace bc {
using VehiclePivots=std::map<uint32_t,hlslpp::float3>;
using VehicleSkins=std::map<uint32_t,VehiclePivots>;
// Animation bind matrices are sixteen signed 16.16 words, not the packed
// N64 matrix format. Recover the model point which the bind maps to origin.
inline std::optional<hlslpp::float3> bind_pivot(const hlslpp::float4x4& bind) {
 const auto a=bind[0].xyz,b=bind[1].xyz,c=bind[2].xyz,t=bind[3].xyz;
 const float determinant=hlslpp::dot(a,hlslpp::cross(b,c));
 if(!std::isfinite(determinant)||std::abs(determinant)<1e-6f)return std::nullopt;
 const hlslpp::float3 p(-float(hlslpp::dot(t,hlslpp::cross(b,c)))/determinant,
  -float(hlslpp::dot(t,hlslpp::cross(c,a)))/determinant,
  -float(hlslpp::dot(t,hlslpp::cross(a,b)))/determinant);
 if(!std::isfinite(float(p.x))||!std::isfinite(float(p.y))||!std::isfinite(float(p.z)))return std::nullopt;
 return p;
}
inline VehiclePivots read_vehicle_pivots(const uint8_t* ram,uint32_t source,uint32_t output,uint32_t channels) {
 VehiclePivots result;
 source&=0x7FFFFFFF;output&=0x7FFFFFFF;channels&=0x7FFFFFFF;
 auto valid=[](uint64_t a,uint64_t size){return a+size<=0x800000;};
 if(!valid(source,64)||!valid(output,64)||!valid(channels,32*24))return result;
 auto byte=[&](uint32_t a){return ram[a^3];};
 auto word=[&](uint32_t a){uint32_t v;std::memcpy(&v,ram+a,4);return v;};
 for(unsigned channel=0;channel<32;++channel){
  const auto at=channels+channel*24;
  if(!byte(at+0x10))continue;
  const auto blob=word(at)&0x7FFFFFFF;
  if(!valid(blob,2))continue;
  const auto frames=byte(blob);
  if(!frames||!valid(blob,frames+2))continue;
  const auto count=byte(blob+frames+1);
  const auto stride=8+20*uint32_t(frames);
  const auto parts=(blob+frames+5)&~uint32_t(3);
  if(!valid(parts,uint64_t(count)*stride))continue;
  for(unsigned part=0;part<count;++part){
   const auto record=parts+part*stride;
   const auto destination=word(record),binding=word(record+4);
   if((destination&3)||(binding&3)||!valid(uint64_t(output)+destination,64)||!valid(uint64_t(source)+binding,64))continue;
   hlslpp::float4x4 bind;
   for(unsigned row=0;row<4;++row){
    float values[4];
    for(unsigned col=0;col<4;++col)values[col]=int32_t(word(source+binding+row*16+col*4))/65536.0f;
    bind[row]=hlslpp::float4(values[0],values[1],values[2],values[3]);
   }
   if(auto pivot=bind_pivot(bind))result[destination]=*pivot;
  }
 }
 return result;
}
VehicleSkins snapshot_vehicle_pivots();
}
void bc_register_vehicle_pivots(uint8_t*,uint32_t,uint32_t,uint32_t);
