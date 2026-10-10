#include "pc_options.h"
#include "input.h"
#include "support.h"
#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstdio>
static unsigned shown=0;
static bc::InputModel input_view;
bc::InputModel bc_input_view(){return input_view;}
extern "C" void func_8026AF6C(uint8_t*,recomp_context* ctx){if(ctx->r4!=0x8000)throw std::runtime_error("wrong window demand");++shown;}
[[noreturn]] void bc_fatal(const char* message,uint32_t){throw std::runtime_error(message);}
static void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(){
 std::vector<uint8_t> ram(9*1024*1024);auto* rdram=ram.data();recomp_context ctx{};
 auto a=[](uint32_t value){return S32(value);};
 bc::initialize_pc_options({});
 MEM_W(0,a(0x80364A94))=4;MEM_H(0,a(0x8036BB18))=0;
 MEM_W(8,a(0x802F8BDC))=0xF1000039;MEM_H(0x10,a(0x802F8BDC))=5;MEM_H(0x18,a(0x802F8BDC))=1;
 MEM_W(0,a(0x8036BB24))=0x12345678;
 // The original pause greys out View Stats while driving. Custom PC rows
 // remain available even when their cloned original row was disabled.
 MEM_B(0x18,a(0x802F5804+2*28))=8;
 MEM_B(0x19,a(0x802F5804+2*28))=8;
 std::vector<uint8_t> original(28);std::memcpy(original.data(),rdram+0x2F8BDC,28);
 ctx.r4=a(0x8036BB16);
 auto choose=[&](uint16_t row){ctx.r4=a(0x8036BB16);MEM_H(0,ctx.r4)=row;return bc_pc_options_selection(rdram,&ctx);};
 check(!choose(0),"empty selection must stay original");check(!choose(3),"Restart must stay original");
 MEM_H(0,a(0x80370C28))=0x1000;
 check(!choose(1),"Start must retain original pause exit rather than open PC options");
 MEM_H(0,a(0x80370C28))=0;
 check(choose(1),"More opens PC options");bc_pc_options_draw(rdram);
 check(uint32_t(MEM_W(0,a(0x8036BB24)))==0x80800200,"guest custom table bound");
 check((uint32_t(MEM_W(8,a(0x802F8BDC)))&0x8000)!=0,"custom table flag");
 check(MEM_BU(0x18,a(0x80800200+2*28))==7&&MEM_BU(0x19,a(0x80800200+2*28))==4,"custom rows restore enabled and selected colours");
 check(choose(1),"Graphics opens");check(choose(1),"Resolution changes");
 auto change=bc::take_pc_change();check(change&&change->field==bc::SettingsField::Scale&&change->settings.scale==4.5,"selection reaches host save queue");
 bc::acknowledge_pc_change(change->settings,true);
 MEM_B(0,a(0x802E8BD4))=1;check(choose(0xFFFF),"Back returns root");check(MEM_BU(0,a(0x802E8BD4))==0,"Back must not unpause guest");
 check(choose(0xFFFF),"Back returns pause");
 check(std::memcmp(original.data(),rdram+0x2F8BDC,28)==0,"original pause structure restored");
 check(MEM_W(0,a(0x8036BB24))==0x12345678,"original row pointer restored");
 check(shown==5,"each intercepted selection reopens window");
 bc::initialize_pc_options({});check(choose(1),"reopen");
 MEM_W(0,a(0x80364A94))=1;bc_pc_options_draw(rdram);
 check(std::memcmp(original.data(),rdram+0x2F8BDC,28)==0,"leaving pause restores menu");
 check(!choose(1),"nonpause selections unchanged");
 MEM_W(0,a(0x80364A94))=4;
 check(choose(1),"open for game options");check(choose(3),"Controls opens");
 check(!choose(4),"Game Options returns to the original guest dispatch");
 check(MEM_HU(0,a(0x8036BB16))==1,"original More selection retained");
 check(std::memcmp(original.data(),rdram+0x2F8BDC,28)==0,"Game Options restores original table");
 bc::take_pc_controls_cancel();check(choose(1),"open editor path");check(choose(3),"Controls parent");check(choose(2),"Remap opens device picker");check(choose(1),"keyboard action list");check(choose(1),"Confirm requests capture");
 auto remap=bc::take_pc_remap_request();check(remap&&remap->action==bc::InputAction::Confirm&&remap->source==bc::InputSource::Keyboard,"guest action reaches main-thread capture queue");
 input_view.replace(bc::InputAction::Confirm,bc::InputSource::Keyboard,{{bc::BindingKind::Key,SDL_SCANCODE_T}});bc::acknowledge_pc_remap({true,false,"SAVED"});bc_pc_options_draw(rdram);
 auto row_string=[&](int row){auto string=MEM_W(0xC,a(0x80800200+row*28));std::string value;for(int i=0;i<64&&MEM_BU(i,string);i++)value+=char(MEM_BU(i,string));return value;};
 check(row_string(1)=="ACCEL/OK: T","guest rows show actual remapped keyboard binding");
 check(choose(2),"capture Back");bc::take_pc_remap_request();check(choose(0xFFFF),"cancel capture from menu");check(bc::take_pc_controls_cancel(),"editor close cancels main-thread capture");
 check(choose(0xFFFF),"action list Back returns devices");check(choose(0xFFFF),"device picker Back returns parent");check(choose(3),"Help opens");
 input_view.activity(bc::InputSource::Controller);input_view.manual_family(bc::ControllerFamily::Switch);bc_pc_options_draw(rdram);
 check(row_string(1)=="ACCEL/OK: B","in-game help follows physical Switch south");
 bc::take_pc_controls_cancel();MEM_W(0,a(0x80364A94))=1;bc_pc_options_draw(rdram);check(bc::take_pc_controls_cancel(),"leaving pause cancels editor state");
 puts("PC options guest bridge passed");
}
