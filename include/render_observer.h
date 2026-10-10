#pragma once
#include <cstdint>
struct BcRenderWindow {
 uint64_t presents=0,failed=0,renders=0,intermediate=0,mapped=0,transforms=0,quarter=0,half=0,three_quarter=0;
 double seconds=0,present_fps=0;
};
class BcRenderObserver {
 uint64_t begin_=0;bool started_=false;BcRenderWindow window_;
public:
 void present(uint64_t ns,bool success);
 void render(uint64_t ns,bool generated,float weight,uint32_t mapped,uint32_t transforms);
 BcRenderWindow snapshot(uint64_t ns);
};
bool bc_render_observing();
void bc_render_present(bool success);
void bc_render_generated(bool generated,float weight,uint32_t mapped,uint32_t transforms);
void bc_render_report(uint64_t vi);
