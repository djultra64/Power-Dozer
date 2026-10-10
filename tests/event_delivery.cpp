#include "event_delivery.h"
#include "ultramodern/ultramodern.hpp"
#include <array>
#include <cstdio>
#include <stdexcept>

// Use the actual pinned SDK message queue implementation. No guest threads
// wait on this tiny fixture; attempts to schedule one are test failures.
namespace ultramodern {
bool thread_queue_empty(uint8_t* rdram,int32_t queue) { return *TO_PTR(int32_t,queue)==0; }
void thread_queue_insert(uint8_t*,int32_t,int32_t) { throw std::runtime_error("unexpected blocked send"); }
int32_t thread_queue_pop(uint8_t*,int32_t) { throw std::runtime_error("unexpected queued thread"); }
void schedule_running_thread(uint8_t*,int32_t) { throw std::runtime_error("unexpected scheduling"); }
void run_next_thread_and_wait(uint8_t*) { throw std::runtime_error("unexpected guest thread switch"); }
int32_t this_thread() { throw std::runtime_error("unexpected guest thread lookup"); }
}
extern void dequeue_external_messages(uint8_t*);
extern bool do_recv(uint8_t*,int32_t,int32_t,bool);

static void require(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
int main() {
    std::array<uint8_t,512> ram{};auto* rdram=ram.data();
    constexpr int32_t address=int32_t(0x80000040),buffer=int32_t(0x80000080),output=int32_t(0x80000090);
    bc::initialize_event_delivery();
    for(auto source : {ultramodern::EventMessageSource::Sp,ultramodern::EventMessageSource::Dp,ultramodern::EventMessageSource::Timer,ultramodern::EventMessageSource::Si}) {
        osCreateMesgQueue(rdram,address,buffer,1);
        auto* queue=TO_PTR(OSMesgQueue,address);
        queue->validCount=1;*TO_PTR(int32_t,buffer)=11;
        ultramodern::enqueue_external_message_src(address,22,false,source);
        dequeue_external_messages(rdram);
        require(queue->validCount==1,"retry must not overwrite an occupied queue");
        require(do_recv(rdram,address,output,false)&&*TO_PTR(int32_t,output)==11,"older message remains first");
        dequeue_external_messages(rdram);
        require(queue->validCount==1,"critical completion survives a saturated queue");
        require(do_recv(rdram,address,output,false)&&*TO_PTR(int32_t,output)==22,"retried completion has its original value");
        dequeue_external_messages(rdram);
        require(queue->validCount==0,"completion is delivered exactly once");
    }
    osCreateMesgQueue(rdram,address,buffer,1);
    auto* queue=TO_PTR(OSMesgQueue,address);queue->validCount=1;*TO_PTR(int32_t,buffer)=11;
    ultramodern::enqueue_external_message_src(address,33,false,ultramodern::EventMessageSource::Vi);
    dequeue_external_messages(rdram);require(do_recv(rdram,address,output,false),"consume old VI fixture message");dequeue_external_messages(rdram);
    require(queue->validCount==0,"VI ticks do not accumulate a retry backlog");
    puts("Actual SDK completion retries, ordering and VI coalescing passed");
}
