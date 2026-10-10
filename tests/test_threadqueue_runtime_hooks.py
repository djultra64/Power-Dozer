import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('threadqueue_hooks', Path(__file__).resolve().parents[1] / 'tools/prepare_threadqueue_runtime.py')
hooks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hooks)


class ThreadQueuePreparation(unittest.TestCase):
    def setUp(self):
        self.original = hooks.EXPECTED
        self.scratch = tempfile.TemporaryDirectory()
        self.root = Path(self.scratch.name)
        self.source = self.root / 'source/ultramodern/src/threadqueue.cpp'
        self.source.parent.mkdir(parents=True)
        self.source.write_text('// preserved before\n' + hooks.REMOVE_BEGIN + '\n    // old removal\n}\n\n' + hooks.REMOVE_END + '\n    // preserved after\n}\n')
        hooks.EXPECTED = hashlib.sha256(self.source.read_bytes()).hexdigest()
        self.output = self.root / 'output'

    def tearDown(self):
        hooks.EXPECTED = self.original
        self.scratch.cleanup()

    def test_reference_unchanged_output_stable_and_surroundings_preserved(self):
        original = self.source.read_bytes()
        target = hooks.prepare(self.root / 'source', self.output)
        stamp = target.stat().st_mtime_ns
        hooks.prepare(self.root / 'source', self.output)
        self.assertEqual(self.source.read_bytes(), original)
        self.assertEqual(target.stat().st_mtime_ns, stamp)
        self.assertTrue(target.read_text().startswith('// preserved before\n'))
        self.assertTrue(target.read_text().endswith('    // preserved after\n}\n'))

    def test_unknown_source_rejected_before_output(self):
        self.source.write_text('changed dependency')
        with self.assertRaisesRegex(ValueError, 'fingerprint'):
            hooks.prepare(self.root / 'source', self.output)
        self.assertFalse(self.output.exists())

    def test_ambiguous_site_rejected_before_output(self):
        self.source.write_bytes(self.source.read_bytes() * 2)
        hooks.EXPECTED = hashlib.sha256(self.source.read_bytes()).hexdigest()
        with self.assertRaisesRegex(ValueError, 'not unique'):
            hooks.prepare(self.root / 'source', self.output)
        self.assertFalse(self.output.exists())
