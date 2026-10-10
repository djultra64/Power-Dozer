import importlib.util
from pathlib import Path
import tempfile
import subprocess
import unittest
s=importlib.util.spec_from_file_location('sdk_paths',Path(__file__).resolve().parents[1]/'tools/sdk_paths.py');p=importlib.util.module_from_spec(s);s.loader.exec_module(p)
class SDKPathCompatibility(unittest.TestCase):
    def test_independent_sdk_never_needs_reference_checkout(self):
        with tempfile.TemporaryDirectory(prefix='Blast Corps SDK ') as d:
            root=Path(d);paths=p.sdk_paths(sdk_root=root)
            self.assertEqual(paths['deps'],root/'deps')
            self.assertEqual(paths['tools'],root/'toolchain/n64recomp-build')
            self.assertEqual(paths['rt64_build'],root/'rt64-build')
            self.assertEqual(paths['analysis_venv'],root/'analysis-venv')
    def test_existing_reference_layout_is_preserved(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);paths=p.sdk_paths(tetrisphere=root)
            self.assertEqual(paths['deps'],root/'.local/deps')
            self.assertEqual(paths['tools'],root/'.local/m1/toolchain/n64recomp-build')
            self.assertEqual(paths['rt64_build'],root/'.local/m1/build-linux/rt64')
    def test_ambiguous_or_missing_root_is_rejected(self):
        for kwargs in [{},{'sdk_root':Path('/tmp/a'),'tetrisphere':Path('/tmp/b')}]:
            with self.subTest(kwargs=kwargs),self.assertRaises(ValueError):p.sdk_paths(**kwargs)

    def test_switching_sdk_and_legacy_clears_cached_selectors_and_library_root(self):
        with tempfile.TemporaryDirectory(prefix='SDK switches ') as d:
            root=Path(d);source=root/'source';source.mkdir();build=root/'build'
            (source/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(SDKSwitch NONE)\nset(BC_SDK_ROOT "" CACHE PATH "SDK")\nset(TETRISPHERE_ROOT "" CACHE PATH "Legacy")\nset(BC_RT64_BUILD_ROOT "" CACHE PATH "Libraries")\nfile(WRITE "${CMAKE_BINARY_DIR}/selection.txt" "${BC_SDK_ROOT}\\n${TETRISPHERE_ROOT}\\n${BC_RT64_BUILD_ROOT}")\n')
            for sdk,legacy in [(root/'SDK A',None),(root/'SDK B',None),(None,root/'Reference')]:
                options=p.sdk_cmake_options(sdk_root=sdk,tetrisphere=legacy)
                subprocess.run(['cmake','-S',str(source),'-B',str(build),*options],check=True,capture_output=True)
                lines=(build/'selection.txt').read_text().splitlines()
                self.assertEqual(lines,[str(sdk or ''),str(legacy or ''),str(p.sdk_paths(sdk_root=sdk,tetrisphere=legacy)['rt64_build'])])
