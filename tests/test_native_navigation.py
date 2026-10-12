import importlib.util
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
spec=importlib.util.spec_from_file_location('native_navigation',Path(__file__).resolve().parents[1]/'tools/probe_gameplay.py')
probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)
class NativeNavigation(unittest.TestCase):
    def test_guest_files_state_does_not_establish_host_ownership(self):
        self.assertIsNone(probe.frontend_screen('{"event":"guest_state","state_lo":65536}\n'))
    def test_latest_explicit_owner_supersedes_old_main(self):
        log='{"event":"frontend","screen":"main"}\n{"event":"frontend","screen":"files"}\n'
        self.assertEqual(probe.frontend_screen(log),'files')
    def test_back_and_settings_returns_establish_main_again(self):
        for event in ('main','main-options-return'):
            self.assertEqual(probe.frontend_screen('{"event":"frontend","screen":"'+event+'"}\n'),'main')
    def test_unrelated_invalid_and_non_object_lines_are_ignored(self):
        self.assertEqual(probe.frontend_screen('guest noise\n[]\n{"event":"frontend","screen":"main"}\n{"event":"pc_settings_view","open":true}\n{bad'), 'main')

    def test_wide_letterbox_mouse_uses_active_image(self):
        self.assertEqual(probe.reference_point(960,720,True,80,166),(210,277))
    def test_original_letterbox_mouse_centers_four_three_image(self):
        self.assertEqual(probe.reference_point(1280,720,False,80,166),(280,249))
