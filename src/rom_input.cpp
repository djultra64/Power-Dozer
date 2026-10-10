#include "rom_input.h"
#include "atomic_file.h"
#include "miniz.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <fstream>
#include <cstdio>
#include <stdexcept>
namespace bc {
std::string sha1(std::span<const uint8_t> input){
 std::array<uint32_t,5> state={0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476,0xC3D2E1F0};
 const uint64_t bits=uint64_t(input.size())*8;
 const size_t blocks=input.size()/64+((input.size()%64)>=56?2:1);
 for(size_t block=0;block<blocks;++block){
  std::array<uint32_t,80> words{};
  for(size_t i=0;i<64;++i){
   const size_t at=block*64+i;uint8_t value=0;
   if(at<input.size())value=input[at];else if(at==input.size())value=0x80;
   else if(at>=blocks*64-8)value=uint8_t(bits>>((blocks*64-1-at)*8));
   words[i/4]|=uint32_t(value)<<((3-i%4)*8);
  }
  for(size_t i=16;i<80;++i)words[i]=std::rotl(words[i-3]^words[i-8]^words[i-14]^words[i-16],1);
  auto a=state[0],b=state[1],c=state[2],d=state[3],e=state[4];
  for(size_t i=0;i<80;++i){
   uint32_t f,k;
   if(i<20){f=(b&c)|(~b&d);k=0x5A827999;}
   else if(i<40){f=b^c^d;k=0x6ED9EBA1;}
   else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}
   else{f=b^c^d;k=0xCA62C1D6;}
   auto next=std::rotl(a,5)+f+e+k+words[i];e=d;d=c;c=std::rotl(b,30);b=a;a=next;
  }
  state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;state[4]+=e;
 }
 char result[41];for(size_t i=0;i<5;++i)std::snprintf(result+i*8,9,"%08x",state[i]);return result;
}
std::vector<uint8_t> normalize_rom(std::vector<uint8_t> bytes){
 if(bytes.size()!=rom_size)throw std::runtime_error("ROM must be exactly 8 MiB");
 std::array<uint8_t,4> header{};std::copy_n(bytes.begin(),4,header.begin());
 if(header==std::array<uint8_t,4>{0x37,0x80,0x40,0x12})for(size_t i=0;i<bytes.size();i+=2)std::swap(bytes[i],bytes[i+1]);
 else if(header==std::array<uint8_t,4>{0x40,0x12,0x37,0x80})for(size_t i=0;i<bytes.size();i+=4)std::reverse(bytes.begin()+i,bytes.begin()+i+4);
 else if(header!=std::array<uint8_t,4>{0x80,0x37,0x12,0x40})throw std::runtime_error("Unknown N64 ROM byte order");
 return bytes;
}
namespace {
std::string extension(const std::string& path){auto ext=std::filesystem::path(path).extension().string();for(auto& c:ext)c=char(std::tolower(static_cast<unsigned char>(c)));return ext;}
struct ZipReader {mz_zip_archive archive{};~ZipReader(){mz_zip_reader_end(&archive);}};
std::vector<uint8_t> read_zip(const std::vector<uint8_t>& bytes){
 ZipReader reader;auto* archive=&reader.archive;
 if(!mz_zip_reader_init_mem(archive,bytes.data(),bytes.size(),0))throw std::runtime_error("Invalid ZIP archive");
 const auto count=mz_zip_reader_get_num_files(archive);
 if(count>128)throw std::runtime_error("ZIP has too many entries (maximum 128)");
 int selected=-1;
 for(mz_uint i=0;i<count;++i){
  mz_zip_archive_file_stat info{};if(!mz_zip_reader_file_stat(archive,i,&info))throw std::runtime_error("Invalid ZIP directory");
  if(info.m_is_directory)continue;
  auto ext=extension(info.m_filename);if(ext!=".z64"&&ext!=".v64"&&ext!=".n64")continue;
  if(selected>=0)throw std::runtime_error("ZIP must contain exactly one N64 ROM");
  if(info.m_is_encrypted||!info.m_is_supported)throw std::runtime_error("Encrypted or unsupported ZIP ROM");
  if(info.m_uncomp_size!=rom_size)throw std::runtime_error("ZIP ROM must expand to exactly 8 MiB");
  selected=int(i);
 }
 if(selected<0)throw std::runtime_error("ZIP contains no N64 ROM");
 std::vector<uint8_t> rom(rom_size);
 if(!mz_zip_reader_extract_to_mem(archive,mz_uint(selected),rom.data(),rom.size(),0))throw std::runtime_error("ZIP ROM extraction or checksum failed");
 return normalize_rom(std::move(rom));
}
uint32_t little32(std::span<const uint8_t> bytes){return uint32_t(bytes[0])|(uint32_t(bytes[1])<<8)|(uint32_t(bytes[2])<<16)|(uint32_t(bytes[3])<<24);}
std::vector<uint8_t> overlay(std::span<const uint8_t> member,size_t size){
 // This helper only receives slices from an already SHA-1-validated ROM.
 if(member.size()<18||member[0]!=0x1F||member[1]!=0x8B||member[2]!=8||(member[3]&0xE0))throw std::runtime_error("Invalid executable overlay header");
 size_t start=10,end=member.size()-8;
 if(member[3]&4){if(start+2>end)throw std::runtime_error("Truncated overlay extra header");size_t length=member[start]|(size_t(member[start+1])<<8);start+=2;if(length>end-start)throw std::runtime_error("Invalid overlay extra header");start+=length;}
 for(auto flag:{8,16})if(member[3]&flag){while(start<end&&member[start])++start;if(start==end)throw std::runtime_error("Truncated overlay name/comment");++start;}
 if(member[3]&2){if(start+2>end)throw std::runtime_error("Truncated overlay header checksum");start+=2;}
 if(start>=end||little32(member.subspan(end+4,4))!=size)throw std::runtime_error("Executable overlay size mismatch");
 std::vector<uint8_t> result(size);
 if(tinfl_decompress_mem_to_mem(result.data(),result.size(),member.data()+start,end-start,0)!=size||uint32_t(mz_crc32(0,result.data(),result.size()))!=little32(member.subspan(end,4)))throw std::runtime_error("Executable overlay inflate/checksum failed");
 return result;
}
}
std::vector<uint8_t> read_rom_input(const std::filesystem::path& path){
 std::error_code error;auto size=std::filesystem::file_size(path,error);
 if(error)throw std::runtime_error("Cannot read ROM: "+path.string());
 if(size>32*1024*1024)throw std::runtime_error("ROM input exceeds 32 MiB limit");
 std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot open ROM: "+path.string());
 std::vector<uint8_t> bytes(size);input.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size));
 if(input.gcount()!=std::streamsize(size))throw std::runtime_error("Incomplete ROM read");
 if(extension(path.string())==".zip"||(bytes.size()>=4&&bytes[0]=='P'&&bytes[1]=='K'))return read_zip(bytes);
 return normalize_rom(std::move(bytes));
}
void validate_rom(std::span<const uint8_t> bytes){
 if(bytes.size()!=rom_size||sha1(bytes)!="483f7161aea39de8b45c9fbc70a2c3883c4dea8c")throw std::runtime_error("Unmodified Blast Corps USA Rev 1 ROM required");
}
RomImage load_rom_image(const std::filesystem::path& path){
 RomImage image;image.rom=read_rom_input(path);validate_rom(image.rom);std::span<const uint8_t> rom=image.rom;
 auto text=overlay(rom.subspan(0x787FD0,0x7D73B4-0x787FD0),0xA4410);
 auto data=overlay(rom.subspan(0x7D73B4,0xC713),0x26A90);text.insert(text.end(),data.begin(),data.end());image.code=std::move(text);
 text=overlay(rom.subspan(0x7E3AD0,0x7F63A1-0x7E3AD0),0x21040);
 data=overlay(rom.subspan(0x7F63A1,0x383C),0x8E50);text.insert(text.end(),data.begin(),data.end());image.frontend=std::move(text);return image;
}
RomImage load_cached_rom(const std::filesystem::path& data,const std::filesystem::path& source){
 const auto cache=data/"cache/rom.z64";
 if(source.empty()&&std::filesystem::exists(cache))return load_rom_image(cache);
 const auto input=source.empty()?data/"rom.z64":source;
 if(!std::filesystem::exists(input))throw std::runtime_error("No ROM installed. Select your Blast Corps USA Rev 1 ROM with --rom PATH");
 auto image=load_rom_image(input);
 // Validate and extract completely before replacing any private cache.
 // Existing progress/settings are never part of this transaction.
 bool matches=false;
 if(std::filesystem::exists(cache))try{matches=read_rom_input(cache)==image.rom;}catch(const std::exception&){}
 if(!matches)atomic_write_file(cache,image.rom);
 return image;
}
}
