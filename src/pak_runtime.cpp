#include "pak_runtime.h"
#include "pak_store.h"
#include "pak_session.h"
#include "save_migration.h"
#include "prototype.h"
#include <mutex>
#include <cstdio>
extern "C" void osContGetQuery_recomp(uint8_t*,recomp_context*);
extern "C" void osContInit_recomp(uint8_t*,recomp_context*);
namespace {
std::mutex mutex;
std::optional<bc::PakSession> session;
const bc::PakImage& image(){if(!session)bc_fatal("pak_not_initialized");return session->image();}
bc::PakStore store(){return bc::PakStore(bc_data_directory/"controller-pak.bin",bc::blast_pak_committed);}
// N64Recomp MEM macros expect sign-extended KSEG0 addresses. Validate the
// original 8 MiB before canonicalizing; no native pointer may escape this range.
bool valid(gpr value,size_t bytes,unsigned align=1){auto p=uint32_t(value);return p>=0x80000000&&p<=0x80800000&&bytes<=0x80800000ull-p&&p%align==0;}
gpr ptr(gpr value){return S32(uint32_t(value));}
bool initialized(uint8_t* rdram,gpr pfs){return valid(pfs,104,4)&&MEM_W(0,ptr(pfs))==1&&MEM_W(8,ptr(pfs))==0;}
bool stack(uint8_t*,recomp_context* ctx,size_t bytes){return valid(ctx->r29,bytes,4);}
bool key(uint8_t* rdram,recomp_context* ctx,gpr extension,bc::PakKey& out){
 if(!valid(ctx->r7,16)||!valid(extension,4))return false;
 out.company=uint16_t(ctx->r5);out.game=uint32_t(ctx->r6);
 for(size_t i=0;i<16;i++)out.name[i]=MEM_BU(i,ptr(ctx->r7));
 for(size_t i=0;i<4;i++)out.extension[i]=MEM_BU(i,ptr(extension));return true;
}
template<class F>int mutation(F&& call,const char* operation,int index=-1){
 try{
  const int result=call();
  if(result==0)fprintf(stderr,"{\"event\":\"%s\",\"operation\":\"%s\",\"free_bytes\":%d}\n",session->pending(index)?"pak_staged":"pak_saved",operation,image().free_bytes());
  return result;
 }catch(const std::exception& error){fprintf(stderr,"Controller Pak save: %s\n",error.what());return 4;}
}
}
void bc_initialize_pak(){
 std::lock_guard lock(mutex);try{
  const auto path=bc_data_directory/"controller-pak.bin";
  const bool fresh=!std::filesystem::exists(path)&&!std::filesystem::exists(path.string()+".bak");
  auto candidate=store().load();
  if(fresh&&bc::import_eeprom_player(candidate,bc::SaveStore(bc_data_directory/"eeprom.bin").load())){
   store().save(candidate);fprintf(stderr,"{\"event\":\"legacy_progress_imported\",\"source\":\"eeprom\",\"slot\":0}\n");
  }
  session.emplace(std::move(candidate));
 }
 catch(const std::exception& error){fprintf(stderr,"Controller Pak: %s\n",error.what());bc_fatal("pak_load");}
}
extern "C" {
void bc_osContGetQuery(uint8_t* rdram,recomp_context* ctx){
 if(!valid(ctx->r4,16,2)){ctx->r2=5;return;}ctx->r4=ptr(ctx->r4);osContGetQuery_recomp(rdram,ctx);
 // Ultramodern has no ControllerPak enum; expose card status alongside the
 // host Pfs services, without pretending this is its RumblePak implementation.
 if(MEM_BU(3,ctx->r4)==0)MEM_B(2,ctx->r4)=MEM_BU(2,ctx->r4)|1;
}
void bc_osContInit(uint8_t* rdram,recomp_context* ctx){
 if(!valid(ctx->r5,1)||!valid(ctx->r6,16,2)){ctx->r2=5;return;}
 ctx->r5=ptr(ctx->r5);ctx->r6=ptr(ctx->r6);const auto output=ctx->r6;osContInit_recomp(rdram,ctx);
 if(ctx->r2==0&&MEM_BU(3,output)==0)MEM_B(2,output)=MEM_BU(2,output)|1;
}
void bc_osPfsInit(uint8_t* rdram,recomp_context* ctx){
 if(uint32_t(ctx->r6)!=0){ctx->r2=1;return;}if(!valid(ctx->r5,104,4)){ctx->r2=5;return;}
 auto p=ptr(ctx->r5);for(int i=0;i<104;i++)MEM_B(i,p)=0;
 MEM_W(0,p)=1;MEM_W(4,p)=uint32_t(ctx->r4);MEM_W(8,p)=0;MEM_W(76,p)=1;MEM_W(80,p)=16;
 MEM_W(84,p)=8;MEM_W(88,p)=16;MEM_W(92,p)=24;MEM_W(96,p)=5;MEM_B(100,p)=1;ctx->r2=0;
}
void bc_osPfsIsPlug(uint8_t* rdram,recomp_context* ctx){if(!valid(ctx->r5,1)){ctx->r2=5;return;}MEM_B(0,ptr(ctx->r5))=1;ctx->r2=0;}
void bc_osPfsChecker(uint8_t* rdram,recomp_context* ctx){ctx->r2=initialized(rdram,ctx->r4)?0:5;}
void bc_osPfsFreeBlocks(uint8_t* rdram,recomp_context* ctx){
 std::lock_guard lock(mutex);if(!initialized(rdram,ctx->r4)||!valid(ctx->r5,4,4)){ctx->r2=5;return;}
 MEM_W(0,ptr(ctx->r5))=image().free_bytes();ctx->r2=0;
}
void bc_osPfsAllocateFile(uint8_t* rdram,recomp_context* ctx){
 std::lock_guard lock(mutex);bc::PakKey identity;
 if(!initialized(rdram,ctx->r4)||!stack(rdram,ctx,28)){ctx->r2=5;return;}
 auto sp=ptr(ctx->r29);auto extension=MEM_W(16,sp),output=MEM_W(24,sp);auto bytes=MEM_W(20,sp);
 if(!key(rdram,ctx,extension,identity)||!valid(output,4,4)){ctx->r2=5;return;}
 int index=-1;int result=mutation([&]{return session->allocate(identity,bytes,index,store());},"allocate");MEM_W(0,ptr(output))=index;ctx->r2=result;
}
void bc_osPfsFindFile(uint8_t* rdram,recomp_context* ctx){
 std::lock_guard lock(mutex);bc::PakKey identity;
 if(!initialized(rdram,ctx->r4)||!stack(rdram,ctx,24)){ctx->r2=5;return;}
 auto sp=ptr(ctx->r29);auto extension=MEM_W(16,sp),output=MEM_W(20,sp);
 if(!valid(output,4,4)){ctx->r2=5;return;}
 if(uint16_t(ctx->r5)==0&&uint32_t(ctx->r6)==0){
  int index=-1;for(int i=0;i<16;i++)if(!image().file(i)){index=i;break;}
  MEM_W(0,ptr(output))=index;ctx->r2=index<0?5:0;return;
 }
 if(!key(rdram,ctx,extension,identity)||!valid(output,4,4)){ctx->r2=5;return;}
 int index=-1;ctx->r2=image().find(identity,index);MEM_W(0,ptr(output))=index;
}
void bc_osPfsDeleteFile(uint8_t* rdram,recomp_context* ctx){
 std::lock_guard lock(mutex);bc::PakKey identity;
 if(!initialized(rdram,ctx->r4)||!stack(rdram,ctx,20)||!key(rdram,ctx,MEM_W(16,ptr(ctx->r29)),identity)){ctx->r2=5;return;}
 ctx->r2=mutation([&]{return session->remove(identity,store());},"delete");
}
void bc_osPfsReadWriteFile(uint8_t* rdram,recomp_context* ctx){
 std::lock_guard lock(mutex);
 if(!initialized(rdram,ctx->r4)||!stack(rdram,ctx,24)||uint32_t(ctx->r6)>1){ctx->r2=5;return;}
 auto bytes=MEM_W(16,ptr(ctx->r29)),buffer=MEM_W(20,ptr(ctx->r29));auto index=int32_t(ctx->r5),offset=int32_t(ctx->r7);
 if(bytes<=0||bytes>123*256||!valid(buffer,bytes)){ctx->r2=5;return;}
 std::vector<uint8_t> data(bytes);int result;
 if(ctx->r6==0){result=image().read(index,offset,data);if(result==0)for(int i=0;i<bytes;i++)MEM_B(i,ptr(buffer))=data[i];}
 else{for(int i=0;i<bytes;i++)data[i]=MEM_BU(i,ptr(buffer));result=mutation([&]{return session->write(index,offset,data,store());},"write",index);}
 ctx->r2=result;
}
void bc_osPfsFileState(uint8_t* rdram,recomp_context* ctx){
 std::lock_guard lock(mutex);if(!initialized(rdram,ctx->r4)||!valid(ctx->r6,32,4)){ctx->r2=5;return;}
 auto file=image().file(int32_t(ctx->r5));if(!file){ctx->r2=5;return;}auto output=ptr(ctx->r6);
 MEM_W(0,output)=file->data.size();MEM_W(4,output)=file->key.game;MEM_H(8,output)=file->key.company;
 for(int i=0;i<4;i++)MEM_B(10+i,output)=file->key.extension[i];for(int i=0;i<16;i++)MEM_B(14+i,output)=file->key.name[i];MEM_H(30,output)=0;ctx->r2=0;
}
}
