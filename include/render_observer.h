#pragma once
#include <cstdint>
enum class BcRenderSkip { Deadline, NewWorkload, Presentation, Reduction };
struct BcRenderWindow {
 uint64_t presents=0,failed=0,renders=0,intermediate=0,mapped=0,transforms=0,quarter=0,half=0,three_quarter=0;
 double seconds=0,present_fps=0;
 uint64_t deadline_skips=0,new_workload_skips=0,presentation_skips=0,reduction_skips=0;
};
class BcRenderObserver {
 uint64_t begin_=0;bool started_=false;BcRenderWindow window_;
public:
 void present(uint64_t ns,bool success);
 void render(uint64_t ns,bool generated,float weight,uint32_t mapped,uint32_t transforms);
 void skip(uint64_t ns,BcRenderSkip reason,uint32_t count);
 BcRenderWindow snapshot(uint64_t ns);
};
bool bc_render_observing();
bool bc_render_selecting();
void bc_render_skipped(BcRenderSkip reason,uint32_t count);
void bc_render_present(bool success);
void bc_render_generated(bool generated,float weight,uint32_t mapped,uint32_t transforms);
void bc_render_report(uint64_t vi);
void bc_render_selection(uint32_t vi_fb,uint32_t selected_fb,int selected_frames,int batch_count,int available,bool enabled,int modified_count,bool modified_match);
