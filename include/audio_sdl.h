#pragma once
#include "audio_output.h"
#include <SDL.h>
namespace bc {
class SDLAudioBackend final:public AudioBackend {
 SDL_AudioDeviceID device_=0;
public:
 bool open(uint32_t) override;
 void close() override;
 bool running() override;
 uint32_t queued() override;
 bool enqueue(std::span<const int16_t>) override;
 SDL_AudioDeviceID device() const{return device_;}
};
}
