#include "atomic_file.h"
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <atomic>
#include <chrono>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
namespace bc {
void atomic_write_file(const std::filesystem::path& path,std::span<const uint8_t> image){
 auto directory=path.parent_path();if(directory.empty())directory=".";
 std::filesystem::create_directories(directory);
#ifdef _WIN32
 static std::atomic<uint64_t> sequence{};
 std::filesystem::path temporary;
 HANDLE file=INVALID_HANDLE_VALUE;
 for(unsigned attempt=0;attempt<128;++attempt){
  temporary=path;temporary+=L".tmp."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count())+L"."+std::to_wstring(sequence.fetch_add(1));
  file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,nullptr);
  if(file!=INVALID_HANDLE_VALUE)break;
  if(GetLastError()!=ERROR_FILE_EXISTS&&GetLastError()!=ERROR_ALREADY_EXISTS)throw std::runtime_error("Cannot create temporary file");
 }
 if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot reserve temporary file");
 try{
  size_t count=0;
  while(count<image.size()){
   DWORD written=0;const DWORD chunk=DWORD(std::min<size_t>(image.size()-count,0x7FFFFFFFu));
   if(!WriteFile(file,image.data()+count,chunk,&written,nullptr)||written==0)throw std::runtime_error("Cannot write complete file");
   count+=written;
  }
  if(!FlushFileBuffers(file))throw std::runtime_error("Cannot synchronize file");
  const bool closed=CloseHandle(file);file=INVALID_HANDLE_VALUE;
  if(!closed)throw std::runtime_error("Cannot close file");
  if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot commit complete file");
 }catch(...){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);std::error_code ec;std::filesystem::remove(temporary,ec);throw;}
#else
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
#endif
}
}
