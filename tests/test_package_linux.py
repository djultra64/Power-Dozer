import importlib.util
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
spec=importlib.util.spec_from_file_location('package_linux',Path(__file__).resolve().parents[1]/'tools/package_linux.py')
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)
class PackagePrivacy(unittest.TestCase):
    def fixture_licenses(self,root):
        source=root/'source';source.mkdir();data=b'Fixture license text';(source/'LICENSE').write_bytes(data)
        manifest={'files':{'LICENSE':{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}}}
        (source/'provenance.json').write_text(json.dumps(manifest))
        return source,root/'destination',manifest
    def test_verified_license_text_and_provenance_are_packaged(self):
        with tempfile.TemporaryDirectory() as d:
            source,target,_=self.fixture_licenses(Path(d))
            self.assertEqual(package.verified_licenses(source,target),1)
            self.assertEqual((target/'LICENSE').read_bytes(),(source/'LICENSE').read_bytes())
            self.assertEqual((target/'provenance.json').read_bytes(),(source/'provenance.json').read_bytes())
    def test_changed_license_is_rejected_before_any_copy(self):
        with tempfile.TemporaryDirectory() as d:
            source,target,_=self.fixture_licenses(Path(d));(source/'LICENSE').write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'provenance mismatch'):package.verified_licenses(source,target)
            self.assertFalse(target.exists())
    def test_license_manifest_cannot_traverse_source_tree(self):
        with tempfile.TemporaryDirectory() as d:
            source,target,manifest=self.fixture_licenses(Path(d));manifest['files']['../outside']=manifest['files'].pop('LICENSE')
            (source/'provenance.json').write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError,'path'):package.verified_licenses(source,target)
            self.assertFalse(target.exists())
    def test_symlinked_license_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            source,target,_=self.fixture_licenses(Path(d));(source/'LICENSE').rename(source/'outside');(source/'LICENSE').symlink_to(source/'outside')
            with self.assertRaisesRegex(ValueError,'symlink'):package.verified_licenses(source,target)
            self.assertFalse(target.exists())
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
