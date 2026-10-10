#include "pak_runtime.h"
#include "support.h"
#include "pak_store.h"
#include "save_store.h"
#include "blast_save_format.h"
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <cstdio>
std::filesystem::path bc_data_directory;
[[noreturn]] void bc_fatal(const char* name,uint32_t){throw std::runtime_error(name);}
extern "C" void osContGetQuery_recomp(uint8_t* rdram,recomp_context* ctx){for(int i=0;i<16;i++)MEM_B(i,ctx->r4)=0;MEM_B(7,ctx->r4)=8;}
extern "C" void osContInit_recomp(uint8_t* rdram,recomp_context* ctx){auto out=ctx->r6;for(int i=0;i<16;i++)MEM_B(i,out)=0;ctx->r2=0;}
static void check(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
int main(){
 bc_data_directory=std::filesystem::temp_directory_path()/("bc-pak-abi-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(bc_data_directory);
 struct Cleanup{~Cleanup(){std::filesystem::remove_all(bc_data_directory);}} cleanup;
 std::vector<uint8_t> ram(9*1024*1024);auto* rdram=ram.data();recomp_context ctx{};
 auto a=[](uint32_t p){return S32(p);};constexpr uint32_t pfs=0x80001000,stack=0x80003000,name=0x80002000,ext=0x80002020,out=0x80002100,buffer=0x80002200;
 bc_initialize_pak();ctx.r29=a(stack);ctx.r4=a(0x80004000);ctx.r5=a(pfs);ctx.r6=0;bc_osPfsInit(rdram,&ctx);
 check(ctx.r2==0&&MEM_W(0,a(pfs))==1&&MEM_W(80,a(pfs))==16&&MEM_BU(100,a(pfs))==1,"OSPfs init and layout");
 ctx.r4=a(0x80004000);ctx.r5=a(out);bc_osPfsIsPlug(rdram,&ctx);check(ctx.r2==0&&MEM_BU(0,a(out))==1,"port-zero mask");
 ctx.r4=a(out);bc_osContGetQuery(rdram,&ctx);check(MEM_BU(2,a(out))==1&&MEM_BU(6,a(out))==0&&MEM_BU(7,a(out))==8,"query status matches emulated Pak and absent ports");
 for(int i=0;i<16;i++)MEM_B(i,a(name))=i+1;for(int i=0;i<4;i++)MEM_B(i,a(ext))=i+20;
 auto identity=[&](){ctx.r4=a(pfs);ctx.r5=0x3031;ctx.r6=0x4E424345;ctx.r7=a(name);MEM_W(16,a(stack))=a(ext);};
 identity();MEM_W(20,a(stack))=3584;MEM_W(24,a(stack))=a(out);bc_osPfsAllocateFile(rdram,&ctx);check(ctx.r2==0&&MEM_W(0,a(out))==0,"O32 allocate stack arguments");
 identity();MEM_W(20,a(stack))=a(out);bc_osPfsFindFile(rdram,&ctx);check(ctx.r2==0&&MEM_W(0,a(out))==0,"O32 find stack arguments");
 ctx.r4=a(pfs);ctx.r5=0;ctx.r6=0;ctx.r7=0;MEM_W(16,a(stack))=0;MEM_W(20,a(stack))=a(out);bc_osPfsFindFile(rdram,&ctx);
 check(ctx.r2==0&&MEM_W(0,a(out))==1,"libultra empty-directory lookup");
 for(int i=0;i<32;i++)MEM_B(i,a(buffer))=i+1;
 auto io=[&](int flag){ctx.r4=a(pfs);ctx.r5=0;ctx.r6=flag;ctx.r7=3520;MEM_W(16,a(stack))=32;MEM_W(20,a(stack))=a(buffer);bc_osPfsReadWriteFile(rdram,&ctx);};
 io(1);check(ctx.r2==0,"write last page");for(int i=0;i<32;i++)MEM_B(i,a(buffer))=0;io(0);check(ctx.r2==0,"read last page");for(int i=0;i<32;i++)check(MEM_BU(i,a(buffer))==i+1,"guest byte order");
 ctx.r4=a(pfs);ctx.r5=0;ctx.r6=a(out);bc_osPfsFileState(rdram,&ctx);check(ctx.r2==0&&MEM_W(0,a(out))==3584&&uint32_t(MEM_W(4,a(out)))==0x4E424345&&MEM_HU(8,a(out))==0x3031,"OSPfsState fields");
 check(MEM_BU(10,a(out))==20&&MEM_BU(14,a(out))==1,"OSPfsState extension/name offsets");
 ctx.r4=a(pfs);ctx.r5=a(out);bc_osPfsFreeBlocks(rdram,&ctx);check(ctx.r2==0&&MEM_W(0,a(out))==123*256-3584,"free byte count");
 bc_initialize_pak();io(0);check(ctx.r2==0&&MEM_BU(0,a(buffer))==1,"restart reload");
 auto backup=bc_data_directory/"controller-pak.bin.bak";std::filesystem::remove(backup);std::filesystem::create_directory(backup);
 for(int i=0;i<32;i++)MEM_B(i,a(buffer))=0xEE;
 io(1);check(ctx.r2==4,"failed atomic write returns controller error");
 io(0);check(ctx.r2==0&&MEM_BU(0,a(buffer))==1,"failed commit retains in-memory progress");
 std::filesystem::remove(backup);bc_initialize_pak();io(0);check(ctx.r2==0&&MEM_BU(0,a(buffer))==1,"failed commit retains persisted progress");
 ctx.r4=a(pfs);ctx.r5=0;ctx.r6=0;ctx.r7=0;MEM_W(16,a(stack))=32;MEM_W(20,a(stack))=a(0x807FFFF0);bc_osPfsReadWriteFile(rdram,&ctx);check(ctx.r2==5,"guest bounds checked");
 ctx.r29=a(0x807FFFF0);identity();bc_osPfsAllocateFile(rdram,&ctx);check(ctx.r2==5,"stack bounds checked before access");ctx.r29=a(stack);
 identity();bc_osPfsDeleteFile(rdram,&ctx);check(ctx.r2==0,"delete");identity();MEM_W(20,a(stack))=a(out);bc_osPfsFindFile(rdram,&ctx);check(ctx.r2==5&&MEM_W(0,a(out))==-1,"deleted result");
 ctx.r4=a(pfs);ctx.r5=a(0x80800000);bc_osPfsFreeBlocks(rdram,&ctx);check(ctx.r2==5,"RAM scratch not accepted as original guest save pointer");
 ctx.r4=a(0x80004000);ctx.r5=a(pfs);ctx.r6=1;bc_osPfsInit(rdram,&ctx);check(ctx.r2==1,"other ports have no Pak");
 bc::SaveStore::Image legacy{};const char legacy_name[]="OLD TEST";
 for(size_t i=0;i<sizeof(legacy_name);i++)legacy[i]=legacy_name[i];legacy[100]=0x55;legacy[252]=0x81;legacy[255]=0x60;
 const std::array<uint8_t,8> marker={0x87,0x56,0x9A,0xB6,0xCD,0x07,0x6A,0xEC};for(size_t i=0;i<8;i++)legacy[504+i]=marker[i];
 bc::SaveStore(bc_data_directory/"eeprom.bin").save(legacy);
 bc_initialize_pak();ctx.r4=a(pfs);ctx.r5=a(out);bc_osPfsFreeBlocks(rdram,&ctx);
 check(ctx.r2==0&&MEM_W(0,a(out))==123*256,"an existing deliberately empty Pak is not repopulated from EEPROM");
 std::filesystem::remove(bc_data_directory/"controller-pak.bin");std::filesystem::remove(bc_data_directory/"controller-pak.bin.bak");
 bc_initialize_pak();ctx.r4=a(pfs);ctx.r5=0;ctx.r6=0;ctx.r7=0;MEM_W(16,a(stack))=256;MEM_W(20,a(stack))=a(buffer);bc_osPfsReadWriteFile(rdram,&ctx);
 check(ctx.r2==0&&MEM_BU(0,a(buffer))=='O'&&MEM_BU(100,a(buffer))==0x55,"fresh upgrade imports the complete legacy player");
 legacy[100]^=1;bc::SaveStore(bc_data_directory/"eeprom.bin").save(legacy);bc_initialize_pak();bc_osPfsReadWriteFile(rdram,&ctx);
 check(ctx.r2==0&&MEM_BU(100,a(buffer))==0x55,"existing Pak never reimports a changed legacy source");
 auto packet=[&](int offset,std::span<const uint8_t> bytes){
  for(size_t i=0;i<bytes.size();i++)MEM_B(i,a(buffer))=bytes[i];
  ctx.r4=a(pfs);ctx.r5=0;ctx.r6=1;ctx.r7=offset;MEM_W(16,a(stack))=bytes.size();MEM_W(20,a(stack))=a(buffer);bc_osPfsReadWriteFile(rdram,&ctx);
 };
 std::array<uint8_t,32> footer{};std::copy(bc::blast_save_in_progress.begin(),bc::blast_save_in_progress.end(),footer.begin());
 packet(0xDE0,footer);check(ctx.r2==0,"guest begins a save transaction");
 std::array<uint8_t,32> progress{};progress.fill(0xAA);packet(96,progress);check(ctx.r2==0,"guest stages a new progress packet");
 auto persisted=bc::PakStore(bc_data_directory/"controller-pak.bin").load();
 check(persisted.file(0)->data[100]==0x55&&persisted.file(0)->data[0xDE0]==0x87,"unfinished guest transaction leaves complete disk save intact");
 bc_initialize_pak();ctx.r4=a(pfs);ctx.r5=0;ctx.r6=0;ctx.r7=96;MEM_W(16,a(stack))=32;MEM_W(20,a(stack))=a(buffer);bc_osPfsReadWriteFile(rdram,&ctx);
 check(ctx.r2==0&&MEM_BU(4,a(buffer))==0x55,"restart during guest transaction restores the complete save");
 packet(0xDE0,footer);packet(96,progress);
 std::filesystem::remove(backup);std::filesystem::create_directory(backup);
 std::copy(bc::blast_save_complete.begin(),bc::blast_save_complete.end(),footer.begin());packet(0xDE0,footer);
 check(ctx.r2==4,"failed guest transaction commit reports a Pak error");
 ctx.r4=a(pfs);ctx.r5=0;ctx.r6=0;ctx.r7=96;MEM_W(16,a(stack))=32;MEM_W(20,a(stack))=a(buffer);bc_osPfsReadWriteFile(rdram,&ctx);
 check(ctx.r2==0&&MEM_BU(4,a(buffer))==0xAA,"failed transaction retains staged progress for retry");
 std::filesystem::remove(backup);packet(0xDE0,footer);check(ctx.r2==0,"guest retries the complete marker successfully");
 bc_initialize_pak();ctx.r4=a(pfs);ctx.r5=0;ctx.r6=0;ctx.r7=96;MEM_W(16,a(stack))=32;MEM_W(20,a(stack))=a(buffer);bc_osPfsReadWriteFile(rdram,&ctx);
 check(ctx.r2==0&&MEM_BU(4,a(buffer))==0xAA,"successful transaction survives restart");
 puts("Controller Pak ABI passed");
}
