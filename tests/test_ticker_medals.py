"""Execute the user's private, unmodified generated USA Rev 1 ticker/sprite paths.
No game source or assets are embedded in this test. Set BC_TICKER_FIXTURE to a
folder containing the four original generated function bodies listed below.
"""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import recompile
from test_widescreen_generated import SDK
FIXTURE = Path(os.environ.get('BC_TICKER_FIXTURE', ROOT / '.local/qa/nextwork-20261011/ticker-fixture'))
NAMES = ('801E8DCC', '801E9528', '801E9718', '80272ED8')

@unittest.skipUnless(all((FIXTURE / (n + '.cpp')).exists() for n in NAMES), 'Private original generated ticker fixture required')
class TickerMedals(unittest.TestCase):
    def bodies(self):
        return [(FIXTURE / (n + '.cpp')).read_text() for n in NAMES]

    def test_actual_generated_producer_and_sprite_commands(self):
        bodies = self.bodies()
        normalize = getattr(recompile, 'normalize_ticker_medals_body', lambda b, f: b)
        front = (ROOT / '.local/hd_front_end.bin').read_bytes()
        bodies = [normalize(b, front) for b in bodies]
        calls = set(re.findall(r'\b((?:func_[0-9A-F]+|[a-zA-Z_]+_recomp))\(rdram, ctx\)', '\n'.join(bodies)))
        real = {'func_' + n for n in NAMES}
        declarations = '\n'.join('void ' + n + '(uint8_t*, recomp_context*);' for n in calls | real)
        # External text/matrix/cache routines are irrelevant to the tested
        # sprite/FIFO path. Original sprite rendering itself runs in full.
        stubs = '\n'.join('void ' + n + '(uint8_t*, recomp_context* ctx) { ctx->r2=ctx->r4; }' for n in calls - real)
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'guest.cpp'
            source.write_text('#include "recomp.h"\n#include "widescreen_guest.h"\n' + declarations + '\n' + stubs + '\n' + '\n'.join(bodies) + '\n' + (ROOT / 'tests/ticker_medals_guest.cpp').read_text())
            exe = Path(tmp) / 'guest'
            result = subprocess.run(['c++', '-std=c++20', '-O0', '-w', '-I', str(SDK), '-I', str(ROOT / 'include'), str(source), '-o', str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_normalization_is_scoped_idempotent_and_rejects_drift(self):
        normalize = getattr(recompile, 'normalize_ticker_medals_body', None)
        self.assertIsNotNone(normalize, 'Ticker lifecycle/producer normalizer is absent')
        front = (ROOT / '.local/hd_front_end.bin').read_bytes()
        for body in self.bodies()[:3]:
            patched = normalize(body, front)
            self.assertNotEqual(patched, body)
            self.assertEqual(normalize(patched, front), patched)
            address = int(re.search(r'func_([0-9A-F]+)', body)[1], 16)
            corrupt = bytearray(front);corrupt[address-0x801E7000] ^= 1
            with self.assertRaisesRegex(ValueError, 'ticker'):normalize(body, corrupt)
            with self.assertRaisesRegex(ValueError, 'ticker'):normalize(patched, corrupt)
        other = self.bodies()[3]
        self.assertEqual(normalize(other, b''), other)

    def test_missing_duplicate_and_partial_sites_are_rejected(self):
        front = (ROOT / '.local/hd_front_end.bin').read_bytes()
        normalize = recompile.normalize_ticker_medals_body
        for body,site in zip(self.bodies()[:3], [
            '    return;',
            '    MEM_W(0X54E8, ctx->r1) = 0;',
            '    ctx->r13 = SUB32(ctx->r10, ctx->r11);']):
            for bad in (body.replace(site, ''), body.replace(site, site+'\n'+site)):
                with self.assertRaisesRegex(ValueError, 'ticker'):normalize(bad, front)
            patched = normalize(body, front)
            with self.assertRaisesRegex(ValueError, 'ticker'):normalize(patched.replace('bc::ticker::', 'bc::ticker::broken_', 1), front)
        # Both draw sites and both origin/cull instruction groups are pinned.
        for address in (0x801E9D40,0x801E9D60,0x801E9D64,0x801E9D70,0x801E9DAC,
                        0x801E9DE8,0x801E9E14,0x801E9E18,0x801E9E24,0x801E9E64):
            corrupt=bytearray(front);corrupt[address-0x801E7000]^=1
            with self.assertRaisesRegex(ValueError, 'ticker'):normalize(self.bodies()[2], corrupt)
