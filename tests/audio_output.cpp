#include "audio_output.h"
#include "audio_fifo.h"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cstdio>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct FakeBackend:bc::AudioBackend {
 bool available=false,active=false,fail_queue=false;
 unsigned opens=0,closes=0;uint32_t bytes=0,rate=0;
 std::vector<int16_t> received;
 bool open(uint32_t hz) override{++opens;rate=hz;active=available;bytes=0;return active;}
 void close() override{++closes;active=false;bytes=0;}
 bool running() override{return active;}
 uint32_t queued() override{return bytes;}
 bool enqueue(std::span<const int16_t> pcm) override{if(fail_queue)return false;received.assign(pcm.begin(),pcm.end());bytes+=pcm.size()*2;return true;}
};
int main(){
 constexpr uint64_t sec=1000000000;
 std::vector<int16_t> pcm(4410,123); // 100 ms of stereo at 22,050 Hz.
 FakeBackend backend;bc::AudioOutput output(backend);AudioFifo fifo;
 output.frequency(22050,0);check(!output.online(),"unavailable startup is supported");
 output.push(pcm,0);fifo.push(pcm.size()*2);
 check(output.queued(0)==8820,"unavailable output retains the AI byte budget");
 check(output.queued(sec/20)==4412,"silent clock consumes whole stereo frames with fractional remainder");
 check(fifo.head_bytes(output.queued(sec/10))==0,"silent clock completes guest AI buffer on time");
 output.poll(sec/2);check(backend.opens==1,"failed output retries are throttled");
 backend.available=true;output.poll(sec);check(output.online()&&backend.opens==2,"default output recovers automatically");
 output.push(pcm,sec);fifo.push(pcm.size()*2);backend.bytes=6000;
 check(output.queued(sec)==6000,"connected output uses actual backend queue");
 output.lost(sec);check(!output.online()&&output.queued(sec)==6000,"device loss preserves the pending AI budget");
 check(output.queued(sec+sec/100)==5120,"lost output consumes at the original sample rate");
 output.push(pcm,sec+sec/100);fifo.push(pcm.size()*2);check(output.queued(sec+sec/100)==13940,"new silent PCM joins the same guest FIFO");
 // Recover with a still pending buffer, so guest AI does not jump or hang.
 output.poll(2*sec); // Existing budget drained before this retry.
 output.push(pcm,2*sec);output.lost(2*sec);
 output.push(pcm,3*sec-10000000);output.poll(3*sec);
 check(output.online()&&output.queued(3*sec)==7940,"recovery queues exactly the outstanding budget");
 check(std::all_of(backend.received.begin(),backend.received.end(),[](int16_t value){return value==0;}),"recovery never replays old effects");
 backend.fail_queue=true;output.push(pcm,3*sec);
 check(!output.online()&&output.queued(3*sec)==16760,"queue failure retains existing and incoming buffers");
 backend.fail_queue=false;backend.available=false;output.frequency(48000,4*sec);
 check(output.queued(4*sec)==0,"frequency change clears old rate budget");
 output.push(std::vector<int16_t>(9600,1),4*sec);
 check(output.queued(4*sec+sec/10)==0,"fallback respects a changed sample rate");
 output.push(pcm,5*sec);check(output.queued(500*sec)==0,"long output loss cannot overflow or leave a buffer stuck");

 // Repeated sub-frame observations must preserve the fractional clock.
 FakeBackend fractional;bc::AudioOutput precise(fractional);precise.frequency(22050,0);precise.push(pcm,0);
 for(uint64_t tick=1;tick<=1000;++tick)precise.queued(tick*100000);
 check(precise.queued(sec/10)==0,"frequent polling cannot lose fractional frame progress");
 // Opening succeeds but padding the outstanding budget fails: stay offline.
 FakeBackend padding;bc::AudioOutput pending(padding);pending.frequency(22050,0);
 pending.push(pcm,sec-10000000);padding.available=true;padding.fail_queue=true;
 pending.poll(sec);check(!pending.online()&&pending.queued(sec)==7940,"failed recovery padding preserves the silent budget");
 padding.fail_queue=false;pending.poll(2*sec);check(pending.online(),"failed padding is retried safely");
 bool invalid=false;try{pending.push(std::vector<int16_t>(3),2*sec);}catch(const std::invalid_argument&){invalid=true;}
 check(invalid,"partial stereo frames rejected before reaching either sink");
 puts("Audio output loss, timing and recovery passed");
}
