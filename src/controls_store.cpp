#include "controls_store.h"
#include "atomic_file.h"
#include <json.hpp>
#include <fstream>
#include <stdexcept>
#include <algorithm>
namespace bc {
namespace {
using Json=nlohmann::json;
struct Document {Json value=Json::object();std::string text;};
InputBinding binding(const Json& value){
 if(!value.is_object())throw std::invalid_argument("Input binding must be an object");
 unsigned kinds=value.contains("key")+value.contains("button")+value.contains("axis");if(kinds!=1)throw std::invalid_argument("Input binding needs exactly one key, button or axis");
 InputBinding result;
 if(value.contains("key"))result={BindingKind::Key,SDL_GetScancodeFromName(value.at("key").get<std::string>().c_str())};
 else if(value.contains("button"))result={BindingKind::Button,SDL_GameControllerGetButtonFromString(value.at("button").get<std::string>().c_str())};
 else result={BindingKind::Axis,SDL_GameControllerGetAxisFromString(value.at("axis").get<std::string>().c_str())};
 if(value.contains("direction")){
  const auto& sign=value["direction"];
  if(result.kind!=BindingKind::Axis||!sign.is_number_integer())throw std::invalid_argument("Axis direction must be +1 or -1");
  if(sign.is_number_unsigned()){
   if(sign.get<uint64_t>()!=1)throw std::invalid_argument("Axis direction must be +1 or -1");result.direction=1;
  }else{
   auto number=sign.get<int64_t>();if(number!=1&&number!=-1)throw std::invalid_argument("Axis direction must be +1 or -1");result.direction=int(number);
  }
 }
 InputModel::validate(result);return result;
}
std::vector<InputBinding> bindings(const Json& value){
 if(!value.is_array()||value.size()>4)throw std::invalid_argument("Input bindings must be an array of at most four controls");
 std::vector<InputBinding> output;
 for(const auto& item:value){auto parsed=binding(item);if(std::find(output.begin(),output.end(),parsed)!=output.end())throw std::invalid_argument("Duplicate input binding");output.push_back(parsed);}
 return output;
}
InputModel decode(const Json& value){
 if(!value.is_object())throw std::invalid_argument("Controls must be an object");InputModel model;
 if(!value.contains("schema")){
  if(value.contains("actions"))throw std::invalid_argument("Versioned controls need a schema version");
  for(size_t i=0;i<input_action_keys.size();i++)if(value.contains(input_action_keys[i])){
   auto name=value.at(input_action_keys[i]).get<std::string>();model.replace(InputAction(i),InputSource::Controller,{{BindingKind::Button,SDL_GameControllerGetButtonFromString(name.c_str())}});
  }
  return model;
 }
 if(!value["schema"].is_number_integer()||value["schema"]!=1)throw std::invalid_argument("Unsupported controls version");
 if(!value.contains("actions"))return model;const auto& actions=value["actions"];if(!actions.is_object())throw std::invalid_argument("Control actions must be an object");
 for(size_t i=0;i<input_action_keys.size();i++)if(actions.contains(input_action_keys[i])){
  const auto& entry=actions[input_action_keys[i]];if(!entry.is_object())throw std::invalid_argument("Logical action must be an object");
  if(entry.contains("keyboard"))model.replace(InputAction(i),InputSource::Keyboard,bindings(entry["keyboard"]));
  if(entry.contains("controller"))model.replace(InputAction(i),InputSource::Controller,bindings(entry["controller"]));
 }
 return model;
}
bool read_valid(const std::filesystem::path& path,Document& out){
 std::ifstream file(path,std::ios::binary);if(!file)return false;std::string text(65537,'\0');file.read(text.data(),text.size());auto count=file.gcount();
 if(file.bad()||count>65536)return false;text.resize(count);
 try{auto value=Json::parse(text);decode(value);out={std::move(value),std::move(text)};return true;}catch(const std::exception&){return false;}
}
Document recover(const std::filesystem::path& path){
 Document value;if(read_valid(path,value)||read_valid(path.string()+".bak",value))return value;
 if(std::filesystem::exists(path)||std::filesystem::exists(path.string()+".bak"))throw std::runtime_error("Controls and backup are invalid; restore a valid controls.json");return {};
}
Json encode(InputBinding binding){
 InputModel::validate(binding);const char* name=nullptr;const char* field=nullptr;
 if(binding.kind==BindingKind::Key){field="key";name=SDL_GetScancodeName(SDL_Scancode(binding.code));}
 if(binding.kind==BindingKind::Button){field="button";name=SDL_GameControllerGetStringForButton(SDL_GameControllerButton(binding.code));}
 if(binding.kind==BindingKind::Axis){field="axis";name=SDL_GameControllerGetStringForAxis(SDL_GameControllerAxis(binding.code));}
 if(!name||!*name)throw std::invalid_argument("Input binding has no portable name");Json value={{field,name}};if(binding.kind==BindingKind::Axis)value["direction"]=binding.direction;return value;
}
void write(const std::filesystem::path& path,const std::string& text){atomic_write_file(path,{reinterpret_cast<const uint8_t*>(text.data()),text.size()});}
}
InputModel ControlsStore::load() const{return decode(recover(path_).value);}
void ControlsStore::save(const InputModel& model) const{
 auto previous=recover(path_);auto value=previous.value;if(!value.contains("schema"))for(auto name:input_action_keys)value.erase(name);
 value["schema"]=1;if(!value.contains("actions"))value["actions"]=Json::object();
 for(size_t i=0;i<input_action_keys.size();i++){
  auto& entry=value["actions"][input_action_keys[i]];if(entry.is_null())entry=Json::object();entry["keyboard"]=Json::array();entry["controller"]=Json::array();
  for(auto binding:model.bindings(InputAction(i)))entry[binding.kind==BindingKind::Key?"keyboard":"controller"].push_back(encode(binding));
 }
 auto text=value.dump(2)+'\n';if(text.size()>65536)throw std::runtime_error("Controls exceed the 64 KiB limit");decode(value);
 if(std::filesystem::exists(path_)||std::filesystem::exists(path_.string()+".bak"))write(path_.string()+".bak",previous.text);
 write(path_,text);
}
}
