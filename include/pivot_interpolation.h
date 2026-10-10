#pragma once
#include "common/rt64_math.h"

namespace bc {
// The guest keeps animated vertices in model space and builds a rotation
// about an off-origin pivot. Express the same draw in pivot space so RT64's
// independent translation/rotation interpolation keeps the axle attached.
inline hlslpp::float4x4 rebase_pivot_transform(const hlslpp::float4x4& matrix,
 const hlslpp::float3& pivot) {
 auto result=matrix;
 result[3]=hlslpp::mul(hlslpp::float4(pivot,1),matrix);
 return result;
}
inline hlslpp::float3 rebase_pivot_vertex(const hlslpp::float3& vertex,
 const hlslpp::float3& pivot) {
 return vertex-pivot;
}
}
