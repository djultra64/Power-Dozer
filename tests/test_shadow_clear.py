from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import recompile

# Only the original submission boundary is represented here, not a generated
# game function. Execution tests below use this minimal ABI harness.
SITE = '''    // 0x80258B60: jal         0x80284E54
    // 0x80258B64: sw          $zero, 0x14($sp)
    MEM_W(0X14, ctx->r29) = 0;
    func_80284E54(rdram, ctx);'''
BODY = 'RECOMP_FUNC void func_80258544(uint8_t* rdram, recomp_context* ctx) {\n' + SITE + '\n;}'
FINGERPRINT = bytes.fromhex('8fb9009c3c0e803625ce50b0032e2823000540c32409061fafa900100100282501c0202524060001000038250c0a1395afa00014')
SITE_OFFSET = 0x80258B34 - 0x802447C0


class ShadowClear(unittest.TestCase):
    def rom(self):
        return bytes(SITE_OFFSET) + FINGERPRINT

    def test_clear_coverage_preserves_silhouette_and_rejects_invalid_lists(self):
        with tempfile.TemporaryDirectory() as scratch:
            exe = Path(scratch) / 'shadow-clear'
            result = subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                                     '-I', str(ROOT / 'include'), str(ROOT / 'tests/shadow_clear.cpp'),
                                     str(ROOT / 'src/shadow_clear.cpp'), '-o', str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_generated_hook_executes_after_delay_store_before_submission(self):
        changed = recompile.normalize_shadow_clear_body(BODY, self.rom())
        with tempfile.TemporaryDirectory() as scratch:
            source = Path(scratch) / 'hook.cpp'
            source.write_text('''#include <cstdint>
#include <cstddef>
#include <cstdlib>
#define RECOMP_FUNC
struct recomp_context { uint64_t r4 = 0x803650B0, r5 = 26, r29 = 0; };
uint32_t delay_word = 99;
bool rewritten = false, submitted = false;
#define MEM_W(a, b) delay_word
uint32_t bc_shadow_clear_rewrite(uint8_t* memory, size_t size, uint32_t address, uint32_t count) {
    if (delay_word != 0 || memory != nullptr || size != 0x800000 || address != 0x803650B0 || count != 26) std::abort();
    rewritten = true; return 27;
}
void func_80284E54(uint8_t*, recomp_context* ctx) {
    if (!rewritten || ctx->r5 != 27) std::abort();
    submitted = true;
}
''' + changed + '\nint main() { recomp_context ctx; func_80258544(nullptr, &ctx); return !submitted; }\n')
            exe = Path(scratch) / 'hook'
            result = subprocess.run(['c++', '-std=c++20', str(source), '-o', str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, 'Submission did not receive the rewritten count after its delay-slot store')

    def test_hook_is_scoped_idempotent_and_checks_original_instructions(self):
        changed = recompile.normalize_shadow_clear_body(BODY, self.rom())
        self.assertNotEqual(changed, BODY)
        self.assertEqual(recompile.normalize_shadow_clear_body(changed, self.rom()), changed)
        other = BODY.replace('func_80258544(', 'func_80258548(')
        self.assertEqual(recompile.normalize_shadow_clear_body(other, b''), other)
        for index in range(len(FINGERPRINT)):
            corrupted = bytearray(self.rom())
            corrupted[SITE_OFFSET + index] ^= 1
            with self.assertRaisesRegex(ValueError, 'shadow'):
                recompile.normalize_shadow_clear_body(BODY, corrupted)
            with self.assertRaisesRegex(ValueError, 'shadow'):
                recompile.normalize_shadow_clear_body(changed, corrupted)
        with self.assertRaisesRegex(ValueError, 'shadow'):
            recompile.normalize_shadow_clear_body(BODY, self.rom()[:-1])

    def test_changed_missing_duplicate_or_partial_hook_sites_reject(self):
        changed = recompile.normalize_shadow_clear_body(BODY, self.rom())
        for invalid in (BODY.replace('0x80258B60', '0x80258B5C'),
                        BODY.replace('0X14', '0X10'),
                        BODY.replace('    func_80284E54(rdram, ctx);', ''),
                        BODY.replace(SITE, SITE + '\n' + SITE),
                        BODY.replace(SITE, SITE + '\n    func_80284E54(rdram, ctx);'),
                        changed.replace('ctx->r5 = bc_shadow_clear_rewrite', 'ctx->r4 = bc_shadow_clear_rewrite')):
            with self.assertRaisesRegex(ValueError, 'shadow'):
                recompile.normalize_shadow_clear_body(invalid, self.rom())

    @unittest.skipUnless((ROOT / '.local/hd_code.bin').is_file() and
                         (ROOT / '.local/hd_front_end.bin').is_file(),
                         'Private validated overlay images required for full normalization')
    def test_full_normalizer_composes_the_shadow_hook(self):
        # Run the real normalization entry point on synthetic function text.
        # Its existing ROM guards read local private inputs; none are copied.
        other = BODY.replace('func_80258544(', 'func_80258548(')
        with tempfile.TemporaryDirectory() as scratch:
            folder = Path(scratch)
            source = folder / 'funcs_0.c'
            source.write_text(BODY + '\n' + other)
            recompile.normalize(folder)
            expected = recompile.normalize_shadow_clear_body(BODY, self.rom()) + '\n' + other
            self.assertEqual(source.read_text(), expected)
            recompile.normalize(folder)
            self.assertEqual(source.read_text(), expected)


if __name__ == '__main__':
    unittest.main()
