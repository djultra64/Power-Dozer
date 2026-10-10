#include "render_observer.h"
#include <stdexcept>
#include <cmath>
#include <cstdio>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 BcRenderObserver observer;observer.present(1000000000,true);observer.present(1010000000,false);observer.present(1020000000,true);
 auto sample=observer.snapshot(2000000000);check(sample.presents==2&&sample.failed==1&&sample.seconds==1&&sample.present_fps==2,"success/error counts over actual elapsed time");
 sample=observer.snapshot(3000000000);check(sample.presents==0&&sample.failed==0&&sample.seconds==1,"completed window resets counters without replay");
 observer.render(3000000000,true,.25f,4,10);observer.render(3001000000,true,.5f,5,12);observer.render(3002000000,true,.75f,6,13);observer.render(3003000000,true,1.f,7,14);observer.render(3004000000,false,.5f,0,9);
 sample=observer.snapshot(4000000000);check(sample.renders==5&&sample.intermediate==3&&sample.quarter==1&&sample.half==1&&sample.three_quarter==1,"fractional renders distinguish actual generated weights from a request or duplication");
 check(sample.mapped==22&&sample.transforms==58,"mapping observations retain actual sample totals");
 BcRenderObserver empty;sample=empty.snapshot(0);check(sample.seconds==0&&sample.present_fps==0,"empty snapshot is finite");
 empty.present(0,true);sample=empty.snapshot(0);check(sample.seconds==0&&std::isfinite(sample.present_fps),"zero-duration snapshot never divides by zero");
 puts("Actual present/render windows, weights and reset passed");
}
