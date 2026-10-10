#include "audio_sdl.h"
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <chrono>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static uint64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int main(){
 check(SDL_Init(SDL_INIT_AUDIO)==0,"SDL audio initialization");
 bc::SDLAudioBackend backend;bc::AudioOutput output(backend);
 auto time=now();output.frequency(22050,time);check(output.online()&&backend.device(),"real SDL output opens");
 std::vector<int16_t> pcm(4410,100);output.push(pcm,time);
 auto bytes=output.queued(now());check(bytes>0&&bytes<=8820,"PCM reaches SDL queue");
 SDL_Delay(25);auto remaining=output.queued(now());check(remaining<bytes,"SDL consumes queued PCM");
 backend.close();time=now();output.poll(time);check(!output.online(),"stopped SDL backend detected without a removal event");
 check(output.queued(time)==remaining,"lost SDL backend retains last observed AI budget");
 check(output.queued(time+100000000)==0,"silent clock finishes the pending guest buffer");
 output.poll(time+1000000000);check(output.online()&&backend.device(),"real SDL backend reopens after loss");
 output.push(pcm,time+1000000000);check(output.queued(time+1000000000)>0,"new PCM reaches recovered output");
 backend.close();SDL_Quit();puts("Real SDL output queue and recovery passed");
}
