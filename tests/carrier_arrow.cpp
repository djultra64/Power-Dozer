#include "carrier_arrow.h"
#include <cassert>

int main() {
 assert(bc::carrier_arrow_vertices(0x002FD9B8,10,0));
 assert(!bc::carrier_arrow_vertices(0x002FD9B8,9,0));
 assert(!bc::carrier_arrow_vertices(0x002FD9B8,10,1));
 assert(!bc::carrier_arrow_vertices(0x002FD9B0,10,0));
 std::vector<RT64::TransformGroup> groups(1);
 uint32_t world=0, projection=0;
 bc::annotate_carrier_arrow(groups,world,projection);
 assert(world != projection && world != 0 && projection != 0);
 assert(groups[0].matrixId == G_EX_ID_AUTO);
 assert(groups[0].aspectMode == G_EX_ASPECT_AUTO);
 assert(groups[0].positionInterpolation == G_EX_COMPONENT_AUTO);
 assert(groups[world].matrixId != G_EX_ID_AUTO);
 assert(groups[projection].matrixId != G_EX_ID_AUTO);
 assert(groups[world].matrixId != groups[projection].matrixId);
 assert(groups[projection].aspectMode == G_EX_ASPECT_ADJUST);
 for(auto index:{world,projection}) {
  const auto& group=groups[index];
  assert(group.positionInterpolation == G_EX_COMPONENT_SKIP);
  assert(group.rotationInterpolation == G_EX_COMPONENT_SKIP);
  assert(group.scaleInterpolation == G_EX_COMPONENT_SKIP);
  assert(group.skewInterpolation == G_EX_COMPONENT_SKIP);
  assert(group.perspectiveInterpolation == G_EX_COMPONENT_SKIP);
  assert(group.vertexInterpolation == G_EX_COMPONENT_SKIP);
 }
 auto count=groups.size();
 uint32_t next_world=0,next_projection=0;
 bc::annotate_carrier_arrow(groups,next_world,next_projection);
 assert(groups.size()==count+2);
 assert(groups[next_world].matrixId==groups[world].matrixId);
 assert(groups[next_projection].matrixId==groups[projection].matrixId);
}
