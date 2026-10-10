#include "rom_selection.h"
#include <SDL.h>
#include <nfd.h>
#include <stdexcept>
namespace bc {
namespace {
void chooser_unavailable(){
 SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Power Dozer — select ROM","The system file chooser is unavailable.\n\nYou can select your ROM by launching Power Dozer with --rom PATH instead.",nullptr);
}
struct DialogSession {
 bool native=false;
 DialogSession(){
  if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(std::string("ROM selection: ")+SDL_GetError());
  if(NFD_Init()!=NFD_OKAY){auto error=std::string("Cannot open ROM chooser: ")+(NFD_GetError()?NFD_GetError():"unknown error");chooser_unavailable();SDL_Quit();throw std::runtime_error(error+". Use --rom PATH to select a ROM manually.");}
  native=true;
 }
 ~DialogSession(){if(native)NFD_Quit();SDL_Quit();}
};
}
std::optional<RomImage> select_rom_gui(const std::filesystem::path& data){
 DialogSession session;
 RomSelection ui;
 ui.load=[&](const auto& path){return load_cached_rom(data,path);};
 ui.choose=[]()->std::optional<std::filesystem::path>{
  const nfdu8filteritem_t filter{"Blast Corps USA Rev 1 ROM","z64,v64,n64,zip"};nfdu8char_t* path=nullptr;
  const auto result=NFD_OpenDialogU8(&path,&filter,1,nullptr);
  if(result==NFD_CANCEL)return {};
  if(result!=NFD_OKAY){auto error=std::string("ROM chooser: ")+(NFD_GetError()?NFD_GetError():"unknown error");chooser_unavailable();throw std::runtime_error(error);}
  auto selected=std::filesystem::u8path(path);NFD_FreePathU8(path);return selected;
 };
 ui.retry=[](const std::string& error){
  // Some SDL message-box backends focus the first listed button even when
  // another carries RETURNKEY_DEFAULT. Keep the intended retry action first.
  const SDL_MessageBoxButtonData buttons[]={{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,1,"Choose another ROM"},{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Quit"}};
  auto message="Power Dozer could not import this ROM.\n\n"+error+"\n\nChoose an unmodified Blast Corps USA Rev 1 ROM (z64, v64, n64 or ZIP).";
  const SDL_MessageBoxData dialog{SDL_MESSAGEBOX_ERROR,nullptr,"Power Dozer — ROM import",message.c_str(),2,buttons,nullptr};int selected=0;
  if(SDL_ShowMessageBox(&dialog,&selected)!=0)throw std::runtime_error(std::string("ROM import dialog: ")+SDL_GetError());
  return selected==1;
 };
 return select_rom(std::filesystem::exists(data/"cache/rom.z64")||std::filesystem::exists(data/"rom.z64"),ui);
}
}
