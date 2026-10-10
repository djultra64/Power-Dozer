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
