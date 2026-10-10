#include "render_observer.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include "prototype.h"
namespace {
BcRenderObserver observer;std::mutex mutex;
uint64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}
bool bc_render_observing(){static bool enabled=std::getenv("BC_TRACE_RENDER")!=nullptr;return enabled;}
bool bc_render_selecting(){static bool enabled=std::getenv("BC_TRACE_SELECTION")!=nullptr;return enabled;}
void bc_render_skipped(BcRenderSkip reason,uint32_t count){if(!bc_render_observing())return;auto ns=now();std::lock_guard lock(mutex);observer.skip(ns,reason,count);}
void bc_render_selection(uint32_t vi_fb,uint32_t selected_fb,int selected_frames,int batch_count,int available,bool enabled,int modified_count,bool modified_match){
 if(!bc_render_selecting())return;
 fprintf(stderr,"{\"event\":\"rt64_present_selection\",\"vi\":%llu,\"vi_framebuffer\":%u,\"selected_framebuffer\":%u,\"selected_frames\":%d,\"batch_count\":%d,\"available\":%d,\"interpolation_enabled\":%s,\"modified_framebuffers\":%d,\"modified_match\":%s}\n",(unsigned long long)bc_vis.load(),vi_fb,selected_fb,selected_frames,batch_count,available,enabled?"true":"false",modified_count,modified_match?"true":"false");
}
void bc_render_present(bool success){if(!bc_render_observing())return;auto ns=now();std::lock_guard lock(mutex);observer.present(ns,success);}
void bc_render_generated(bool generated,float weight,uint32_t mapped,uint32_t transforms){if(!bc_render_observing())return;auto ns=now();std::lock_guard lock(mutex);observer.render(ns,generated,weight,mapped,transforms);}
void bc_render_report(uint64_t vi){
 if(!bc_render_observing())return;auto ns=now();BcRenderWindow window;
 {std::lock_guard lock(mutex);window=observer.snapshot(ns);}
 fprintf(stderr,"{\"event\":\"rt64_actual_frames\",\"vi\":%llu,\"seconds\":%.6f,\"successful_present_submissions\":%llu,\"failed_present_submissions\":%llu,\"present_submission_fps\":%.3f,\"completed_renders\":%llu,\"fractional_renders\":%llu,\"mapped_transforms\":%llu,\"observed_transforms\":%llu,\"weight_quarter\":%llu,\"weight_half\":%llu,\"weight_three_quarter\":%llu,\"deadline_skips\":%llu,\"new_workload_skips\":%llu,\"presentation_skips\":%llu,\"reduction_skips\":%llu}\n",(unsigned long long)vi,window.seconds,(unsigned long long)window.presents,(unsigned long long)window.failed,window.present_fps,(unsigned long long)window.renders,(unsigned long long)window.intermediate,(unsigned long long)window.mapped,(unsigned long long)window.transforms,(unsigned long long)window.quarter,(unsigned long long)window.half,(unsigned long long)window.three_quarter,(unsigned long long)window.deadline_skips,(unsigned long long)window.new_workload_skips,(unsigned long long)window.presentation_skips,(unsigned long long)window.reduction_skips);
}
