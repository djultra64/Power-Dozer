import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

script = Path(__file__).resolve().parents[1] / 'tools/check_audio_probe.py'
spec = importlib.util.spec_from_file_location('check_audio_probe', script)
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)

class AudioProbeContract(unittest.TestCase):
    def check_events(self, events):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / 'probe.log'
            log.write_text('\n'.join(json.dumps(e) for e in events))
            probe.check(log)

    def test_frozen_vi_and_audio_are_rejected(self):
        events = [{'event': 'audio_signal', 'peak': 7334}]
        events += [{'event': 'progress', 'vi': 301, 'audio': 9}] * 4
        events += [{'event': 'probe_end', 'vi': 301, 'audio': 9}]
        with self.assertRaises(ValueError):
            self.check_events(events)

    def test_complete_advancing_audio_is_accepted(self):
        events = [{'event': 'audio_signal', 'peak': 7334}]
        events += [{'event': 'progress', 'vi': vi, 'audio': vi // 2}
                   for vi in (301, 601, 901, 1201)]
        events += [{'event': 'probe_end', 'vi': 1201, 'audio': 600}]
        self.check_events(events) # Duplicate final snapshot is valid.

    def test_incomplete_run_is_rejected(self):
        events = [{'event': 'audio_signal', 'peak': 7334}]
        events += [{'event': 'progress', 'vi': vi, 'audio': vi // 2}
                   for vi in (301, 601, 901, 1201)]
        with self.assertRaises(ValueError):
            self.check_events(events)
