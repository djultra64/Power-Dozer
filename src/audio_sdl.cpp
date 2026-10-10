#include "audio_sdl.h"
namespace bc {
bool SDLAudioBackend::open(uint32_t rate){
 SDL_AudioSpec wanted{};wanted.freq=rate;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=512;
 device_=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);
 if(device_)SDL_PauseAudioDevice(device_,0);
 return device_!=0;
}
void SDLAudioBackend::close(){if(device_)SDL_CloseAudioDevice(device_);device_=0;}
bool SDLAudioBackend::running(){return device_&&SDL_GetAudioDeviceStatus(device_)==SDL_AUDIO_PLAYING;}
uint32_t SDLAudioBackend::queued(){return device_?SDL_GetQueuedAudioSize(device_):0;}
bool SDLAudioBackend::enqueue(std::span<const int16_t> pcm){return device_&&SDL_QueueAudio(device_,pcm.data(),uint32_t(pcm.size()*2))==0;}
}
