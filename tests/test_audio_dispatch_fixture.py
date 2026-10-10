import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('audio_dispatch_fixture', Path(__file__).resolve().parents[1] / 'tools/prepare_audio_dispatch_fixture.py')
hooks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hooks)


class OriginalAudioFixturePreparation(unittest.TestCase):
    def setUp(self):
        self.original_expected = hooks.EXPECTED
        self.scratch = tempfile.TemporaryDirectory()
        self.root = Path(self.scratch.name)
        self.source = self.root / 'source'
        self.source.mkdir()
        self.name = 'func_80267A9C'
        self.function = 'RECOMP_FUNC void ' + self.name + '(uint8_t*, recomp_context*) {\n    return;\n;}\n'
        self.path = self.source / 'funcs_3.c'
        self.path.write_text(self.function)
        hooks.EXPECTED = {self.name: ('funcs_3.c', hashlib.sha256(self.function.encode()).hexdigest())}
        self.output = self.root / 'output'

    def tearDown(self):
        hooks.EXPECTED = self.original_expected
        self.scratch.cleanup()

    def test_exact_source_copy_and_idempotence(self):
        before = self.path.read_bytes()
        result = hooks.prepare(self.source, self.output)
        self.assertTrue(result.read_text().endswith(self.function))
        stamp = result.stat().st_mtime_ns
        hooks.prepare(self.source, self.output)
        self.assertEqual(result.stat().st_mtime_ns, stamp)
        self.assertEqual(self.path.read_bytes(), before)

    def test_unknown_function_rejected_before_output(self):
        self.path.write_text(self.function.replace('return;', 'return /* changed */;'))
        with self.assertRaisesRegex(ValueError, 'fingerprint'):
            hooks.prepare(self.source, self.output)
        self.assertFalse(self.output.exists())

    def test_missing_or_ambiguous_function_rejected_before_output(self):
        for text in ('// missing function\n', self.function * 2):
            self.path.write_text(text)
            with self.assertRaisesRegex(ValueError, 'sites changed'):
                hooks.prepare(self.source, self.output)
            self.assertFalse(self.output.exists())
