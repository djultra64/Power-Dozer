#pragma once
namespace RT64 { struct Interpreter; }
void bc_load_l3d(RT64::Interpreter& interpreter, unsigned text, unsigned data);
void bc_load_hud_annotations(RT64::Interpreter& interpreter, unsigned text);
