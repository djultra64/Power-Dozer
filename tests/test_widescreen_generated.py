import importlib.util
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

class WidescreenGenerated(unittest.TestCase):
    def test_geometry_and_guest_memory_boundaries(self):
        with tempfile.TemporaryDirectory() as scratch:
            exe=Path(scratch)/'guest-geometry'
            result=subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'include'),'-I',str(SDK),str(ROOT/'tests/widescreen_guest.cpp'),'-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            subprocess.run([str(exe)],check=True)
    def test_unrelated_generated_function_is_unchanged(self):
        body='RECOMP_FUNC void func_80244870(uint8_t* rdram, recomp_context* ctx) {\n    return;\n;}'
        self.assertEqual(recompile.normalize_widescreen_body(body),body)
    def test_ticker_hooks_are_scoped_idempotent_and_refuse_changed_layout(self):
        body=('RECOMP_FUNC void func_801E9718(uint8_t* rdram, recomp_context* ctx) {\n'
              '    func_80259CCC(rdram, ctx);\n    func_80259DC8(rdram, ctx);\n    return;\n;}')
        changed=recompile.normalize_widescreen_body(body)
        self.assertIn('bc_widescreen_ticker(rdram);',changed)
        self.assertEqual(changed.count('bc_widescreen_ticker_text(rdram,ctx);'),2)
        self.assertEqual(recompile.normalize_widescreen_body(changed),changed)
        with self.assertRaisesRegex(ValueError,'ticker'):recompile.normalize_widescreen_body(body.replace('func_80259CCC','func_changed'))
    def test_sky_hook_precedes_double_buffer_flip(self):
        body=('RECOMP_FUNC void func_80271FD0(uint8_t* rdram, recomp_context* ctx) {\n'
              '    MEM_B(-0X403B, ctx->r1) = ctx->r12;\n    return;\n;}')
        changed=recompile.normalize_widescreen_body(body)
        self.assertLess(changed.index('bc_widescreen_sky(rdram);'),changed.index('MEM_B(-0X403B'))
        self.assertEqual(recompile.normalize_widescreen_body(changed),changed)
        with self.assertRaisesRegex(ValueError,'sky'):recompile.normalize_widescreen_body(body.replace('0X403B','0X403A'))
    def test_failure_overlay_is_scoped_and_initial_fifo_has_space_for_terminator(self):
        fade='RECOMP_FUNC void func_8025E67C(uint8_t* rdram, recomp_context* ctx) {\n    return;\n;}'
        changed=recompile.normalize_widescreen_body(fade)
        self.assertIn('bc_widescreen_whiteout(rdram);',changed)
        self.assertEqual(recompile.normalize_widescreen_body(changed),changed)
        init='RECOMP_FUNC void func_801E8DCC(uint8_t* rdram, recomp_context* ctx) {\n    ctx->r1 = SIGNED(ctx->r10) < 0X1B ? 1 : 0;\n    return;\n;}'
        changed=recompile.normalize_widescreen_body(init)
        self.assertIn('(bc_wide ? 0X24 : 0X1B)',changed)
        self.assertEqual(recompile.normalize_widescreen_body(changed),changed)

    def test_frontend_ownership_hooks_leave_live_background_and_ticker_producers_intact(self):
        for name,sites in [('80200BE0','    ctx->r1 = S32(0XE400 << 16);\n'*2),
                           ('801E9718','    func_80259CCC(rdram, ctx);\n    func_80259DC8(rdram, ctx);\n')]:
            body=f'RECOMP_FUNC void func_{name}(uint8_t* rdram, recomp_context* ctx) {{\n'+sites+'    return;\n;}'
            wide=recompile.normalize_widescreen_body(body)
            self.assertEqual(recompile.normalize_frontend_menu_body(wide,b'',b''),wide)

    def test_menu_background_marks_only_its_two_rectangle_paths(self):
        body=('RECOMP_FUNC void func_80200BE0(uint8_t* rdram, recomp_context* ctx) {\n'
              '    ctx->r1 = S32(0XE400 << 16);\n    ctx->r1 = S32(0XE400 << 16);\n    return;\n;}')
        changed=recompile.normalize_widescreen_body(body)
        self.assertEqual(changed.count('S32(0XD400 << 16)'),2)
        self.assertEqual(recompile.normalize_widescreen_body(changed),changed)
        unrelated=body.replace('func_80200BE0','func_80200BE4')
        self.assertEqual(recompile.normalize_widescreen_body(unrelated),unrelated)
        with self.assertRaisesRegex(ValueError,'background'):
            recompile.normalize_widescreen_body(body.replace('0XE400','0XE401',1))
