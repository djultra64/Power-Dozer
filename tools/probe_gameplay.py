#!/usr/bin/env python3
"""Run native keyboard gameplay in an isolated display and private data folder."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from check_gameplay_probe import check
from check_pak_save import check as check_pak
from check_audio_recovery import check as check_audio_recovery
from check_controller_probe import check as check_controller_probe
from check_instruction_probe import check as check_instruction_probe
from check_render_probe import check as check_render_probe

ROOT=Path(__file__).resolve().parents[1]

def run(args):
    if os.environ.get('DISPLAY') in (None,':0',':0.0'):
        raise ValueError('Use xvfb-run -a; this probe must not send input to your desktop')
    if args.gpu=='radv':
        extensions=subprocess.check_output(['xdpyinfo'],text=True)
        if 'DRI3' not in {line.strip() for line in extensions.splitlines()}:
            raise ValueError('Radeon presentation requires DRI3. Use a private headless cage/Xwayland display; Xvfb supports the software driver only.')
    qa=ROOT/'.local/qa';qa.mkdir(parents=True,exist_ok=True)
    directory=Path(tempfile.mkdtemp(prefix='gameplay-',dir=qa))
    data=directory/'data';data.mkdir()
    if args.glyphs:(data/'settings.json').write_text(json.dumps({'family':args.glyphs})+'\n')
    source_hash=None
    pak_hash=None
    if args.seed_save:
        source_hash=hashlib.sha256(args.seed_save.read_bytes()).hexdigest()
        shutil.copyfile(args.seed_save,data/'eeprom.bin')
    if args.seed_pak:
        pak_hash=hashlib.sha256(args.seed_pak.read_bytes()).hexdigest()
        shutil.copyfile(args.seed_pak,data/'controller-pak.bin')
    driver={'software':'/usr/share/vulkan/icd.d/lvp_icd.x86_64.json','radv':'/usr/share/vulkan/icd.d/radeon_icd.x86_64.json'}[args.gpu]
    env=dict(os.environ,BC_TRACE_INPUT='1',BC_TRACE_GAMEPLAY='1',BC_TRACE_RENDER='1',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',VK_DRIVER_FILES=driver)
    if args.audio_loss:
        subprocess.run(['c++','-std=c++20','-shared','-fPIC','-I/home/linuxbrew/.linuxbrew/include/SDL2','tools/qa_audio_fault.cpp','-ldl','-L/home/linuxbrew/.linuxbrew/lib','-lSDL2','-Wl,-rpath,/home/linuxbrew/.linuxbrew/lib','-o',str(directory/'libaudio-fault.so')],cwd=ROOT,check=True)
        # ld.so splits LD_PRELOAD on spaces, including spaces inside paths.
        # Use a relative path without spaces and set the native working folder.
        env.update(LD_PRELOAD=str((directory/'libaudio-fault.so').relative_to(ROOT)),BC_QA_AUDIO_FAULT_AFTER='45',BC_QA_AUDIO_OUTAGE='8',BC_QA_AUDIO_STARTUP_OUTAGE='3',BC_QA_AUDIO_FAULT_KIND=args.audio_loss)
    if args.controller:
        subprocess.run(['c++','-std=c++20','-shared','-fPIC','-I/home/linuxbrew/.linuxbrew/include/SDL2','tools/qa_controller.cpp','-ldl','-L/home/linuxbrew/.linuxbrew/lib','-lSDL2','-Wl,-rpath,/home/linuxbrew/.linuxbrew/lib','-o',str(directory/'libcontroller.so')],cwd=ROOT,check=True)
        preload=str((directory/'libcontroller.so').relative_to(ROOT))
        if env.get('LD_PRELOAD'):preload+=':'+env['LD_PRELOAD']
        env.update(LD_PRELOAD=preload,BC_QA_CONTROLLER=args.controller,BC_QA_INPUT_FILE=str(directory/'controller-input'))
    log=directory/'native.log'
    print(f'Private test directory: {directory}',flush=True)
    confirm='z'
    prompt_captured=False
    with log.open('w') as output:
        process=subprocess.Popen([str(ROOT/'build/blastcorps-prototype'),'--rom',str(ROOT/'.local/rom.z64'),'--seconds',str(args.seconds),'--data-dir',str(data),'--scale',str(args.scale),'--original-aspect' if args.original_aspect else '--wide','--refresh',str(args.refresh),'--texture',args.texture],cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT)
        def text():return log.read_text(errors='replace')
        def states():
            result=[]
            for line in text().splitlines():
                try:event=json.loads(line)
                except json.JSONDecodeError:continue
                if isinstance(event,dict) and event.get('event')=='guest_state':result.append(event)
            return result
        def wait(condition,timeout=20):
            nonlocal prompt_captured
            deadline=time.monotonic()+timeout
            while time.monotonic()<deadline:
                value=text()
                if args.prompts and not prompt_captured:
                    for line in value.splitlines():
                        try:event=json.loads(line)
                        except json.JSONDecodeError:continue
                        if isinstance(event,dict) and event.get('event')=='game_action_prompt' and event.get('original')=='PRESS START' and event.get('text')=='PRESS ENTER':
                            time.sleep(.25);screenshot('mission-action-prompt')
                            time.sleep(.25);screenshot('mission-action-prompt-next')
                            prompt_captured=True
                            break
                if condition(value):return
                if process.poll() is not None:raise ValueError(f'Native process exited {process.returncode}')
                time.sleep(.1)
            raise ValueError('Native transition timed out: '+text()[-1800:])
        def control_view(content,title,first=None):
            for line in reversed(content.splitlines()):
                try:event=json.loads(line)
                except json.JSONDecodeError:continue
                if isinstance(event,dict) and event.get('event')=='pc_controls_view':
                    return event.get('title')==title and (first is None or event.get('rows',[None])[0]==first)
            return False
        def command(*values):subprocess.run(['xdotool',*values],env=env,check=True,stdout=subprocess.DEVNULL)
        def key(name,hold=.4):
            command('keydown',name);time.sleep(hold);command('keyup',name);time.sleep(.3)
        def screenshot(name):subprocess.run(['import','-window',window,str(directory/f'{name}.png')],env=env,check=True)
        controller_serial=0
        def pad(kind,name='unused',value=0):
            nonlocal controller_serial
            controller_serial+=1
            request=directory/'controller-input';temporary=directory/'controller-input.tmp'
            temporary.write_text(f'{controller_serial} {kind} {name} {value}\n');temporary.replace(request)
            wait(lambda content:f'"serial":{controller_serial},"ok":true' in content)
            time.sleep(.15)
        def button(name,hold=.4):
            pad('button',name,1);time.sleep(hold);pad('button',name,0);time.sleep(.3)
        try:
            wait(lambda value:'rt64_device_ready' in value)
            window=subprocess.check_output(['xdotool','search','--pid',str(process.pid),'--name','Blast Corps'],env=env,text=True).split()[-1]
            command('windowfocus',window)
            wait(lambda value:'game mode switch from 4 to 5' in value)
            time.sleep(1);screenshot('title-controls');key('Return')
            wait(lambda value:'game mode switch from 58 to 1' in value)
            time.sleep(4)
            for attempt in range(12):
                if 'game mode switch from 1 to 53' in text():break
                key('Return' if attempt%2==0 else 'z',.6);time.sleep(1.5)
            wait(lambda value:' to 18\n' in value or 'l3d_adapter_ready' in value or '"state_lo":65536,"paused":0' in value)
            if 'l3d_adapter_ready' not in text() and states()[-1]['state_lo']==65536:
                time.sleep(2);screenshot('pak-player-selection');key(confirm)
                wait(lambda value:' to 18\n' in value or 'l3d_adapter_ready' in value)
            if 'l3d_adapter_ready' not in text():
                if args.require_existing_player:raise ValueError('Existing Pak player was not recognized; name-entry flow reached')
                time.sleep(2);screenshot('new-game')
                key(confirm);key(confirm);key(confirm);key('Return')
                wait(lambda value:'game mode switch from 24 to 44' in value)
                time.sleep(3);key('Return')
            wait(lambda value:'l3d_adapter_ready' in value)
            time.sleep(5);screenshot('map')
            print('Map reached; selecting Simian Acres',flush=True)
            key(confirm)
            wait(lambda value:'game mode switch from 13 to 2' in value or 'game mode switch from 14 to 7' in value,30)
            if 'game mode switch from 14 to 7' in text():
                wait(lambda value:bool(states()) and states()[-1]['state_lo']==128 and states()[-1]['window']==15)
                time.sleep(2);screenshot('completed-level-stats')
                screenshot('completed-level-replay');key(confirm)
            wait(lambda value:'game mode switch from 13 to 2' in value,30)
            time.sleep(2);screenshot('mission-start')
            if args.exercise:
                key(confirm);time.sleep(1);screenshot('briefing-dismissed')
                before=states()[-1]
                if args.capture_motion:
                    with (directory/'motion-capture.log').open('w') as capture_log:
                        capture=subprocess.Popen(['ffmpeg','-nostdin','-y','-f','x11grab','-framerate','120','-window_id',window,'-i',env['DISPLAY'],'-t','3','-vf','scale=640:360','-c:v','libx264','-preset','ultrafast','-crf','12','-threads','2',str(directory/'mission-motion.mp4')],env=env,stdout=capture_log,stderr=subprocess.STDOUT)
                        try:
                            time.sleep(.3);key(confirm,2)
                            capture.wait(timeout=15)
                            if capture.returncode:raise ValueError('Private motion capture failed; inspect motion-capture.log')
                        finally:
                            if capture.poll() is None:capture.terminate();capture.wait(timeout=10)
                else:key(confirm,2)
                screenshot('mission-move')
                after=states()[-1]
                if (before['x'],before['z'])==(after['x'],after['z']):raise ValueError('Vehicle position did not change during acceleration')
                key('Return');time.sleep(1);screenshot('pause')
                if not states()[-1]['paused']:raise ValueError('Start did not pause the guest')
                if args.options:
                    key(confirm);wait(lambda value:'"event":"pc_options_selection"' in value);time.sleep(2);screenshot('pc-options-root')
                    key(confirm);time.sleep(2);screenshot('pc-options-graphics')
                    key(confirm);time.sleep(2);screenshot('pc-options-saved')
                    settings=json.loads((data/'settings.json').read_text())
                    if settings.get('scale')!=4.5:raise ValueError('PC resolution selection was not saved')
                    key('Down');key('Down');key('Down');key(confirm);time.sleep(1)
                    key('Down');key('Down');key(confirm);time.sleep(2);screenshot('pc-options-filter')
                    wait(lambda value:'"event":"pc_filters_applied","presentation":"pixel"' in value)
                    key('Down');key('Down');key('Down');key(confirm);time.sleep(1)
                    key(confirm);time.sleep(2);screenshot('pc-options-texture')
                    wait(lambda value:'"event":"pc_filters_applied","presentation":"pixel","texture":"linear"' in value)
                    settings=json.loads((data/'settings.json').read_text())
                    if settings.get('presentation')!='pixel' or settings.get('texture')!='linear':raise ValueError('PC filters were not saved')
                    # Each page reuses the original closing/opening animation;
                    # wait for it before sending the next Back edge.
                    key('x');time.sleep(1);key('x');time.sleep(1)
                    key('x');time.sleep(1);key('Down');key(confirm);time.sleep(1);screenshot('pc-options-audio')
                    key(confirm);time.sleep(2)
                    settings=json.loads((data/'settings.json').read_text())
                    if settings.get('volume')!=0:raise ValueError('PC volume selection was not saved')
                    key('x');time.sleep(1);key('x');time.sleep(2);screenshot('pc-options-return')
                if args.controls:
                    key(confirm);time.sleep(2)
                    key('Down');key('Down');key(confirm);time.sleep(1)
                    key('Down');key(confirm);time.sleep(1);screenshot('controls-devices')
                    key(confirm);time.sleep(1);screenshot('controls-keyboard')
                    key(confirm)
                    wait(lambda value:control_view(value,'REMAP ACCEL/OK'));time.sleep(1);screenshot('controls-capture')
                    key('t');confirm='t'
                    wait(lambda value:control_view(value,'SAVED','ACCEL/OK: T'))
                    time.sleep(1);screenshot('controls-saved')
                    controls=(data/'controls.json').read_bytes()
                    saved=json.loads(controls)
                    if saved.get('schema')!=1 or saved['actions']['confirm']['keyboard']!=[{'key':'T'}]:raise ValueError('Keyboard confirm remap was not persisted')
                    key('Down');key(confirm)
                    wait(lambda value:control_view(value,'REMAP BACK/REVERSE'));time.sleep(1);key('Escape')
                    wait(lambda value:control_view(value,'CANCELLED'));time.sleep(1);screenshot('controls-cancelled')
                    if (data/'controls.json').read_bytes()!=controls:raise ValueError('Cancel changed the saved controls')
                    key('x');time.sleep(1);key('x');time.sleep(1)
                    key('Down');key('Down');key(confirm)
                    wait(lambda value:control_view(value,'KEYBOARD HELP','ACCEL/OK: T'));time.sleep(1);screenshot('controls-help')
                    key(confirm);time.sleep(1);screenshot('controls-help-camera')
                    key('x');time.sleep(1);key('x');time.sleep(1);key('x');time.sleep(1)
                    print('Controls remap, cancellation and in-game help verified',flush=True)
                if args.glyphs:
                    key(confirm);time.sleep(1);key('Down');key('Down');key(confirm);time.sleep(1)
                    key('Down');key(confirm);time.sleep(1);key('Down');key(confirm)
                    labels={'xbox':'A','playstation':'CROSS','switch':'B'}
                    wait(lambda value:control_view(value,'PAD 1/5','ACCEL/OK: '+labels[args.glyphs]))
                    for part in range(5):
                        time.sleep(1);screenshot(f'{args.glyphs}-actions-{part+1}')
                        time.sleep(.13);screenshot(f'{args.glyphs}-actions-{part+1}-next')
                        if part<4:
                            key('Down');key('Down');key('Down');key(confirm)
                            wait(lambda value:control_view(value,f'PAD {part+2}/5'))
                    for _ in range(4):key('x');time.sleep(1)
                    print('Manual-family controller editor pages captured: '+args.glyphs,flush=True)
                if args.controller:
                    family,caption,south,trigger={'xbox360':('xbox','XBOX HELP','A','LT'),'ps5':('playstation','PLAYSTATION HELP','CROSS','L2'),'switch':('switch','SWITCH HELP','B','ZL')}[args.controller]
                    key(confirm);time.sleep(1);key('Down');key('Down');key(confirm);time.sleep(1)
                    key('Down');key('Down');key(confirm)
                    wait(lambda content:control_view(content,'KEYBOARD HELP','ACCEL/OK: '+confirm.upper()))
                    button('guide')
                    wait(lambda content:control_view(content,caption,'ACCEL/OK: '+south))
                    time.sleep(1);screenshot(args.controller+'-active-help');time.sleep(.13);screenshot(args.controller+'-active-help-next')
                    # Physical South performs Confirm and advances the Help page.
                    button('a');wait(lambda content:control_view(content,caption,'BRAKE/EXIT: '+trigger))
                    screenshot(args.controller+'-active-help-camera')
                    # An unbound keyboard key changes prompts without navigating.
                    key('v');wait(lambda content:control_view(content,'KEYBOARD HELP','BRAKE/EXIT: SHIFT'))
                    screenshot(args.controller+'-keyboard-handover')
                    button('guide');wait(lambda content:control_view(content,caption,'BRAKE/EXIT: '+trigger))
                    pad('detach');wait(lambda content:control_view(content,'KEYBOARD HELP','BRAKE/EXIT: SHIFT'))
                    screenshot(args.controller+'-disconnected')
                    pad('attach',value=1);time.sleep(1)
                    if not control_view(text(),'KEYBOARD HELP','BRAKE/EXIT: SHIFT'):raise ValueError('Held South changed the Help page when reconnecting')
                    pad('button','a',0);button('guide')
                    wait(lambda content:control_view(content,caption,'BRAKE/EXIT: '+trigger));button('a')
                    wait(lambda content:control_view(content,caption) and 'CAM UP:' in content.split('"event":"pc_controls_view"')[-1])
                    screenshot(args.controller+'-reconnected')
                    for _ in range(3):key('x');time.sleep(1)
                    print('Native SDL controller help, keyboard handover and held-button reconnect verified: '+args.controller,flush=True)
                key('Return');time.sleep(1);key('Right',1);key(confirm,2);screenshot('resume')
                if states()[-1]['paused']:raise ValueError('Start did not resume the guest')
                print('Guest position changed; pause and resume confirmed by guest state',flush=True)
                if args.controller:
                    before=states()[-1];pad('axis','leftx',24000);button('a',2);pad('axis','leftx',0)
                    after=states()[-1]
                    if (before['x'],before['z'])==(after['x'],after['z']):raise ValueError('Virtual controller did not move the vehicle')
                    screenshot(args.controller+'-controller-drive')
                    button('start');time.sleep(1)
                    if not states()[-1]['paused']:raise ValueError('Controller Start did not pause the guest')
                    button('start');time.sleep(1)
                    # Original Start opens the map, then selects More, which
                    # opens window 1 with CONTINUE selected. Confirm resumes.
                    current=states()[-1]
                    if not current['paused'] or current['window']!=1 or current['state_lo']!=4:raise ValueError('Controller Start did not reach the original Continue menu')
                    screenshot(args.controller+'-original-continue');button('a')
                    wait(lambda content:bool(states()) and not states()[-1]['paused'])
                    print('Native SDL controller movement and pause/resume verified: '+args.controller,flush=True)
                if args.retry:
                    restart_at=len(text());key('Return');time.sleep(1)
                    key('Down')
                    # View Stats is enabled for previously completed missions;
                    # a fresh mission skips that row and reaches Restart first.
                    if 'game mode switch from 14 to 7' in text():key('Down')
                    screenshot('restart-focused');key(confirm);time.sleep(2);screenshot('restart-selected')
                    wait(lambda value:re.search(r'game mode switch from (?:2|8) to 13',value[restart_at:]))
                    key(confirm);time.sleep(3);screenshot('restarted')
                    key(confirm);time.sleep(1)
                    print('Restart selected; transition and capture require review',flush=True)
                if args.wait_failure:
                    deadline=time.monotonic()+150
                    while time.monotonic()<deadline:
                        current=states()
                        if any(e['failed'] for e in current):break
                        if process.poll() is not None:break
                        if current and current[-1]['paused'] and current[-1]['window']>=70:
                            screenshot(f"tutorial-{current[-1]['window']}");key(confirm);time.sleep(1)
                        time.sleep(.3)
                    if not any(e['failed'] for e in states()):raise ValueError('No mission failure observed')
                    screenshot('mission-failure');print('Mission failure confirmed by guest state',flush=True)
            process.wait(timeout=args.seconds+15)
            if process.returncode!=0:raise ValueError(f'Native process exited {process.returncode}')
            # Mode 0 is the new-player introduction. Existing completed players
            # legitimately go map (14) → statistics (7) → mission (2) instead.
            result=check(log,required_modes=(14,2))
            if args.verify_render:result['render_submission_acceptance']=check_render_probe(log,args.refresh)
            if args.audio_loss:result['audio_recovery']=check_audio_recovery(log)
            if args.controller:result['controller_actions']=check_controller_probe(log,args.controller)
            if args.instruction_glyphs:result['instruction_glyphs']=check_instruction_probe(log,args.controller)
            result['seconds_requested']=args.seconds
            result['graphics_requested']={'gpu_driver':args.gpu,'scale':args.scale,'wide':not args.original_aspect,'refresh':args.refresh,'texture':args.texture}
            result['motion_capture']=str(directory/'mission-motion.mp4') if args.capture_motion else None
            result['exercise_verified']=args.exercise
            result['options_verified']=args.options
            result['controls_verified']=args.controls
            result['prompt_captured']=prompt_captured
            result['manual_family_glyphs_captured']=args.glyphs
            result['virtual_controller_verified']=args.controller
            if args.prompts and not prompt_captured:raise ValueError('Original mission prompt was not adapted and captured')
            result['restart_verified']=args.retry
            result['failure_verified']=args.wait_failure
            result['seed_save_unchanged']=not args.seed_save or hashlib.sha256(args.seed_save.read_bytes()).hexdigest()==source_hash
            result['seed_pak_unchanged']=not args.seed_pak or hashlib.sha256(args.seed_pak.read_bytes()).hexdigest()==pak_hash
            result['existing_player_loaded']=args.require_existing_player
            result['completed_level_replayed']='game mode switch from 14 to 7' in text() and 'game mode switch from 7 to 13' in text()
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
    parser.add_argument('--gpu',choices=('software','radv'),default='software',help='Select software Vulkan or the local Radeon GPU; target rates still depend on the private display')
    parser.add_argument('--scale',type=float,default=1,help='Internal rendering multiplier, 1..9; 9 requests a 2160-line target')
    parser.add_argument('--refresh',type=int,choices=(0,30,60,120),default=120)
    parser.add_argument('--texture',choices=('linear','three_point'),default='three_point',help='Exercise the existing runtime texture-filter option')
    parser.add_argument('--verify-render',action='store_true',help='Require sustained actual mission presentations with fractional mapped renders; does not certify physical display or visual quality')
    parser.add_argument('--capture-motion',action='store_true',help='Record three seconds of private mission movement at 120 samples/sec for visual review; recording overhead can affect timing')
    parser.add_argument('--original-aspect',action='store_true')
    parser.add_argument('--seed-save',type=Path)
    parser.add_argument('--seed-pak',type=Path)
    parser.add_argument('--require-existing-player',action='store_true')
    parser.add_argument('--exercise',action='store_true')
    parser.add_argument('--retry',action='store_true')
    parser.add_argument('--options',action='store_true')
    parser.add_argument('--controls',action='store_true')
    parser.add_argument('--prompts',action='store_true')
    parser.add_argument('--instruction-glyphs',action='store_true',help='Check original HUD/tutorial glyph commands and matching action names; captures still require visual review')
    parser.add_argument('--audio-loss',choices=('queue','removal'),help='Privately simulate startup output unavailability and mid-game audio loss')
    parser.add_argument('--controller',choices=('xbox360','ps5','switch'),help='Exercise native UI and guest actions using an SDL virtual controller')
    parser.add_argument('--glyphs',choices=('xbox','playstation','switch'),help='Capture controller editor glyphs using the manual family fallback')
    parser.add_argument('--wait-failure',action='store_true')
    args=parser.parse_args()
    if args.seconds<90:parser.error('At least 90 seconds are required for the mission transition')
    if not 1<=args.scale<=9:parser.error('Scale must be finite and between 1 and 9')
    if args.verify_render and args.refresh not in (60,120):parser.error('Render interpolation acceptance requires --refresh 60 or 120')
    if args.capture_motion and not args.exercise:parser.error('Motion capture requires --exercise')
    if (args.retry or args.wait_failure or args.options or args.controls or args.glyphs or args.controller or args.instruction_glyphs) and not args.exercise:parser.error('Retry/failure/options/controls/glyphs/controller/instruction-glyphs needs --exercise')
    if args.controller and args.glyphs:parser.error('Virtual device detection and manual-family rendering need separate runs')
    try:run(args)
    except (ValueError,subprocess.SubprocessError) as error:parser.exit(1,str(error)+'\n')
