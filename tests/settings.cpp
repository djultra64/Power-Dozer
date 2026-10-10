#include "settings.h"
#include <cstdlib>
static void check(bool valid){if(!valid)std::abort();}
#include <cmath>
#include <fstream>
#include <limits>
#include <unistd.h>

static void put(const std::filesystem::path& path,const char* value){std::ofstream(path)<<value;}
template<class F>void rejects(F action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected);}
int main(){
 char directory[]="/tmp/blastcorps-settings-XXXXXX";check(mkdtemp(directory));
 auto path=std::filesystem::path(directory)/"settings.json";bc::SettingsStore store(path);
 auto defaults=store.load();check(defaults.scale==3&&defaults.wide&&defaults.refresh==120);
 auto changed=defaults;changed.scale=9;changed.msaa=4;changed.volume=35;changed.family="switch";store.save(changed);
 auto loaded=store.load();check(loaded.scale==9&&loaded.msaa==4&&loaded.volume==35&&loaded.family=="switch");
 put(path,"{\"schema\":1,\"scale\":4.5,\"future\":{\"keep\":true}}");
 loaded=store.load();check(loaded.scale==4.5&&loaded.wide);loaded.fullscreen=true;store.save(loaded);
 std::ifstream input(path);std::string text{std::istreambuf_iterator<char>(input),{}};check(text.find("keep")!=std::string::npos);
 put(path,"{broken");loaded=store.load();check(loaded.scale==4.5);store.save(loaded);
 put(path,"{broken");put(path.string()+".bak","{broken");rejects([&]{store.load();});rejects([&]{store.save(defaults);});
 std::filesystem::remove(path.string()+".bak");
 for(auto invalid:{"{\"schema\":2}","{\"scale\":\"9\"}","{\"scale\":0}","{\"scale\":100}","{\"volume\":101}","{\"volume\":-1}","{\"msaa\":3}","{\"refresh\":-120}","{\"refresh\":17}","{\"wide\":1}","{\"texture\":\"broken\"}","{\"family\":\"broken\"}"}){
  put(path,invalid);rejects([&]{store.load();});
 }
 changed=defaults;changed.scale=std::numeric_limits<double>::quiet_NaN();rejects([&]{bc::validate(changed);});
 changed.scale=std::numeric_limits<double>::infinity();rejects([&]{bc::validate(changed);});
 for(auto invalid:{"{\"volume\":4294967371}","{\"refresh\":4294967416}","{\"volume\":18446744073709551615}","{\"msaa\":-4294967296}"}){
  put(path,invalid);rejects([&]{store.load();});
 }
 for(int msaa:{0,2,4,8})for(int rate:{0,30,60,120})for(const char* filter:{"nearest","linear","pixel"}){
  changed=defaults;changed.msaa=msaa;changed.refresh=rate;changed.presentation=filter;bc::validate(changed);
 }
 std::filesystem::remove_all(directory);
}
