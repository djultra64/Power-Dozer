#include "rom_input.h"
#include "miniz.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <iterator>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F f,const char* message){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,message);}
static void write(const std::filesystem::path& p,const std::vector<uint8_t>& bytes){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());}
static std::vector<uint8_t> zip(const std::vector<uint8_t>& bytes,bool twice=false){
 mz_zip_archive archive{};check(mz_zip_writer_init_heap(&archive,0,0),"zip fixture init");
 check(mz_zip_writer_add_mem(&archive,"folder/BLAST.Z64",bytes.data(),bytes.size(),MZ_BEST_SPEED),"zip fixture add");
 if(twice)check(mz_zip_writer_add_mem(&archive,"other.n64",bytes.data(),bytes.size(),MZ_BEST_SPEED),"zip second fixture");
 void* memory=nullptr;size_t size=0;check(mz_zip_writer_finalize_heap_archive(&archive,&memory,&size),"zip fixture finalize");
 std::vector<uint8_t> result(static_cast<uint8_t*>(memory),static_cast<uint8_t*>(memory)+size);mz_free(memory);mz_zip_writer_end(&archive);return result;
}
int main(int argc,char** argv){
 check(bc::sha1({})=="da39a3ee5e6b4b0d3255bfef95601890afd80709","SHA-1 empty vector");
 std::string abc="abc";check(bc::sha1({reinterpret_cast<const uint8_t*>(abc.data()),abc.size()})=="a9993e364706816aba3e25717850c26c9cd0d89d","SHA-1 abc vector");
 std::string boundary="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
 check(bc::sha1({reinterpret_cast<const uint8_t*>(boundary.data()),boundary.size()})=="84983e441c3bd26ebaae4aa1f95129e5e54670f1","SHA-1 padding spanning two blocks");
 std::vector<uint8_t> million(1000000,'a');check(bc::sha1(million)=="34aa973cd4c4daa4f61eeb2bdbad27316534016f","SHA-1 multiblock vector");
 std::vector<uint8_t> rom(bc::rom_size);rom[0]=0x80;rom[1]=0x37;rom[2]=0x12;rom[3]=0x40;rom[4097]=73;
 check(bc::normalize_rom(rom)==rom,"canonical ROM unchanged");
 auto swapped=rom;for(size_t i=0;i<swapped.size();i+=2)std::swap(swapped[i],swapped[i+1]);check(bc::normalize_rom(swapped)==rom,"v64 normalized");
 swapped=rom;for(size_t i=0;i<swapped.size();i+=4)std::reverse(swapped.begin()+i,swapped.begin()+i+4);check(bc::normalize_rom(swapped)==rom,"n64 normalized");
 rejects([&]{bc::normalize_rom({0x80,0x37,0x12,0x40});},"truncated ROM rejected");
 auto extra=rom;extra.push_back(0);rejects([&]{bc::normalize_rom(extra);},"oversized ROM rejected");
 extra=rom;extra[0]=0;rejects([&]{bc::normalize_rom(extra);},"unknown header rejected");
 rejects([&]{bc::validate_rom(rom);},"synthetic/wrong revision rejected by complete hash");
 char temporary[]="/tmp/blastcorps-rom-input-XXXXXX";auto* made=mkdtemp(temporary);check(made,"temporary fixture directory");std::filesystem::path dir=made;
 try{
  write(dir/"raw.z64",rom);check(bc::read_rom_input(dir/"raw.z64")==rom,"raw file accepted");
  write(dir/"archive.bin",zip(rom));check(bc::read_rom_input(dir/"archive.bin")==rom,"ZIP magic and nested uppercase ROM accepted without extraction");
  write(dir/"ambiguous.zip",zip(rom,true));rejects([&]{bc::read_rom_input(dir/"ambiguous.zip");},"ambiguous ZIP rejected");
  write(dir/"small.zip",zip({0x80,0x37,0x12,0x40}));rejects([&]{bc::read_rom_input(dir/"small.zip");},"wrong expanded ZIP size rejected");
  write(dir/"broken.zip",{1,2,3});rejects([&]{bc::read_rom_input(dir/"broken.zip");},"invalid archive rejected");
  rejects([&]{bc::read_rom_input(dir/"missing");},"missing input reported");
  write(dir/"large",{});std::filesystem::resize_file(dir/"large",33*1024*1024);rejects([&]{bc::read_rom_input(dir/"large");},"bounded file reads");
  check(!std::filesystem::exists(dir/"folder"),"ZIP member paths never extracted");
  std::filesystem::remove_all(dir);
 }catch(...){std::filesystem::remove_all(dir);throw;}
 puts("Bounded native ROM input, byte ordering and revision validation passed");
 if(argc==4){
  const auto image=bc::load_rom_image(argv[1]);
  auto expected=[](const char* path){std::ifstream f(path,std::ios::binary);check(bool(f),"private overlay reference readable");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
  check(image.code==expected(argv[2]),"native code overlay exactly matches private preparation");
  check(image.frontend==expected(argv[3]),"native frontend overlay exactly matches private preparation");
  char cache_template[]="/tmp/blastcorps-rom-cache-XXXXXX";auto* cache_dir=mkdtemp(cache_template);check(cache_dir,"private cache directory");std::filesystem::path data=cache_dir;
  try{
   write(data/"eeprom.bin",{1,2,3,4});
   auto imported=bc::load_cached_rom(data,argv[1]);check(imported.rom==image.rom,"explicit import returns validated ROM");
   auto restarted=bc::load_cached_rom(data);check(restarted.rom==image.rom&&restarted.code==image.code,"restart regenerates overlays from validated cache");
   write(data/"invalid.z64",{0x80,0x37,0x12,0x40});rejects([&]{bc::load_cached_rom(data,data/"invalid.z64");},"invalid import rejected");
   check(bc::load_cached_rom(data).rom==image.rom,"invalid import preserves valid cache");
   check(expected((data/"eeprom.bin").c_str())==std::vector<uint8_t>({1,2,3,4}),"ROM import never writes progress");
   write(data/"cache/rom.z64",{1,2,3});rejects([&]{bc::load_cached_rom(data);},"corrupt cache never launches");
   check(bc::load_cached_rom(data,argv[1]).rom==image.rom,"explicit valid ROM repairs corrupt cache");
   if(getuid()!=0){
    write(data/"cache/rom.z64",{1,2,3});
    auto writable=std::filesystem::perms::owner_all;
    std::filesystem::permissions(data/"cache",std::filesystem::perms::owner_read|std::filesystem::perms::owner_exec);
    bool refused=false;try{bc::load_cached_rom(data,argv[1]);}catch(const std::exception&){refused=true;}
    std::filesystem::permissions(data/"cache",writable);
    check(refused&&expected((data/"cache/rom.z64").c_str())==std::vector<uint8_t>({1,2,3}),"read-only cache fails without replacing existing bytes");
   }
   std::filesystem::remove_all(data/"cache");write(data/"rom.z64",image.rom);
   check(bc::load_cached_rom(data).rom==image.rom,"legacy data-relative ROM bootstraps missing cache");
   std::filesystem::remove_all(data);
  }catch(...){std::filesystem::remove_all(data);throw;}
  std::printf("Private ROM accepted: sha1=%s code=%zu frontend=%zu\n",bc::sha1(image.rom).c_str(),image.code.size(),image.frontend.size());
 }else check(argc==1,"private acceptance needs ROM and two overlay references");
}
