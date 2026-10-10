import importlib.util
import math
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('waypoint_input', Path(__file__).resolve().parents[1] / 'tools/waypoint_input.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)

class WaypointInput(unittest.TestCase):
    def test_world_axes_at_zero_camera_yaw(self):
        for delta, expected in [((1,0),(32767,0)),((-1,0),(-32767,0)),((0,1),(0,32767)),((0,-1),(0,-32767))]:
            with self.subTest(delta=delta):self.assertEqual(helper.axes(*delta,0),expected)

    def test_camera_rotation_changes_physical_stick(self):
        x,y = helper.axes(0,-100,90)
        self.assertEqual((x,y),(32767,0))
        self.assertEqual(helper.axes(0,-100,360),helper.axes(0,-100,0))

    def test_diagonal_is_normalized_and_opposite_is_symmetric(self):
        a = helper.axes(100,100,135)
        b = helper.axes(-100,-100,135)
        self.assertAlmostEqual(math.hypot(*a),32767,delta=1)
        self.assertEqual(a,tuple(-v for v in b))

    def test_arrival_and_nonfinite_values(self):
        self.assertEqual(helper.axes(0,0,45),(0,0))
        for values in ((math.nan,0,0),(1,math.inf,0),(1,0,math.nan)):
            with self.assertRaises(ValueError):helper.axes(*values)

    def test_ramdozer_uses_relative_turning_and_wraps_heading(self):
        self.assertEqual(helper.relative_axes(0,100,0),(0,0))
        self.assertEqual(helper.relative_axes(100,0,0),(-32767,0))
        self.assertEqual(helper.relative_axes(-100,0,0),(32767,0))
        self.assertEqual(helper.relative_axes(0,100,4090),(0,0))
        with self.assertRaises(ValueError):helper.relative_axes(1,0,float('nan'))
