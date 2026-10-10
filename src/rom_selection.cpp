#include "rom_selection.h"
namespace bc {
std::optional<RomImage> select_rom(bool try_cache,const RomSelection& ui){
 if(try_cache)try{return ui.load({});}catch(const std::exception& error){if(!ui.retry(error.what()))return {};}
 for(;;){
  auto path=ui.choose();if(!path)return {};
  try{return ui.load(*path);}catch(const std::exception& error){if(!ui.retry(error.what()))return {};}
 }
}
}
