#include "pc_options.h"
#include "support.h"
#include "prototype.h"
#include "funcs.h"
#include "controls_menu.h"
#include "input.h"
#include "input_glyph.h"
#include <json.hpp>
#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <cstring>
namespace {
std::mutex mutex;
bc::Settings saved;
std::unique_ptr<bc::SettingsMenu> menu;
std::optional<bc::SettingsChange> pending;
std::optional<bc::Settings> filters;
std::unique_ptr<bc::ControlsMenu> controls;
std::optional<bc::RemapRequest> remap;
bool cancel_controls=false;
uint64_t controls_revision=UINT64_MAX;
std::string controls_log;
std::array<uint8_t,28> window_original{};
int32_t rows_original=0;
constexpr uint32_t window=0x802F8BDC,entries=0x802F5804,custom=0x80800200,strings=0x80801000,more=0x80800000;
// The runtime allocates 9 MiB. These tables and strings occupy host-only RAM
// beyond the original 8 MiB; guest font code turns them into normal geometry.
// They must never be submitted as RSP/RDP addresses.
gpr address(uint32_t value){return S32(value);}
void text(uint8_t* rdram,uint32_t destination,const std::string& value){
 if(value.size()>=64)bc_fatal("pc_options_text_bounds",destination);
 for(size_t i=0;i<=value.size();i++)MEM_B(i,address(destination))=i<value.size()?value[i]:0;
}
void restore(uint8_t* rdram){
 std::memcpy(rdram+(window-0x80000000),window_original.data(),window_original.size());
 MEM_W(0,address(0x8036BB24))=rows_original;menu.reset();controls.reset();remap.reset();cancel_controls=true;controls_log.clear();
}
void show(uint8_t* rdram,recomp_context* ctx){
 ctx->r4=0x8000;func_8026AF6C(rdram,ctx);
}
void paint(uint8_t* rdram){
 auto view=controls?std::optional<bc::InputModel>(bc_input_view()):std::nullopt;
 auto rows=controls?controls->rows(*view):menu->rows();if(rows.size()>4)bc_fatal("pc_options_row_bounds");
 auto title=controls?controls->title(*view):menu->status();if(title.empty())title="PC OPTIONS";
 if(controls&&controls_revision!=controls->revision()){MEM_H(0x18,address(window))=1;controls_revision=controls->revision();}
 text(rdram,strings,title);MEM_W(0xC,address(custom))=S32(strings);MEM_B(0x18,address(custom))=7;
 for(size_t i=0;i<rows.size();i++){
  auto row=custom+(i+1)*28;auto string=strings+(i+1)*64;
  const bool info=controls&&!controls->interactive(i);
  text(rdram,string,rows[i]);MEM_H(0,address(row))=info?0x60:0x61;MEM_W(0xC,address(row))=S32(string);
  // Original View Stats can be greyed out at this point. PC rows use the
  // original enabled white/red palette independently of that guest state.
  MEM_B(0x18,address(row))=7;MEM_B(0x19,address(row))=4;
 }
 MEM_W(8,address(window))=uint32_t(MEM_W(8,address(window)))|0x8000u;
 MEM_H(0xE,address(window))=0;MEM_H(0x10,address(window))=rows.size()+1;
 MEM_H(0x18,address(window))=std::clamp<int>(MEM_HU(0x18,address(window)),1,rows.size());
 MEM_W(0,address(0x8036BB24))=S32(custom);
 if(controls){
  auto value=nlohmann::json{{"event","pc_controls_view"},{"title",title},{"rows",rows}}.dump();
  if(value!=controls_log){fprintf(stderr,"%s\n",value.c_str());controls_log=std::move(value);}
 }
}
}
namespace bc {
void initialize_pc_options(const Settings& settings){std::lock_guard lock(mutex);saved=settings;menu.reset();pending.reset();filters.reset();controls.reset();remap.reset();cancel_controls=false;controls_log.clear();}
void refresh_pc_settings(const Settings& settings){std::lock_guard lock(mutex);saved=settings;if(menu)menu->refresh(settings);}
std::optional<SettingsChange> take_pc_change(){std::lock_guard lock(mutex);auto out=pending;pending.reset();return out;}
void acknowledge_pc_change(const Settings& settings,bool success){std::lock_guard lock(mutex);if(success)saved=settings;if(menu)menu->acknowledge(saved,success);}
void queue_pc_filter_change(const Settings& settings){std::lock_guard lock(mutex);filters=settings;}
std::optional<Settings> take_pc_filter_change(){std::lock_guard lock(mutex);auto out=filters;filters.reset();return out;}
std::optional<RemapRequest> take_pc_remap_request(){std::lock_guard lock(mutex);auto out=remap;remap.reset();return out;}
bool take_pc_controls_cancel(){std::lock_guard lock(mutex);auto out=cancel_controls;cancel_controls=false;return out;}
void acknowledge_pc_remap(const RemapResult& result){std::lock_guard lock(mutex);if(controls)controls->acknowledge(result);}
}
extern "C" void bc_pc_options_draw(uint8_t* rdram){
 std::lock_guard lock(mutex);
 text(rdram,more,"PC OPTIONS");MEM_W(0xC,address(entries+28))=S32(more);
 if(!menu)return;
 const auto state=uint32_t(MEM_W(0,address(0x80364A94)));
 if(MEM_W(0,address(0x80364A90))!=0||(state!=4&&state!=256)){restore(rdram);return;}
 paint(rdram);
}
extern "C" void bc_pc_options_glyphs(uint8_t* rdram,recomp_context* ctx,uint32_t frame){
 std::lock_guard lock(mutex);
 if(!menu||!controls||MEM_H(0,address(0x8036BB1C))!=2)return;
 if(frame<0x80000000||frame>0x807DEB68)return;
 auto pointer=uint32_t(ctx->r2);const auto begin=frame+0x48B0,end=frame+0xA3A0;
 if(pointer<begin||pointer>end||(pointer&7))return;
 auto view=bc_input_view();auto prompts=controls->prompts(view);
 if(prompts.empty())return;
 auto opacity=std::clamp<int>(MEM_H(0,address(0x8036BB20)),0,255);
 if(!opacity)return;
 std::vector<bc::GlyphDraw> draws;
 for(size_t i=0;i<prompts.size();i++){
  auto row=custom+(i+1)*28;
  // Match the original selected-row blink. Help information rows never blink.
  if(MEM_HU(0x18,address(window))==i+1&&(MEM_HU(0,address(row))&0x40)&&uint32_t(MEM_W(0,address(0x803156C4)))%15>=11)continue;
  int x=MEM_H(4,address(window))+MEM_H(2,address(row))-18;
  int y=MEM_H(6,address(window))+MEM_H(4,address(row))+2;
  auto glyph=bc::input_glyph(prompts[i],view.family());
  if(glyph)draws.push_back({*glyph,x,y,16,opacity});
 }
 auto commands=bc::glyph_commands(draws);
 if(std::getenv("BC_TRACE_INPUT")&&!commands.empty()){
  static std::string previous;std::string names;for(auto d:draws)names+=std::string(bc::glyph_name(d.sprite))+",";
  if(names!=previous){fprintf(stderr,"{\"event\":\"pc_atlas_commands\",\"sprites\":\"%s\",\"x\":%d,\"y\":%d}\n",names.c_str(),draws.front().x,draws.front().y);previous=names;}
 }
 // Reserve space for the original caller's end-of-frame commands as well.
 if(commands.empty()||commands.size()*8+256>end-pointer)return;
 for(auto command:commands){MEM_W(0,address(pointer))=command[0];MEM_W(4,address(pointer))=command[1];pointer+=8;}
 ctx->r2=address(pointer);
}
extern "C" bool bc_pc_options_selection(uint8_t* rdram,recomp_context* ctx){
 std::lock_guard lock(mutex);
 const auto state=uint32_t(MEM_W(0,address(0x80364A94)));
 if(MEM_W(0,address(0x80364A90))!=0||(state!=4&&state!=256))return false;
 const auto selection_address=ctx->r4;auto choice=MEM_HU(0,selection_address);
 if(!menu){
  // The original pause accepts Start as a selection, then exits via its
  // submenu transition. Preserve that path; only Confirm opens PC options.
  if(MEM_HU(0,address(0x80370C28))&0x1000)return false;
  if(choice!=1||MEM_H(0,address(0x8036BB18))!=0)return false;
  std::memcpy(window_original.data(),rdram+(window-0x80000000),28);rows_original=MEM_W(0,address(0x8036BB24));
  std::memcpy(rdram+(custom-0x80000000),rdram+(entries-0x80000000),5*28);
  menu=std::make_unique<bc::SettingsMenu>(saved);
 }else{
  MEM_B(0,address(0x802E8BD4))=0; // Back remains inside PC pages.
  if(controls){
   if(choice==0xFFFF)controls->back();else controls->select(int(choice)-1);
   if(auto request=controls->take_request())remap=*request;
   if(controls->take_cancel_request())cancel_controls=true;
   if(controls->closed()){controls.reset();remap.reset();cancel_controls=true;controls_log.clear();MEM_H(0x18,address(window))=1;}
  }else if(choice==0xFFFF)menu->back();else menu->select(int(choice)-1);
  if(auto change=menu->take_change())pending=*change;
  if(menu->take_game_options_request()){
   restore(rdram);MEM_H(0,selection_address)=1;return false;
  }
  if(menu->take_controls_request()){controls=std::make_unique<bc::ControlsMenu>(false);controls_revision=UINT64_MAX;}
  if(menu->take_help_request()){controls=std::make_unique<bc::ControlsMenu>(true);controls_revision=UINT64_MAX;}
 }
 if(menu->closed())restore(rdram);else paint(rdram);
 MEM_H(0,selection_address)=0;show(rdram,ctx);
 fprintf(stderr,"{\"event\":\"pc_options_selection\",\"choice\":%u,\"active\":%s}\n",choice,menu?"true":"false");
 return true;
}
