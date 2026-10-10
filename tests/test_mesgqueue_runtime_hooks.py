import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('mesgqueue_hooks', Path(__file__).resolve().parents[1] / 'tools/prepare_mesgqueue_runtime.py')
hooks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hooks)


class MessageQueuePreparation(unittest.TestCase):
    def setUp(self):
        self.expected = hooks.EXPECTED
        self.scratch = tempfile.TemporaryDirectory()
        self.root = Path(self.scratch.name)
        self.source = self.root / 'source/ultramodern/src/mesgqueue.cpp'
        self.source.parent.mkdir(parents=True)
        self.source.write_text(hooks.BATCH_SITE + '\n' + hooks.WAIT_SITE + '\n' + (hooks.RETRY_SITE + '\n') * 2)
        hooks.EXPECTED = hashlib.sha256(self.source.read_bytes()).hexdigest()
        self.output = self.root / 'output'

    def tearDown(self):
        hooks.EXPECTED = self.expected
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

    def test_missing_or_ambiguous_sites_rejected_before_output(self):
        for text in (self.source.read_text() * 2, '// missing sites\n'):
            self.source.write_text(text)
            hooks.EXPECTED = hashlib.sha256(self.source.read_bytes()).hexdigest()
            with self.assertRaisesRegex(ValueError, 'sites changed'):
                hooks.prepare(self.root / 'source', self.output)
            self.assertFalse(self.output.exists())
