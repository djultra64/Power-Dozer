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

def check(path, required_modes=()):
    content=Path(path).read_text(errors='replace')
    if re.search(r'ASSERTION\s+FAULT|assertion[^\n]*failed',content,re.I):
        raise ValueError('Guest assertion fault: run is not release acceptance')
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
    return {key:progress[-1][key] for key in ('vi','gfx','audio')}|{'modes':sorted(modes)}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=Path)
    parser.add_argument('--require-mode',type=int,action='append',default=[])
    args=parser.parse_args()
    try:print(json.dumps(check(args.log,args.require_mode)))
    except ValueError as error:parser.exit(1,str(error)+'\n')
