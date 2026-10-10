#include "pak_session.h"
#include "blast_save_format.h"
#include <algorithm>
namespace bc {
namespace {
bool original_save(const PakFile* file){
 if(!file||file->data.size()!=3584)return false;
 auto key=file->key;auto extension=key.extension[0];key.extension[0]=0x11;
 return extension>=0x11&&extension<=0x14&&key==blast_pak_key;
}
}
bool blast_pak_committed(const PakImage& image){
 for(int i=0;i<16;i++){
  auto file=image.file(i);
  if(original_save(file)&&file->occupied&&std::equal(blast_save_in_progress.begin(),blast_save_in_progress.end(),file->data.begin()+0xDE0))return false;
 }
 return true;
}
void PakSession::persist_file(const PakImage& candidate,int index,const PakStore& store){
 // Other players can still have staged writes. Only replace this directory
 // slot in the last committed image; never persist their unfinished data.
 auto durable=committed_;durable.files_[index]=candidate.files_[index];
 store.save(durable); // Failure leaves both working and committed state intact.
 committed_=std::move(durable);working_=candidate;
}
int PakSession::allocate(const PakKey& key,int bytes,int& index,const PakStore& store){
 auto candidate=working_;auto result=candidate.allocate(key,bytes,index);
 if(result==0)persist_file(candidate,index,store);return result;
}
int PakSession::remove(const PakKey& key,const PakStore& store){
 int index;auto result=working_.find(key,index);if(result)return result;
 auto candidate=working_;result=candidate.remove(key);if(result)return result;
 persist_file(candidate,index,store);pending_[index]=false;return 0;
}
int PakSession::write(int index,int offset,std::span<const uint8_t> bytes,const PakStore& store){
 auto candidate=working_;auto result=candidate.write(index,offset,bytes);if(result)return result;
 bool begin=false,end=false;
 if(original_save(candidate.file(index))&&offset<=0xDE0&&bytes.size()>=size_t(0xDE8-offset)){
  auto marker=candidate.file(index)->data.begin()+0xDE0;
  begin=std::equal(blast_save_in_progress.begin(),blast_save_in_progress.end(),marker);
  end=std::equal(blast_save_complete.begin(),blast_save_complete.end(),marker);
 }
 if(begin){working_=std::move(candidate);pending_[index]=true;return 0;}
 if(pending_[index]&&!end){working_=std::move(candidate);return 0;}
 persist_file(candidate,index,store);pending_[index]=false;return 0;
}
}
