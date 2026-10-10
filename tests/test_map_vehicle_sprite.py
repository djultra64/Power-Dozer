from pathlib import Path
import subprocess
import sys
import os
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SDK=Path(os.environ.get('BC_SDK_ROOT',''))/'deps/N64Recomp/include'
if not (SDK/'recomp.h').is_file():SDK=ROOT.parent/'vendor/N64Recomp/include'
if not (SDK/'recomp.h').is_file():SDK=ROOT.parent/'Tetrisphere/.worktrees/options-beta/.local/deps/N64Recomp/include'
sys.path.insert(0,str(ROOT/'tools'))
import recompile

class MapVehicleSprite(unittest.TestCase):
    def test_commands_bounds_and_tmem_rows(self):
        with tempfile.TemporaryDirectory() as scratch:
            exe=Path(scratch)/'sprite-commands'
            compile=subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'include'),'-I',str(SDK),str(ROOT/'tests/map_vehicle_sprite.cpp'),'-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(compile.returncode,0,compile.stderr)
            subprocess.run([str(exe)],check=True)
    def test_scoped_calls_and_layout_rejection(self):
        body='RECOMP_FUNC void func_801F9258(uint8_t* rdram, recomp_context* ctx) {\n    func_80272ED8(rdram, ctx);\n    func_80272ED8(rdram, ctx);\n    return;\n;}'
        patched=recompile.normalize_map_vehicle_body(body)
        self.assertEqual(patched.count('bc_map_vehicle_sprite_rewrite(rdram,'),2)
        self.assertEqual(recompile.normalize_map_vehicle_body(patched),patched)
        with self.assertRaisesRegex(ValueError,'map vehicle'):recompile.normalize_map_vehicle_body(body.replace('    func_80272ED8(rdram, ctx);\n','',1))
        other=body.replace('func_801F9258','func_801F9259')
        self.assertEqual(recompile.normalize_map_vehicle_body(other),other)
