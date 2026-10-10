import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('render_probe', Path(__file__).resolve().parents[1] / 'tools/check_render_probe.py')
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


class RenderAcceptance(unittest.TestCase):
    def inspect(self, fps=120, fractional=180, failed=0, paused=False, count=12):
        rows = ['guest: game mode switch from 13 to 2', json.dumps({'event': 'guest_state', 'paused': int(paused), 'failed': 0, 'complete': 0, 'window': -1})]
        for i in range(count):
            rows.append(json.dumps({'event': 'rt64_actual_frames', 'vi': (i + 1) * 120, 'seconds': 2, 'successful_present_submissions': fps * 2, 'failed_present_submissions': failed, 'present_submission_fps': fps, 'fractional_renders': fractional, 'mapped_transforms': 400}))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'native.log'
            path.write_text('\n'.join(rows))
            return probe.check(path)

    def test_actual_interpolated_output_is_accepted(self):
        self.assertEqual(self.inspect()['minimum_fps'], 120)

    def test_generated_frames_with_thirty_presentations_fail(self):
        with self.assertRaisesRegex(ValueError, 'presentation rate'):
            self.inspect(fps=30)

    def test_repeated_original_frames_are_not_interpolation(self):
        with self.assertRaisesRegex(ValueError, 'fractional'):
            self.inspect(fractional=0)

    def test_failed_swapchain_submission_rejects_run(self):
        with self.assertRaisesRegex(ValueError, 'failed'):
            self.inspect(failed=1)

    def test_pause_does_not_count_as_mission_acceptance(self):
        with self.assertRaisesRegex(ValueError, 'coverage'):
            self.inspect(paused=True)

    def test_short_trial_has_insufficient_coverage(self):
        with self.assertRaisesRegex(ValueError, 'coverage'):
            self.inspect(count=3)

    def test_nonfinite_rate_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'finite'):
            self.inspect(fps=float('nan'))
