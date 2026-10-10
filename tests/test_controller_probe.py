import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
_spec=importlib.util.spec_from_file_location('controller_probe',Path(__file__).resolve().parents[1]/'tools/check_controller_probe.py')
module=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(module)

class NativeControllerEvidence(unittest.TestCase):
    def events(self,device='switch'):
        family,title,south,east,start=module.LAYOUTS[device]
        return [dict(event='qa_controller',phase='attached',family=device,held=False),dict(event='active_device',family=family),dict(event='pc_controls_view',title=title,rows=['ACCEL/OK: '+south,'BACK/REVERSE: '+east,'PAUSE: '+start,'NEXT PAGE']),dict(event='input',buttons=32768,stick_x=.666),dict(event='input',buttons=4096,stick_x=0),dict(event='active_device',family='keyboard'),dict(event='qa_controller',phase='detached'),dict(event='qa_controller',phase='attached',family=device,held=True),dict(event='qa_controller_request',serial=8,ok=True),dict(event='input',buttons=0,stick_x=0),dict(event='qa_controller_request',serial=9,ok=True)]
    def check(self,events,device='switch'):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'native.log';path.write_text('\n'.join(json.dumps(event) for event in events));return module.check(path,device)
    def test_all_layouts(self):
        for device in module.LAYOUTS:self.assertFalse(self.check(self.events(device),device)['physical_hardware_tested'])
    def test_swapped_nintendo_letters_rejected(self):
        events=self.events();events[2]['rows'][0]='ACCEL/OK: A'
        with self.assertRaisesRegex(ValueError,'layout'):self.check(events)
    def test_manual_only_capture_rejected(self):
        events=self.events();del events[0]
        with self.assertRaisesRegex(ValueError,'connections'):self.check(events)
    def test_no_guest_start_rejected(self):
        events=self.events();events[4]['buttons']=0
        with self.assertRaisesRegex(ValueError,'actual controller'):self.check(events)
    def test_held_button_action_rejected(self):
        events=self.events();events[9]['buttons']=32768
        with self.assertRaisesRegex(ValueError,'held reconnect'):self.check(events)
    def test_no_keyboard_return_rejected(self):
        events=self.events();del events[5]
        with self.assertRaisesRegex(ValueError,'handover'):self.check(events)
