#!/usr/bin/env python3
"""Check emitted original-instruction glyph commands alongside mapped names."""
import argparse
import json
from pathlib import Path

CONTROLS={'xbox360':['LT','RB'],'ps5':['L2','R1'],'switch':['ZL','R']}

def check(path,controller=None):
    events=[]
    for line in Path(path).read_text(errors='replace').splitlines():
        try:event=json.loads(line)
        except json.JSONDecodeError:continue
        if isinstance(event,dict):events.append(event)
    glyphs=[event for event in events if event.get('event')=='game_action_glyphs']
    for event in glyphs:
        labels=event.get('labels');size=event.get('size',0)
        if not isinstance(labels,list) or not 1<=len(labels)<=2 or not 8<=size<=16 or not 1<=event.get('alpha',0)<=255 or event.get('commands',0)<=10:
            raise ValueError('Invalid or missing actual instruction glyph commands')
        if not 0<=event.get('x',-1)<=320-len(labels)*(size+2)+2 or not 0<=event.get('y',-1)<=240-size:
            raise ValueError('Instruction glyphs exceed screen bounds')
    required=[('PRESS START',['ENTER'],'PRESS ENTER'),('PRESSING START WILL',['ENTER'],'PRESSING ENTER WILL'),('USE Z/R TO MOVE MAP',['SHIFT','E'],'USE SHIFT/E TO MOVE MAP')]
    if controller:
        labels=CONTROLS[controller];required.append(('USE Z/R TO MOVE MAP',labels,'USE '+'/'.join(labels)+' TO MOVE MAP'))
    for original,labels,text in required:
        if not any(event.get('original')==original and event.get('labels')==labels for event in glyphs):
            raise ValueError('Missing glyphs for '+original+' / '+','.join(labels))
        if not any(event.get('event')=='game_action_prompt' and event.get('original')==original and event.get('text')==text for event in events):
            raise ValueError('Mapped instruction text disagrees with glyph labels')
    return {'original_instruction_commands_verified':True,'controller':controller,'instructions_observed':len(required),'rendered_capture_review_required':True}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('log',type=Path);parser.add_argument('--controller',choices=CONTROLS);args=parser.parse_args()
    try:print(json.dumps(check(args.log,args.controller)))
    except ValueError as error:parser.exit(1,str(error)+'\n')
