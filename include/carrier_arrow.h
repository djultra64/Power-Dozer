#pragma once
#include <cstdint>
#include <vector>
#include "hle/rt64_transform_group.h"

namespace bc {
inline bool carrier_arrow_vertices(uint32_t address, uint32_t count, uint32_t first) {
 return address == 0x002FD9B8 && count == 10 && first == 0;
}
inline void annotate_carrier_arrow(std::vector<RT64::TransformGroup>& groups,
 uint32_t& world_group, uint32_t& projection_group) {
 // Keep the HUD out of the automatic matching of world/camera matrices.
 // Clone groups: group zero and the RSP's current stack remain untouched.
 auto hud=groups.at(world_group), projection=groups.at(projection_group);
 auto fixed=[](RT64::TransformGroup& group,uint32_t id) {
  group.matrixId=id;
  group.positionInterpolation=group.rotationInterpolation=G_EX_COMPONENT_SKIP;
  group.scaleInterpolation=group.skewInterpolation=G_EX_COMPONENT_SKIP;
  group.perspectiveInterpolation=group.vertexInterpolation=G_EX_COMPONENT_SKIP;
  group.texcoordInterpolation=group.tileInterpolation=G_EX_COMPONENT_SKIP;
  group.lookAtInterpolation=G_EX_COMPONENT_SKIP;
 };
 fixed(hud,0xBC000001);fixed(projection,0xBC000002);
 projection.aspectMode=G_EX_ASPECT_ADJUST;
 world_group=uint32_t(groups.size());groups.push_back(hud);
 projection_group=uint32_t(groups.size());groups.push_back(projection);
}
}
