#include "render_observer.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <mutex>
namespace {
BcRenderObserver observer;std::mutex mutex;
uint64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}
bool bc_render_observing(){static bool enabled=std::getenv("BC_TRACE_RENDER")!=nullptr;return enabled;}
void bc_render_present(bool success){if(!bc_render_observing())return;auto ns=now();std::lock_guard lock(mutex);observer.present(ns,success);}
void bc_render_generated(bool generated,float weight,uint32_t mapped,uint32_t transforms){if(!bc_render_observing())return;auto ns=now();std::lock_guard lock(mutex);observer.render(ns,generated,weight,mapped,transforms);}
void bc_render_report(uint64_t vi){
 if(!bc_render_observing())return;auto ns=now();BcRenderWindow window;
 {std::lock_guard lock(mutex);window=observer.snapshot(ns);}
 fprintf(stderr,"{\"event\":\"rt64_actual_frames\",\"vi\":%llu,\"seconds\":%.6f,\"successful_present_submissions\":%llu,\"failed_present_submissions\":%llu,\"present_submission_fps\":%.3f,\"completed_renders\":%llu,\"fractional_renders\":%llu,\"mapped_transforms\":%llu,\"observed_transforms\":%llu,\"weight_quarter\":%llu,\"weight_half\":%llu,\"weight_three_quarter\":%llu}\n",(unsigned long long)vi,window.seconds,(unsigned long long)window.presents,(unsigned long long)window.failed,window.present_fps,(unsigned long long)window.renders,(unsigned long long)window.intermediate,(unsigned long long)window.mapped,(unsigned long long)window.transforms,(unsigned long long)window.quarter,(unsigned long long)window.half,(unsigned long long)window.three_quarter);
}
