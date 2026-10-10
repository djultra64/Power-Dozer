#include "game_prompts.h"
namespace bc {
std::optional<std::string> game_prompt(std::string_view text,const InputModel& model){
 auto label=[&](InputAction action){return model.prompt(action).label;};
 if(text=="PRESS START")return "PRESS "+label(InputAction::Start);
 if(text=="SELECT START THEN")return "PRESS "+label(InputAction::Start)+" THEN";
 if(text=="PRESSING START WILL")return "PRESSING "+label(InputAction::Start)+" WILL";
 if(text=="USE A TO THRUST")return "USE "+label(InputAction::Confirm)+" TO THRUST";
 if(text=="THEN HIT B TO")return "THEN HIT "+label(InputAction::Back)+" TO";
 if(text=="USE R TO SKID")return "USE "+label(InputAction::R)+" TO SKID";
 if(text=="WITH L/R TO LAUNCH")return "WITH "+label(InputAction::L)+"/"+label(InputAction::R)+" TO LAUNCH";
 if(text=="THE Z BUTTON.")return "THE "+label(InputAction::Trigger)+" CONTROL.";
 if(text=="USE Z TO GET OUT")return "USE "+label(InputAction::Trigger)+" TO GET OUT";
 if(text=="USE Z/R TO TURN PAGES")return "USE "+label(InputAction::Trigger)+"/"+label(InputAction::R)+" TO TURN PAGES";
 if(text=="USE Z/R TO MOVE MAP")return "USE "+label(InputAction::Trigger)+"/"+label(InputAction::R)+" TO MOVE MAP";
 return std::nullopt;
}
}
