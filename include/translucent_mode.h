#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace bc {
struct TranslucentMode {unsigned index;uint32_t value;};
// ROM mb.c's three rules, extended only to the G_RM_PASS first cycle
// already present in its level assets and ghost rendering table. GBI blend
// cycle 1 occupies CCCC0000; cycle 2 and shared flags occupy 3333FFFF.
inline std::optional<TranslucentMode> translucent_mode(uint32_t mode){
 constexpr std::array<uint32_t,3> from{0x00552078,0x005049D8,0x00553078};
 constexpr std::array<uint32_t,3> to{0x00552230,0x00504A50,0x00553078};
 for(unsigned i=0;i<from.size();i++){
  if(mode==from[i])return TranslucentMode{i,to[i]};
  if(mode==((from[i]&0x3333FFFFu)|0x0C080000u))return TranslucentMode{i,(to[i]&0x3333FFFFu)|0x0C080000u};
 }
 return {};
}
}
