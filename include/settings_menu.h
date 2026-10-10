#pragma once
#include "settings.h"
#include <optional>
#include <vector>
namespace bc {
enum class SettingsField {Scale,Aspect,Refresh,Fullscreen,MSAA,Presentation,Texture,Volume,Mute,Family};
struct SettingsChange {Settings settings;SettingsField field;};
Settings merge_setting(const Settings&,const SettingsChange&);
class SettingsMenu {
public:
 enum class Page {Root,Graphics1,Graphics2,Graphics3,Audio,Controls,Closed};
 explicit SettingsMenu(Settings saved):settings_(std::move(saved)){}
 Page page() const{return page_;}
 bool closed() const{return page_==Page::Closed;}
 bool saving() const{return saving_;}
 std::string status() const{return status_;}
 std::vector<std::string> rows() const;
 std::string value(SettingsField) const;
 void select(int);
 void back();
 void acknowledge(const Settings&,bool success);
 void refresh(const Settings& settings){if(!saving_)settings_=settings;}
 std::optional<SettingsChange> take_change(){auto value=change_;change_.reset();return value;}
 bool take_controls_request(){bool value=controls_;controls_=false;return value;}
 bool take_help_request(){bool value=help_;help_=false;return value;}
 bool take_game_options_request(){bool value=game_options_;game_options_=false;return value;}
private:
 Settings settings_;
 Page page_=Page::Root;
 bool saving_=false,controls_=false,help_=false,game_options_=false;
 SettingsField last_field_=SettingsField::Scale;
 std::string status_;
 std::optional<SettingsChange> change_;
 void adjust(SettingsField);
};
}
