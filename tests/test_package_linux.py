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
    def test_license_bundle_checks_exact_linked_input_before_copy(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);source,target,manifest=self.fixture_licenses(root)
            deps=root/'deps';(deps/'vendor').mkdir(parents=True);binary=deps/'vendor/library.so';binary.write_bytes(b'original')
            manifest['inputs']={'vendor/library.so':{'bytes':8,'sha256':hashlib.sha256(b'original').hexdigest()}}
            (source/'provenance.json').write_text(json.dumps(manifest))
            self.assertEqual(package.verified_licenses(source,target,inputs_root=deps),1)
            for path in target.iterdir():path.unlink()
            target.rmdir();binary.write_bytes(b'modified')
            with self.assertRaisesRegex(ValueError,'input provenance mismatch'):
                package.verified_licenses(source,target,inputs_root=deps)
            self.assertFalse(target.exists())
    def test_license_bundle_rejects_input_traversal(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);source,target,manifest=self.fixture_licenses(root)
            manifest['inputs']={'../outside':{'bytes':8,'sha256':'ignored'}}
            (source/'provenance.json').write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError,'path'):
                package.verified_licenses(source,target,inputs_root=root)
            self.assertFalse(target.exists())
    def test_vendor_notices_include_license_directory_and_third_party_notices(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);source=root/'vendor';target=root/'out'
            texts={'LICENSES/Apache-2.0.txt':b'Apache text','LICENSES/MIT.txt':b'MIT text','ThirdPartyNotices.txt':b'third party text','NOTICE':b'notice text','src/game.c':b'private source'}
            for name,data in texts.items():
                path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
            package.copy_vendor_notices(source,target)
            self.assertEqual({p.relative_to(target).as_posix():p.read_bytes() for p in target.rglob('*') if p.is_file()}, {name:data for name,data in texts.items() if name!='src/game.c'})
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
