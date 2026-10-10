#!/usr/bin/env python3
"""Run native keyboard gameplay in an isolated display and private data folder."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from check_gameplay_probe import check
from check_pak_save import check as check_pak

ROOT=Path(__file__).resolve().parents[1]

def run(args):
    if os.environ.get('DISPLAY') in (None,':0',':0.0'):
        raise ValueError('Use xvfb-run -a; this probe must not send input to your desktop')
    qa=ROOT/'.local/qa';qa.mkdir(parents=True,exist_ok=True)
    directory=Path(tempfile.mkdtemp(prefix='gameplay-',dir=qa))
    data=directory/'data';data.mkdir()
    source_hash=None
    pak_hash=None
    if args.seed_save:
        source_hash=hashlib.sha256(args.seed_save.read_bytes()).hexdigest()
        shutil.copyfile(args.seed_save,data/'eeprom.bin')
    if args.seed_pak:
        pak_hash=hashlib.sha256(args.seed_pak.read_bytes()).hexdigest()
        shutil.copyfile(args.seed_pak,data/'controller-pak.bin')
    env=dict(os.environ,BC_TRACE_INPUT='1',BC_TRACE_GAMEPLAY='1',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',VK_DRIVER_FILES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json')
    log=directory/'native.log'
    print(f'Private test directory: {directory}',flush=True)
    with log.open('w') as output:
        process=subprocess.Popen([str(ROOT/'build/blastcorps-prototype'),'--seconds',str(args.seconds),'--data-dir',str(data),'--scale','1','--wide','--refresh','120'],env=env,stdout=output,stderr=subprocess.STDOUT)
        def text():return log.read_text(errors='replace')
        def states():
            result=[]
            for line in text().splitlines():
                try:event=json.loads(line)
                except json.JSONDecodeError:continue
                if isinstance(event,dict) and event.get('event')=='guest_state':result.append(event)
            return result
        def wait(condition,timeout=20):
            deadline=time.monotonic()+timeout
            while time.monotonic()<deadline:
                value=text()
                if condition(value):return
                if process.poll() is not None:raise ValueError(f'Native process exited {process.returncode}')
                time.sleep(.1)
            raise ValueError('Native transition timed out: '+text()[-1800:])
        def command(*values):subprocess.run(['xdotool',*values],env=env,check=True,stdout=subprocess.DEVNULL)
        def key(name,hold=.4):
            command('keydown',name);time.sleep(hold);command('keyup',name);time.sleep(.3)
        def screenshot(name):subprocess.run(['import','-window',window,str(directory/f'{name}.png')],env=env,check=True)
        try:
            wait(lambda value:'rt64_device_ready' in value)
            window=subprocess.check_output(['xdotool','search','--pid',str(process.pid),'--name','Blast Corps'],env=env,text=True).split()[-1]
            command('windowfocus',window)
            wait(lambda value:'game mode switch from 4 to 5' in value)
            time.sleep(1);key('Return')
            wait(lambda value:'game mode switch from 58 to 1' in value)
            time.sleep(4)
            for attempt in range(12):
                if 'game mode switch from 1 to 53' in text():break
                key('Return' if attempt%2==0 else 'z',.6);time.sleep(1.5)
            wait(lambda value:' to 18\n' in value or 'l3d_adapter_ready' in value or '"state_lo":65536,"paused":0' in value)
            if 'l3d_adapter_ready' not in text() and states()[-1]['state_lo']==65536:
                time.sleep(2);screenshot('pak-player-selection');key('z')
                wait(lambda value:' to 18\n' in value or 'l3d_adapter_ready' in value)
            if 'l3d_adapter_ready' not in text():
                if args.require_existing_player:raise ValueError('Existing Pak player was not recognized; name-entry flow reached')
                time.sleep(2);screenshot('new-game')
                key('z');key('z');key('z');key('Return')
                wait(lambda value:'game mode switch from 24 to 44' in value)
                time.sleep(3);key('Return')
            wait(lambda value:'l3d_adapter_ready' in value)
            time.sleep(5);screenshot('map')
            print('Map reached; selecting Simian Acres',flush=True)
            key('z')
            wait(lambda value:'game mode switch from 13 to 2' in value,30)
            time.sleep(2);screenshot('mission-start')
            if args.exercise:
                key('z');time.sleep(1);screenshot('briefing-dismissed')
                before=states()[-1]
                key('z',2);screenshot('mission-move')
                after=states()[-1]
                if (before['x'],before['z'])==(after['x'],after['z']):raise ValueError('Vehicle position did not change during acceleration')
                key('Return');time.sleep(1);screenshot('pause')
                if not states()[-1]['paused']:raise ValueError('Start did not pause the guest')
                if args.options:
                    key('z');wait(lambda value:'"event":"pc_options_selection"' in value);time.sleep(2);screenshot('pc-options-root')
                    key('z');time.sleep(2);screenshot('pc-options-graphics')
                    key('z');time.sleep(2);screenshot('pc-options-saved')
                    settings=json.loads((data/'settings.json').read_text())
                    if settings.get('scale')!=4.5:raise ValueError('PC resolution selection was not saved')
                    key('Down');key('Down');key('Down');key('z');time.sleep(1)
                    key('Down');key('Down');key('z');time.sleep(2);screenshot('pc-options-filter')
                    wait(lambda value:'"event":"pc_filters_applied","presentation":"pixel"' in value)
                    key('Down');key('Down');key('Down');key('z');time.sleep(1)
                    key('z');time.sleep(2);screenshot('pc-options-texture')
                    wait(lambda value:'"event":"pc_filters_applied","presentation":"pixel","texture":"linear"' in value)
                    settings=json.loads((data/'settings.json').read_text())
                    if settings.get('presentation')!='pixel' or settings.get('texture')!='linear':raise ValueError('PC filters were not saved')
                    # Each page reuses the original closing/opening animation;
                    # wait for it before sending the next Back edge.
                    key('x');time.sleep(1);key('x');time.sleep(1)
                    key('x');time.sleep(1);key('Down');key('z');time.sleep(1);screenshot('pc-options-audio')
                    key('z');time.sleep(2)
                    settings=json.loads((data/'settings.json').read_text())
                    if settings.get('volume')!=0:raise ValueError('PC volume selection was not saved')
                    key('x');time.sleep(1);key('x');time.sleep(2);screenshot('pc-options-return')
                key('Return');time.sleep(1);key('Right',1);key('z',2);screenshot('resume')
                if states()[-1]['paused']:raise ValueError('Start did not resume the guest')
                print('Guest position changed; pause and resume confirmed by guest state',flush=True)
                if args.retry:
                    key('Return');time.sleep(1)
                    key('Down');screenshot('restart-focused');key('z');time.sleep(2);screenshot('restart-selected')
                    wait(lambda value:'game mode switch from 8 to 13' in value)
                    key('z');time.sleep(3);screenshot('restarted')
                    key('z');time.sleep(1)
                    print('Restart selected; transition and capture require review',flush=True)
                if args.wait_failure:
                    deadline=time.monotonic()+150
                    while time.monotonic()<deadline:
                        current=states()
                        if any(e['failed'] for e in current):break
                        if process.poll() is not None:break
                        if current and current[-1]['paused'] and current[-1]['window']>=70:
                            screenshot(f"tutorial-{current[-1]['window']}");key('z');time.sleep(1)
                        time.sleep(.3)
                    if not any(e['failed'] for e in states()):raise ValueError('No mission failure observed')
                    screenshot('mission-failure');print('Mission failure confirmed by guest state',flush=True)
            process.wait(timeout=args.seconds+15)
            if process.returncode!=0:raise ValueError(f'Native process exited {process.returncode}')
            result=check(log,required_modes=(14,0,2))
            result['seconds_requested']=args.seconds
            result['exercise_verified']=args.exercise
            result['options_verified']=args.options
            result['restart_verified']=args.retry
            result['failure_verified']=args.wait_failure
            result['seed_save_unchanged']=not args.seed_save or hashlib.sha256(args.seed_save.read_bytes()).hexdigest()==source_hash
            result['seed_pak_unchanged']=not args.seed_pak or hashlib.sha256(args.seed_pak.read_bytes()).hexdigest()==pak_hash
            result['existing_player_loaded']=args.require_existing_player
            if (data/'controller-pak.bin').exists():result['pak']=check_pak(data/'controller-pak.bin')
            if not result['seed_save_unchanged']:raise ValueError('Source save changed during isolated test')
            if not result['seed_pak_unchanged']:raise ValueError('Source Pak changed during isolated test')
            (directory/'result.json').write_text(json.dumps(result,indent=2)+'\n')
            print(json.dumps(result),flush=True)
        finally:
            if process.poll() is None:process.terminate();process.wait(timeout=10)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds',type=int,default=180)
    parser.add_argument('--seed-save',type=Path)
    parser.add_argument('--seed-pak',type=Path)
    parser.add_argument('--require-existing-player',action='store_true')
    parser.add_argument('--exercise',action='store_true')
    parser.add_argument('--retry',action='store_true')
    parser.add_argument('--options',action='store_true')
    parser.add_argument('--wait-failure',action='store_true')
    args=parser.parse_args()
    if args.seconds<90:parser.error('At least 90 seconds are required for the mission transition')
    if (args.retry or args.wait_failure or args.options) and not args.exercise:parser.error('Retry/failure/options needs --exercise')
    try:run(args)
    except (ValueError,subprocess.SubprocessError) as error:parser.exit(1,str(error)+'\n')
