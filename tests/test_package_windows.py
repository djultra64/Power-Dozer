import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))

spec=importlib.util.spec_from_file_location('package_windows',Path(__file__).resolve().parents[1]/'tools/package_windows.py')
package=importlib.util.module_from_spec(spec)
if spec.loader and Path(spec.origin).exists():spec.loader.exec_module(package)

class WindowsPackagePrivacy(unittest.TestCase):
    def test_public_zip_retains_hashes_not_machine_paths_or_rom(self):
        with tempfile.TemporaryDirectory() as d:
            stage=Path(d)/'stage with spaces';stage.mkdir()
            for name,data in {'power-dozer.exe':b'MZfixture','SDL2.dll':b'MZSDL','Run Power Dozer.cmd':b'launcher','licenses/Power-Dozer/NOTICE':b'rights unresolved'}.items():
                p=stage/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
            output=Path(d)/'package.zip'
            manifest=package.write_archive(stage,output,version='0.82',commit='a'*40)
            self.assertFalse(manifest['ready_for_release']);self.assertFalse(manifest['license_review_complete'])
            with zipfile.ZipFile(output) as archive:
                self.assertTrue(all(name.startswith('power-dozer-v0.82-windows-x86_64/') for name in archive.namelist()))
                packaged=json.loads(archive.read('power-dozer-v0.82-windows-x86_64/BUILD.json'))
                self.assertEqual(packaged['files'],manifest['files'])
                self.assertNotIn(str(stage),json.dumps(packaged))
            repeated=Path(d)/'again.zip';package.write_archive(stage,repeated,version='0.82',commit='a'*40)
            self.assertEqual(output.read_bytes(),repeated.read_bytes())
    def test_zip_rejects_private_members_and_disguised_rom(self):
        for name,data in [('rom.z64',b'private'),('data/eeprom.bin',b'private'),('capture.png',b'private'),('licenses/ROM.txt',bytes.fromhex('80371240'))]:
            with self.subTest(name=name),tempfile.TemporaryDirectory() as d:
                stage=Path(d);p=stage/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
                with self.assertRaises(ValueError):package.audit(stage)
    def test_launcher_quotes_executable_and_forwards_arguments(self):
        text=package.launcher_text()
        self.assertIn('"%~dp0power-dozer.exe" %*',text)
    def test_untrusted_version_cannot_escape_archive_root(self):
        with tempfile.TemporaryDirectory() as d:
            for version in ('../0.82','0.82/other',''):
                with self.subTest(version=version),self.assertRaises(ValueError):package.write_archive(Path(d),Path(d)/'bad.zip',version=version,commit='a'*40)
