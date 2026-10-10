#include "support.h"
#include "prototype.h"
#include "funcs.h"
#include "ultramodern/ultramodern.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <cmath>
#include <bit>
#include <limits>
#include <unordered_map>
#include "rsp_scheduler.h"
#include "save_store.h"
#include "translucent_mode.h"
#include <execinfo.h>
#include <dlfcn.h>

std::vector<uint8_t> bc_rom;
std::atomic<uint64_t> bc_vis{0},bc_gfx{0},bc_audio{0},bc_polls{0};
std::atomic_bool exited{false};
std::atomic_bool bc_dp_sync{false};
std::filesystem::path bc_data_directory;
static std::mutex task_mutex;
struct TaskMetadata { uint32_t address; bool swap; };
static std::unordered_map<uint32_t,TaskMetadata> task_swaps;
static RspScheduler rsp_scheduler;
void sp_complete();
bool bc_should_signal_dp(){return bc_dp_sync.exchange(false);}
bool bc_take_task_swap(uint32_t list){std::lock_guard lock(task_mutex);auto it=task_swaps.find(list);if(it==task_swaps.end())bc_fatal("missing_task_metadata",list);bool swap=it->second.swap;task_swaps.erase(it);return swap;}
uint32_t bc_graphics_task_address(uint32_t list){std::lock_guard lock(task_mutex);auto it=task_swaps.find(list);if(it==task_swaps.end())bc_fatal("missing_task_metadata",list);return it->second.address;}
bool bc_complete_graphics(uint32_t address){std::lock_guard lock(task_mutex);return rsp_scheduler.finish_graphics(address);}
void bc_complete_audio(){std::lock_guard lock(task_mutex);rsp_scheduler.finish_audio();}
moodycamel::LightweightSemaphore graphics_shutdown_ready;
static std::array<uint8_t,512> eeprom{};
static std::mutex eep_mutex;
static uint32_t dp_status=0;
static int32_t pending_gfx=0;
void bc_load_save() {
 try {eeprom=bc::SaveStore(bc_data_directory/"eeprom.bin").load();}
 catch(const std::exception& error){fprintf(stderr,"Save: %s\n",error.what());bc_fatal("save_load");}
}
[[noreturn]] void bc_fatal(const char* name,uint32_t address) {
 fprintf(stderr,"{\"event\":\"fatal\",\"symbol\":\"%s\",\"address\":\"0x%08X\",\"vi\":%llu,\"gfx\":%llu,\"audio\":%llu}\n",name,address,(unsigned long long)bc_vis.load(),(unsigned long long)bc_gfx.load(),(unsigned long long)bc_audio.load());
 fflush(stderr); std::_Exit(71);
}
namespace ultramodern { bool is_game_started() { return true; } }
extern "C" recomp_func_t* get_function(int32_t address) {
 switch(uint32_t(address)) {
#include "dispatch.inc"
 }
 bc_fatal("unresolved_indirect",address);
}
void run_thread_function(uint8_t* rdram,uint64_t address,uint64_t stack,uint64_t argument) {
 fprintf(stderr,"{\"event\":\"guest_thread\",\"entry\":\"0x%08X\"}\n",uint32_t(address));fflush(stderr);
 recomp_context ctx{};ctx.status_reg=0x2000FF01;ctx.r29=stack;ctx.r4=argument;ctx.f_odd=&ctx.f0.u32h;
 get_function(address)(rdram,&ctx);
}
extern "C" {
void bc_original_window_off(uint8_t* rdram,unsigned phase) {
 fprintf(stderr,"{\"event\":\"original_window_off\",\"phase\":\"%s\",\"pc\":\"8026BF7C\",\"pending\":%u,\"failed\":%u,\"state_hi\":%u,\"state_lo\":%u}\n",phase==0?"begin":"end",MEM_HU(0,S32(0x8036BB14)),MEM_BU(0,S32(0x803643D6)),uint32_t(MEM_W(0,S32(0x80364A90))),uint32_t(MEM_W(0,S32(0x80364A94))));
}
void bc_guest_translucent_mode(uint8_t* rdram,recomp_context* ctx,unsigned phase) {
 auto mode=uint32_t(MEM_W(0x28,ctx->r29));auto mapped=bc::translucent_mode(mode);
 if(!mapped){bc_guest_render_mode(rdram,ctx);bc_fatal("unsupported_translucent_mode",mode);}
 if(phase==0){
  if(MEM_W(0x2C,ctx->r29)>=3){
   fprintf(stderr,"{\"event\":\"translucent_mode_extended\",\"from\":\"%08X\",\"to\":\"%08X\",\"level\":%d}\n",mode,mapped->value,MEM_W(0,S32(0x802E8BDC)));
   MEM_W(0x2C,ctx->r29)=mapped->index;MEM_B(0x33,ctx->r29)=1;
  }
 }else ctx->r15=S32(mapped->value);
}
void bc_guest_state(uint8_t* rdram) {
 if(!std::getenv("BC_TRACE_GAMEPLAY"))return;
 static uint64_t next=0;auto vi=bc_vis.load();if(vi<next)return;next=vi+15;
 fprintf(stderr,"{\"event\":\"guest_state\",\"vi\":%llu,\"level\":%d,\"state_hi\":%u,\"state_lo\":%u,\"paused\":%u,\"failed\":%u,\"complete\":%u,\"window\":%d,\"x\":%d,\"y\":%d,\"z\":%d,\"money\":%u,\"buildings_destroyed\":%u,\"rdus_found\":%u,\"camera_yaw\":%.3f,\"vehicle_heading\":%u,\"vehicle_type\":%u}\n",(unsigned long long)vi,MEM_W(0,S32(0x802E8BDC)),uint32_t(MEM_W(0,S32(0x80364A90))),uint32_t(MEM_W(0,S32(0x80364A94))),MEM_BU(0,S32(0x802E8BD0)),MEM_BU(0,S32(0x803643D6)),MEM_BU(0,S32(0x803643D7)),MEM_H(0,S32(0x8036BB18)),MEM_W(0,S32(0x803643E0)),MEM_W(0,S32(0x803643E4)),MEM_W(0,S32(0x803643E8)),uint32_t(MEM_W(0,S32(0x8036EA70))),MEM_BU(0,S32(0x8036EA78)),MEM_HU(0,S32(0x8036EA7C)),double(std::bit_cast<float>(uint32_t(MEM_W(0,S32(0x80364414))))),MEM_HU(0,S32(0x80370C70)),MEM_BU(0,S32(0x80364456)));
}
void bc_guest_render_mode(uint8_t* rdram,recomp_context* ctx) {
 fprintf(stderr,"{\"event\":\"guest_render_mode_fault\",\"pc\":\"80278D38\",\"mode\":\"%08X\",\"source\":\"%08X\",\"index\":%d,\"level\":%d,\"vi\":%llu}\n",uint32_t(MEM_W(0x28,ctx->r29)),uint32_t(MEM_W(0x40,ctx->r29)),MEM_W(0x2C,ctx->r29),MEM_W(0,S32(0x802E8BDC)),(unsigned long long)bc_vis.load());
}
void bc_original_fade_window(uint8_t* rdram,recomp_context* ctx,unsigned phase) {
 fprintf(stderr,"{\"event\":\"original_fade_window\",\"phase\":\"%s\",\"pending\":%u,\"saved\":%u,\"demand\":%u,\"frame\":%u,\"state_hi\":%u,\"state_lo\":%u,\"vi\":%llu}\n",phase==0?"begin":phase==1?"end":"restore",MEM_HU(0,S32(0x8036BB14)),MEM_HU(0,S32(0x8036C770)),uint16_t(ctx->r4),uint32_t(MEM_W(0,S32(0x80358060))),uint32_t(MEM_W(0,S32(0x80364A90))),uint32_t(MEM_W(0,S32(0x80364A94))),(unsigned long long)bc_vis.load());
}
void bc_guest_window_trace(uint8_t* rdram,recomp_context* ctx,bool draw) {
 if(!std::getenv("BC_TRACE_GUEST_WINDOWS"))return;
 const uint16_t pending=MEM_HU(0,S32(0x8036BB14));
 if(draw&&!pending)return;
 fprintf(stderr,"{\"event\":\"guest_window\",\"draw\":%s,\"demand\":%u,\"pending\":%u,\"frame\":%u,\"state_hi\":%u,\"state_lo\":%u}\n",draw?"true":"false",uint16_t(ctx->r4),pending,uint32_t(MEM_W(0,S32(0x80358060))),uint32_t(MEM_W(0,S32(0x80364A90))),uint32_t(MEM_W(0,S32(0x80364A94))));
 if(!draw&&pending){
  void* frames[12];int size=backtrace(frames,12);
  for(int i=0;i<size;i++){Dl_info info{};if(dladdr(frames[i],&info)&&info.dli_fbase)fprintf(stderr,"window_stack[%d] module_offset=%zx\n",i,size_t(reinterpret_cast<uintptr_t>(frames[i])-reinterpret_cast<uintptr_t>(info.dli_fbase)));}
 }
}
void bc_trace(uint32_t address) { static std::atomic<uint32_t> count=0; if(++count<300) {fprintf(stderr,"{\"event\":\"call\",\"vram\":\"0x%08X\"}\n",address);fflush(stderr);} }
void recomp_syscall_handler(uint8_t*,recomp_context*,int32_t address) { bc_fatal("mips_syscall",address); }
void do_break(uint32_t address) { bc_fatal("mips_break",address); }
void switch_error(const char* name,uint32_t address,uint32_t) { bc_fatal(name,address); }
gpr cop0_status_read(recomp_context* ctx) { return (gpr)(int32_t)ctx->status_reg; }
void cop0_status_write(recomp_context* ctx,gpr value) { ctx->status_reg=value; ctx->mips3_float_mode=(value&0x4000000)!=0;ctx->f_odd=ctx->mips3_float_mode? &ctx->f1.u32l : &ctx->f0.u32h; }
void bc_osDpSetStatus(uint8_t* rdram,recomp_context* ctx) {
 if(ctx->r4&8)dp_status|=2;
 if(ctx->r4&4){dp_status&=~2u;if(pending_gfx){auto task=pending_gfx;pending_gfx=0;ultramodern::submit_rsp_task(rdram,task);}}
}
void bc_osCreatePiManager(uint8_t*,recomp_context*) { bc_trace(0x802D4560); /* PI reads are synchronous with completion messages. */ }
void bc_osPiRawReadIo(uint8_t* rdram,recomp_context* ctx) {
 uint32_t device=ctx->r4; uint32_t offset=device % bc_rom.size();
 if(offset+4>bc_rom.size() || (device&3)) bc_fatal("pio_bounds",device);
 uint32_t value=(uint32_t(bc_rom[offset])<<24)|(uint32_t(bc_rom[offset+1])<<16)|(uint32_t(bc_rom[offset+2])<<8)|bc_rom[offset+3];
 MEM_W(0,ctx->r5)=value;ctx->r2=0;
}
void bc_osPiStartDma(uint8_t* rdram,recomp_context* ctx) {
 const uint32_t device=ctx->r7, dest=MEM_W(0x10,ctx->r29), size=MEM_W(0x14,ctx->r29);
 const int32_t queue=MEM_W(0x18,ctx->r29);
 if(ctx->r6!=0 || device>bc_rom.size() || size>bc_rom.size()-device || (dest&0x7FFFFF)+size>0x800000) bc_fatal("dma_bounds",device);
 for(uint32_t i=0;i<size;i++) rdram[((dest&0x7FFFFF)+i)^3]=bc_rom[device+i];
 ultramodern::enqueue_external_message_src(queue,0,false,ultramodern::EventMessageSource::Pi);ctx->r2=0;
}
void bc_osSpTaskLoad(uint8_t*,recomp_context*) {}
void bc_osSpTaskStartGo(uint8_t* rdram,recomp_context* ctx) {
 const int32_t address=ctx->r4;const uint32_t type=MEM_W(0,ctx->r4);
 if(type==M_GFXTASK){
  RspScheduler::Start action;
  {std::lock_guard lock(task_mutex);action=rsp_scheduler.start_graphics(uint32_t(address));
   if(action==RspScheduler::Start::Submit)task_swaps[uint32_t(MEM_W(0x30,ctx->r4))]={uint32_t(address),(MEM_W(-8,ctx->r4)&0x40)!=0};}
  MEM_W(4,ctx->r4)&=~1u;
  if(action==RspScheduler::Start::Complete)sp_complete();
  if(action!=RspScheduler::Start::Submit){ctx->r2=0;return;}
 }
 static int n=0;if(n++<12){fprintf(stderr,"submit task %08X type=%u flags=%x\n",uint32_t(address),type,uint32_t(MEM_W(-8,ctx->r4)));fflush(stderr);}
 if(type==M_GFXTASK&&(dp_status&2)){if(pending_gfx)bc_fatal("pending_gfx_overflow");pending_gfx=address;}
 else ultramodern::submit_rsp_task(rdram,address);
 ctx->r2=0;
}
void bc_osSpTaskYield(uint8_t*,recomp_context*) {std::lock_guard lock(task_mutex);if(rsp_scheduler.yield_graphics())sp_complete();}
void bc___osSpGetStatus(uint8_t*,recomp_context* ctx) {std::lock_guard lock(task_mutex);ctx->r2=rsp_scheduler.status();}
void bc_osSpTaskYielded(uint8_t*,recomp_context* ctx) {ctx->r2=0;}
void bc_osAiSetFrequency(uint8_t*,recomp_context* ctx) {ultramodern::set_audio_frequency(ctx->r4);ctx->r2=ctx->r4;}
void bc_osAiGetLength(uint8_t*,recomp_context* ctx) {ctx->r2=ultramodern::get_remaining_audio_bytes();}
void bc_osAiSetNextBuffer(uint8_t* rdram,recomp_context* ctx) {
 if(!bc_audio_room()){ctx->r2=-1;return;}
 if(uint32_t(ctx->r5)>65536 || (uint32_t(ctx->r4)&0x7FFFFF)+uint32_t(ctx->r5)>0x800000) bc_fatal("audio_dma_bounds",ctx->r5);
 ultramodern::queue_audio_buffer(rdram,ctx->r4,ctx->r5);ctx->r2=0;
}
void bc_osEepromProbe(uint8_t*,recomp_context* ctx) {ctx->r2=1;}
static void eep_rw(uint8_t* rdram,recomp_context* ctx,bool write,bool long_op) {
 std::lock_guard lock(eep_mutex); uint32_t offset=uint32_t(ctx->r5)*8,count=long_op?uint32_t(ctx->r7):8;
 if(offset>512 || count>512-offset) bc_fatal("eeprom_bounds",offset);
 for(uint32_t i=0;i<count;i++) if(write)eeprom[offset+i]=MEM_BU(i,ctx->r6);else MEM_B(i,ctx->r6)=eeprom[offset+i];
 if(write){try{bc::SaveStore(bc_data_directory/"eeprom.bin").save(eeprom);}
  catch(const std::exception& error){fprintf(stderr,"Save: %s\n",error.what());bc_fatal("eeprom_write");}}
 ctx->r2=0;
}
void bc_osEepromRead(uint8_t* r,recomp_context* c){eep_rw(r,c,false,false);}
void bc_osEepromWrite(uint8_t* r,recomp_context* c){eep_rw(r,c,true,false);}
void bc_osEepromLongRead(uint8_t* r,recomp_context* c){eep_rw(r,c,false,true);}
void bc_osEepromLongWrite(uint8_t* r,recomp_context* c){eep_rw(r,c,true,true);}
static std::string guest_string(uint8_t* rdram,uint32_t addr){
 if(addr<0x80000000||addr>=0x80800000)return "<invalid-string>";
 std::string text;for(unsigned i=0;i<1024&&addr+i<0x80800000;i++){char c=rdram[((addr-0x80000000)+i)^3];if(!c)break;text+=c;}return text;
}
void bc_osSyncPrintf(uint8_t* rdram,recomp_context* ctx) {
 auto format=guest_string(rdram,uint32_t(ctx->r4));unsigned arg=1;
 auto next=[&](){unsigned i=arg++;return uint32_t(i==1?ctx->r5:i==2?ctx->r6:i==3?ctx->r7:MEM_W(i*4,ctx->r29));};
 std::string output;
 for(size_t i=0;i<format.size();i++){
  if(format[i]!='%'||i+1>=format.size()){output+=format[i];continue;}
  ++i;if(format[i]=='%'){output+='%';continue;}
  while(i<format.size()&&(format[i]=='0'||(format[i]>='1'&&format[i]<='9')||format[i]=='l'))++i;
  if(i>=format.size())break;auto v=next();char num[40];
  switch(format[i]){case 's':output+=guest_string(rdram,v);break;case 'x':case 'X':snprintf(num,sizeof(num),"%x",v);output+=num;break;
   case 'd':case 'i':snprintf(num,sizeof(num),"%d",int32_t(v));output+=num;break;case 'u':snprintf(num,sizeof(num),"%u",v);output+=num;break;
   case 'c':output+=char(v);break;default:output+="<format>";break;}
 }
 fprintf(stderr,"guest: %s",output.c_str());fflush(stderr);
}

void bc_rmonPrintf(uint8_t* r,recomp_context* c){bc_osSyncPrintf(r,c);}
void bc_osDpGetStatus(uint8_t*,recomp_context* ctx) {ctx->r2=dp_status;}
static void result64(recomp_context* ctx,uint64_t v){ctx->r2=S32(v>>32);ctx->r3=S32(v);}
static void to_signed(recomp_context* ctx,double v){
 uint64_t n=uint64_t(INT64_MIN);
 if(std::isfinite(v)&&v>=-0x1p63&&v<0x1p63)n=uint64_t(int64_t(v));
 result64(ctx,n);
}
static void to_unsigned(recomp_context* ctx,double v){
 uint64_t n=UINT64_MAX;
 if(std::isfinite(v)&&v>=0&&v<0x1p64)n=uint64_t(v);
 result64(ctx,n);
}
void bc___d_to_ll(uint8_t*,recomp_context* ctx){to_signed(ctx,ctx->f12.d);}
void bc___f_to_ll(uint8_t*,recomp_context* ctx){to_signed(ctx,ctx->f12.fl);}
void bc___d_to_ull(uint8_t*,recomp_context* ctx){to_unsigned(ctx,ctx->f12.d);}
void bc___f_to_ull(uint8_t*,recomp_context* ctx){to_unsigned(ctx,ctx->f12.fl);}
static uint64_t argument64(recomp_context* ctx){return (uint64_t(uint32_t(ctx->r4))<<32)|uint32_t(ctx->r5);}
void bc___ll_to_d(uint8_t*,recomp_context* ctx){ctx->f0.d=double(int64_t(argument64(ctx)));}
void bc___ll_to_f(uint8_t*,recomp_context* ctx){ctx->f0.fl=float(int64_t(argument64(ctx)));}
void bc___ull_to_d(uint8_t*,recomp_context* ctx){ctx->f0.d=double(argument64(ctx));}
void bc___ull_to_f(uint8_t*,recomp_context* ctx){ctx->f0.fl=float(argument64(ctx));}

void __real_osCreateThread_recomp(uint8_t*,recomp_context*);
void __real_osStartThread_recomp(uint8_t*,recomp_context*);
void __real_osSetThreadPri_recomp(uint8_t*,recomp_context*);
void __wrap_osCreateThread_recomp(uint8_t* rdram,recomp_context* ctx){
 fprintf(stderr,"create thread id=%llu entry=%08X stack=%08X\n",(unsigned long long)ctx->r5,uint32_t(ctx->r6),uint32_t(MEM_W(0x10,ctx->r29)));fflush(stderr);
 __real_osCreateThread_recomp(rdram,ctx);bc_trace(0xC0DE0001);
}
void __wrap_osStartThread_recomp(uint8_t* rdram,recomp_context* ctx){
 fprintf(stderr,"start thread object=%08X\n",uint32_t(ctx->r4));fflush(stderr);
 auto* t=TO_PTR(OSThread,int32_t(ctx->r4));
 if(t->state==OSThreadState::QUEUED||t->state==OSThreadState::RUNNING){bc_trace(0xC0DE00D0);return;}
 __real_osStartThread_recomp(rdram,ctx);bc_trace(0xC0DE0002);
}
void __wrap_osSetThreadPri_recomp(uint8_t* rdram,recomp_context* ctx){
 fprintf(stderr,"set priority object=%08X priority=%llu\n",uint32_t(ctx->r4),(unsigned long long)ctx->r5);fflush(stderr);
 __real_osSetThreadPri_recomp(rdram,ctx);bc_trace(0xC0DE0003);
}

void __real_osRecvMesg_recomp(uint8_t*,recomp_context*);
void __wrap_osRecvMesg_recomp(uint8_t* rdram,recomp_context* ctx){
 uint32_t queue=ctx->r4,dest=ctx->r5;gpr guest_dest=uint64_t(int64_t(int32_t(dest)));
 __real_osRecvMesg_recomp(rdram,ctx);
 static const bool trace_audio_state=std::getenv("BC_TRACE_AUDIO_STATE")!=nullptr;
 static uint64_t last_audio_state_vi=0;
 if(trace_audio_state&&queue==0x80315440&&ctx->r2==0){
  const auto vi=bc_vis.load();
  if(vi>=last_audio_state_vi+120){
   last_audio_state_vi=vi;
   auto word=[&](uint32_t a){return uint32_t(MEM_W(0,S32(a)));};
   auto* frame=TO_PTR(OSMesgQueue,S32(0x80368238));
   auto* reply=TO_PTR(OSMesgQueue,S32(0x80368270));
   auto* thread=TO_PTR(OSThread,S32(0x80368088));
   fprintf(stderr,"{\"event\":\"audio_guest_state\",\"vi\":%llu,\"audio\":%llu,\"frame_messages\":%d,\"frame_waiter\":%u,\"reply_messages\":%d,\"reply_waiter\":%u,\"audio_thread_state\":%u,\"current_rsp_task\":%u,\"audio_list_head\":%u}\n",(unsigned long long)vi,(unsigned long long)bc_audio.load(),frame->validCount,uint32_t(frame->blocked_on_recv),reply->validCount,uint32_t(reply->blocked_on_recv),unsigned(thread->state),word(0x803156B4),word(0x803156A4));
  }
 }
 static int count=0;if(queue==0x80315440&&dest&&count++<40){fprintf(stderr,"scheduler message=%08X\n",uint32_t(MEM_W(0,guest_dest)));fflush(stderr);}
 if(std::getenv("BC_TRACE_AUDIO")&&dest&&ctx->r2==0&&(queue==0x80315440||queue==0x80368238||queue==0x80368270)){
  auto word=[&](uint32_t a){return uint32_t(MEM_W(0,S32(a)));};
  uint32_t cur=word(0x803156B4),head=word(0x803156A4);
  fprintf(stderr,"audio_trace q=%08X msg=%08X cur=%08X type=%u state=%u flags=%x head=%08X\n",queue,word(dest),cur,cur?word(cur+16):0,cur?word(cur+4):0,cur?word(cur+8):0,head);
 }
}

}
