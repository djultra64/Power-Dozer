#include "pc_options.h"
#include "support.h"
#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstdio>
static unsigned shown=0;
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
 puts("PC options guest bridge passed");
}
