import importlib.util
import unittest
import os
from pathlib import Path

class RomContract(unittest.TestCase):
    def test_original_rom_and_overlay_contract(self):
        script = Path(__file__).resolve().parents[1] / 'tools/prepare.py'
        self.assertTrue(script.exists(), 'ROM preparation script is missing')
        spec = importlib.util.spec_from_file_location('prepare', script)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        explicit=os.environ.get('BC_TEST_ROM')
        fixture=Path(explicit) if explicit else script.parents[1]/'.local/rom.z64'
        if not fixture.is_file() and not explicit:fixture=next(script.parents[1].glob('*.zip'),None)
        if fixture is None:self.skipTest('Private ROM unavailable; set BC_TEST_ROM to a valid own USA Rev 1 ROM')
        rom = mod.read_rom(fixture)
        self.assertEqual(len(rom), 8388608)
        sections = mod.extract_sections(rom)
        self.assertEqual([(s['name'], s['vram'], len(s['data'])) for s in sections],
                         [('hd_code', 0x802447C0, 0xCAEA0), ('hd_front_end', 0x801E7000, 0x29E90)])
        self.assertEqual(sections[0]['data'][:4].hex(), '27bdff88')

    def test_corrupted_rom_is_rejected(self):
        script = Path(__file__).resolve().parents[1] / 'tools/prepare.py'
        self.assertTrue(script.exists(), 'ROM preparation script is missing')
        spec = importlib.util.spec_from_file_location('prepare', script)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        with self.assertRaises(ValueError):
            mod.validate_rom(bytes(8388608))

if __name__ == '__main__': unittest.main()
