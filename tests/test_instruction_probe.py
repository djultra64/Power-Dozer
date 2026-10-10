import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
_spec=importlib.util.spec_from_file_location('instruction_probe',Path(__file__).resolve().parents[1]/'tools/check_instruction_probe.py')
module=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(module)

class InstructionEvidence(unittest.TestCase):
    def events(self,controller=None):
        rows=[('PRESS START',['ENTER'],'PRESS ENTER'),('PRESSING START WILL',['ENTER'],'PRESSING ENTER WILL'),('USE Z/R TO MOVE MAP',['SHIFT','E'],'USE SHIFT/E TO MOVE MAP')]
        if controller:
            labels=module.CONTROLS[controller];rows.append(('USE Z/R TO MOVE MAP',labels,'USE '+'/'.join(labels)+' TO MOVE MAP'))
        result=[]
        for original,labels,text in rows:
            result.extend([dict(event='game_action_prompt',original=original,text=text),dict(event='game_action_glyphs',original=original,labels=labels,sprites=[module.SPRITES[label] for label in labels],x=2,y=23,size=9,alpha=255,commands=len(labels))])
        return result
    def check(self,events,controller=None):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'native.log';path.write_text('\n'.join(json.dumps(event) for event in events));return module.check(path,controller)
    def test_all_families(self):
        for controller in (None,*module.CONTROLS):self.assertTrue(self.check(self.events(controller),controller)['rendered_capture_review_required'])
    def test_no_commands_rejected(self):
        events=self.events();del events[1]['commands']
        with self.assertRaisesRegex(ValueError,'commands'):self.check(events)
    def test_missing_second_map_glyph_rejected(self):
        events=self.events();events[-1]['labels']=['SHIFT'];events[-1]['commands']=1;events[-1]['sprites']=['key_shift']
        with self.assertRaisesRegex(ValueError,'Missing glyphs'):self.check(events)
    def test_wrong_name_rejected(self):
        events=self.events('ps5');events[-2]['text']='USE LT/RB TO MOVE MAP'
        with self.assertRaisesRegex(ValueError,'disagrees'):self.check(events,'ps5')
    def test_wrong_atlas_sprite_rejected(self):
        events=self.events('switch');events[-1]['sprites']=['xbox_lt','xbox_rb']
        with self.assertRaisesRegex(ValueError,'Missing glyphs'):self.check(events,'switch')
    def test_missing_sprite_identity_rejected(self):
        events=self.events();del events[1]['sprites']
        with self.assertRaisesRegex(ValueError,'identities'):self.check(events)
    def test_out_of_bounds_rejected(self):
        events=self.events();events[1]['x']=319
        with self.assertRaisesRegex(ValueError,'bounds'):self.check(events)
    def test_empty_trace_rejected(self):
        with self.assertRaisesRegex(ValueError,'Missing glyphs'):self.check([])
