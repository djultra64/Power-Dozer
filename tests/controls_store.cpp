#include "controls_store.h"
#include <json.hpp>
#include <chrono>
#include <fstream>
#include <stdexcept>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static std::string read(const std::filesystem::path& path){std::ifstream file(path);return {std::istreambuf_iterator<char>(file),{}};}
static void write(const std::filesystem::path& path,const std::string& text){std::ofstream file(path);file<<text;}
int main(){
 auto root=std::filesystem::temp_directory_path()/("bc-controls-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(root);
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::filesystem::remove_all(path);}} cleanup{root};
 auto path=root/"controls.json";bc::ControlsStore store(path);auto model=store.load();check(model.prompt(bc::InputAction::Confirm).label=="Z","missing controls use defaults");
 write(path,R"({"confirm":"x","future":{"keep":12}})");auto legacy=read(path);model=store.load();model.activity(bc::InputSource::Controller);model.manual_family(bc::ControllerFamily::Switch);
 check(model.prompt(bc::InputAction::Confirm).label=="Y","legacy physical X remap has Switch Y label");check(read(path)==legacy,"loading a legacy config does not replace it");
 model.replace(bc::InputAction::Confirm,bc::InputSource::Keyboard,{{bc::BindingKind::Key,SDL_SCANCODE_Q}});store.save(model);
 auto saved=nlohmann::json::parse(read(path));check(saved["schema"]==1&&saved["future"]["keep"]==12,"versioned save preserves unknown root fields");check(read(path.string()+".bak")==legacy,"legacy config retained as complete backup");
 auto reload=store.load();check(reload.prompt(bc::InputAction::Confirm).label=="Q","keyboard mapping persists independently");reload.activity(bc::InputSource::Controller);reload.manual_family(bc::ControllerFamily::PlayStation);check(reload.prompt(bc::InputAction::Confirm).label=="SQUARE","pad mapping persists independently");
 saved["actions"]["confirm"]["future"]=55;saved["actions"]["later_action"]={{"future",42}};write(path,saved.dump());store.save(reload);
 auto kept=nlohmann::json::parse(read(path));check(kept["actions"]["confirm"]["future"]==55&&kept["actions"]["later_action"]["future"]==42,"unknown action metadata retained");
 auto expected=read(path.string()+".bak");write(path,"damaged");auto recovered=store.load();check(recovered.prompt(bc::InputAction::Confirm).label=="Q","corrupt primary recovers validated backup");store.save(recovered);check(read(path.string()+".bak")==expected,"repair preserves validated backup contents");
 auto original=read(path);auto backup=path.string()+".bak";std::filesystem::remove(backup);std::filesystem::create_directory(backup);
 recovered.replace(bc::InputAction::Back,bc::InputSource::Keyboard,{{bc::BindingKind::Key,SDL_SCANCODE_T}});bool refused=false;try{store.save(recovered);}catch(const std::exception&){refused=true;}check(refused&&read(path)==original,"failed atomic write retains saved mappings");std::filesystem::remove(backup);
 for(auto invalid:{R"({"schema":2})",R"({"actions":{}})",R"({"schema":1,"actions":[]})",R"({"confirm":9})",R"({"schema":1,"actions":{"confirm":{"keyboard":[{"key":"not-a-key"}]}}})",R"({"schema":1,"actions":{"confirm":{"keyboard":[{"button":"a"}]}}})",R"({"schema":1,"actions":{"trigger":{"controller":[{"axis":"lefttrigger","direction":0}]}}})",R"({"schema":1,"actions":{"trigger":{"controller":[{"axis":"lefttrigger","direction":18446744073709551615}]}}})",R"({"schema":1,"actions":{"confirm":{"controller":[{"button":"a"},{"button":"a"}]}}})"}){
  write(path,invalid);refused=false;try{store.load();}catch(const std::exception&){refused=true;}check(refused&&read(path)==invalid,(std::string("invalid mapping is refused without reset: ")+invalid).c_str());
 }
 write(path,std::string(65537,' '));refused=false;try{store.load();}catch(const std::exception&){refused=true;}check(refused,"oversized controls refused");
 write(path,"damaged");write(backup,"damaged");refused=false;try{store.save(model);}catch(const std::exception&){refused=true;}check(refused&&read(path)=="damaged"&&read(backup)=="damaged","two invalid copies never reset or overwrite controls");
}
