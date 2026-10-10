// Oracle: original generated amMain branch/message ordering under controlled input.
// Not a model of native timing, DSP synthesis, or complete thread scheduling.
#include "support.h"
#include "funcs.h"
#include "audio_dispatch.h"
#include "librecomp/rsp.hpp"
#include "ultramodern/ultramodern.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <memory>
#include BC_MESGQUEUE_SOURCE
#include BC_THREADQUEUE_SOURCE
namespace {
constexpr uint32_t frame=0x80368238,reply=0x80368270,interrupt=0x80315440,cmd=0x80315498,thread=0x80368088;
static std::array<uint8_t,0x800000> memory{};
static bool healthy;
static unsigned created,requests,checks,started,woken;
static uint32_t self=thread,task_list;
struct Blocked {uint32_t queue;};
void require(bool condition){if(!condition)throw std::runtime_error("CPU fixture oracle failed");}
uint32_t& word(uint32_t a){return *reinterpret_cast<uint32_t*>(memory.data()+a-0x80000000);}
OSMesgQueue* mq(uint32_t q){auto* rdram=memory.data();return TO_PTR(OSMesgQueue,int32_t(q));}
}
namespace ultramodern {
int32_t this_thread(){return int32_t(self);}
bool is_game_thread(){return true;}
void check_running_queue(uint8_t*){checks++;} // No scheduler simulation or fake completion.
void schedule_running_thread(uint8_t*,int32_t t){require(uint32_t(t)==thread);woken++;}
void run_next_thread_and_wait(uint8_t* rdram){
 // Actual do_recv has already inserted the thread into its real SDK queue.
 // Stop instead of inventing an OS task execution/reply.
 throw Blocked{uint32_t(TO_PTR(OSThread,int32_t(thread))->queue)};
}
}
extern "C" void osSendMesg_recomp(uint8_t* rdram,recomp_context* ctx){
 auto q=uint32_t(ctx->r4),m=uint32_t(ctx->r5);
 ctx->r2=osSendMesg(rdram,int32_t(q),int32_t(m),int32_t(ctx->r6));
 if(q==interrupt&&m==0x29e){requests++;std::printf("request29E head=%08x cmd=%d\n",word(0x803156a4),mq(cmd)->validCount);}
}
extern "C" void osRecvMesg_recomp(uint8_t* rdram,recomp_context* ctx){
 auto q=uint32_t(ctx->r4),dest=uint32_t(ctx->r5);
 std::printf("recv_before q=%08x head=%08x cmd=%d frame=%d reply=%d\n",q,word(0x803156a4),mq(cmd)->validCount,mq(frame)->validCount,mq(reply)->validCount);
 ctx->r2=osRecvMesg(rdram,int32_t(q),int32_t(dest),int32_t(ctx->r6));
 std::printf("recv_after q=%08x payload=%08x\n",q,word(dest));
}
extern "C" void osGetTime_recomp(uint8_t*,recomp_context* ctx){ctx->r2=0;ctx->r3=1;}
// Real generated amMain calls these voice/client/synthesis-adjacent bodies;
// they are neutral here, except the precisely bounded task submission stub.
#define NEUTRAL(name) extern "C" void name(uint8_t*,recomp_context*){}
NEUTRAL(func_80270E50) // osScAddClient; queues/client already initialized by fixture.
NEUTRAL(func_80267F88) // amHandleDoneMessage; no reply is invented to reach this.
NEUTRAL(func_802613C8) NEUTRAL(func_80261068) NEUTRAL(func_80261284)
NEUTRAL(func_802611F0) NEUTRAL(func_80261528) NEUTRAL(func_802D6F04)
extern "C" void bc_rmonPrintf(uint8_t*,recomp_context*){throw std::runtime_error("original __scAppendList assertion");}
void append_one(uint8_t* rdram){
 auto received=osRecvMesg(rdram,int32_t(cmd),int32_t(0x80368300),OS_MESG_NOBLOCK);require(received==0);
 recomp_context scheduler{};scheduler.r4=int32_t(interrupt);scheduler.r5=int32_t(word(0x80368300));scheduler.r29=int32_t(0x807fe000);
 func_80271C24(rdram,&scheduler); // ORIGINAL __scAppendList, not rewritten list logic.
}
extern "C" void func_80267CDC(uint8_t* rdram,recomp_context*){
 // Explicit stub boundary: no DSP/AI synthesis. Create two distinct task records
 // and submit via actual SDK cmd queue. The original amMain decides 29E/reply.
 require(created<2);uint32_t task=0x80397c18+created*0x100;
 word(task+16)=M_AUDTASK;word(task+8)=1;word(task+80)=0x803682f8;word(task+84)=reply;word(task+88)=task-8;
 require(osSendMesg(rdram,int32_t(cmd),int32_t(task),OS_MESG_NOBLOCK)==0);
 created++;
 std::printf("created_task=%08x cmd=%d\n",task,mq(cmd)->validCount);
 if(created==1){
  if(healthy)append_one(rdram); // Control sets head via original guest append.
  // Forced external input: two timer5 messages before any command-draining VI.
  // This proves conditional behavior only, not how native timer backlog arose.
  ultramodern::enqueue_external_message_src(int32_t(frame),5,false,ultramodern::EventMessageSource::Timer);
  ultramodern::enqueue_external_message_src(int32_t(frame),5,false,ultramodern::EventMessageSource::Timer);
 }
}
// Hardware/task execution boundaries: record the original dispatch, then run
// the actual recompiled audio RSP before accepting a completion interrupt.
RspExitReason blastcorps_audio_rsp(uint8_t*,uint32_t);
extern "C" void bc_osSpTaskLoad(uint8_t*,recomp_context*){} // Production also has no-op load.
extern "C" void bc_osSpTaskStartGo(uint8_t*,recomp_context* c){require(!started);started++;task_list=c->r4;}
extern "C" void bc_osSpTaskYield(uint8_t*,recomp_context*){throw std::runtime_error("unexpected graphics yield");}
extern "C" void bc_osSpTaskYielded(uint8_t*,recomp_context*){throw std::runtime_error("unexpected graphics yielded query");}
extern "C" void func_80271E88(uint8_t*,recomp_context*){throw std::runtime_error("unexpected active-RSP dispatch");}
extern "C" void func_802DAC80(uint8_t*,recomp_context*){throw std::runtime_error("unexpected graphics yielded branch");}
extern "C" void do_break(uint32_t){throw std::runtime_error("original arithmetic break");}
extern "C" void func_80271F48(uint8_t*,recomp_context*){throw std::runtime_error("unexpected swapbuffer completion");}
static RspUcodeFunc* select_ucode(const OSTask* t){return t->t.type==M_AUDTASK?blastcorps_audio_rsp:nullptr;}
void check_guards(uint8_t* rdram){
 auto original=std::make_unique<decltype(memory)>(memory);
 auto rejected=[&](const char* label){auto before=std::make_unique<decltype(memory)>(memory);require(!bc::request_pending_audio_after_vi(rdram,interrupt,0x29a));require(memory==*before);memory=*original;std::printf("guard_%s=unchanged\n",label);};
 word(0x803156a4)=0;rejected("no_head");
 word(0x803156b4)=0x80397c18;rejected("active_rsp");
 word(0x803156a4)=0x80800000;rejected("invalid_head");
 word(0x80397c18+16)=M_GFXTASK;rejected("graphics_head");
 word(0x80397c18+4)=1;rejected("active_head");
 word(0x80397c18+8)=5;rejected("completed_flags");
 word(0x80397c18+84)=frame;rejected("wrong_reply_target");
 word(0x803682fc)=reply;rejected("wrong_client_frame_queue");
 mq(reply)->blocked_on_recv=0;rejected("no_reply_waiter");
 TO_PTR(OSThread,int32_t(thread))->id=3;rejected("wrong_waiter_id");
 TO_PTR(OSThread,int32_t(thread))->queue=int32_t(frame);rejected("wrong_waiter_queue");
 auto before=std::make_unique<decltype(memory)>(memory);require(!bc::request_pending_audio_after_vi(rdram,frame,0x29a));require(memory==*before);
 require(!bc::request_pending_audio_after_vi(rdram,interrupt,0x29b));require(memory==*before);
 self=thread;require(!bc::request_pending_audio_after_vi(rdram,interrupt,0x29a));require(memory==*before);self=0x80004000;
 for(uint32_t message:{0x29eu,0x29bu}){require(osSendMesg(rdram,int32_t(interrupt),int32_t(message),0)==0);rejected(message==0x29e?"duplicate_request":"pending_sp_done");}
 for(unsigned n=0;n<16;n++)require(osSendMesg(rdram,int32_t(interrupt),42,0)==0);rejected("full_interrupt_queue");
}
int main(int argc,char** argv){try{
 require(argc==2);healthy=std::string(argv[1])=="healthy";auto* rdram=memory.data();
 for(uint32_t q:{interrupt,cmd,frame,reply})osCreateMesgQueue(rdram,int32_t(q),int32_t(q+0x18),q==frame?8:16);
 auto* t=TO_PTR(OSThread,int32_t(thread));t->id=4;t->priority=50;
 word(0x803682fc)=frame;
 word(0x803156ac)=0x803156a4; // Original scheduler audio tail initially aliases head.
 ultramodern::MessageQueueControl controls{};controls.requeue_timer=true;controls.requeue_sp=true;ultramodern::set_message_queue_control(controls);
 recomp_context guest{};guest.r29=int32_t(0x807ff000);
 uint32_t blocked=0;
 try{func_80267A9C(rdram,&guest);}catch(const Blocked& stop){blocked=stop.queue;}
 require(blocked==reply&&created==2&&requests==(healthy?1u:0u));
 require(mq(reply)->blocked_on_recv==int32_t(thread));
 require(mq(frame)->validCount==1&&mq(reply)->validCount==0);
 require(mq(cmd)->validCount==(healthy?1:2));
 auto before_head=word(0x803156a4);auto before_cmd=mq(cmd)->validCount;
 if(!healthy){
  // Represent a later command-draining retrace using the actual SDK receive
  // and exact original guest append. amMain remains stopped on its reply.
  append_one(rdram);append_one(rdram);
  require(word(0x803156a4)==0x80397c18&&word(0x80397c18)==0x80397d18);
  require(word(0x80397c18+4)==2&&word(0x803156b4)==0&&requests==0);
  for(unsigned n=0;n<7;n++)ultramodern::enqueue_external_message_src(int32_t(frame),5,false,ultramodern::EventMessageSource::Timer);
  dequeue_external_messages(rdram);require(mq(frame)->validCount==8);
 }
 if(!healthy){
  self=0x80004000;TO_PTR(OSThread,int32_t(self))->id=5;check_guards(rdram);
  bool baseline=std::string(argv[1])=="baseline";
  if(!baseline)require(bc::request_pending_audio_after_vi(rdram,interrupt,0x29a));
  // RED baseline fails the progress oracle without producing any fake reply.
  if(baseline){require(mq(interrupt)->validCount==0&&started==0&&mq(reply)->validCount==0);std::puts("RED: ready audio task stays undispatched");return 1;}
  require(!bc::request_pending_audio_after_vi(rdram,interrupt,0x29a)); // Same request not duplicated.
  require(osRecvMesg(rdram,int32_t(interrupt),int32_t(0x80368300),0)==0&&word(0x80368300)==0x29e);
  recomp_context sched{};sched.r4=int32_t(interrupt);sched.r29=int32_t(0x807fe000);
  func_802712FC(rdram,&sched); // ORIGINAL ExecAudioIfIdle + Exec; task A really dispatched.
  require(started==1&&task_list==0x80397c18+16&&word(0x803156b4)==0x80397c18);
  require(word(0x803156a4)==0x80397d18&&mq(reply)->validCount==0&&woken==0);
  // A single NOP Acmd is processed by the actual generated audio RSP. The
  // synthetic data table supplies only opcode0's existing 0x1118 NOP target;
  // no ROM/sample data and no DSP output stub is used.
  auto* task=TO_PTR(OSTask,int32_t(task_list));task->t.ucode_data=int32_t(0x80005000);task->t.ucode_data_size=0x800;
  task->t.data_ptr=int32_t(0x80006000);task->t.data_size=8;word(0x80005010)=0x11180000;
  recomp::rsp::constants_init();recomp::rsp::set_callbacks({select_ucode});
  require(recomp::rsp::run_task(rdram,task)); // Only successful execution permits SP completion.
  ultramodern::enqueue_external_message_src(int32_t(interrupt),0x29b,false,ultramodern::EventMessageSource::Sp);
  require(osRecvMesg(rdram,int32_t(interrupt),int32_t(0x80368300),0)==0&&word(0x80368300)==0x29b);
  sched={};sched.r4=int32_t(interrupt);sched.r29=int32_t(0x807fe000);
  func_802715DC(rdram,&sched); // ORIGINAL RspDone + task completion sends the real task reply.
  require(mq(reply)->validCount==1&&woken==1&&mq(reply)->blocked_on_recv==0);
  require(word(0x803156b4)==0&&word(0x80397c18+8)==5);
  require(osRecvMesg(rdram,int32_t(reply),int32_t(0x80368300),0)==0&&word(0x80368300)==0x80397c10);
  require(osRecvMesg(rdram,int32_t(reply),int32_t(0x80368300),0)==-1); // Exactly one authentic ACK.
 }
 std::printf("{\"scenario\":\"%s\",\"request29E_original\":%u,\"task_A_dispatched\":%u,\"real_RSP_completed\":%s,\"original_ACK_woke_thread4\":%u,\"native_cause_proven\":false}\n",healthy?"healthy_control":"late5_recovery",requests,started,healthy?"false":"true",woken);
 return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
