#include "render_observer.h"
#include <cmath>
void BcRenderObserver::present(uint64_t ns,bool success){
 if(!started_){begin_=ns;started_=true;}
 if(success)++window_.presents;else ++window_.failed;
}
void BcRenderObserver::render(uint64_t ns,bool generated,float weight,uint32_t mapped,uint32_t transforms){
 if(!started_){begin_=ns;started_=true;}
 ++window_.renders;window_.mapped+=mapped;window_.transforms+=transforms;
 if(generated&&std::isfinite(weight)&&weight>0&&weight<1){
  ++window_.intermediate;
  window_.quarter+=std::abs(weight-.25f)<.00001f;
  window_.half+=std::abs(weight-.5f)<.00001f;
  window_.three_quarter+=std::abs(weight-.75f)<.00001f;
 }
}
BcRenderWindow BcRenderObserver::snapshot(uint64_t ns){
 auto result=window_;window_={};if(!started_)return result;
 result.seconds=ns>begin_?double(ns-begin_)/1e9:0;
 result.present_fps=result.seconds>0?result.presents/result.seconds:0;
 if(ns>begin_)begin_=ns;return result;
}
