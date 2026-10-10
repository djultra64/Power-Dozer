import hashlib
import importlib.util
import json
from pathlib import Path
import tarfile
import tempfile
import unittest

spec=importlib.util.spec_from_file_location('source_package',Path(__file__).resolve().parents[1]/'tools/package_source.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class SourcePrivacy(unittest.TestCase):
    def test_host_export_includes_public_release_notices_and_omits_private_qa(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            files={'LICENSE':'license','NOTICE':'asset scope','CREDITS.md':'dependency acknowledgments','CHANGELOG.md':'release notes','Run Power Dozer.sh':'#!/bin/sh\n',
                   'include/control_atlas_data.h':'constexpr int sprite=0;', 'config/control-atlas.json':'{}',
                   'docs/install.md':'installation','docs/build-source.md':'recipe',
                   'docs/qa/private-checkpoint.md':'private home path','docs/evidence/capture.md':'private evidence',
                   '.local/rom.z64':'private ROM','build/main.cpp':'private generated code'}
            for name,data in files.items():
                path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(data)
            inventory=module.host_inventory(root)
            self.assertEqual(set(inventory),set(files)-{'docs/qa/private-checkpoint.md','docs/evidence/capture.md','.local/rom.z64','build/main.cpp'})
            output=root/'source.tar.gz'
            module.write_archive(output,[(root,inventory,'host')],{'product':'Power Dozer','version':'0.81','ready_for_release':False})
            with tarfile.open(output) as archive:
                self.assertTrue(all(name.startswith('power-dozer-v0.81-source/') for name in archive.getnames()))
                manifest=json.load(archive.extractfile('power-dozer-v0.81-source/SOURCES.json'))
                self.assertFalse(manifest['ready_for_release'])
                for name,entry in inventory.items():
                    data=archive.extractfile('power-dozer-v0.81-source/host/'+name).read()
                    self.assertEqual(hashlib.sha256(data).hexdigest(),entry['sha256'])
    def test_host_export_requires_license_and_scope_notice(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'LICENSE').write_text('license')
            with self.assertRaisesRegex(ValueError,'Missing host notice: NOTICE'):module.host_inventory(root)
    def test_export_keeps_exact_synthetic_fixture_but_rejects_other_hex_progress(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);fixture=root/'tests/fixtures/legacy-completed.hex';fixture.parent.mkdir(parents=True);fixture.write_text('00112233')
            self.assertIn('tests/fixtures/legacy-completed.hex',module.inventory_files(root,['tests/fixtures/legacy-completed.hex']))
            private=root/'tests/fixtures/player-progress.hex';private.write_text('00112233')
            with self.assertRaisesRegex(ValueError,'Unsupported'):module.inventory_files(root,['tests/fixtures/player-progress.hex'])
    def test_inline_cxx_sources_required_by_headers_are_preserved(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);path=root/'include/impl/std_string.inl';path.parent.mkdir(parents=True);path.write_bytes(b'inline void function() {}')
            inventory=module.inventory_files(root,['include/impl/std_string.inl'])
            self.assertEqual(set(inventory),{'include/impl/std_string.inl'})
    def test_source_inventory_omits_binary_and_private_generated_data(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            files={'src/main.cpp':b'int main() {}','include/a.h':b'#pragma once','LICENSE':b'license', '.local/generated/functions/game.c':b'private ROM translation','docs/qa/native.log':b'private log','src/disguised.cpp':bytes.fromhex('80371240')+b'ROM','src/tool':b'\x7fELFbinary','assets/image.png':b'private image'}
            for name,data in files.items():
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
            with self.assertRaisesRegex(ValueError,'binary or ROM'):module.inventory_files(root,['src/disguised.cpp'])
            with self.assertRaisesRegex(ValueError,'source path'):module.inventory_files(root,['.local/generated/functions/game.c'])
            inventory=module.inventory_files(root,['src/main.cpp','include/a.h','LICENSE'])
            self.assertEqual(set(inventory),{'src/main.cpp','include/a.h','LICENSE'})
            self.assertEqual(inventory['src/main.cpp']['sha256'],hashlib.sha256(b'int main() {}').hexdigest())
    def test_symlinks_and_traversal_cannot_include_private_inputs(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'src').mkdir();(root/'src/leak.h').symlink_to('/etc/passwd')
            for name in ['src/leak.h','../private.cpp','/etc/passwd']:
                with self.subTest(name=name),self.assertRaises(ValueError):module.inventory_files(root,[name])
    def test_archive_is_deterministic_and_checks_every_input_before_write(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'src').mkdir();f=root/'src/main.cpp';f.write_bytes(b'int main() {}')
            inventory=module.inventory_files(root,['src/main.cpp']);a=root/'a.tar.gz';b=root/'b.tar.gz'
            module.write_archive(a,[(root,inventory,'host')],{'verified':False})
            module.write_archive(b,[(root,inventory,'host')],{'verified':False})
            self.assertEqual(a.read_bytes(),b.read_bytes())
            f.write_bytes(b'modified source')
            with self.assertRaisesRegex(ValueError,'changed'):module.write_archive(root/'bad.tar.gz',[(root,inventory,'host')],{'verified':False})
            self.assertFalse((root/'bad.tar.gz').exists())
