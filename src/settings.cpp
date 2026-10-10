#include "settings.h"
#include "atomic_file.h"
#include <json.hpp>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <limits>
namespace bc {
Settings settings;
void validate(const Settings& s){
 if(!std::isfinite(s.scale)||s.scale<1||s.scale>9)throw std::invalid_argument("Scale must be finite, between 1 and 9");
 if(s.refresh!=0&&s.refresh!=30&&s.refresh!=60&&s.refresh!=120)throw std::invalid_argument("Refresh must be original (0), 30, 60 or 120");
 if(s.msaa!=0&&s.msaa!=2&&s.msaa!=4&&s.msaa!=8)throw std::invalid_argument("MSAA must be 0, 2, 4 or 8");
 if(s.volume<0||s.volume>100)throw std::invalid_argument("Volume must be between 0 and 100");
 if(s.presentation!="nearest"&&s.presentation!="linear"&&s.presentation!="pixel")throw std::invalid_argument("Unknown presentation filter");
 if(s.texture!="linear"&&s.texture!="three_point")throw std::invalid_argument("Unknown texture filter");
 if(s.family!="auto"&&s.family!="xbox"&&s.family!="playstation"&&s.family!="switch")throw std::invalid_argument("Unknown controller family");
}
namespace {
using Json=nlohmann::json;
Settings decode(const Json& value){
 if(!value.is_object())throw std::invalid_argument("Settings must be an object");
 Settings s;
 auto integer=[&](const char* key,int& out){if(value.contains(key)){
  auto& entry=value[key];if(!entry.is_number_integer())throw std::invalid_argument(std::string(key)+" must be an integer");
  if(entry.is_number_unsigned()){
   auto number=entry.get<uint64_t>();if(number>uint64_t(std::numeric_limits<int>::max()))throw std::invalid_argument("Integer setting is out of range");out=int(number);
  }else{
   auto number=entry.get<int64_t>();if(number<std::numeric_limits<int>::min()||number>std::numeric_limits<int>::max())throw std::invalid_argument("Integer setting is out of range");out=int(number);
  }
 }};
 int schema=1;integer("schema",schema);if(schema!=1)throw std::invalid_argument("Unsupported settings version");
 if(value.contains("scale")){if(!value["scale"].is_number())throw std::invalid_argument("Scale must be a number");s.scale=value["scale"].get<double>();}
 integer("refresh",s.refresh);integer("msaa",s.msaa);integer("volume",s.volume);
 for(auto field:{std::pair{"wide",&s.wide},{"fullscreen",&s.fullscreen},{"mute",&s.mute}})if(value.contains(field.first)){if(!value[field.first].is_boolean())throw std::invalid_argument("Setting must be boolean");*field.second=value[field.first].get<bool>();}
 for(auto field:{std::pair{"presentation",&s.presentation},{"texture",&s.texture},{"family",&s.family}})if(value.contains(field.first)){if(!value[field.first].is_string())throw std::invalid_argument("Setting must be text");*field.second=value[field.first].get<std::string>();}
 validate(s);return s;
}
bool read_valid(const std::filesystem::path& path,Json& out){
 std::ifstream file(path,std::ios::binary);if(!file)return false;
 std::vector<char> bytes(65537);file.read(bytes.data(),bytes.size());auto size=file.gcount();
 if(file.bad()||size>65536)return false;
 try{auto value=Json::parse(bytes.data(),bytes.data()+size);decode(value);out=std::move(value);return true;}catch(const std::exception&){return false;}
}
Json recover(const std::filesystem::path& path){
 Json value;
 if(read_valid(path,value)||read_valid(path.string()+".bak",value))return value;
 if(std::filesystem::exists(path)||std::filesystem::exists(path.string()+".bak"))throw std::runtime_error("Settings and backup are invalid; restore a valid settings.json");
 return Json::object();
}
void write(const std::filesystem::path& path,const Json& value){auto text=value.dump(2)+'\n';if(text.size()>65536)throw std::runtime_error("Settings exceed the 64 KiB limit");atomic_write_file(path,{reinterpret_cast<const uint8_t*>(text.data()),text.size()});}
}
Settings SettingsStore::load() const {return decode(recover(path_));}
void SettingsStore::save(const Settings& s) const {
 validate(s);auto previous=recover(path_);auto value=previous;
 value["schema"]=1;value["scale"]=s.scale;value["wide"]=s.wide;value["fullscreen"]=s.fullscreen;value["mute"]=s.mute;
 value["refresh"]=s.refresh;value["msaa"]=s.msaa;value["volume"]=s.volume;value["presentation"]=s.presentation;value["texture"]=s.texture;value["family"]=s.family;
 if(std::filesystem::exists(path_)||std::filesystem::exists(path_.string()+".bak"))write(path_.string()+".bak",previous);
 write(path_,value);
}
}
