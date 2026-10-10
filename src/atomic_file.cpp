#include "atomic_file.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
namespace bc {
void atomic_write_file(const std::filesystem::path& path,std::span<const uint8_t> image){
 auto directory=path.parent_path();if(directory.empty())directory=".";
 std::filesystem::create_directories(directory);
 std::string pattern=path.string()+".tmp.XXXXXX";
 std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
 int fd=mkstemp(name.data());if(fd<0)throw std::runtime_error("Cannot create temporary file: "+std::string(strerror(errno)));
 try{
  size_t count=0;
  while(count<image.size()){
   auto n=write(fd,image.data()+count,image.size()-count);
   if(n<0&&errno==EINTR)continue;
   if(n<=0)throw std::runtime_error("Cannot write complete file");count+=size_t(n);
  }
  if(fsync(fd)!=0)throw std::runtime_error("Cannot synchronize file");
  const int closed=close(fd);fd=-1;if(closed!=0)throw std::runtime_error("Cannot close file");
  std::filesystem::rename(name.data(),path);
  int directory_fd=open(directory.c_str(),O_RDONLY|O_DIRECTORY);
  if(directory_fd<0)throw std::runtime_error("Cannot open directory for synchronization");
  const bool synced=fsync(directory_fd)==0;const int directory_closed=close(directory_fd);
  if(!synced||directory_closed!=0)throw std::runtime_error("Cannot synchronize directory");
 }catch(...){if(fd>=0)close(fd);std::error_code ec;std::filesystem::remove(name.data(),ec);throw;}
}
}
