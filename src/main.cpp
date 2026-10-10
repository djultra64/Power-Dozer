#include "support.h"
#include "prototype.h"
#include "funcs.h"
#include "ultramodern/ultramodern.hpp"
#include "librecomp/rsp.hpp"
#include <SDL.h>
#include <fstream>
#include <iterator>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstring>
#include <mutex>
#include "audio_fifo.h"
#include "audio_pcm.h"
#include "wav_capture.h"

double bc_scale=3;
bool bc_wide=false;
int bc_refresh=0;
void bc_load_save();
void bc_input_init(const std::string&);
void bc_input_update(SDL_Window*);
bool bc_input_event(const SDL_Event&);
void bc_poll_input();
bool bc_get_input(int,uint16_t*,float*,float*);
RspExitReason blastcorps_audio_rsp(uint8_t*,uint32_t);
static SDL_AudioDeviceID audio_device=0;
static std::mutex audio_mutex;
static AudioFifo audio_fifo;
static uint64_t audio_buffers=0,audio_frames=0,audio_empty_queues=0,audio_rejected=0;
static uint32_t audio_hz=0,audio_max_queue=0;
static int audio_peak=0;
static std::string capture_path;
static std::unique_ptr<WavCapture> capture;
bool bc_audio_room(){std::lock_guard lock(audio_mutex);bool room=audio_fifo.room(audio_device?SDL_GetQueuedAudioSize(audio_device):0);if(!room)++audio_rejected;return room;}
static bool has_audio=false;
static bool mute_rsp=false;
static void set_frequency(uint32_t freq){
 std::lock_guard lock(audio_mutex); if(audio_device)SDL_CloseAudioDevice(audio_device);
 audio_fifo.reset();
 audio_hz=freq;
 SDL_AudioSpec wanted{};wanted.freq=freq;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=512;
 audio_device=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);
 if(!audio_device)bc_fatal("audio_open");SDL_PauseAudioDevice(audio_device,0);
 fprintf(stdout,"{\"event\":\"audio_frequency\",\"hz\":%u}\n",freq);
}
static void queue_audio(int16_t* samples,size_t count){
 std::lock_guard lock(audio_mutex);
 auto pcm=n64_stereo_pcm(std::span(samples,count));
 auto queued=SDL_GetQueuedAudioSize(audio_device);
 if(audio_buffers&&queued==0)++audio_empty_queues;
 if(SDL_QueueAudio(audio_device,pcm.data(),pcm.size()*2)!=0)bc_fatal("audio_queue");
 audio_fifo.push(count*2);
 ++audio_buffers;audio_frames+=count/2;audio_max_queue=std::max(audio_max_queue,queued+uint32_t(count*2));
 for(auto sample:pcm)audio_peak=std::max(audio_peak,std::abs(int(sample)));
 if(!capture_path.empty()){
  if(!capture)capture=std::make_unique<WavCapture>(capture_path,audio_hz);
  capture->append(pcm);
 }
 static bool logged_signal=false;if(!logged_signal){int peak=0;for(size_t i=0;i<count;i++)peak=std::max(peak,std::abs(int(samples[i])));if(peak){logged_signal=true;fprintf(stdout,"{\"event\":\"audio_signal\",\"peak\":%d,\"samples\":%zu}\n",peak,count);}}
 if(!has_audio){has_audio=true;fprintf(stdout,"{\"event\":\"audio_samples\",\"count\":%zu}\n",count);}
}
static size_t remaining_audio(){std::lock_guard lock(audio_mutex);return audio_fifo.head_bytes(audio_device?SDL_GetQueuedAudioSize(audio_device):0)/4;}
static RspUcodeFunc* select_ucode(const OSTask* task){return task->t.type==M_AUDTASK?blastcorps_audio_rsp:nullptr;}
static void rsp_init(){recomp::rsp::constants_init();recomp::rsp::set_callbacks({select_ucode});}
void bc_complete_audio();
static bool run_rsp(uint8_t* rdram,const OSTask* task){if(mute_rsp){bc_complete_audio();return true;}auto n=++bc_audio;if(n<=3){fprintf(stderr,"audio rsp ucode=%08X type=%u\n",task->t.ucode,task->t.type);fflush(stderr);}bool ok=recomp::rsp::run_task(rdram,task);bc_complete_audio();return ok;}
static void count_vi(){++bc_vis;}
static void error(const char* text){fprintf(stderr,"runtime: %s\n",text);bc_fatal("runtime_error");}
static std::vector<uint8_t> read_file(const std::string& name){std::ifstream f(name,std::ios::binary);if(!f)bc_fatal("file_open");return {std::istreambuf_iterator<char>(f),{}};}
static void load(uint8_t* ram,const std::vector<uint8_t>& bytes,uint32_t offset){if(bytes.size()>0x800000-offset)bc_fatal("load_bounds");for(size_t i=0;i<bytes.size();i++)ram[(offset+i)^3]=bytes[i];}
int main(int argc,char** argv){
 setvbuf(stdout,nullptr,_IOLBF,0);
 double seconds=8;std::string family="auto";
 for(int i=1;i<argc;i++) {
  std::string a=argv[i];
  if(a=="--seconds"&&i+1<argc)seconds=std::stod(argv[++i]);
  else if(a=="--data-dir"&&i+1<argc)bc_data_directory=argv[++i];
  else if(a=="--scale"&&i+1<argc)bc_scale=std::stod(argv[++i]);
  else if(a=="--mute")mute_rsp=true;
  else if(a=="--wide")bc_wide=true;
  else if(a=="--refresh"&&i+1<argc)bc_refresh=std::stoi(argv[++i]);
  else if(a=="--family"&&i+1<argc)family=argv[++i];
  else if(a=="--capture-audio"&&i+1<argc)capture_path=argv[++i];
  else if(a=="--help"){printf("Blast Corps experimental native beta\n--seconds N --scale N --wide --refresh N --mute --family auto|xbox|playstation|switch --capture-audio FILE.wav --data-dir PATH\nKeyboard: arrows move, Z confirm/A, X back/B, Enter start, Shift Z-trigger, I/J/K/L C-buttons, Q/E L/R\nAlt+Enter: toggle desktop fullscreen (does not send Start to the game)\n");return 0;}
  else{fprintf(stderr,"Unknown option: %s\n",a.c_str());return 64;}
 }
 if(seconds<0||bc_scale<1||bc_scale>9||(family!="auto"&&family!="xbox"&&family!="playstation"&&family!="switch"))return 64;
 bc_rom=read_file(BC_ROOT "/.local/rom.z64");if(bc_rom.size()!=8388608)bc_fatal("rom_size");
 std::vector<uint8_t> ram(0x900000);
 load(ram.data(),read_file(BC_ROOT "/.local/hd_code.bin"),0x2447C0);
 load(ram.data(),read_file(BC_ROOT "/.local/hd_front_end.bin"),0x1E7000);
 auto* rdram=ram.data();
 MEM_W(0,S32(0x80000300))=1;MEM_W(0,S32(0x80000308))=S32(0xB0000000);MEM_W(0,S32(0x80000318))=0x800000;
 // Boot loader mailboxes supplying compressed front-end ROM boundaries.
 MEM_W(0,S32(0x803FFFF8))=0x7E3AD0;MEM_W(0,S32(0x803FFFFC))=0x7F9BDD;
 bc_load_save();
 SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS,"0");
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)!=0)bc_fatal(SDL_GetError());
 auto* window=SDL_CreateWindow("Blast Corps prototype",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,bc_wide?1280:960,720,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);
 if(!window)bc_fatal(SDL_GetError());bc_input_init(family);
 ultramodern::renderer::GraphicsConfig graphics{};graphics.api_option=ultramodern::renderer::GraphicsApi::Vulkan;
 ultramodern::renderer::set_graphics_config(graphics);
 ultramodern::set_callbacks({rsp_init,run_rsp},{bc_create_context},{queue_audio,remaining_audio,set_frequency},
  {bc_poll_input,bc_get_input,[](int,bool){},[](int port){return ultramodern::input::connected_device_info_t{port==0?ultramodern::input::Device::Controller:ultramodern::input::Device::None,ultramodern::input::Pak::None};}}, {},{count_vi,nullptr},{error},{});
 ultramodern::preinit(rdram,window);
 fprintf(stdout,"{\"event\":\"native_entry\",\"entry\":\"0x802447C0\",\"scale\":%.1f,\"wide_requested\":%s,\"refresh_requested\":%d}\n",bc_scale,bc_wide?"true":"false",bc_refresh);fflush(stdout);
 std::thread guest([rdram]{ultramodern::set_entrypoint_thread();recomp_context ctx{};ctx.status_reg=0x2000FF01;ctx.r29=S32(0x801E6FF0);ctx.f_odd=&ctx.f0.u32h;recomp_entrypoint(rdram,&ctx);});guest.detach();
 auto start=std::chrono::steady_clock::now(),next=start;
 while(!exited){
  SDL_Event event;while(SDL_PollEvent(&event)){
   if(bc_input_event(event)){
    bool fullscreen=(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0;
    if(SDL_SetWindowFullscreen(window,fullscreen?0:SDL_WINDOW_FULLSCREEN_DESKTOP)!=0)fprintf(stderr,"Fullscreen: %s\n",SDL_GetError());
    else fprintf(stdout,"{\"event\":\"fullscreen\",\"enabled\":%s}\n",fullscreen?"false":"true");
   }
   if(event.type==SDL_QUIT)exited=true;
  }
  bc_input_update(window);
  auto now=std::chrono::steady_clock::now();
  if(now>=next){fprintf(stdout,"{\"event\":\"progress\",\"vi\":%llu,\"gfx\":%llu,\"audio\":%llu,\"polls\":%llu}\n",(unsigned long long)bc_vis.load(),(unsigned long long)bc_gfx.load(),(unsigned long long)bc_audio.load(),(unsigned long long)bc_polls.load());fflush(stdout);next=now+std::chrono::seconds(5);}
  if(seconds>0&&std::chrono::duration<double>(now-start).count()>=seconds)break;
  SDL_Delay(4);
 }
 fprintf(stdout,"{\"event\":\"probe_end\",\"vi\":%llu,\"gfx\":%llu,\"audio\":%llu,\"polls\":%llu}\n",(unsigned long long)bc_vis.load(),(unsigned long long)bc_gfx.load(),(unsigned long long)bc_audio.load(),(unsigned long long)bc_polls.load());fflush(stdout);
 {std::lock_guard lock(audio_mutex);
  if(capture)capture->finish();capture.reset();capture_path.clear();
  fprintf(stdout,"{\"event\":\"audio_summary\",\"hz\":%u,\"buffers\":%llu,\"frames\":%llu,\"peak\":%d,\"empty_queue_observations\":%llu,\"rejected_buffers\":%llu,\"max_queue_bytes\":%u}\n",audio_hz,(unsigned long long)audio_buffers,(unsigned long long)audio_frames,audio_peak,(unsigned long long)audio_empty_queues,(unsigned long long)audio_rejected,audio_max_queue);fflush(stdout);
 }
 // Experimental probe: terminate all cooperative guest threads together.
 std::_Exit(bc_gfx>0?0:72);
}
