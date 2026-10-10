#pragma once
#include <SDL.h>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
namespace bc {
inline std::filesystem::path application_data_directory(){
 if(auto* path=std::getenv("BLASTCORPS_DATA_DIR");path&&*path)return std::filesystem::u8path(path);
 auto* base=SDL_GetBasePath();if(!base)throw std::runtime_error("Cannot locate executable directory");
 auto directory=std::filesystem::u8path(base);SDL_free(base);return directory/"data";
}
}
