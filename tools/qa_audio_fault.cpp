// Private LD_PRELOAD fault injector. Never linked into the port or distributed.
#include <SDL.h>
#include <dlfcn.h>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <mutex>
namespace {
std::mutex mutex;
std::atomic<bool> failed{false};
std::atomic<double> blocked_until{0};
std::atomic<SDL_AudioDeviceID> output_device{0};
double started=0;
double now(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
bool enabled(){const char* display=std::getenv("DISPLAY");return display&&std::strcmp(display,":0")&&std::strcmp(display,":0.0")&&std::getenv("BC_QA_AUDIO_FAULT_AFTER");}
double value(const char* name,double fallback){const char* text=std::getenv(name);return text?std::strtod(text,nullptr):fallback;}
bool removal(){const char* kind=std::getenv("BC_QA_AUDIO_FAULT_KIND");return kind&&!std::strcmp(kind,"removal");}
}
extern "C" SDL_AudioDeviceID SDL_OpenAudioDevice(const char* name,int capture,const SDL_AudioSpec* wanted,SDL_AudioSpec* actual,int changes){
 static auto original=reinterpret_cast<decltype(&SDL_OpenAudioDevice)>(dlsym(RTLD_NEXT,"SDL_OpenAudioDevice"));
 if(enabled()&&!capture){
  std::lock_guard lock(mutex);
  auto time=now();
  if(!started){started=time;blocked_until=time+value("BC_QA_AUDIO_STARTUP_OUTAGE",0);}
  if(time<blocked_until.load()){SDL_SetError("Private QA output unavailable");return 0;}
 }
 auto device=original(name,capture,wanted,actual,changes);
 if(enabled()&&!capture)output_device=device;
 return device;
}
extern "C" int SDL_QueueAudio(SDL_AudioDeviceID device,const void* data,Uint32 length){
 static auto original=reinterpret_cast<decltype(&SDL_QueueAudio)>(dlsym(RTLD_NEXT,"SDL_QueueAudio"));
 if(enabled()&&!removal()){
  std::lock_guard lock(mutex);auto time=now();
  if(started&&time-started>=value("BC_QA_AUDIO_FAULT_AFTER",45)&&!failed.exchange(true)){
   blocked_until=time+value("BC_QA_AUDIO_OUTAGE",8);
   fprintf(stderr,"{\"event\":\"qa_audio_fault\",\"phase\":\"queue_failure\"}\n");
   SDL_SetError("Private QA output disconnected");return -1;
  }
 }
 return original(device,data,length);
}
extern "C" int SDL_PollEvent(SDL_Event* event){
 static auto original=reinterpret_cast<decltype(&SDL_PollEvent)>(dlsym(RTLD_NEXT,"SDL_PollEvent"));
 if(enabled()&&removal()&&event){
  std::lock_guard lock(mutex);auto time=now();
  if(started&&output_device&&time-started>=value("BC_QA_AUDIO_FAULT_AFTER",45)&&!failed.exchange(true)){
   blocked_until=time+value("BC_QA_AUDIO_OUTAGE",8);
   *event={};event->type=SDL_AUDIODEVICEREMOVED;event->adevice.which=output_device;event->adevice.iscapture=0;
   fprintf(stderr,"{\"event\":\"qa_audio_fault\",\"phase\":\"device_removed\"}\n");return 1;
  }
 }
 return original(event);
}
