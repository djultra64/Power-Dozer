import importlib.util
from pathlib import Path
import tempfile
import unittest
spec=importlib.util.spec_from_file_location('package_linux',Path(__file__).resolve().parents[1]/'tools/package_linux.py')
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)
class PackagePrivacy(unittest.TestCase):
    def test_private_files_cannot_enter_package(self):
        for name in ('data/eeprom.bin','rom.z64','.local/hd_code.bin','capture.wav','saved.state'):
            with self.subTest(name=name),tempfile.TemporaryDirectory() as d:
                p=Path(d)/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'private')
                with self.assertRaises(ValueError):package.audit(Path(d))
    def test_symlink_cannot_smuggle_external_data(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'blastcorps').symlink_to('/etc/passwd')
            with self.assertRaises(ValueError):package.audit(root)
    def test_rom_header_disguised_as_license_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'licenses').mkdir();(root/'licenses/LICENSE').write_bytes(bytes.fromhex('80371240'))
            with self.assertRaises(ValueError):package.audit(root)
