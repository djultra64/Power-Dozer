#include "vehicle_pivots.h"
#include <mutex>
namespace {
std::mutex pivot_mutex;
bc::VehicleSkins skins;
}
void bc_register_vehicle_pivots(uint8_t* ram,uint32_t source,uint32_t output,uint32_t channels) {
 source&=0x7FFFFFFF;output&=0x7FFFFFFF;
 auto pivots=channels?bc::read_vehicle_pivots(ram,source,output,channels):bc::VehiclePivots{};
 std::lock_guard lock(pivot_mutex);
 if(!channels){skins.erase(output);return;}
 // Animation is built in source and copied to the alternate buffer later.
 // Keep the same coordinate space in both buffers and after a channel ends;
 // only a new model load invalidates its bind pivots.
 for(auto buffer:{source,output})for(const auto& [offset,pivot]:pivots)
  skins[buffer][offset]=pivot;

}
bc::VehicleSkins bc::snapshot_vehicle_pivots() {
 std::lock_guard lock(pivot_mutex);
 return skins;
}
