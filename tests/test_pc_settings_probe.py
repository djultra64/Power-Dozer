import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
_spec=importlib.util.spec_from_file_location('pc_settings_probe',Path(__file__).resolve().parents[1]/'tools/check_pc_settings_probe.py')
module=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(module)

class CompleteSettingsEvidence(unittest.TestCase):
    def events(self):
        sections={
            'DISPLAY':['INTERNAL RESOLUTION','ASPECT RATIO','FRAME RATE','WINDOW MODE'],
            'GRAPHICS':['ANTI-ALIASING','SCREEN FILTER','TEXTURE FILTER'],
            'AUDIO':['MASTER VOLUME','MUTE'],
            'CONTROLS':['ACCEL/OK','BACK/REVERSE','PAUSE','BRAKE/EXIT','CAM LEFT','CAM DOWN','CAM UP','CAM RIGHT','SPECIAL L','SPECIAL R','MOVE LEFT','MOVE RIGHT','MOVE UP','MOVE DOWN']}
        events=[]
        for width in (960,1280):
            for section,labels in sections.items():
                events.append(dict(event='pc_settings_view',open=True,section=section,modal='none',width=width,height=720,rows=[dict(label=x,value='VALUE') for x in labels]))
            for modal in ('capture','defaults'):
                events.append(dict(events[-1],modal=modal))
            events.append(dict(event='pc_settings_view',open=False))
        return events
    def check(self,events):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'native.log';path.write_text('\n'.join(json.dumps(x) for x in events));return module.check(path)
    def test_complete_screens_both_aspects(self):
        self.assertTrue(self.check(self.events()).get('native_capture_review_required'))
    def test_old_four_row_controls_rejected(self):
        events=self.events()
        for event in events:
            if event.get('section')=='CONTROLS':event['rows']=event['rows'][:3]+[dict(label='MORE',value='')]
        with self.assertRaisesRegex(ValueError,'CONTROLS'):self.check(events)
    def test_missing_modal_rejected(self):
        with self.assertRaisesRegex(ValueError,'modal'):self.check([x for x in self.events() if x.get('modal')!='capture'])
    def test_missing_wide_audio_rejected(self):
        with self.assertRaisesRegex(ValueError,'AUDIO'):self.check([x for x in self.events() if not(x.get('section')=='AUDIO' and x.get('width')==1280)])
    def test_no_reopen_rejected(self):
        with self.assertRaisesRegex(ValueError,'reopen'):self.check([x for x in self.events() if x.get('open')])
    def test_empty_rejected(self):
        with self.assertRaises(ValueError):self.check([])

class SettingsReloadEvidence(unittest.TestCase):
    def check(self, events):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'reload.log';path.write_text('\n'.join(json.dumps(x) for x in events))
            return module.check_reload(path,{'scale':4.5,'wide':False,'volume':40},True)
    def events(self):
        return [dict(event='effective_settings',scale=4.5,wide=False,volume=40),dict(event='input',buttons=32768)]
    def test_saved_settings_and_remapped_confirm_reload(self):
        self.assertTrue(self.check(self.events())['saved_settings_reloaded'])
    def test_cli_or_default_value_does_not_count(self):
        events=self.events();events[0]['volume']=100
        with self.assertRaisesRegex(ValueError,'volume'):self.check(events)
    def test_remapped_key_must_activate_confirm(self):
        with self.assertRaisesRegex(ValueError,'Confirm'):self.check(self.events()[:1])

class LiveFilterEvidence(unittest.TestCase):
    def content(self, pairs):
        return "\n".join(json.dumps(dict(event='pc_filters_applied',presentation=p,texture=t)) for p,t in pairs)
    def test_saved_only_is_not_live_application(self):
        self.assertFalse(module.filters_applied(json.dumps(dict(event='effective_settings',presentation='pixel',texture='linear')), 'pixel', 'linear'))
    def test_correct_event_and_both_fields_required(self):
        content=self.content([('pixel','three_point')])
        self.assertTrue(module.filters_applied(content,'pixel','three_point'))
        self.assertFalse(module.filters_applied(content,'pixel','linear'))
    def test_all_changes_must_apply_in_order(self):
        pairs=[('pixel','three_point'),('linear','three_point'),('linear','linear'),('linear','three_point')]
        self.assertTrue(module.check_live_filters(self.content(pairs)))
        with self.assertRaisesRegex(ValueError,'live filter'):module.check_live_filters(self.content(pairs[:-1]))
