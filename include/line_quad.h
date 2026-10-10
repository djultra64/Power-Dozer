#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace bc {
struct LinePoint { float x, y; };
inline bool line_quad(LinePoint a, LinePoint b, int width, std::array<LinePoint,4>& out) {
 const float dx=b.x-a.x,dy=b.y-a.y,length=std::hypot(dx,dy);
 const float thickness=1.5f+width*0.5f;
 if(!std::isfinite(length)||length<0.0001f||thickness<=0)return false;
 const float x=-dy/length*thickness*0.5f,y=dx/length*thickness*0.5f;
 out={LinePoint{a.x+x,a.y+y},LinePoint{a.x-x,a.y-y},LinePoint{b.x-x,b.y-y},LinePoint{b.x+x,b.y+y}};
 for(auto p:out)if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>8191||std::abs(p.y)>8191)return false;
 return true;
}
inline uint32_t line_xy(LinePoint p) {
 return uint32_t(uint16_t(int16_t(std::lround(p.x*4))))<<16 | uint16_t(int16_t(std::lround(p.y*4)));
}
}
