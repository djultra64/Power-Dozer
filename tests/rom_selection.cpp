#include "rom_selection.h"
#include <stdexcept>
#include <cstdio>
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 int loads=0,choices=0,retries=0;bc::RomImage image;image.rom={87};
 bc::RomSelection ui{[&](const auto& path){++loads;return image;},[&]()->std::optional<std::filesystem::path>{++choices;return "chosen.zip";},[&](const std::string&){++retries;return true;}};
 auto result=bc::select_rom(true,ui);check(result&&result->rom==image.rom&&loads==1&&choices==0,"valid cache boots without opening chooser");
 loads=choices=retries=0;result=bc::select_rom(false,ui);check(result&&loads==1&&choices==1&&retries==0,"fresh install selects ROM without a spurious error");
 ui.choose=[&]()->std::optional<std::filesystem::path>{++choices;return {};};loads=choices=0;result=bc::select_rom(false,ui);check(!result&&loads==0&&choices==1,"cancel never imports or resets progress");
 ui.choose=[&]()->std::optional<std::filesystem::path>{++choices;return choices==1?"invalid.zip":"valid.zip";};
 ui.load=[&](const auto& path){++loads;if(path=="invalid.zip")throw std::runtime_error("wrong revision");return image;};
 loads=choices=retries=0;result=bc::select_rom(false,ui);check(result&&loads==2&&choices==2&&retries==1,"invalid ROM can be replaced without launching it");
 ui.retry=[&](const std::string& error){++retries;check(error=="wrong revision","loader error retained for user");return false;};
 loads=choices=retries=0;result=bc::select_rom(false,ui);check(!result&&loads==1&&choices==1&&retries==1,"quit after invalid ROM ends import flow");
 ui.load=[&](const auto& path){++loads;if(path.empty())throw std::runtime_error("corrupt cache");return image;};
 ui.retry=[&](const std::string& error){++retries;check(error=="corrupt cache","cache error retained");return true;};
 loads=choices=retries=0;result=bc::select_rom(true,ui);check(result&&loads==2&&choices==1&&retries==1,"corrupt cache can be repaired through selection");
 puts("ROM selection cancellation, retry and cache flow passed");
}
