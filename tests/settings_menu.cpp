#include "settings_menu.h"
#include <cstdlib>
#include <cstdio>
static void check(bool valid){if(!valid)std::abort();}
int main(){
 bc::Settings endpoint;bc::SettingsMenu bounded(endpoint);bounded.select(1);bounded.select(0);
 if(bounded.value(bc::SettingsField::Volume)!="100%") {std::fputs("FAIL: volume at 100 must stay bounded, not wrap to 0\n",stderr);return 1;}
 bc::Settings initial;initial.family="switch";initial.volume=75;
 bc::SettingsMenu menu(initial);
 check(menu.rows().size()==4);menu.select(0);check(menu.page()==bc::SettingsMenu::Page::Graphics1);
 menu.select(0);auto change=menu.take_change();check(change&&change->field==bc::SettingsField::Scale);
 check(change->settings.family=="switch"&&change->settings.volume==75);
 check(menu.saving());menu.back();check(menu.page()==bc::SettingsMenu::Page::Graphics1);
 menu.acknowledge(initial,false);check(!menu.saving());check(menu.value(bc::SettingsField::Scale)=="720P");
 menu.select(2);change=menu.take_change();check(change&&change->settings.refresh==0);
 menu.acknowledge(change->settings,true);check(menu.status()=="SAVED RESTART");
 menu.select(3);check(menu.page()==bc::SettingsMenu::Page::Graphics2);
 menu.select(3);check(menu.page()==bc::SettingsMenu::Page::Graphics3);check(menu.rows().size()==3);
 menu.back();check(menu.page()==bc::SettingsMenu::Page::Graphics2);menu.back();menu.back();check(menu.page()==bc::SettingsMenu::Page::Root);
 menu.select(1);menu.select(0);change=menu.take_change();check(change&&change->settings.volume==80);
 menu.acknowledge(change->settings,true);menu.select(1);change=menu.take_change();check(change&&change->field==bc::SettingsField::Mute&&change->settings.mute);
 menu.acknowledge(change->settings,true);check(menu.status()=="SAVED");menu.back();
 menu.select(2);menu.select(0);change=menu.take_change();check(change&&change->settings.family=="auto");
 menu.acknowledge(change->settings,true);menu.select(1);check(menu.take_controls_request());
 menu.select(2);check(menu.take_help_request());menu.back();menu.back();check(menu.closed());
 auto wanted=initial;wanted.scale=9;wanted.volume=30;
 auto merged=bc::merge_setting(initial,{wanted,bc::SettingsField::Scale});check(merged.scale==9&&merged.volume==75);
 bc::SettingsMenu game_options(initial);game_options.select(2);game_options.select(3);check(game_options.take_game_options_request()&&game_options.closed());
 bc::SettingsMenu shortcut(initial);auto fullscreen=initial;fullscreen.fullscreen=true;
 shortcut.refresh(fullscreen);check(shortcut.value(bc::SettingsField::Fullscreen)=="FULL");
 shortcut.select(0);shortcut.select(0);shortcut.refresh(initial);
 check(shortcut.value(bc::SettingsField::Scale)=="1080P"&&shortcut.saving());
}
