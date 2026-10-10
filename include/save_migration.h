#pragma once
#include "pak_store.h"
#include "save_store.h"
namespace bc {
// Imports only into an empty Pak; retains the entire original player packet.
// The caller persists the candidate before making it visible to the game.
bool import_eeprom_player(PakImage&,const SaveStore::Image&);
}
