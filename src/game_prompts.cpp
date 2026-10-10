#include "game_prompts.h"
namespace bc {
namespace {
struct Rule {std::string_view text;std::array<std::string_view,3> parts;std::array<InputAction,2> actions;unsigned count;};
constexpr std::array<Rule,11> rules={{
 {"PRESS START",{"PRESS ","",""},{InputAction::Start},1},
 {"SELECT START THEN",{"PRESS "," THEN",""},{InputAction::Start},1},
 {"PRESSING START WILL",{"PRESSING "," WILL",""},{InputAction::Start},1},
 {"USE A TO THRUST",{"USE "," TO THRUST",""},{InputAction::Confirm},1},
 {"THEN HIT B TO",{"THEN HIT "," TO",""},{InputAction::Back},1},
 {"USE R TO SKID",{"USE "," TO SKID",""},{InputAction::R},1},
 {"WITH L/R TO LAUNCH",{"WITH ","/"," TO LAUNCH"},{InputAction::L,InputAction::R},2},
 {"THE Z BUTTON.",{"THE "," CONTROL.",""},{InputAction::Trigger},1},
 {"USE Z TO GET OUT",{"USE "," TO GET OUT",""},{InputAction::Trigger},1},
 {"USE Z/R TO TURN PAGES",{"USE ","/"," TO TURN PAGES"},{InputAction::Trigger,InputAction::R},2},
 {"USE Z/R TO MOVE MAP",{"USE ","/"," TO MOVE MAP"},{InputAction::Trigger,InputAction::R},2}
}};
const Rule* rule(std::string_view text){for(auto& value:rules)if(value.text==text)return &value;return nullptr;}
}
std::span<const InputAction> game_prompt_actions(std::string_view text){if(auto value=rule(text))return {value->actions.data(),value->count};return {};}
std::optional<std::string> game_prompt(std::string_view text,const InputModel& model){
 auto value=rule(text);if(!value)return std::nullopt;
 std::string result(value->parts[0]);
 for(unsigned i=0;i<value->count;++i){result+=model.prompt(value->actions[i]).label;result+=value->parts[i+1];}
 return result;
}
}
