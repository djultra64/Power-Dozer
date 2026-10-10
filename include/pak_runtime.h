#pragma once
#include "recomp.h"
void bc_initialize_pak();
extern "C" {
void bc_osPfsInit(uint8_t*,recomp_context*);
void bc_osPfsIsPlug(uint8_t*,recomp_context*);
void bc_osPfsFreeBlocks(uint8_t*,recomp_context*);
void bc_osPfsChecker(uint8_t*,recomp_context*);
void bc_osPfsAllocateFile(uint8_t*,recomp_context*);
void bc_osPfsFindFile(uint8_t*,recomp_context*);
void bc_osPfsDeleteFile(uint8_t*,recomp_context*);
void bc_osPfsReadWriteFile(uint8_t*,recomp_context*);
void bc_osPfsFileState(uint8_t*,recomp_context*);
void bc_osContGetQuery(uint8_t*,recomp_context*);
void bc_osContInit(uint8_t*,recomp_context*);
}
