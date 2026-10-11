#!/usr/bin/env python3
"""Check native virtual-controller prompts alongside actual guest action traces."""
import argparse
import json
from pathlib import Path

LAYOUTS={'xbox360':('xbox','XBOX HELP','A','B','START'),'ps5':('playstation','PLAYSTATION HELP','CROSS','CIRCLE','OPTIONS'),'switch':('switch','SWITCH HELP','B','A','PLUS')}

def check(path,device):
    family,title,south,east,start=LAYOUTS[device]
    events=[]
    for line in Path(path).read_text(errors='replace').splitlines():
        try:event=json.loads(line)
        except json.JSONDecodeError:continue
        if isinstance(event,dict):events.append(event)
    attached=[event for event in events if event.get('event')=='qa_controller' and event.get('phase')=='attached']
    if len(attached)!=2 or any(event.get('family')!=device for event in attached) or [event.get('held') for event in attached]!=[False,True]:
        raise ValueError('Missing initial and held-button virtual-controller connections')
    if sum(event.get('event')=='qa_controller' and event.get('phase')=='detached' for event in events)!=1:
        raise ValueError('Missing virtual-controller disconnection')
    if any(event.get('event')=='qa_controller_request' and event.get('ok') is not True for event in events):
        raise ValueError('A private controller request failed')
    expected=[('ACCEL/OK',south),('BACK/REVERSE',east),('PAUSE',start)]
    sprites={'xbox360':['xbox_a','xbox_b','xbox_menu'],'ps5':['playstation_cross','playstation_circle','playstation_options'],'switch':['switch_b','switch_a','switch_plus']}[device]
    views=[event for event in events if event.get('event')=='pc_settings_view' and event.get('open') and event.get('section')=='CONTROLS' and event.get('device')=='CONTROLLER']
    if not any(len(event.get('rows',[]))==14 and all(row.get('label')==label and row.get('value')==value and row.get('binding',{}).get('label')==value and row.get('binding',{}).get('glyph')==sprite
               for row,(label,value),sprite in zip(event['rows'],expected,sprites)) for event in views):
        raise ValueError('Detected controller action captions or glyphs disagree with its layout')
    source='keyboard';confirm=False;pause=False;steering=False;keyboard_return=False;held=False;acks=0
    for event in events:
        kind=event.get('event')
        if kind=='active_device':
            if source==family and event.get('family')=='keyboard':keyboard_return=True
            source=event.get('family')
        elif kind=='qa_controller' and event.get('phase')=='attached' and event.get('held'):
            held=True;acks=0
        elif kind=='qa_controller_request' and held:
            acks+=1
            if acks==2:held=False
        elif kind=='input':
            if held and event.get('buttons',0):raise ValueError('A held reconnect button leaked a guest action')
            if source==family:
                confirm|=event.get('buttons')==32768
                pause|=event.get('buttons')==4096
                steering|=abs(event.get('stick_x',0))>.5 and event.get('buttons')==32768
    if not (confirm and pause and steering and keyboard_return):
        raise ValueError('Missing actual controller Confirm/Start/steering or keyboard handover')
    return {'virtual_controller_action_prompts_verified':device,'physical_hardware_tested':False}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('log',type=Path);parser.add_argument('device',choices=LAYOUTS);args=parser.parse_args()
    try:print(json.dumps(check(args.log,args.device)))
    except ValueError as error:parser.exit(1,str(error)+'\n')
