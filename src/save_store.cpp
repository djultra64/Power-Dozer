#include "save_store.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

namespace bc {
namespace {
namespace fs=std::filesystem;
using CheckedImage=std::array<uint8_t,524>;
constexpr std::array<uint8_t,4> magic={'B','C','E','P'};
uint32_t get32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
void put32(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=uint8_t(n>>(8*i));}
uint32_t crc(const SaveStore::Image& image){
 uint32_t value=~0u;
 for(auto byte:image){value^=byte;for(unsigned bit=0;bit<8;bit++)value=(value>>1)^(0xEDB88320u&uint32_t(-int32_t(value&1)));}
 return ~value;
}
bool read_valid(const fs::path& path,SaveStore::Image& out){
 std::ifstream file(path,std::ios::binary);if(!file)return false;
 std::array<uint8_t,525> bytes{};
 file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
 const auto size=file.gcount();
 if(file.bad())return false;
 const bool checked=std::equal(magic.begin(),magic.end(),bytes.begin());
 if(size==512&&!checked){std::copy_n(bytes.begin(),512,out.begin());return true;}
 if(size!=524||!std::equal(magic.begin(),magic.end(),bytes.begin())||get32(bytes.data()+4)!=1)return false;
 SaveStore::Image candidate{};std::copy_n(bytes.begin()+12,512,candidate.begin());
 if(get32(bytes.data()+8)!=crc(candidate))return false;
 out=candidate;return true;
}
CheckedImage encode(const SaveStore::Image& image){
 CheckedImage bytes{};std::copy(magic.begin(),magic.end(),bytes.begin());
 put32(bytes.data()+4,1);put32(bytes.data()+8,crc(image));
 std::copy(image.begin(),image.end(),bytes.begin()+12);return bytes;
}
void sync_directory(const fs::path& directory){
 int fd=open(directory.c_str(),O_RDONLY|O_DIRECTORY);
 if(fd<0)throw std::runtime_error("Cannot open save directory for synchronization");
 const bool ok=fsync(fd)==0;const int closed=close(fd);
 if(!ok||closed!=0)throw std::runtime_error("Cannot synchronize save directory");
}
void atomic_write(const fs::path& path,const CheckedImage& image){
 auto directory=path.parent_path();if(directory.empty())directory=".";
 std::string pattern=path.string()+".tmp.XXXXXX";
 std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
 int fd=mkstemp(name.data());
 if(fd<0)throw std::runtime_error("Cannot create temporary save: "+std::string(strerror(errno)));
 try{
  size_t count=0;
  while(count<image.size()){
   auto n=write(fd,image.data()+count,image.size()-count);
   if(n<0&&errno==EINTR)continue;
   if(n<=0)throw std::runtime_error("Cannot write complete save");
   count+=size_t(n);
  }
  if(fsync(fd)!=0)throw std::runtime_error("Cannot synchronize save");
  const int closed=close(fd);fd=-1;
  if(closed!=0)throw std::runtime_error("Cannot close save");
  fs::rename(name.data(),path);
  sync_directory(directory);
 }catch(...){if(fd>=0)close(fd);std::error_code ec;fs::remove(name.data(),ec);throw;}
}
}
SaveStore::Image SaveStore::load() const {
 Image image{};
 if(read_valid(path_,image)||read_valid(path_.string()+".bak",image))return image;
 if(fs::exists(path_)||fs::exists(path_.string()+".bak"))throw std::runtime_error("Progress and backup are invalid; restore a valid EEPROM save");
 return {};
}
void SaveStore::save(const Image& image) const {
 const auto previous=load(); // Validate recovery before touching either copy.
 auto directory=path_.parent_path();if(directory.empty())directory=".";
 fs::create_directories(directory);
 if(fs::exists(path_)||fs::exists(path_.string()+".bak"))atomic_write(path_.string()+".bak",encode(previous));
 atomic_write(path_,encode(image));
}
}
