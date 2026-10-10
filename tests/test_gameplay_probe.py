import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

script=Path(__file__).resolve().parents[1]/'tools/check_gameplay_probe.py'
spec=importlib.util.spec_from_file_location('gameplay_probe',script)
probe=importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)

class GameplayAcceptance(unittest.TestCase):
    def log(self, extra='', frozen_graphics=False):
        lines=['guest: game mode switch from 53 to 14','guest: game mode switch from 11 to 0',json.dumps({'event':'audio_signal','peak':12})]
        for vi in (301,601,901,1201):
            lines.append(json.dumps({'event':'progress','vi':vi,'gfx':9 if frozen_graphics else vi,'audio':vi//2}))
        lines.append(json.dumps({'event':'probe_end','vi':1201,'gfx':9 if frozen_graphics else 1201,'audio':600}))
        return '\n'.join(lines)+'\n'+extra

    def inspect(self,text):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'game.log';path.write_text(text)
            return probe.check(path,required_modes=(14,0))

    def test_complete_map_and_gameplay_is_accepted(self):
        result=self.inspect(self.log());self.assertEqual(result['vi'],1201)

    def test_guest_assertion_rejects_even_normal_exit(self):
        with self.assertRaisesRegex(ValueError,'assertion'):
            self.inspect(self.log('guest: --- ASSERTION FAULT - !yoshiDemandV - yoshi.c, line 1312'))

    def test_frozen_graphics_with_running_audio_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'Graphics stopped'):
            self.inspect(self.log(frozen_graphics=True))

    def test_missing_mission_transition_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'mode 0'):
            self.inspect(self.log().replace('guest: game mode switch from 11 to 0',''))

    def test_crash_text_rejects_even_complete_counters(self):
        with self.assertRaisesRegex(ValueError,'crash'):
            self.inspect(self.log('Segmentation fault (core dumped)'))

    def fade(self, restore=True, extra=''):
        begin={'event':'original_fade_window','phase':'begin','pending':32779,'saved':32779,'demand':0,'frame':0,'state_hi':0,'state_lo':262144,'vi':100}
        end=begin|{'phase':'end','pending':0}
        restored=begin|{'phase':'restore','pending':0,'demand':32779,'frame':33,'vi':133}
        lines=[json.dumps(begin),' --- ASSERTION FAULT - !yoshiDemandV - yoshi.c, line 1312','guest: NEW: 0 OLD:800b',' --- ASSERTION FAULT - !yoshiDemandV - yoshi.c, line 1331','guest: GOING FOR NEW: 0 OLD:800b',extra,json.dumps(end)]
        if restore:lines.append(json.dumps(restored))
        return '\n'.join(lines)

    def test_original_fade_cancel_and_restore_is_classified(self):
        result=self.inspect(self.log(self.fade()))
        self.assertEqual(result['original_fade_diagnostics'],1)

    def test_fade_without_restore_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'assertion'):
            self.inspect(self.log(self.fade(restore=False)))

    def test_other_assertion_in_fade_scope_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'assertion'):
            self.inspect(self.log(self.fade(extra=' --- ASSERTION FAULT - found - mb.c, line 157')))

    def test_unexpected_yoshi_assertion_after_fade_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'assertion'):
            self.inspect(self.log(self.fade()+'\n --- ASSERTION FAULT - !yoshiDemandV - yoshi.c, line 1312'))

    def test_inconsistent_cancel_marker_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'assertion'):
            self.inspect(self.log(self.fade().replace('"saved": 32779','"saved": 32780')))

    def test_restore_after_bounded_fade_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'assertion'):
            self.inspect(self.log(self.fade().replace('"vi": 133','"vi": 301')))

    def test_restore_in_different_game_state_is_rejected(self):
        lines=self.fade().splitlines();event=json.loads(lines[-1]);event['state_lo']=4;lines[-1]=json.dumps(event)
        with self.assertRaisesRegex(ValueError,'assertion'):
            self.inspect(self.log('\n'.join(lines)))

    def off(self, pending=16384):
        begin={'event':'original_window_off','phase':'begin','pc':'8026BF7C','pending':pending,'failed':1,'state_hi':0,'state_lo':512}
        end=begin|{'phase':'end','pending':16384}
        return '\n'.join([json.dumps(begin),' --- ASSERTION FAULT - !yoshiDemandV - yoshi.c, line 1312',f'guest: NEW: 4000 OLD:{pending:x}',' --- ASSERTION FAULT - !yoshiDemandV - yoshi.c, line 1331',f'guest: GOING FOR NEW: 4000 OLD:{pending:x}',json.dumps(end)])

    def test_original_duplicate_off_demand_is_classified(self):
        self.assertEqual(self.inspect(self.log(self.off()))['original_off_diagnostics'],1)

    def test_original_off_must_not_replace_another_pending_window(self):
        with self.assertRaisesRegex(ValueError,'assertion'):self.inspect(self.log(self.off(32768)))
