#include "translucent_mode.h"
#include <cstdlib>
static void check(bool valid){if(!valid)std::abort();}

int main(){
 constexpr uint32_t from[]{0x00552078,0x005049D8,0x00553078};
 constexpr uint32_t to[]{0x00552230,0x00504A50,0x00553078};
 for(unsigned i=0;i<3;i++){
  auto original=bc::translucent_mode(from[i]);check(original&&original->index==i&&original->value==to[i]);
  // Original assets also use G_RM_PASS in cycle 1. Preserve that cycle
  // and apply exactly the original rule to cycle 2 and the low flags.
  auto pass_input=(from[i]&0x3333FFFFu)|0x0C080000u;
  auto result=bc::translucent_mode(pass_input);
  check(result&&result->index==i);
  check(result->value==((to[i]&0x3333FFFFu)|0x0C080000u));
  check((result->value&0xCCCC0000u)==(pass_input&0xCCCC0000u));
 }
 check(!bc::translucent_mode(0));check(!bc::translucent_mode(0xDEADBEEF));
 check(!bc::translucent_mode(0x0C192079));
 check(!bc::translucent_mode(0xC8192078)); // Fog is not a validated rule.
}
