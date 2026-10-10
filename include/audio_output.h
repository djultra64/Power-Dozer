#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <limits>
#include <stdexcept>
namespace bc {
struct AudioBackend {
 virtual ~AudioBackend()=default;
 virtual bool open(uint32_t rate)=0;
 virtual void close()=0;
 virtual bool running()=0;
 virtual uint32_t queued()=0;
 virtual bool enqueue(std::span<const int16_t>)=0;
};
// Caller serializes access. All timestamps are monotonic nanoseconds.
// Device loss changes the sink, not the guest AI's pending-buffer timeline.
class AudioOutput {
 static constexpr uint64_t second=1000000000;
 AudioBackend& backend_;
 uint32_t rate_=0,bytes_=0;
 uint64_t last_=0,fraction_=0,retry_=0;
 bool online_=false;
 void drain(uint64_t now){
  if(now<last_)throw std::logic_error("Audio clock moved backwards");
  auto delta=now-last_;last_=now;
  if(!bytes_||!rate_){fraction_=0;return;}
  // Bound multiplication by the time needed to consume the entire queue.
  const uint64_t duration=(uint64_t(bytes_/4)*second+rate_-1)/rate_;
  if(delta>=duration){bytes_=0;fraction_=0;return;}
  auto ticks=delta*rate_+fraction_;
  auto frames=ticks/second;fraction_=ticks%second;
  if(frames>=bytes_/4){bytes_=0;fraction_=0;}
  else bytes_-=uint32_t(frames*4);
 }
 void unavailable(uint64_t now){
  backend_.close();online_=false;last_=now;fraction_=0;retry_=now+second;
 }
 bool open(){
  if(!backend_.open(rate_))return false;
  if(bytes_){
   std::vector<int16_t> silence(bytes_/2,0);
   if(!backend_.enqueue(silence)){backend_.close();return false;}
  }
  online_=true;return true;
 }
public:
 explicit AudioOutput(AudioBackend& backend):backend_(backend){}
 void frequency(uint32_t rate,uint64_t now){
  if(!rate)throw std::invalid_argument("Audio sample rate must be positive");
  backend_.close();online_=false;rate_=rate;bytes_=0;last_=now;fraction_=0;
  retry_=now+second;open();
 }
 uint32_t queued(uint64_t now){
  if(online_){
   if(backend_.running())bytes_=backend_.queued();
   else unavailable(now);
  }
  if(!online_)drain(now);
  return bytes_;
 }
 void push(std::span<const int16_t> pcm,uint64_t now){
  if(pcm.size()%2||pcm.size()>std::numeric_limits<uint32_t>::max()/2)throw std::invalid_argument("Invalid stereo PCM size");
  if(pcm.empty())return;
  queued(now);
  auto count=uint32_t(pcm.size()*2);
  if(count>std::numeric_limits<uint32_t>::max()-bytes_)throw std::overflow_error("Audio queue overflow");
  if(online_&&!backend_.enqueue(pcm))unavailable(now);
  bytes_+=count;
 }
 // Called only for this backend's output device, never a capture/other device.
 void lost(uint64_t now){if(online_)unavailable(now);}
 void poll(uint64_t now){
  queued(now);
  if(!online_&&rate_&&now>=retry_){retry_=now+second;open();}
 }
 bool online() const{return online_;}
};
}
