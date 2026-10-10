#include "pak_store.h"
#include "atomic_file.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
namespace bc {
namespace {
constexpr size_t capacity=123*256,limit=16+16*32+capacity;
bool range(const PakFile* file,int offset,size_t size){return file&&offset>=0&&(offset%32)==0&&size>0&&(size%32)==0&&size<=file->data.size()&&size_t(offset)<=file->data.size()-size;}
uint32_t crc(std::span<const uint8_t> bytes){uint32_t v=~0u;for(auto b:bytes){v^=b;for(unsigned i=0;i<8;i++)v=(v>>1)^(0xEDB88320u&uint32_t(-int32_t(v&1)));}return ~v;}
void put(std::vector<uint8_t>& out,uint32_t value,unsigned count){for(unsigned i=0;i<count;i++)out.push_back(uint8_t(value>>(8*i)));}
struct Reader {
 std::span<const uint8_t> bytes;size_t pos=0;
 uint32_t get(unsigned count){if(count>bytes.size()-pos)throw std::runtime_error("Truncated Pak");uint32_t v=0;for(unsigned i=0;i<count;i++)v|=uint32_t(bytes[pos++])<<(8*i);return v;}
 template<size_t N>void array(std::array<uint8_t,N>& out){for(auto& b:out)b=get(1);}
};
}
const PakFile* PakImage::file(int index) const{return index>=0&&index<16&&files_[index]?&*files_[index]:nullptr;}
int PakImage::free_bytes() const{int bytes=capacity;for(const auto& f:files_)if(f)bytes-=f->data.size();return bytes;}
int PakImage::find(const PakKey& key,int& index) const{index=-1;for(int i=0;i<16;i++)if(files_[i]&&files_[i]->key==key){index=i;return 0;}return 5;}
int PakImage::allocate(const PakKey& key,int bytes,int& index){
 index=-1;if(!key.company||!key.game||bytes<=0)return 5;
 if(find(key,index)==0)return 9;
 if(bytes>int(capacity))return 7;
 const auto size=(bytes+255)/256*256;
 if(size>free_bytes())return 7;
 for(int i=0;i<16;i++)if(!files_[i]){files_[i]=PakFile{key,false,std::vector<uint8_t>(size)};index=i;return 0;}
 return 8;
}
int PakImage::remove(const PakKey& key){int index;if(find(key,index)!=0)return 5;files_[index].reset();return 0;}
int PakImage::read(int index,int offset,std::span<uint8_t> bytes) const{
 auto* f=file(index);if(!range(f,offset,bytes.size()))return 5;if(!f->occupied)return 6;
 std::copy_n(f->data.begin()+offset,bytes.size(),bytes.begin());return 0;
}
int PakImage::write(int index,int offset,std::span<const uint8_t> bytes){
 if(!range(file(index),offset,bytes.size()))return 5;
 auto& f=*files_[index];std::copy(bytes.begin(),bytes.end(),f.data.begin()+offset);f.occupied=true;return 0;
}
std::vector<uint8_t> PakStore::encode(const PakImage& image){
 std::vector<uint8_t> payload;
 for(auto& file:image.files_){
  payload.push_back(file?1:0);if(!file)continue;
  put(payload,file->key.company,2);put(payload,file->key.game,4);
  payload.insert(payload.end(),file->key.name.begin(),file->key.name.end());
  payload.insert(payload.end(),file->key.extension.begin(),file->key.extension.end());
  payload.push_back(file->occupied?1:0);put(payload,file->data.size(),4);
  payload.insert(payload.end(),file->data.begin(),file->data.end());
 }
 std::vector<uint8_t> out={'B','C','P','K'};put(out,1,4);put(out,payload.size(),4);put(out,crc(payload),4);out.insert(out.end(),payload.begin(),payload.end());return out;
}
PakImage PakStore::decode(std::span<const uint8_t> bytes){
 if(bytes.size()<16||bytes.size()>limit||bytes[0]!='B'||bytes[1]!='C'||bytes[2]!='P'||bytes[3]!='K')throw std::runtime_error("Invalid Pak header");
 Reader r{bytes};r.pos=4;
 if(r.get(4)!=1||r.get(4)!=bytes.size()-16)throw std::runtime_error("Invalid Pak version or size");
 if(r.get(4)!=crc(bytes.subspan(16)))throw std::runtime_error("Invalid Pak checksum");
 PakImage image;size_t used=0;
 for(size_t i=0;i<16;i++){
  auto present=r.get(1);if(present>1)throw std::runtime_error("Invalid Pak slot");if(!present)continue;
  PakFile file;file.key.company=r.get(2);file.key.game=r.get(4);r.array(file.key.name);r.array(file.key.extension);
  auto occupied=r.get(1);auto size=r.get(4);int duplicate;
  if(occupied>1||!file.key.company||!file.key.game||size==0||size%256||size>capacity-used||size>bytes.size()-r.pos||image.find(file.key,duplicate)==0)throw std::runtime_error("Invalid Pak directory");
  file.occupied=occupied;file.data.assign(bytes.begin()+r.pos,bytes.begin()+r.pos+size);r.pos+=size;used+=size;image.files_[i]=std::move(file);
 }
 if(r.pos!=bytes.size())throw std::runtime_error("Unexpected Pak trailing bytes");return image;
}
bool PakStore::read_valid(const std::filesystem::path& path,PakImage& out) const{
 std::ifstream file(path,std::ios::binary);if(!file)return false;
 std::vector<uint8_t> bytes(limit+1);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(file.bad())return false;bytes.resize(file.gcount());
 try{out=decode(bytes);return !accept_||accept_(out);}catch(const std::runtime_error&){return false;}
}
PakImage PakStore::load() const{
 PakImage image;if(read_valid(path_,image)||read_valid(path_.string()+".bak",image))return image;
 if(std::filesystem::exists(path_)||std::filesystem::exists(path_.string()+".bak"))throw std::runtime_error("Controller Pak and backup are invalid; restore a valid Pak save");return {};
}
void PakStore::save(const PakImage& image) const{
 if(accept_&&!accept_(image))throw std::runtime_error("Refusing to persist an unfinished Controller Pak save");
 auto previous=load();auto directory=path_.parent_path();if(directory.empty())directory=".";std::filesystem::create_directories(directory);
 if(std::filesystem::exists(path_)||std::filesystem::exists(path_.string()+".bak"))atomic_write_file(path_.string()+".bak",encode(previous));
 atomic_write_file(path_,encode(image));
}
}
