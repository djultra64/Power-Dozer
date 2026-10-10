#include "pak_session.h"
#include "blast_save_format.h"
#include <chrono>
#include <filesystem>
#include <stdexcept>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 auto root=std::filesystem::temp_directory_path()/("bc-pak-session-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(root);
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::filesystem::remove_all(path);}} cleanup{root};
 bc::PakStore store(root/"pak.bin");bc::PakSession session(store.load());int index;
 check(session.allocate(bc::blast_pak_key,3584,index,store)==0&&index==0,"allocate");
 std::array<uint8_t,32> before{},after{},tail{};before[0]='B';after[0]='A';std::copy(bc::blast_save_complete.begin(),bc::blast_save_complete.end(),tail.begin());
 check(session.write(0,0,before,store)==0&&session.write(0,0xDE0,tail,store)==0,"initial complete save");
 auto begin=tail;std::copy(bc::blast_save_in_progress.begin(),bc::blast_save_in_progress.end(),begin.begin());
 check(session.write(0,0xDE0,begin,store)==0&&session.pending(0),"in-progress opens transaction");
 check(session.write(0,0,after,store)==0&&session.image().file(0)->data[0]=='A',"guest reads staged data");
 check(store.load().file(0)->data[0]=='B'&&store.load().file(0)->data[0xDE0]==0x87,"disk keeps previous complete save");
 bc::PakSession restarted(store.load());check(restarted.image().file(0)->data[0]=='B'&&!restarted.pending(0),"interrupted transaction discarded on restart");
 auto backup=root/"pak.bin.bak";std::filesystem::remove(backup);std::filesystem::create_directory(backup);
 bool failed=false;try{session.write(0,0xDE0,tail,store);}catch(const std::exception&){failed=true;}
 check(failed&&session.pending(0)&&session.image().file(0)->data[0]=='A',"failed commit keeps staged data for retry");
 check(store.load().file(0)->data[0]=='B',"failed commit keeps previous disk contents");std::filesystem::remove(backup);
 check(session.write(0,0xDE0,tail,store)==0&&!session.pending(0)&&store.load().file(0)->data[0]=='A',"retry atomically commits whole save");
 auto old=bc::PakStore(backup).load();check(old.file(0)->data[0]=='B'&&old.file(0)->data[0xDE0]==0x87,"backup is previous complete version");
 auto key=bc::blast_pak_key;key.extension[0]=0x12;check(session.allocate(key,3584,index,store)==0&&index==1,"second player");
 check(session.write(1,0,before,store)==0&&session.write(1,0xDE0,tail,store)==0,"second complete save");
 session.write(0,0xDE0,begin,store);after[0]='C';session.write(0,0,after,store);
 after[0]='D';session.write(1,0,after,store);
 check(store.load().file(0)->data[0]=='A'&&store.load().file(1)->data[0]=='D',"unrelated write never flushes incomplete player");
 session.write(1,0xDE0,begin,store);after[0]='E';session.write(1,0,after,store);session.write(0,0xDE0,tail,store);
 check(store.load().file(0)->data[0]=='C'&&store.load().file(1)->data[0]=='D'&&session.pending(1),"independent concurrent player transactions");
 check(session.remove(key,store)==0&&!session.pending(1)&&!store.load().file(1),"delete cancels only that player's transaction");
 check(session.write(16,0,before,store)==5&&session.write(0,1,before,store)==5,"invalid writes do not mutate or commit");
 // Older prototypes persisted the ROM's in-progress marker on every write.
 // Recover their last complete backup rather than let the guest reset a slot.
 auto unfinished=store.load();unfinished.write(0,0xDE0,begin);store.save(unfinished);
 bc::PakStore checked(root/"pak.bin",bc::blast_pak_committed);
 auto recovered=checked.load();check(recovered.file(0)->data[0]=='C'&&recovered.file(0)->data[0xDE0]==0x87,"older interrupted save recovers complete backup");
 checked.save(recovered);check(bc::PakStore(backup).load().file(0)->data[0xDE0]==0x87,"recovery never replaces backup with interrupted primary");
 bool refused=false;try{checked.save(unfinished);}catch(const std::exception&){refused=true;}check(refused,"durable store refuses explicit incomplete game image");
 store.save(unfinished);std::filesystem::remove(backup);refused=false;try{checked.load();}catch(const std::exception&){refused=true;}
 check(refused&&store.load().file(0)->data[0xDE0]==0x27,"unfinished older primary without valid backup is preserved and refused");
}
