import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('demolition_probe', Path(__file__).resolve().parents[1] / 'tools/check_demolition_probe.py')
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)

class DemolitionAcceptance(unittest.TestCase):
    def inspect(self, rows):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / 'native.log'
            log.write_text('\n'.join(json.dumps(row) for row in rows))
            return probe.check(log)

    def state(self, vi, buildings=0, money=0, **changes):
        return dict(event='guest_state', vi=vi, level=0, state_hi=0, state_lo=4, paused=0, failed=0, complete=0, window=-1, buildings_destroyed=buildings, money=money) | changes

    def test_damage_and_new_building_are_required(self):
        result = self.inspect([self.state(15), self.state(30, money=100), self.state(45, buildings=1, money=500)])
        self.assertEqual(result['buildings_destroyed_delta'], 1)
        self.assertEqual(result['money_delta'], 500)

    def test_existing_statistics_do_not_prove_destruction(self):
        with self.assertRaises(ValueError):self.inspect([self.state(15, 3, 500), self.state(30, 3, 500)])

    def test_damage_alone_is_insufficient(self):
        with self.assertRaises(ValueError):self.inspect([self.state(15), self.state(30, money=500)])

    def test_menu_and_failed_mission_do_not_count(self):
        for field in ('paused', 'failed', 'complete', 'state_hi', 'state_lo'):
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.inspect([self.state(15), self.state(30, 1, 500, **{field:1})])

    def test_level_transition_does_not_count(self):
        with self.assertRaises(ValueError):self.inspect([self.state(15), self.state(30, 1, 500, level=48)])

    def test_counter_reset_starts_new_episode(self):
        with self.assertRaises(ValueError):self.inspect([self.state(15, 5, 5000), self.state(30), self.state(45, 1, 0)])

    def test_invalid_counter_or_time_rejected(self):
        for row in (self.state(30, -1, 500), self.state(30, 1, float('nan')), self.state(10, 1, 500)):
            with self.assertRaises(ValueError):self.inspect([self.state(15), row])
