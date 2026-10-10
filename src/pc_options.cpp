#include "pc_options.h"
#include "support.h"
#include "prototype.h"
#include "funcs.h"
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
 MEM_W(0,address(0x8036BB24))=rows_original;menu.reset();
}
void show(uint8_t* rdram,recomp_context* ctx){
 ctx->r4=0x8000;func_8026AF6C(rdram,ctx);
}
void paint(uint8_t* rdram){
 auto rows=menu->rows();if(rows.size()>4)bc_fatal("pc_options_row_bounds");
 auto title=menu->status();if(title.empty())title="PC OPTIONS";
 text(rdram,strings,title);MEM_W(0xC,address(custom))=S32(strings);
 for(size_t i=0;i<rows.size();i++){
  auto row=custom+(i+1)*28;auto string=strings+(i+1)*64;
  text(rdram,string,rows[i]);MEM_H(0,address(row))=0x61;MEM_W(0xC,address(row))=S32(string);
 }
 MEM_W(8,address(window))=uint32_t(MEM_W(8,address(window)))|0x8000u;
 MEM_H(0xE,address(window))=0;MEM_H(0x10,address(window))=rows.size()+1;
 MEM_H(0x18,address(window))=std::clamp<int>(MEM_HU(0x18,address(window)),1,rows.size());
 MEM_W(0,address(0x8036BB24))=S32(custom);
}
}
namespace bc {
void initialize_pc_options(const Settings& settings){std::lock_guard lock(mutex);saved=settings;menu.reset();pending.reset();filters.reset();}
void refresh_pc_settings(const Settings& settings){std::lock_guard lock(mutex);saved=settings;if(menu)menu->refresh(settings);}
std::optional<SettingsChange> take_pc_change(){std::lock_guard lock(mutex);auto out=pending;pending.reset();return out;}
void acknowledge_pc_change(const Settings& settings,bool success){std::lock_guard lock(mutex);if(success)saved=settings;if(menu)menu->acknowledge(saved,success);}
void queue_pc_filter_change(const Settings& settings){std::lock_guard lock(mutex);filters=settings;}
std::optional<Settings> take_pc_filter_change(){std::lock_guard lock(mutex);auto out=filters;filters.reset();return out;}
}
extern "C" void bc_pc_options_draw(uint8_t* rdram){
 std::lock_guard lock(mutex);
 text(rdram,more,"PC OPTIONS");MEM_W(0xC,address(entries+28))=S32(more);
 if(!menu)return;
 const auto state=uint32_t(MEM_W(0,address(0x80364A94)));
 if(MEM_W(0,address(0x80364A90))!=0||(state!=4&&state!=256)){restore(rdram);return;}
 paint(rdram);
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
  if(choice==0xFFFF)menu->back();else menu->select(int(choice)-1);
  if(auto change=menu->take_change())pending=*change;
  if(menu->take_game_options_request()){
   restore(rdram);MEM_H(0,selection_address)=1;return false;
  }
  // Controls editor and help are connected by the controls implementation.
  if(menu->take_controls_request())fprintf(stderr,"{\"event\":\"pc_controls_requested\"}\n");
  if(menu->take_help_request())fprintf(stderr,"{\"event\":\"pc_help_requested\"}\n");
 }
 if(menu->closed())restore(rdram);else paint(rdram);
 MEM_H(0,selection_address)=0;show(rdram,ctx);
 fprintf(stderr,"{\"event\":\"pc_options_selection\",\"choice\":%u,\"active\":%s}\n",choice,menu?"true":"false");
 return true;
}
