#include "audio_dispatch.h"
#include "ultramodern/ultramodern.hpp"
#include <cstddef>
#include <cstring>

namespace bc {
namespace {
constexpr uint32_t interrupt_queue=0x80315440,frame_queue=0x80368238,reply_queue=0x80368270,audio_thread=0x80368088;
bool valid(uint32_t address,size_t size){return (address&3)==0&&address>=0x80000000&&uint64_t(address)+size<=0x80800000;}
uint32_t word(const uint8_t* rdram,uint32_t address){uint32_t value;std::memcpy(&value,rdram+address-0x80000000,4);return value;}
}
bool request_pending_audio_after_vi(uint8_t* rdram,uint32_t queue,uint32_t message){
 if(!rdram||queue!=interrupt_queue||message!=0x29a||!ultramodern::is_game_thread())return false;
 uint32_t caller=uint32_t(ultramodern::this_thread());
 if(!valid(caller,sizeof(OSThread))||word(rdram,caller+offsetof(OSThread,id))!=5)return false;
 uint32_t head=word(rdram,0x803156a4);
 if(word(rdram,0x803156b4)!=0||!valid(head,96))return false;
 // Original __scAppendList makes an unstarted task state 2. amHandleFrameMessage
 // gives audio flags 1. Do not request graphics, active, or completed tasks.
 if(word(rdram,head+16)!=M_AUDTASK||word(rdram,head+4)!=2||word(rdram,head+8)!=1)return false;
 // Blast Corps' custom OSScTask layout differs from standard libultra: client
 // +0x50, msgQ +0x54, msg +0x58. AudioInfo embeds its task at offset +8.
 uint32_t client=word(rdram,head+80);
 if(!valid(client,16)||word(rdram,client+4)!=frame_queue||word(rdram,head+84)!=reply_queue||word(rdram,head+88)!=head-8)return false;
 if(word(rdram,reply_queue+8)!=0||word(rdram,reply_queue)!=audio_thread||
    word(rdram,audio_thread+offsetof(OSThread,id))!=4||word(rdram,audio_thread+offsetof(OSThread,queue))!=reply_queue)return false;
 // Respect the ordinary interrupt FIFO. A pending execution request/completion
 // already has priority; a full/invalid queue is left entirely unchanged.
 int32_t count=int32_t(word(rdram,interrupt_queue+8)),first=int32_t(word(rdram,interrupt_queue+12)),capacity=int32_t(word(rdram,interrupt_queue+16));
 uint32_t buffer=word(rdram,interrupt_queue+20);
 if(capacity<=0||capacity>16||count<0||count>=capacity||first<0||first>=capacity||!valid(buffer,4*capacity))return false;
 for(int32_t k=0;k<count;k++){
  uint32_t pending=word(rdram,buffer+4*((first+k)%capacity));
  if(pending==0x29e||pending==0x29b)return false;
 }
 // amMain can wait for a reply before a later VI appends its previous task.
 // Ask the original scheduler to dispatch that real ready task. Its normal SP
 // execution/completion path remains solely responsible for sending the reply.
 return osSendMesg(rdram,int32_t(interrupt_queue),int32_t(0x29e),OS_MESG_NOBLOCK)==0;
}
}
