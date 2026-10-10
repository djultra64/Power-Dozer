#pragma once
#include "input_model.h"
#include <optional>
#include <string_view>
#include <array>
#include <cstdint>
#include <span>
namespace bc {
struct OriginalGamePrompt {uint32_t address;const char* text;};
inline constexpr std::array<OriginalGamePrompt,13> original_game_prompts={{
 {0x8030821C,"PRESS START"},{0x80309AC8,"PRESS START"},
 {0x8030B784,"SELECT START THEN"},{0x8030B7F8,"PRESSING START WILL"},
 {0x8030AECC,"USE A TO THRUST"},{0x8030AF04,"THEN HIT B TO"},
 {0x8030AC00,"USE R TO SKID"},{0x8030AE98,"WITH L/R TO LAUNCH"},
 {0x8030B298,"THE Z BUTTON."},{0x8030B324,"USE Z TO GET OUT"},
 {0x8030B57C,"USE Z TO GET OUT"},{0x80308228,"USE Z/R TO TURN PAGES"},
 {0x80308240,"USE Z/R TO MOVE MAP"}
}};
// Exact original instructions only: ordinary narrative text stays untouched.
std::span<const InputAction> game_prompt_actions(std::string_view);
std::optional<std::string> game_prompt(std::string_view,const InputModel&);
}
