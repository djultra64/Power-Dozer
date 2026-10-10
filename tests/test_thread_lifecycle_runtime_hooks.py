import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('lifecycle_hooks', Path(__file__).resolve().parents[1] / 'tools/prepare_thread_lifecycle_runtime.py')
hooks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hooks)


class ThreadLifecyclePreparation(unittest.TestCase):
    def setUp(self):
        self.original = hooks.EXPECTED
        self.scratch = tempfile.TemporaryDirectory()
        self.root = Path(self.scratch.name)
        self.source = self.root / 'source/ultramodern/src/threads.cpp'
        self.source.parent.mkdir(parents=True)
        self.source.write_text('#include <string>\n' + hooks.CALLBACK_SITE + '\n' + hooks.DESTROY_SITE + '\n' + hooks.CLEANUP_SITE + '\n')
        hooks.EXPECTED = hashlib.sha256(self.source.read_bytes()).hexdigest()
        self.output = self.root / 'output'

    def tearDown(self):
        hooks.EXPECTED = self.original
        self.scratch.cleanup()

    def test_reference_unchanged_and_identical_output_keeps_timestamp(self):
        original = self.source.read_bytes()
        target = hooks.prepare(self.root / 'source', self.output)
        stamp = target.stat().st_mtime_ns
        hooks.prepare(self.root / 'source', self.output)
        self.assertEqual(self.source.read_bytes(), original)
        self.assertEqual(target.stat().st_mtime_ns, stamp)

    def test_unknown_source_rejected_before_output(self):
        self.source.write_text('changed dependency')
        with self.assertRaisesRegex(ValueError, 'fingerprint'):
            hooks.prepare(self.root / 'source', self.output)
        self.assertFalse(self.output.exists())

    def test_ambiguous_or_missing_sites_rejected_before_output(self):
        for content in (self.source.read_text() * 2, '// missing sites\n'):
            with self.subTest(content=content[:20]):
                self.source.write_text(content)
                hooks.EXPECTED = hashlib.sha256(self.source.read_bytes()).hexdigest()
                with self.assertRaisesRegex(ValueError, 'not unique'):
                    hooks.prepare(self.root / 'source', self.output)
                self.assertFalse(self.output.exists())
