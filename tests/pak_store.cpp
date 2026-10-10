#include "pak_store.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <climits>
using namespace bc;
static void check(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
static PakKey key(unsigned n=0){PakKey value;value.company=0x3031;value.game=0x4E424345;value.name[0]=0x0B;value.extension[0]=uint8_t(n);return value;}
int main(){
 auto root=std::filesystem::temp_directory_path()/("bc-pak-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::filesystem::remove_all(path);}} cleanup{root};
 PakStore store(root/"pak.bin");auto pak=store.load();
 check(pak.free_bytes()==123*256,"empty Pak capacity");int index=99;
 check(pak.find(key(),index)==5&&index==-1,"missing file");
 check(pak.allocate(key(),3584,index)==0&&index==0,"allocate Blast Corps file");
 check(pak.free_bytes()==123*256-3584,"allocated pages counted");
 int existing=-1;check(pak.allocate(key(),32,existing)==9&&existing==0,"duplicate file");
 std::array<uint8_t,32> block{};block.fill(0x5A);std::array<uint8_t,32> output{};
 check(pak.read(index,0,output)==6,"unwritten file is Bad Data");
 check(pak.write(index,3520,block)==0,"partial last-page write");
 check(pak.read(index,3520,output)==0&&output==block,"readback exact bytes");
 check(pak.read(index,0,output)==0&&std::all_of(output.begin(),output.end(),[](auto b){return b==0;}),"unwritten remaining bytes zero");
 check(pak.write(index,1,block)==5&&pak.write(index,3584,block)==5,"misaligned and oversized access rejected");
 check(pak.read(-1,0,output)==5&&pak.read(16,0,output)==5,"invalid directory index");
 std::span<uint8_t> empty;check(pak.read(index,0,empty)==5,"zero size rejected");
 const auto* file=pak.file(index);check(file&&file->key==key()&&file->data.size()==3584,"metadata and rounded size");
 store.save(pak);auto reload=store.load();check(reload.read(index,3520,output)==0&&output==block,"persisted file content and occupied status");
 check(reload.remove(key())==0&&reload.free_bytes()==123*256,"delete releases pages");
 check(reload.remove(key())==5,"missing delete");
 check(reload.allocate(key(1),1,index)==0&&reload.file(index)->data.size()==256,"size rounds to page");
 for(unsigned i=2;i<=16;i++)check(reload.allocate(key(i),256,index)==0,"fill directory");
 check(reload.allocate(key(17),256,index)==8,"directory full");
 PakImage full;check(full.allocate(key(),123*256,index)==0,"fill all data pages");
 check(full.allocate(key(1),1,index)==7,"data full");
 check(full.allocate(key(2),INT_MAX,index)==7,"allocation overflow rejected");
 check(full.allocate(key(),0,index)==5&&full.allocate(key(),-1,index)==5,"invalid allocation size");
 auto invalid=key();invalid.company=0;check(full.allocate(invalid,256,index)==5,"invalid company");
 store.save(reload);
 {std::ofstream file(root/"pak.bin",std::ios::binary|std::ios::trunc);file<<"damaged";}
 auto recovered=store.load();check(recovered.find(key(),index)==0,"damaged primary recovers previous backup");
 check(recovered.read(index,3520,output)==0&&output==block,"backup preserves progress");
 {std::ofstream file(root/"pak.bin.bak",std::ios::binary|std::ios::trunc);file<<"damaged";}
 bool refused=false;try{store.load();}catch(const std::exception&){refused=true;}check(refused,"two invalid copies must not reset Pak");
 refused=false;try{store.save(pak);}catch(const std::exception&){refused=true;}check(refused,"save must not replace two invalid copies");
 PakStore isolated(root/"other/pak.bin");check(isolated.load().free_bytes()==123*256,"isolated data directory");
 {std::ofstream file(root/"other/pak.bin.tmp",std::ios::binary);file<<"incomplete";}
 check(isolated.load().free_bytes()==123*256,"interrupted temp ignored");
 isolated.save(pak);check(isolated.load().find(key(),index)==0,"isolated persistent Pak");
 auto path=root/"other/pak.bin";
 std::ifstream input(path,std::ios::binary);std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(input),{}};input.close();
 bytes.back()^=1;
 {std::ofstream damaged(path,std::ios::binary|std::ios::trunc);damaged.write(reinterpret_cast<char*>(bytes.data()),bytes.size());}
 refused=false;try{isolated.load();}catch(const std::exception&){refused=true;}check(refused,"checksum damage rejected without reset");
 {std::ofstream damaged(path,std::ios::binary|std::ios::trunc);std::vector<char> huge(40000);damaged.write(huge.data(),huge.size());}
 refused=false;try{isolated.load();}catch(const std::exception&){refused=true;}check(refused,"oversized input rejected");
}
