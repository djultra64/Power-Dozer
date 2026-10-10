import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import subprocess
s=importlib.util.spec_from_file_location('bootstrap_sdk',Path(__file__).resolve().parents[1]/'tools/bootstrap_sdk.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
class SourceBootstrap(unittest.TestCase):
    def test_rt64_shader_environment_patch_uses_local_copy_and_verified_input(self):
        with tempfile.TemporaryDirectory(prefix='SDK with spaces ') as d:
            root=Path(d);source=root/'deps/rt64';source.mkdir(parents=True)
            text='set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/src/contrib/dxc/bin/x64/dxc-linux")\n'
            cmake=source/'CMakeLists.txt';cmake.write_text(text);expected=hashlib.sha256(cmake.read_bytes()).hexdigest()
            result=m.prepare_rt64_source(root,expected)
            self.assertEqual(cmake.read_text(),text)
            self.assertEqual(result,root/'rt64-source')
            driver=result/'src/contrib/dxc/bin/x64/dxc-linux';driver.parent.mkdir(parents=True)
            driver.write_text('#!/usr/bin/env python3\nimport os\nfrom pathlib import Path\nPath("shader-result.txt").write_text(os.environ["LD_LIBRARY_PATH"])\n');driver.chmod(0o755)
            generated=result/'CMakeLists.txt'
            generated.write_text('cmake_minimum_required(VERSION 3.20)\nproject(ShaderProbe NONE)\n'+generated.read_text()+'add_custom_target(shader ALL COMMAND ${DXC} VERBATIM)\n')
            build=root/'probe build'
            subprocess.run(['cmake','-S',str(result),'-B',str(build),'-G','Ninja'],check=True,capture_output=True)
            subprocess.run(['cmake','--build',str(build)],check=True,capture_output=True)
            self.assertEqual((build/'shader-result.txt').read_text(),str(result/'src/contrib/dxc/lib/x64'))
    def test_prepared_source_verification_rejects_source_or_provenance_changes(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);source=root/'deps/rt64';source.mkdir(parents=True)
            text=b'set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/src/contrib/dxc/bin/x64/dxc-linux")\n'
            (source/'CMakeLists.txt').write_bytes(text);(source/'shader.hlsl').write_bytes(b'original')
            files={'vendor/rt64/'+name:{'bytes':len(payload),'sha256':hashlib.sha256(payload).hexdigest()} for name,payload in [('CMakeLists.txt',text),('shader.hlsl',b'original')]}
            (root/'SOURCE-INPUTS.json').write_text(json.dumps({'files':files}))
            expected=hashlib.sha256(text).hexdigest();m.prepare_rt64_source(root,expected)
            self.assertEqual(m.verify_prepared_rt64(root,expected),2)
            shader=root/'rt64-source/shader.hlsl';shader.write_bytes(b'modified')
            with self.assertRaisesRegex(ValueError,'mismatch'):m.verify_prepared_rt64(root,expected)
            shader.write_bytes(b'original');meta=root/'RT64-SOURCE-PATCH.json';data=json.loads(meta.read_text());data['source_sha256']='wrong';meta.write_text(json.dumps(data))
            with self.assertRaisesRegex(ValueError,'provenance'):m.verify_prepared_rt64(root,expected)
    def test_reused_sdk_binary_inputs_require_exact_hashes(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);controller=root/'gamecontrollerdb.txt';controller.write_bytes(b'good')
            dxc=root/'deps/rt64/src/contrib/dxc/bin/tool';dxc.parent.mkdir(parents=True);dxc.write_bytes(b'good')
            entry={'bytes':4,'sha256':hashlib.sha256(b'good').hexdigest()}
            lock={'dxc_build_inputs':{'bin/tool':entry},'controller_database':{'files':{'gamecontrollerdb.txt':entry}}}
            m.verify_sdk_binary_inputs(root,lock)
            controller.write_bytes(b'evil')
            with self.assertRaisesRegex(ValueError,'mismatch'):m.verify_sdk_binary_inputs(root,lock)
            controller.write_bytes(b'good');dxc.write_bytes(b'evil')
            with self.assertRaisesRegex(ValueError,'mismatch'):m.verify_sdk_binary_inputs(root,lock)
    def test_changed_rt64_cmake_is_rejected_before_copy(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);source=root/'deps/rt64';source.mkdir(parents=True);(source/'CMakeLists.txt').write_text('unknown source')
            with self.assertRaisesRegex(ValueError,'fingerprint'):m.prepare_rt64_source(root,'incorrect')
            self.assertFalse((root/'rt64-source').exists())
    def test_source_snapshot_accepts_hashes_and_rejects_same_size_changes(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'src').mkdir();p=root/'src/a.cpp';p.write_bytes(b'original')
            manifest=root/'SOURCES.json';manifest.write_text(json.dumps({'files':{'vendor/runtime/src/a.cpp':{'bytes':8,'sha256':hashlib.sha256(b'original').hexdigest()}}}))
            self.assertEqual(m.verify_source_snapshot(manifest,root,'vendor/runtime'),1)
            p.write_bytes(b'modified')
            with self.assertRaisesRegex(ValueError,'mismatch'):m.verify_source_snapshot(manifest,root,'vendor/runtime')
    def test_snapshot_cannot_read_outside_source_prefix(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);manifest=root/'SOURCES.json';manifest.write_text(json.dumps({'files':{'vendor/runtime/../../secret':{'bytes':1,'sha256':'x'}}}))
            with self.assertRaisesRegex(ValueError,'Unsafe'):m.verify_source_snapshot(manifest,root,'vendor/runtime')
    def test_plan_compiles_sdk_libraries_and_tools_without_game_or_reference(self):
        root=Path('/tmp/isolated SDK');plan=m.build_commands(root,'python3',2)
        flat=json.dumps(plan)
        self.assertNotIn('Tetrisphere',flat);self.assertNotIn('blastcorps-prototype',flat)
        self.assertIn('rabbitizer-1.16.2.tar.gz',flat);self.assertIn('pyyaml-6.0.2.tar.gz',flat);self.assertIn('--no-deps',flat)
        self.assertIn('N64RecompCLI',flat);self.assertIn('RSPRecomp',flat);self.assertIn('RT64_STATIC=ON',flat);self.assertIn('NFD_PORTAL=ON',flat)
        self.assertTrue(any(command[:2]==['cmake','--build'] and 'rt64' in command for command in plan))
