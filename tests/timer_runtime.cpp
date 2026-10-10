#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include "ultramodern/ultramodern.hpp"
namespace {
struct Message { int32_t queue, value; };
std::mutex message_mutex;
std::condition_variable message_ready;
std::vector<Message> messages;
void require(bool ok,const char* text) { if(!ok)throw std::runtime_error(text); }
bool wait_message(int value) {
    std::unique_lock lock(message_mutex);
    return message_ready.wait_for(lock,std::chrono::seconds(2),[&] {
        return std::any_of(messages.begin(),messages.end(),[&](auto m){return m.value==value;});
    });
}
std::vector<Message> snapshot() { std::lock_guard lock(message_mutex);return messages; }
}
namespace ultramodern {
void set_native_thread_name(const std::string&) {}
void set_native_thread_priority(ThreadPriority) {}
void enqueue_external_message_src(PTR(OSMesgQueue) queue,OSMesg value,bool jam,EventMessageSource src) {
    require(!jam&&src==EventMessageSource::Timer,"timer delivery flags changed");
    {std::lock_guard lock(message_mutex);messages.push_back({queue,value});}
    message_ready.notify_all();
}
}
extern "C" int osSetTimer(uint8_t*,int32_t,uint64_t,uint64_t,int32_t,int32_t);
extern "C" int osStopTimer(uint8_t*,int32_t);
int main(int argc,char**) {
    auto* ram=new uint8_t[4096]{}; // Remains alive for SDK detached worker.
    try {
        ultramodern::init_timers(ram);
        if(argc>1) {
            // Also compile this real implementation with -fsanitize=thread.
            for(unsigned i=0;i<50000;i++) {
                auto addr=int32_t(0x80000040+64*(i%8));
                osSetTimer(ram,addr,46875*(1+i%4),46875,int32_t(0x80000300),i);
                if(i%3==0)osStopTimer(ram,addr);
                if(i%20==0)std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            constexpr int32_t a=int32_t(0x80000040),b=int32_t(0x80000080),q=int32_t(0x80000300);
            osSetTimer(ram,a,46875*2,0,q,101);
            require(wait_message(101),"one-shot never delivered");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            require(snapshot().size()==1&&snapshot()[0].queue==q,"one-shot duplicated or wrong queue");
            osSetTimer(ram,a,46875*80,0,q,102);
            osSetTimer(ram,a,46875*2,0,q+16,103);
            require(wait_message(103),"replacement deadline was not used");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto m=snapshot();
            require(m.size()==2&&m.back().queue==q+16,"replacement delivered obsolete timer or destination");
            osSetTimer(ram,a,46875*20,0,q,104);
            osStopTimer(ram,a);
            std::this_thread::sleep_for(std::chrono::milliseconds(35));
            require(snapshot().size()==2,"cancelled timer delivered");
            osSetTimer(ram,a,46875*40,0,q,105);
            osSetTimer(ram,b,46875*2,0,q,106);
            require(wait_message(105)&&wait_message(106),"deadline order timers missing");
            m=snapshot();require(m[2].value==106&&m[3].value==105,"timers delivered in address rather than deadline order");
            // Zero countdown must use interval; repeated rearming must not read guest RAM.
            osSetTimer(ram,a,0,46875*3,q,107);
            require(wait_message(107),"zero countdown interval never delivered");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            m=snapshot();require(std::count_if(m.begin(),m.end(),[](auto x){return x.value==107;})>=3,"periodic timer did not repeat");
            osStopTimer(ram,a);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            auto stopped=snapshot().size();
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            require(snapshot().size()==stopped,"periodic timer continued after cancellation");
        }
        puts("Actual timer replacement, cancellation, deadlines and periodic delivery passed");
        fflush(stdout);std::_Exit(0); // SDK has no detached timer shutdown API.
    } catch(const std::exception& e) { fprintf(stderr,"%s\n",e.what());fflush(stderr);std::_Exit(1); }
}
