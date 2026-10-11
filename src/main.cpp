#include "support.h"
#include "prototype.h"
#include "funcs.h"
#include "ultramodern/ultramodern.hpp"
#include "librecomp/rsp.hpp"
#include "event_delivery.h"
#include <SDL.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#endif
#include <fstream>
#include <iterator>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <json.hpp>
#include <cmath>
#include "audio_fifo.h"
#include "audio_sdl.h"
#include "audio_pcm.h"
#include "wav_capture.h"
#include "settings.h"
#include "pc_options.h"
#include "pc_settings_input.h"
#include "pc_settings_renderer.h"
#include "pak_runtime.h"
#include "input.h"
#include "rom_input.h"
#include "data_import.h"
#include "data_directory.h"
#include "rom_selection.h"

// Optional acceptance telemetry from the same immutable value sent to RT64.
// Serialize only when requested; unchanged frames never produce log traffic.
static void trace_pc_settings(const std::shared_ptr<const bc::PcSettingsSnapshot>& view,int width,int height){
 static const bool enabled=std::getenv("BC_TRACE_PC_SETTINGS")!=nullptr;
 if(!enabled)return;
 nlohmann::json event={{"event","pc_settings_view"},{"open",bool(view)}};
 if(view){
  const auto family=view->controller_family;
  auto prompt=[&](const bc::InputPrompt& value){
   auto sprite=bc::input_glyph(value,family);
   return nlohmann::json{{"label",value.label},{"source",value.source==bc::InputSource::Keyboard?"keyboard":"controller"},{"glyph",sprite?std::string(bc::glyph_name(*sprite)):""}};
  };
  event["width"]=width;event["height"]=height;event["section"]=view->title;
  event["focus"]=view->focus;event["focus_target"]=int(view->focus_target);
  event["device"]=view->device_label;event["family"]=view->family_label;
  event["modal"]=view->modal==bc::PcSettingsModal::Capture?"capture":view->modal==bc::PcSettingsModal::RestoreDefaults?"defaults":"none";
  event["status"]=view->status;event["saving"]=view->saving;
  event["prompts"]={prompt(view->prompts[0]),prompt(view->prompts[1])};
  event["rows"]=nlohmann::json::array();
  for(const auto& row:view->rows){
   nlohmann::json item={{"label",row.label},{"value",row.value},{"restart",row.restart_required}};
   if(row.binding)item["binding"]=prompt(*row.binding);
   event["rows"].push_back(std::move(item));
  }
 }
 static std::string previous;
 const auto text=event.dump();if(text==previous)return;previous=text;
 std::fprintf(stderr,"%s\n",text.c_str());
}

double bc_scale=3;
bool bc_wide=false;
int bc_refresh=0;
void bc_load_save();
void bc_input_init(const std::string&);
void bc_input_set_family(const std::string&);
void bc_input_update(SDL_Window*);
bool bc_input_event(const SDL_Event&);
void bc_poll_input();
bool bc_get_input(int,uint16_t*,float*,float*);
RspExitReason blastcorps_audio_rsp(uint8_t*,uint32_t);
static bc::SDLAudioBackend audio_backend;
static bc::AudioOutput audio_output(audio_backend);
static std::mutex audio_mutex;
static AudioFifo audio_fifo;
static uint64_t audio_buffers=0,audio_frames=0,audio_empty_queues=0,audio_rejected=0;
static uint32_t audio_hz=0,audio_max_queue=0;
static int audio_peak=0;
static std::string capture_path;
static std::unique_ptr<WavCapture> capture;
static std::atomic<int> output_volume{100};
static std::atomic<bool> output_mute{false};
static uint64_t audio_now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static void audio_status(){
 static bool known=false,was_online=false;
 bool online=audio_output.online();
 if(!known||online!=was_online){
  fprintf(stderr,"{\"event\":\"audio_output_status\",\"available\":%s}\n",online?"true":"false");
  known=true;was_online=online;
 }
}
bool bc_audio_room(){std::lock_guard lock(audio_mutex);bool room=audio_fifo.room(audio_output.queued(audio_now()));audio_status();if(!room)++audio_rejected;return room;}
static bool has_audio=false;
void bc_set_audio_output(int volume,bool mute){output_volume.store(volume);output_mute.store(mute);}
static void set_frequency(uint32_t freq){
 std::lock_guard lock(audio_mutex);
 audio_fifo.reset();
 audio_hz=freq;
 audio_output.frequency(freq,audio_now());audio_status();
 fprintf(stderr,"{\"event\":\"audio_frequency\",\"hz\":%u}\n",freq);
}
static void queue_audio(int16_t* samples,size_t count){
 std::lock_guard lock(audio_mutex);
 auto pcm=n64_stereo_pcm(std::span(samples,count));
 apply_audio_output(pcm,output_volume.load(),output_mute.load());
 auto now=audio_now();auto queued=audio_output.queued(now);
 if(audio_buffers&&queued==0)++audio_empty_queues;
 audio_output.push(pcm,now);audio_status();
 audio_fifo.push(count*2);
 ++audio_buffers;audio_frames+=count/2;audio_max_queue=std::max(audio_max_queue,queued+uint32_t(count*2));
 for(auto sample:pcm)audio_peak=std::max(audio_peak,std::abs(int(sample)));
 if(!capture_path.empty()){
  if(!capture)capture=std::make_unique<WavCapture>(capture_path,audio_hz);
  capture->append(pcm);
 }
 static bool logged_signal=false;if(!logged_signal){int peak=0;for(size_t i=0;i<count;i++)peak=std::max(peak,std::abs(int(samples[i])));if(peak){logged_signal=true;fprintf(stderr,"{\"event\":\"audio_signal\",\"peak\":%d,\"samples\":%zu}\n",peak,count);}}
 if(!has_audio){has_audio=true;fprintf(stderr,"{\"event\":\"audio_samples\",\"count\":%zu}\n",count);}
}
static size_t remaining_audio(){std::lock_guard lock(audio_mutex);auto bytes=audio_output.queued(audio_now());audio_status();return audio_fifo.head_bytes(bytes)/4;}
static RspUcodeFunc* select_ucode(const OSTask* task){return task->t.type==M_AUDTASK?blastcorps_audio_rsp:nullptr;}
static void rsp_init(){recomp::rsp::constants_init();recomp::rsp::set_callbacks({select_ucode});}
void bc_complete_audio();
static bool run_rsp(uint8_t* rdram,const OSTask* task){auto n=++bc_audio;if(n<=3){fprintf(stderr,"audio rsp ucode=%08X type=%u\n",task->t.ucode,task->t.type);fflush(stderr);}bool ok=recomp::rsp::run_task(rdram,task);bc_complete_audio();return ok;}
static void count_vi(){++bc_vis;}
static void error(const char* text){fprintf(stderr,"runtime: %s\n",text);bc_fatal("runtime_error");}
static void load(uint8_t* ram,const std::vector<uint8_t>& bytes,uint32_t offset){if(bytes.size()>0x800000-offset)bc_fatal("load_bounds");for(size_t i=0;i<bytes.size();i++)ram[(offset+i)^3]=bytes[i];}
int main(int argc,char** argv){
#ifdef _WIN32
 SDL_SetMainReady();
#endif
#ifdef _WIN32
 // UCRT rejects a zero-sized line buffer; keep redirected diagnostics immediate.
 setvbuf(stdout,nullptr,_IONBF,0);
#else
 setvbuf(stdout,nullptr,_IOLBF,0);
#endif
 double seconds=0;std::string family="auto";bool save_settings=false,rom_dialog=true;std::filesystem::path rom_path,import_path;
 for(int i=1;i<argc;i++)if(std::string(argv[i])=="--help"){
  printf("Power Dozer v0.83 — Blast Corps native beta\n--rom PATH (USA Rev 1 z64/v64/n64/ZIP) --seconds N (0: unlimited)\n--data-dir PATH (default: executable/data) --import-data OLD_DIRECTORY\n--no-rom-dialog (report missing/invalid ROM without opening a chooser)\n--scale N --wide --original-aspect --refresh 0|30|60|120\n--fullscreen --windowed --msaa 0|2|4|8 --presentation nearest|linear|pixel --texture linear|three_point\n--volume 0..100 --mute --no-mute --family auto|xbox|playstation|switch --save-settings --capture-audio FILE.wav\nKeyboard: arrows walk/steer, Z accelerate/confirm, X reverse/back, Enter pause/start, Shift stop/exit, I/J/K/L camera, Q/E horn\nAlt+Enter: toggle fullscreen\n");return 0;
 }
 try {
 bc_data_directory=bc::application_data_directory();
 for(int i=1;i<argc;i++){
  auto option=std::string(argv[i]);
  if(option=="--data-dir"||option=="--import-data"){
   if(i+1>=argc)throw std::invalid_argument(option+" requires a path");
   if(option=="--data-dir")bc_data_directory=argv[++i];else import_path=argv[++i];
  }
 }
 if(!import_path.empty())bc::import_data(import_path,bc_data_directory);
 const auto persisted=bc::SettingsStore(bc_data_directory/"settings.json").load();
 bc::settings=persisted;
 auto number=[](const char* value){std::string text=value;size_t end=0;double result=std::stod(text,&end);if(end!=text.size())throw std::invalid_argument("Invalid numeric option");return result;};
 auto integer=[](const char* value){std::string text=value;size_t end=0;int result=std::stoi(text,&end);if(end!=text.size())throw std::invalid_argument("Invalid integer option");return result;};
 for(int i=1;i<argc;i++) {
  std::string a=argv[i];
  if(a=="--seconds"&&i+1<argc)seconds=number(argv[++i]);
  else if(a=="--rom"&&i+1<argc)rom_path=argv[++i];
  else if(a=="--no-rom-dialog")rom_dialog=false;
  else if(a=="--import-data"&&i+1<argc)++i;
  else if(a=="--data-dir"&&i+1<argc)bc_data_directory=argv[++i];
  else if(a=="--scale"&&i+1<argc)bc::settings.scale=number(argv[++i]);
  else if(a=="--mute")bc::settings.mute=true;
  else if(a=="--no-mute")bc::settings.mute=false;
  else if(a=="--wide")bc::settings.wide=true;
  else if(a=="--original-aspect")bc::settings.wide=false;
  else if(a=="--fullscreen")bc::settings.fullscreen=true;
  else if(a=="--windowed")bc::settings.fullscreen=false;
  else if(a=="--refresh"&&i+1<argc)bc::settings.refresh=integer(argv[++i]);
  else if(a=="--msaa"&&i+1<argc)bc::settings.msaa=integer(argv[++i]);
  else if(a=="--volume"&&i+1<argc)bc::settings.volume=integer(argv[++i]);
  else if(a=="--presentation"&&i+1<argc)bc::settings.presentation=argv[++i];
  else if(a=="--texture"&&i+1<argc)bc::settings.texture=argv[++i];
  else if(a=="--family"&&i+1<argc)bc::settings.family=argv[++i];
  else if(a=="--save-settings")save_settings=true;
  else if(a=="--capture-audio"&&i+1<argc)capture_path=argv[++i];
  else{fprintf(stderr,"Unknown option: %s\n",a.c_str());return 64;}
 }
 if(!std::isfinite(seconds)||seconds<0)throw std::invalid_argument("Seconds must be finite and nonnegative");
 bc::validate(bc::settings);
 bc::initialize_pc_options(save_settings?bc::settings:persisted);
 }catch(const std::exception& error){fprintf(stderr,"Options: %s\n",error.what());return 64;}
 bc_scale=bc::settings.scale;bc_wide=bc::settings.wide;bc_refresh=bc::settings.refresh;family=bc::settings.family;
 bc_set_audio_output(bc::settings.volume,bc::settings.mute);
 fprintf(stderr,"{\"event\":\"effective_settings\",\"scale\":%.2f,\"wide\":%s,\"fullscreen\":%s,\"refresh\":%d,\"msaa\":%d,\"volume\":%d,\"mute\":%s,\"presentation\":\"%s\",\"texture\":\"%s\",\"family\":\"%s\"}\n",bc_scale,bc_wide?"true":"false",bc::settings.fullscreen?"true":"false",bc_refresh,bc::settings.msaa,bc::settings.volume,bc::settings.mute?"true":"false",bc::settings.presentation.c_str(),bc::settings.texture.c_str(),family.c_str());
 bc::RomImage image;
 try{
  if(!rom_path.empty()||!rom_dialog)image=bc::load_cached_rom(bc_data_directory,rom_path);
  else{
   try{image=bc::load_cached_rom(bc_data_directory);}
   catch(const std::exception&){auto selected=bc::select_rom_gui(bc_data_directory);if(!selected)return 0;image=std::move(*selected);}
  }
 }
 catch(const std::exception& error){fprintf(stderr,"ROM: %s\n",error.what());return 66;}
 if(save_settings)try{bc::SettingsStore(bc_data_directory/"settings.json").save(bc::settings);}
 catch(const std::exception& error){fprintf(stderr,"Settings: %s\n",error.what());return 64;}
 bc_rom=std::move(image.rom);
 fprintf(stderr,"{\"event\":\"rom_loaded\",\"revision\":\"blastcorps-us-rev1\",\"code_bytes\":%zu,\"frontend_bytes\":%zu}\n",image.code.size(),image.frontend.size());
 std::vector<uint8_t> ram(0x900000);
 load(ram.data(),image.code,0x2447C0);
 load(ram.data(),image.frontend,0x1E7000);
 auto* rdram=ram.data();
 MEM_W(0,S32(0x80000300))=1;MEM_W(0,S32(0x80000308))=S32(0xB0000000);MEM_W(0,S32(0x80000318))=0x800000;
 // Boot loader mailboxes supplying compressed front-end ROM boundaries.
 MEM_W(0,S32(0x803FFFF8))=0x7E3AD0;MEM_W(0,S32(0x803FFFFC))=0x7F9BDD;
 bc_load_save();
 bc_initialize_pak();
 SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS,"0");
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER)!=0)bc_fatal(SDL_GetError());
 auto* window=SDL_CreateWindow("Power Dozer v0.83",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,bc_wide?1280:960,720,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);
 if(!window)bc_fatal(SDL_GetError());bc_input_init(family);
 if(bc::settings.fullscreen&&SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP)!=0)fprintf(stderr,"Fullscreen: %s\n",SDL_GetError());
 ultramodern::renderer::GraphicsConfig graphics{};graphics.api_option=ultramodern::renderer::GraphicsApi::Vulkan;
 ultramodern::renderer::set_graphics_config(graphics);
 ultramodern::set_callbacks({rsp_init,run_rsp},{bc_create_context},{queue_audio,remaining_audio,set_frequency},
  {bc_poll_input,bc_get_input,[](int,bool){},[](int port){return ultramodern::input::connected_device_info_t{port==0?ultramodern::input::Device::Controller:ultramodern::input::Device::None,ultramodern::input::Pak::None};}}, {},{count_vi,nullptr},{error},{});
 bc::initialize_event_delivery();
#ifdef _WIN32
 SDL_SysWMinfo native_window{};SDL_VERSION(&native_window.version);
 if(!SDL_GetWindowWMInfo(window,&native_window)||native_window.subsystem!=SDL_SYSWM_WINDOWS)bc_fatal("native_window_handle");
 ultramodern::preinit(rdram,ultramodern::renderer::WindowHandle{native_window.info.win.window,GetCurrentThreadId()});
#else
 ultramodern::preinit(rdram,window);
#endif
 fprintf(stderr,"{\"event\":\"native_entry\",\"entry\":\"0x802447C0\",\"scale\":%.1f,\"wide_requested\":%s,\"refresh_requested\":%d}\n",bc_scale,bc_wide?"true":"false",bc_refresh);fflush(stderr);
 std::thread guest([rdram]{ultramodern::set_entrypoint_thread();recomp_context ctx{};ctx.status_reg=0x2000FF01;ctx.r29=S32(0x801E6FF0);ctx.f_odd=&ctx.f0.u32h;recomp_entrypoint(rdram,&ctx);});guest.detach();
 auto start=std::chrono::steady_clock::now(),next=start;
 while(!exited){
  int settings_width=0,settings_height=0;SDL_GetWindowSize(window,&settings_width,&settings_height);
  bc::pc_settings_resize(settings_width,settings_height);
  SDL_Event event;while(SDL_PollEvent(&event)){
   if(event.type==SDL_AUDIODEVICEREMOVED&&!event.adevice.iscapture){
    std::lock_guard lock(audio_mutex);
    if(event.adevice.which==audio_backend.device()){audio_output.lost(audio_now());audio_status();}
   }
   if(bc_input_event(event)){
    bool fullscreen=(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0;
    if(SDL_SetWindowFullscreen(window,fullscreen?0:SDL_WINDOW_FULLSCREEN_DESKTOP)!=0)fprintf(stderr,"Fullscreen: %s\n",SDL_GetError());
    else {
     fprintf(stderr,"{\"event\":\"fullscreen\",\"enabled\":%s}\n",fullscreen?"false":"true");
     try{auto store=bc::SettingsStore(bc_data_directory/"settings.json");auto saved=store.load();saved.fullscreen=!fullscreen;store.save(saved);bc::settings.fullscreen=saved.fullscreen;bc::refresh_pc_settings(saved);}
     catch(const std::exception& error){fprintf(stderr,"Fullscreen preference: %s\n",error.what());}
    }
   }
   if(event.type==SDL_QUIT)exited=true;
  }
  {std::lock_guard lock(audio_mutex);audio_output.poll(audio_now());audio_status();}
  if(auto change=bc::take_pc_change()){
   try{
    auto store=bc::SettingsStore(bc_data_directory/"settings.json");auto updated=bc::merge_setting(store.load(),*change);
    const bool was_fullscreen=(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0;
    if(change->field==bc::SettingsField::Fullscreen&&SDL_SetWindowFullscreen(window,updated.fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0)!=0)throw std::runtime_error(SDL_GetError());
    try{store.save(updated);}catch(...){if(change->field==bc::SettingsField::Fullscreen)SDL_SetWindowFullscreen(window,was_fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0);throw;}
    if(change->field==bc::SettingsField::Volume||change->field==bc::SettingsField::Mute){bc::settings=bc::merge_setting(bc::settings,*change);bc_set_audio_output(bc::settings.volume,bc::settings.mute);}
    if(change->field==bc::SettingsField::Fullscreen)bc::settings.fullscreen=updated.fullscreen;
    if(change->field==bc::SettingsField::Presentation||change->field==bc::SettingsField::Texture){bc::settings=bc::merge_setting(bc::settings,*change);bc::queue_pc_filter_change(bc::settings);}
    if(change->field==bc::SettingsField::Family){bc::settings.family=updated.family;bc_input_set_family(updated.family);}
    bc::acknowledge_pc_change(updated,true);
   }catch(const std::exception& error){fprintf(stderr,"PC options: %s\n",error.what());bc::acknowledge_pc_change({},false);}
  }
  if(bc::take_pc_controls_cancel())bc_input_cancel_remap();
  if(auto request=bc::take_pc_remap_request())bc_input_begin_remap(*request);
  bc_input_update(window);
  if(auto result=bc_input_take_remap_result())bc::acknowledge_pc_remap(*result);
  auto settings_view=bc::pc_settings_current(bc_input_view());
  trace_pc_settings(settings_view,settings_width,settings_height);
  bc::publish_pc_settings_snapshot(std::move(settings_view));
  auto now=std::chrono::steady_clock::now();
  if(now>=next){fprintf(stderr,"{\"event\":\"progress\",\"vi\":%llu,\"gfx\":%llu,\"audio\":%llu,\"polls\":%llu}\n",(unsigned long long)bc_vis.load(),(unsigned long long)bc_gfx.load(),(unsigned long long)bc_audio.load(),(unsigned long long)bc_polls.load());fflush(stderr);next=now+std::chrono::seconds(5);}
  if(seconds>0&&std::chrono::duration<double>(now-start).count()>=seconds)break;
  SDL_Delay(4);
 }
 fprintf(stderr,"{\"event\":\"probe_end\",\"vi\":%llu,\"gfx\":%llu,\"audio\":%llu,\"polls\":%llu}\n",(unsigned long long)bc_vis.load(),(unsigned long long)bc_gfx.load(),(unsigned long long)bc_audio.load(),(unsigned long long)bc_polls.load());fflush(stderr);
 {std::lock_guard lock(audio_mutex);
  if(capture)capture->finish();capture.reset();capture_path.clear();
  fprintf(stderr,"{\"event\":\"audio_summary\",\"hz\":%u,\"buffers\":%llu,\"frames\":%llu,\"peak\":%d,\"empty_queue_observations\":%llu,\"rejected_buffers\":%llu,\"max_queue_bytes\":%u}\n",audio_hz,(unsigned long long)audio_buffers,(unsigned long long)audio_frames,audio_peak,(unsigned long long)audio_empty_queues,(unsigned long long)audio_rejected,audio_max_queue);fflush(stderr);
 }
 // Experimental probe: terminate all cooperative guest threads together.
 std::_Exit(bc_gfx>0?0:72);
}
