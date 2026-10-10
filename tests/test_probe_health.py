import importlib.util
import json
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('probe_health', Path(__file__).resolve().parents[1] / 'tools/probe_health.py')
health = importlib.util.module_from_spec(spec)
spec.loader.exec_module(health)

class LiveProbeHealth(unittest.TestCase):
    def inspect(self, *rows):
        return health.stalled_progress('\n'.join(json.dumps(dict(event='progress',vi=vi,gfx=gfx,audio=audio)) for vi,gfx,audio in rows))

    def test_observed_startup_freeze_is_detected_while_running(self):
        self.assertIn('Graphics', self.inspect((901,882,450),(1201,968,488),(1501,968,488)))

    def test_observed_audio_only_freeze_is_detected(self):
        self.assertIn('Audio', self.inspect((1202,900,485),(1502,1050,485)))

    def test_passing_native_progress_does_not_trigger_debugger(self):
        self.assertIsNone(self.inspect((4803,4502,2401),(5103,4802,2551),(5401,5100,2699)))

    def test_duplicate_final_observation_does_not_trigger(self):
        self.assertIsNone(self.inspect((5401,5100,2699),(5401,5100,2699)))

    def test_initialization_and_short_interval_are_excluded(self):
        self.assertIsNone(self.inspect((0,0,0),(250,0,0),(301,10,10),(320,10,10)))
