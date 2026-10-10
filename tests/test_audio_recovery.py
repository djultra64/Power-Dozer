import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
_spec=importlib.util.spec_from_file_location('audio_recovery',Path(__file__).resolve().parents[1]/'tools/check_audio_recovery.py')
module=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(module)

class RecoveryEvidence(unittest.TestCase):
    def events(self):
        return [dict(event='audio_output_status',available=False),dict(event='audio_output_status',available=True),dict(event='progress',vi=2400,gfx=2300,audio=1200),dict(event='qa_audio_fault',phase='queue_failure'),dict(event='audio_output_status',available=False),dict(event='progress',vi=2700,gfx=2600,audio=1350),dict(event='audio_output_status',available=True)]
    def check(self,events):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'native.log';path.write_text('\n'.join(json.dumps(event) for event in events));return module.check(path)
    def test_native_outage_progress(self):
        self.assertEqual(self.check(self.events())['audio_tasks_during_outage'],150)
    def test_device_removed_event(self):
        events=self.events();events[3]['phase']='device_removed'
        self.assertEqual(self.check(events)['fault_kind'],'device_removed')
    def test_no_injected_fault_rejected(self):
        events=self.events();del events[3]
        with self.assertRaisesRegex(ValueError,'Private fault'):self.check(events)
    def test_no_recovery_rejected(self):
        with self.assertRaisesRegex(ValueError,'Missing'):self.check(self.events()[:-1])
    def test_stalled_audio_rejected(self):
        events=self.events();events[5]['audio']=1200
        with self.assertRaisesRegex(ValueError,'audio did not advance'):self.check(events)
    def test_no_observation_during_outage_rejected(self):
        events=self.events();del events[5]
        with self.assertRaisesRegex(ValueError,'progress observation'):self.check(events)
