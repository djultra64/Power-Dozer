#!/usr/bin/env python3
"""Verify private startup/output-loss injection and uninterrupted guest audio."""
import argparse
import json
from pathlib import Path

def check(path):
    events=[]
    for line in Path(path).read_text(errors='replace').splitlines():
        try:event=json.loads(line)
        except json.JSONDecodeError:continue
        if isinstance(event,dict):events.append(event)
    status=[(i,event.get('available')) for i,event in enumerate(events) if event.get('event')=='audio_output_status']
    if [state for _,state in status]!=[False,True,False,True]:
        raise ValueError('Missing startup unavailability, recovery, loss or final recovery')
    faults=[i for i,event in enumerate(events) if event.get('event')=='qa_audio_fault' and event.get('phase') in ('queue_failure','device_removed')]
    if len(faults)!=1 or not status[1][0]<faults[0]<status[2][0]<status[3][0]:
        raise ValueError('Private fault was not observed by the native audio bridge')
    loss,recovered=status[2][0],status[3][0]
    before=[event for event in events[:loss] if event.get('event')=='progress']
    during=[event for event in events[loss:recovered] if event.get('event')=='progress']
    if not before or not during:
        raise ValueError('Outage needs a progress observation to verify continuity')
    for key in ('vi','gfx','audio'):
        if during[-1].get(key,0)<=before[-1].get(key,0):
            raise ValueError('Guest '+key+' did not advance during output loss')
    return {'simulated_startup_and_output_loss_verified':True,'fault_kind':events[faults[0]]['phase'],'audio_tasks_during_outage':during[-1]['audio']-before[-1]['audio']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('log',type=Path);args=parser.parse_args()
    try:print(json.dumps(check(args.log)))
    except ValueError as error:parser.exit(1,str(error)+'\n')
