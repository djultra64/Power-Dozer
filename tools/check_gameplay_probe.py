#!/usr/bin/env python3
"""Reject crashes, guest assertions, incomplete coverage and frozen rendering."""
import argparse
import json
import re
import importlib.util
from pathlib import Path
_spec=importlib.util.spec_from_file_location('bc_audio_probe',Path(__file__).with_name('check_audio_probe.py'))
check_audio_probe=importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(check_audio_probe)

ASSERTION=re.compile(r'ASSERTION\s+FAULT|assertion[^\n]*failed',re.I)

def original_fade_diagnostics(content):
    """Recognize only the ROM's bounded cancel→restore, retaining its diagnostics.

    Original UpdateDrawFade calls yoshiShow(0) at 80274CB8 after saving the
    pending demand, then restores it at 802750F0. No guest state is patched.
    Any other assertion, malformed scope or missing restoration still fails.
    """
    lines=content.splitlines();allowed=set();candidates=[];scope=None;off_scope=None;off_count=0
    for index,line in enumerate(lines):
        try:event=json.loads(line)
        except json.JSONDecodeError:continue
        if not isinstance(event,dict):continue
        if event.get('event')=='original_window_off':
            if event.get('phase')=='begin':off_scope=(index,event)
            elif event.get('phase')=='end' and off_scope:
                start,begin=off_scope;off_scope=None
                if not (begin.get('pc')=='8026BF7C' and event.get('pc')=='8026BF7C' and begin.get('pending')==event.get('pending')==16384 and begin.get('state_hi')==event.get('state_hi')==0 and begin.get('state_lo')==event.get('state_lo')==512 and begin.get('failed')==event.get('failed')==1):continue
                assertions=[i for i in range(start+1,index) if ASSERTION.search(lines[i])]
                expected=[f'ASSERTION FAULT - !yoshiDemandV - yoshi.c, line {number}' for number in (1312,1331)]
                block='\n'.join(lines[start+1:index])
                if len(assertions)==2 and all(text in lines[i] for i,text in zip(assertions,expected)) and 'NEW: 4000 OLD:4000' in block and 'GOING FOR NEW: 4000 OLD:4000' in block:
                    allowed.update(assertions);off_count+=1
            continue
        if event.get('event')!='original_fade_window':continue
        phase=event.get('phase')
        if phase=='begin':scope=(index,event)
        elif phase=='end' and scope:
            start,begin=scope;scope=None
            pending=begin.get('pending',0)
            state=(begin.get('state_hi',0)<<32)|begin.get('state_lo',0)
            if not (pending and begin.get('saved')==pending and begin.get('demand')==0 and begin.get('frame')==0 and state&0x51800100040000 and event.get('pending')==0 and event.get('saved')==pending):continue
            assertions=[i for i in range(start+1,index) if ASSERTION.search(lines[i])]
            expected=[f'ASSERTION FAULT - !yoshiDemandV - yoshi.c, line {number}' for number in (1312,1331)]
            if len(assertions)!=2 or any(text not in lines[i] for i,text in zip(assertions,expected)):continue
            block='\n'.join(lines[start+1:index])
            if f'NEW: 0 OLD:{pending:x}' not in block or f'GOING FOR NEW: 0 OLD:{pending:x}' not in block:continue
            candidates.append((begin,assertions))
        elif phase=='restore':
            for candidate in list(candidates):
                begin,assertions=candidate
                elapsed=event.get('vi',-1)-begin.get('vi',0)
                if (event.get('demand')==begin['pending'] and event.get('saved')==begin['pending'] and event.get('pending')==0 and event.get('state_hi')==begin.get('state_hi') and event.get('state_lo')==begin.get('state_lo') and 0<elapsed<=180):
                    allowed.update(assertions);candidates.remove(candidate);break
    if any(ASSERTION.search(line) and index not in allowed for index,line in enumerate(lines)):
        raise ValueError('Guest assertion fault: run is not release acceptance')
    return len(allowed)//2-off_count,off_count

def check(path, required_modes=()):
    content=Path(path).read_text(errors='replace')
    fade_count,off_count=original_fade_diagnostics(content)
    if re.search(r'Segmentation fault|core dumped|terminate called|XIO:.*fatal',content,re.I):
        raise ValueError('Native crash reported')
    check_audio_probe.check(path)
    events=[]
    for line in content.splitlines():
        try: event=json.loads(line)
        except json.JSONDecodeError: continue
        if isinstance(event,dict):events.append(event)
    progress=[e for e in events if e.get('event') in ('progress','probe_end') and e.get('vi',0)>=300]
    for before,after in zip(progress,progress[1:]):
        if after['vi']-before['vi']>=60 and after.get('gfx',0)<=before.get('gfx',0):
            raise ValueError(f"Graphics stopped at VI {before['vi']}–{after['vi']}")
    modes={int(mode) for mode in re.findall(r'game mode switch from -?\d+ to (-?\d+)',content)}
    for mode in required_modes:
        if mode not in modes:raise ValueError(f'Missing required game mode {mode}')
    return {key:progress[-1][key] for key in ('vi','gfx','audio')}|{'modes':sorted(modes),'original_fade_diagnostics':fade_count,'original_off_diagnostics':off_count}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=Path)
    parser.add_argument('--require-mode',type=int,action='append',default=[])
    args=parser.parse_args()
    try:print(json.dumps(check(args.log,args.require_mode)))
    except ValueError as error:parser.exit(1,str(error)+'\n')
