#!/usr/bin/env python3
"""Run native keyboard gameplay in an isolated display and private data folder."""
import argparse
import ctypes
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
from check_pc_settings_probe import check as check_pc_settings_probe, check_reload, filters_applied, check_live_filters
from check_demolition_probe import check as check_demolition_probe
from probe_health import stalled_progress
from waypoint_input import axes as waypoint_axes, relative_axes
from native_hang_capture import capture as capture_native_hang

ROOT=Path(__file__).resolve().parents[1]

def run(args):
    if os.environ.get('DISPLAY') in (None,':0',':0.0'):
        raise ValueError('Use xvfb-run -a; this probe must not send input to your desktop')
    if args.gpu=='radv':
        extensions=subprocess.check_output(['xdpyinfo'],text=True)
        if 'DRI3' not in {line.strip() for line in extensions.splitlines()}:
            raise ValueError('Radeon presentation requires DRI3. Use a private headless cage/Xwayland display; Xvfb supports the software driver only.')
    if args.compositor_capture and (os.environ.get('BC_PRIVATE_CAGE_CHILD')!='1' or not os.environ.get('XDG_RUNTIME_DIR','').startswith('/tmp/blastcorps-')):
        raise ValueError('Compositor capture requires the private headless cage runtime')
    full_controller_flow=bool(args.controller and not (args.waypoint or args.tutorial_handover))
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
    env=dict(os.environ,BC_TRACE_INPUT='1',BC_TRACE_GAMEPLAY='1',BC_TRACE_RENDER='1',BC_TRACE_PC_SETTINGS='1',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',VK_DRIVER_FILES=driver)
    preload_aliases=tempfile.TemporaryDirectory(prefix='blastcorps-qa-preload-',dir='/tmp') if args.audio_loss or args.controller else None
    def preload_path(path):
        # ld.so splits on spaces. A private alias also works when the tested
        # package starts from /tmp rather than this checkout's directory.
        alias=Path(preload_aliases.name)/path.name;alias.symlink_to(path.resolve());return str(alias)
    if args.audio_loss:
        subprocess.run(['c++','-std=c++20','-shared','-fPIC','-I/home/linuxbrew/.linuxbrew/include/SDL2','tools/qa_audio_fault.cpp','-ldl','-L/home/linuxbrew/.linuxbrew/lib','-lSDL2','-Wl,-rpath,/home/linuxbrew/.linuxbrew/lib','-o',str(directory/'libaudio-fault.so')],cwd=ROOT,check=True)
        env.update(LD_PRELOAD=preload_path(directory/'libaudio-fault.so'),BC_QA_AUDIO_FAULT_AFTER='45',BC_QA_AUDIO_OUTAGE='8',BC_QA_AUDIO_STARTUP_OUTAGE='3',BC_QA_AUDIO_FAULT_KIND=args.audio_loss)
    if args.controller:
        subprocess.run(['c++','-std=c++20','-shared','-fPIC','-I/home/linuxbrew/.linuxbrew/include/SDL2','tools/qa_controller.cpp','-ldl','-L/home/linuxbrew/.linuxbrew/lib','-lSDL2','-Wl,-rpath,/home/linuxbrew/.linuxbrew/lib','-o',str(directory/'libcontroller.so')],cwd=ROOT,check=True)
        preload=preload_path(directory/'libcontroller.so')
        if env.get('LD_PRELOAD'):preload+=':'+env['LD_PRELOAD']
        env.update(LD_PRELOAD=preload,BC_QA_CONTROLLER=args.controller,BC_QA_INPUT_FILE=str(directory/'controller-input'))
    log=directory/'native.log'
    print(f'Private test directory: {directory}',flush=True)
    confirm='z'
    prompt_captured=False
    def permit_private_debugger():
        # Yama may otherwise reject a debugger sibling of the native process.
        # Allow only this QA parent and its descendants for this one child.
        libc=ctypes.CDLL(None,use_errno=True)
        if libc.prctl(0x59616D61,os.getppid(),0,0,0)!=0:
            raise OSError(ctypes.get_errno(),'Cannot authorize private QA debugger')
    debugger_setup=permit_private_debugger if args.capture_hang and os.uname().sysname=='Linux' else None
    with log.open('w') as output:
        process=subprocess.Popen([str(args.executable.resolve()),'--rom',str(args.rom.resolve()),'--seconds',str(args.seconds),'--data-dir',str(data),'--scale',str(args.scale),'--original-aspect' if args.original_aspect else '--wide','--refresh',str(args.refresh),'--texture',args.texture],cwd=args.launch_dir.resolve(),env=env,stdout=output,stderr=subprocess.STDOUT,preexec_fn=debugger_setup)
        def text():return log.read_text(errors='replace')
        def capture_hang():
            if not args.capture_hang:return
            debugger=shutil.which('gdb')
            with (directory/'native-hang.txt').open('w') as dump:
                if debugger:
                    capture_native_hang(debugger, process.pid, dump)
                else:dump.write('gdb unavailable; native transition still failed\n')
        def check_health(value):
            if args.capture_hang and (reason:=stalled_progress(value)):
                capture_hang()
                raise ValueError(reason)
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
                check_health(value)
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
            capture_hang()
            raise ValueError('Native transition timed out: '+text()[-1800:])
        def settings_view(content=None):
            for line in reversed((text() if content is None else content).splitlines()):
                try:event=json.loads(line)
                except json.JSONDecodeError:continue
                if isinstance(event,dict) and event.get('event')=='pc_settings_view':return event
            return {}
        def wait_view(section=None,modal='none',**expected):
            wait(lambda content:(view:=settings_view(content)).get('open') and
                 (section is None or view.get('section')==section) and view.get('modal')==modal and
                 all(view.get(k)==v for k,v in expected.items()))
        def saved_field(field,value):
            def matches(_):
                try:return json.loads((data/'settings.json').read_text()).get(field)==value
                except (OSError,ValueError):return False
            wait(matches)
        def mouse(x,y,click=False):
            view=settings_view();width,height=view['width'],view['height'];scale=min(width/640,height/480)
            command('mousemove','--window',window,str(round((width-640*scale)/2+x*scale)),str(round((height-480*scale)/2+y*scale)))
            if click:command('click','1')
            time.sleep(.15)
        def tab(index):
            mouse(102+144*index,76,True);wait_view(('DISPLAY','GRAPHICS','AUDIO','CONTROLS')[index])
        def resize(width,height):
            if args.compositor_capture:
                subprocess.run(['wlr-randr','--output','HEADLESS-1','--custom-mode',f'{width}x{height}@60Hz'],env=env,check=True)
            else:command('windowsize',window,str(width),str(height))
            wait_view(width=width,height=height);time.sleep(.3)
        def command(*values):subprocess.run(['xdotool',*values],env=env,check=True,stdout=subprocess.DEVNULL)
        def key(name,hold=.4):
            command('keydown',name);time.sleep(hold);command('keyup',name);time.sleep(.3)
        def screenshot(name):
            if args.compositor_capture:
                # Xwayland GetImage can retain an old Vulkan window image even
                # while the compositor presents new frames. Keep that readback
                # only as a diagnostic; visual acceptance uses screencopy.
                subprocess.run(['import','-window',window,str(directory/f'{name}-x11.png')],env=env,check=True)
                subprocess.run([str(args.compositor_capture.resolve()),'-o','HEADLESS-1','-l','1',str(directory/f'{name}-compositor.png')],env=env,check=True)
                shutil.copyfile(directory/f'{name}-compositor.png',directory/f'{name}.png')
            else:
                subprocess.run(['import','-window',window,str(directory/f'{name}.png')],env=env,check=True)
        controller_serial=0
        def pad(kind,name='unused',value=0,settle=.15):
            nonlocal controller_serial
            controller_serial+=1
            request=directory/'controller-input';temporary=directory/'controller-input.tmp'
            temporary.write_text(f'{controller_serial} {kind} {name} {value}\n');temporary.replace(request)
            wait(lambda content:f'"serial":{controller_serial},"ok":true' in content)
            time.sleep(settle)
        def button(name,hold=.4):
            pad('button',name,1);time.sleep(hold);pad('button',name,0);time.sleep(.3)
        try:
            wait(lambda value:'rt64_device_ready' in value)
            window=subprocess.check_output(['xdotool','search','--pid',str(process.pid),'--name','Blast Corps|Power Dozer'],env=env,text=True).split()[-1]
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
            if args.tutorial_handover:
                wait(lambda content:bool(states()) and states()[-1]['paused'] and states()[-1]['window']>=70)
                handover={'before':states()[-1]}
                at=len(text());pad('axis','leftx',25000);time.sleep(.2);pad('axis','leftx',0)
                expected={'xbox360':'xbox','ps5':'playstation','switch':'switch'}[args.controller]
                wait(lambda content:f'"event":"active_device","family":"{expected}"' in content[at:])
                label={'xbox360':'START','ps5':'OPTIONS','switch':'PLUS'}[args.controller]
                wait(lambda content:f'"text":"PRESSING {label} WILL"' in content[at:])
                time.sleep(1);screenshot('tutorial-controller-handover');handover['controller']=states()[-1]
                at=len(text());key('Left',.2)
                wait(lambda content:'"event":"active_device","family":"keyboard"' in content[at:])
                wait(lambda content:'"text":"PRESSING ENTER WILL"' in content[at:])
                time.sleep(1);screenshot('tutorial-keyboard-handover');handover['keyboard']=states()[-1]
                if not all(state['paused'] and state['window']==handover['before']['window'] for state in handover.values()):
                    raise ValueError('Tutorial closed or changed during device handover')
                (directory/'tutorial-handover.json').write_text(json.dumps(handover,indent=2)+'\n')
            if args.exercise:
                key(confirm);time.sleep(1);screenshot('briefing-dismissed')
                if args.waypoint:
                    target_x,target_z=args.waypoint
                    initial=states()[-1]
                    deadline=time.monotonic()+args.waypoint_seconds
                    def route_pad(kind,name,value):pad(kind,name,value,settle=0)
                    reached=False
                    route_finished=False
                    try:
                        route_pad('button','a',1)
                        while time.monotonic()<deadline:
                            check_health(text())
                            if process.poll() is not None:raise ValueError('Native process exited during waypoint routing')
                            current=states()[-1]
                            if current['failed'] or current['complete']:break
                            if current['paused']:
                                route_pad('axis','leftx',0);route_pad('axis','lefty',0);route_pad('button','a',0)
                                if current['window']<70:raise ValueError('Unexpected menu during waypoint routing')
                                button('a');route_pad('button','a',1)
                                continue
                            dx,dz=target_x-current['x'],target_z-current['z']
                            if dx*dx+dz*dz<=256*256 or current['buildings_destroyed']>initial['buildings_destroyed']:
                                reached=True;break
                            if current['vehicle_type']==4:
                                x,y=relative_axes(dx,dz,current['vehicle_heading'])
                            else:x,y=waypoint_axes(dx,dz,current['camera_yaw'])
                            route_pad('axis','leftx',x);route_pad('axis','lefty',y)
                        route_finished=True
                    finally:
                        # Failed probes terminate the whole private process; do
                        # not trigger repeated debugger captures during cleanup.
                        if route_finished and process.poll() is None:
                            route_pad('button','a',0);route_pad('axis','leftx',0);route_pad('axis','lefty',0)
                    screenshot('waypoint-drive')
                    print('Private waypoint route ended; target/new-building observed: '+str(reached),flush=True)
                    # Subsequent control checks start outside roadside tutorials.
                    deadline=time.monotonic()+8
                    time.sleep(.3)
                    while states()[-1]['paused'] and states()[-1]['window']>=70:
                        if time.monotonic()>=deadline:raise ValueError('Waypoint tutorial did not close')
                        button('a');time.sleep(.5)
                if args.drive_seconds:
                    # Private exploration through physical keys only. Trace reads
                    # original damage counters; no guest position/stat writes.
                    if args.steer_seconds:key('Left',args.steer_seconds)
                    if args.drive_direction:command('keydown',args.drive_direction)
                    command('keydown',confirm)
                    try:
                        deadline=time.monotonic()+args.drive_seconds
                        while time.monotonic()<deadline:
                            if process.poll() is not None:raise ValueError('Native process exited during driving')
                            current=states()[-1]
                            if current['failed'] or current['complete']:break
                            if current['paused'] and current['window']>=70:
                                command('keyup',confirm);key(confirm);command('keydown',confirm)
                            time.sleep(.2)
                    finally:
                        command('keyup',confirm)
                        if args.drive_direction:command('keyup',args.drive_direction)
                    screenshot('demolition-drive')
                    # A roadside tutorial may appear after releasing the pedal.
                    # Dismiss its original confirmation before testing movement.
                    deadline=time.monotonic()+8
                    time.sleep(.3)
                    while states()[-1]['paused'] and states()[-1]['window']>=70:
                        if time.monotonic()>=deadline:raise ValueError('Roadside tutorial did not close')
                        key(confirm);time.sleep(.5)
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
                if args.options or args.controls or args.glyphs or full_controller_flow:
                    key(confirm);wait_view('DISPLAY');time.sleep(1)
                    screenshot('pc-settings-first-open')
                    # Every complete section and both modals at both native extents.
                    for width in (960,1280):
                        resize(width,720)
                        for index,section_name in enumerate(('display','graphics','audio','controls')):
                            tab(index);time.sleep(.25);screenshot(f'pc-settings-{section_name}-{width}')
                        mouse(80,166,True);wait_view('CONTROLS','capture');screenshot(f'pc-settings-capture-{width}')
                        key('Escape',.1);wait_view('CONTROLS',focus=0)
                        mouse(130,394,True);wait_view('CONTROLS','defaults');screenshot(f'pc-settings-defaults-{width}')
                        key('Escape',.1);wait_view('CONTROLS')
                    tab(0);key('Tab',.1);wait_view('DISPLAY',focus=1)
                    key('shift+Tab',.1);wait_view('DISPLAY',focus=0)
                    if args.options:
                        tab(0);mouse(100,140);key('Right',.1);saved_field('scale',4.5)
                        key('Left',.1);saved_field('scale',3)
                        mouse(100,198);key('Left',.1);saved_field('wide',False)
                        key('Right',.1);saved_field('wide',True)
                        mouse(100,256);key('Left',.1);saved_field('refresh',60)
                        key('Right',.1);saved_field('refresh',120)
                        mouse(100,314);key('Right',.1);saved_field('fullscreen',True)
                        key('Left',.1);saved_field('fullscreen',False)
                        tab(1);mouse(100,140);key('Right',.1);saved_field('msaa',2)
                        key('Left',.1);saved_field('msaa',0)
                        def change_filter(field,value,presentation,texture,direction):
                            at=len(text());key(direction,.1);saved_field(field,value)
                            wait(lambda content:filters_applied(content[at:],presentation,texture))
                        mouse(100,198);change_filter('presentation','pixel','pixel','three_point','Right')
                        change_filter('presentation','linear','linear','three_point','Left')
                        mouse(100,256);change_filter('texture','linear','linear','linear','Left')
                        change_filter('texture','three_point','linear','three_point','Right')
                        tab(2);mouse(320,140,True);saved_field('volume',50)
                        key('Left',.1);saved_field('volume',40);key('Right',.1);saved_field('volume',50)
                        mouse(50,140);command('mousedown','1');mouse(-100,140);command('mouseup','1');saved_field('volume',0)
                        mouse(590,140);command('mousedown','1');mouse(740,140);command('mouseup','1');saved_field('volume',100)
                        mouse(100,198,True);saved_field('mute',True);key('Left',.1);saved_field('mute',False)
                        screenshot('pc-settings-audio-live')
                        # Force an actual SettingsStore failure, preserving the private saved file.
                        path=data/'settings.json';backup=data/'settings-before-failure.json';path.rename(backup);path.mkdir()
                        try:
                            key('Right',.1);wait_view('AUDIO',status='SAVE FAILED',focus=1)
                            if settings_view()['rows'][1]['value']!='OFF':raise ValueError('Failed mute write did not restore the saved value')
                            screenshot('pc-settings-save-failed')
                        finally:path.rmdir();backup.rename(path)
                        key('Right',.1);saved_field('mute',True);key('Left',.1);saved_field('mute',False)
                    tab(3)
                    if args.controls:
                        mouse(80,166,True);wait_view('CONTROLS','capture');key('t',.1);confirm='t'
                        wait_view('CONTROLS',status='SAVED',focus=0)
                        stored=json.loads((data/'controls.json').read_text())
                        if stored['actions']['confirm']['keyboard']!=[{'key':'T'}]:raise ValueError('Keyboard remap was not persisted')
                        screenshot('pc-settings-remapped-keyboard')
                        mouse(130,394,True);wait_view('CONTROLS','defaults');screenshot('pc-settings-remapped-defaults')
                        key('t',.1);wait_view('CONTROLS',status='SAVED')
                        if settings_view()['rows'][0]['value']!='Z':raise ValueError('Selected keyboard defaults were not restored')
                        mouse(80,166,True);wait_view('CONTROLS','capture');key('t',.1);wait_view('CONTROLS',status='SAVED',focus=0)
                        before=(data/'controls.json').read_bytes()
                        mouse(80,198,True);wait_view('CONTROLS','capture');key('Escape',.1);wait_view('CONTROLS',focus=1)
                        if (data/'controls.json').read_bytes()!=before:raise ValueError('Capture cancel modified controls')
                    if args.glyphs or full_controller_flow:
                        mouse(100,122,True);wait_view('CONTROLS',device='CONTROLLER')
                        if full_controller_flow:
                            button('guide');time.sleep(.3)
                            screenshot(args.controller+'-active-controls')
                            # South really enters capture; East really cancels defaults.
                            mouse(80,166);button('a');wait_view('CONTROLS','capture');key('Escape',.1);wait_view('CONTROLS',focus=0)
                            mouse(130,394);button('a');wait_view('CONTROLS','defaults');screenshot(args.controller+'-defaults-actions')
                            button('b');wait_view('CONTROLS');key('v',.1)
                            screenshot(args.controller+'-keyboard-handover')
                            button('guide');pad('detach');time.sleep(.3);screenshot(args.controller+'-disconnected')
                            pad('attach',value=1);time.sleep(.5)
                            if settings_view().get('modal')!='none':raise ValueError('Held reconnect activated settings')
                            pad('button','a',0);button('guide');screenshot(args.controller+'-reconnected')
                            # Manual fallback changes glyphs while physical action positions stay fixed.
                            for family,label in (('xbox','A'),('playstation','CROSS'),('switch','B'),('auto',{'xbox360':'A','ps5':'CROSS','switch':'B'}[args.controller])):
                                mouse(470,122);key('Right',.1);saved_field('family',family)
                                if settings_view()['rows'][0]['value']!=label:raise ValueError('Manual family action caption mismatch')
                                button('guide');screenshot('pc-settings-fallback-'+family)
                            mouse(470,122);key('Left',.1);saved_field('family','switch')
                            key('Right',.1);saved_field('family','auto')
                            # West is unbound by default; remap Confirm and verify the real action.
                            mouse(80,166,True);wait_view('CONTROLS','capture');button('x');wait_view('CONTROLS',status='SAVED')
                            west={'xbox360':('X','xbox_x'),'ps5':('SQUARE','playstation_square'),'switch':('Y','switch_y')}[args.controller]
                            binding=settings_view()['rows'][0]['binding']
                            if (binding['label'],binding['glyph'])!=west:raise ValueError('Remapped West action does not match family glyph')
                            screenshot(args.controller+'-remapped-west')
                            # Remapped West really opens/confirms defaults; other-device remaps survive.
                            mouse(130,394);button('x');wait_view('CONTROLS','defaults');screenshot(args.controller+'-remapped-defaults')
                            button('x');wait_view('CONTROLS',status='SAVED')
                            if settings_view()['rows'][0]['value']!={'xbox360':'A','ps5':'CROSS','switch':'B'}[args.controller]:raise ValueError('Controller defaults did not restore South')
                            if args.controls and json.loads((data/'controls.json').read_text())['actions']['confirm']['keyboard']!=[{'key':'T'}]:raise ValueError('Controller defaults changed keyboard bindings')
                            button('rightshoulder');wait_view('DISPLAY');button('leftshoulder');wait_view('CONTROLS')
                            tab(2);mouse(100,140);pad('axis','leftx',-24000);saved_field('volume',90);pad('axis','leftx',0)
                            pad('axis','leftx',24000);saved_field('volume',100);pad('axis','leftx',0)
                        else:screenshot(args.glyphs+'-all-actions')
                    # Leave distinct saved values for the second launch, without CLI overrides.
                    if args.options:
                        tab(0);mouse(100,140);key('Right',.1);saved_field('scale',4.5)
                        mouse(100,198);key('Left',.1);saved_field('wide',False)
                        mouse(100,256);key('Left',.1);saved_field('refresh',60)
                        tab(1);mouse(100,140);key('Right',.1);saved_field('msaa',2)
                    # Existing held-confirm suppression must preserve paused guest state.
                    tab(2);mouse(100,140)
                    command('keydown',confirm);key('Escape',.1);time.sleep(.4)
                    if settings_view().get('open') or not states()[-1]['paused']:raise ValueError('Held confirm leaked while closing settings')
                    # The guest pause window reopens with its original rotating animation.
                    command('keyup',confirm);time.sleep(2);screenshot('pc-settings-return-to-pause')
                    key(confirm);wait_view('DISPLAY');screenshot('pc-settings-reopened')
                    tab(3);mouse(475,394,True);wait(lambda content:not settings_view(content).get('open'));time.sleep(1)
                    if not states()[-1]['paused']:raise ValueError('Original Game Options handoff unpaused gameplay')
                    screenshot('pc-settings-original-game-options')
                    key('x',.1);time.sleep(1);screenshot('pc-settings-original-options-return')
                    print('Complete sections, modals, resize, persistence, return and original options exercised; screenshots require review',flush=True)
                key('Return');time.sleep(1);key('Right',1);key(confirm,2);screenshot('resume')
                if states()[-1]['paused']:raise ValueError('Start did not resume the guest')
                print('Guest position changed; pause and resume confirmed by guest state',flush=True)
                if full_controller_flow:
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
            if args.capture_hang:
                deadline=time.monotonic()+args.seconds+15
                while process.poll() is None:
                    check_health(text())
                    if time.monotonic()>=deadline:raise subprocess.TimeoutExpired(process.args,args.seconds+15)
                    time.sleep(.5)
            else:process.wait(timeout=args.seconds+15)
            if process.returncode!=0:raise ValueError(f'Native process exited {process.returncode}')
            # Mode 0 is the new-player introduction. Existing completed players
            # legitimately go map (14) → statistics (7) → mission (2) instead.
            result=check(log,required_modes=(14,2))
            if args.verify_demolition:result['demolition_acceptance']=check_demolition_probe(log)
            if args.verify_render:result['render_submission_acceptance']=check_render_probe(log,args.refresh)
            if args.audio_loss:result['audio_recovery']=check_audio_recovery(log)
            if full_controller_flow:result['controller_actions']=check_controller_probe(log,args.controller)
            if args.options or args.controls or args.glyphs or full_controller_flow:result['pc_settings']=check_pc_settings_probe(log)
            if args.options:result['live_filters']=check_live_filters(text())
            if args.instruction_glyphs:result['instruction_glyphs']=check_instruction_probe(log,args.controller)
            if args.options or args.controls:
                expected=json.loads((data/'settings.json').read_text())
                reload_env=dict(env)
                # A clean process reads the same private files, with no option overrides.
                for variable in ('LD_PRELOAD','BC_QA_CONTROLLER','BC_QA_INPUT_FILE'):
                    reload_env.pop(variable,None)
                reload_log=directory/'settings-reload.log'
                with reload_log.open('w') as reload_output:
                    reload_process=subprocess.Popen([str(args.executable.resolve()),'--rom',str(args.rom.resolve()),'--seconds','12','--data-dir',str(data)],cwd=args.launch_dir.resolve(),env=reload_env,stdout=reload_output,stderr=subprocess.STDOUT)
                    try:
                        deadline=time.monotonic()+15
                        while 'rt64_device_ready' not in reload_log.read_text(errors='replace'):
                            if reload_process.poll() is not None or time.monotonic()>deadline:raise ValueError('Settings restart did not initialize graphics')
                            time.sleep(.1)
                        reload_window=subprocess.check_output(['xdotool','search','--pid',str(reload_process.pid),'--name','Blast Corps|Power Dozer'],env=reload_env,text=True).split()[-1]
                        command('windowfocus',reload_window);time.sleep(2)
                        if args.controls:key('t',.5)
                        reload_process.wait(timeout=20)
                        if reload_process.returncode:raise ValueError('Native settings restart failed')
                    finally:
                        if reload_process.poll() is None:reload_process.terminate();reload_process.wait(timeout=10)
                result['pc_settings_reload']=check_reload(reload_log,expected,args.controls)
            result['seconds_requested']=args.seconds
            result['native_executable']=str(args.executable.resolve())
            result['native_launch_directory']=str(args.launch_dir.resolve())
            result['graphics_requested']={'gpu_driver':args.gpu,'scale':args.scale,'wide':not args.original_aspect,'refresh':args.refresh,'texture':args.texture}
            result['motion_capture']=str(directory/'mission-motion.mp4') if args.capture_motion else None
            result['waypoint_requested']={'target':args.waypoint,'seconds':args.waypoint_seconds}
            result['exploratory_drive_requested']={'seconds':args.drive_seconds,'left_steer_seconds':args.steer_seconds,'held_direction':args.drive_direction}
            result['exercise_verified']=args.exercise
            result['options_verified']=args.options
            result['controls_verified']=args.controls
            result['prompt_captured']=prompt_captured
            result['manual_family_glyphs_captured']=args.glyphs
            result['virtual_controller_verified']=args.controller if full_controller_flow else None
            result['tutorial_handover_requested']=args.tutorial_handover
            result['tutorial_handover_trace_verified']=True if args.tutorial_handover else None
            result['compositor_capture']=str(args.compositor_capture.resolve()) if args.compositor_capture else None
            result['screenshot_method']='private_compositor_screencopy' if args.compositor_capture else 'x11_window_readback'
            result['waypoint_controller']=args.controller if args.waypoint else None
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
            if preload_aliases:preload_aliases.cleanup()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds',type=int,default=180)
    parser.add_argument('--executable',type=Path,default=ROOT/'build/blastcorps-prototype',help='Native executable or packaged launcher to test')
    parser.add_argument('--rom',type=Path,default=ROOT/'.local/rom.z64',help='Private ROM input for the tested native executable')
    parser.add_argument('--launch-dir',type=Path,default=ROOT,help='Native working directory; use /tmp to verify a clean package outside the checkout')
    parser.add_argument('--gpu',choices=('software','radv'),default='software',help='Select software Vulkan or the local Radeon GPU; target rates still depend on the private display')
    parser.add_argument('--scale',type=float,default=1,help='Internal rendering multiplier, 1..9; 9 requests a 2160-line target')
    parser.add_argument('--refresh',type=int,choices=(0,30,60,120),default=120)
    parser.add_argument('--texture',choices=('linear','three_point'),default='three_point',help='Exercise the existing runtime texture-filter option')
    parser.add_argument('--verify-render',action='store_true',help='Require sustained actual mission presentations with fractional mapped renders; does not certify physical display or visual quality')
    parser.add_argument('--compositor-capture',type=Path,help='Private grim executable to compare X11 and actual headless compositor captures')
    parser.add_argument('--tutorial-handover',action='store_true',help='Observe controller/keyboard handover in an already-open original tutorial; captures require review')
    parser.add_argument('--capture-motion',action='store_true',help='Record three seconds of private mission movement at 120 samples/sec for visual review; recording overhead can affect timing')
    parser.add_argument('--original-aspect',action='store_true')
    parser.add_argument('--seed-save',type=Path)
    parser.add_argument('--seed-pak',type=Path)
    parser.add_argument('--require-existing-player',action='store_true')
    parser.add_argument('--exercise',action='store_true')
    parser.add_argument('--capture-hang',action='store_true',help='On a failed transition, capture private native thread backtraces with gdb before terminating; never turns failure into acceptance')
    parser.add_argument('--verify-demolition',action='store_true',help='Require new original damage and building destruction counters during active gameplay; does not certify objective completion')
    parser.add_argument('--waypoint',type=int,nargs=2,metavar=('X','Z'),help='Experimental private virtual-controller route toward an original world coordinate; does not certify destruction')
    parser.add_argument('--waypoint-seconds',type=float,default=45,help='Private waypoint observation limit, 1..120 seconds')
    parser.add_argument('--drive-seconds',type=float,default=0,help='Private exploratory acceleration before the normal exercise; does not certify mission completion')
    parser.add_argument('--drive-direction',choices=('Up','Down','Left','Right'),help='Hold a physical direction during private exploratory acceleration')
    parser.add_argument('--steer-seconds',type=float,default=0,help='Private left steering before exploratory driving')
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
    if not 0<=args.drive_seconds<=120 or not 0<=args.steer_seconds<=3:parser.error('Exploratory driving requires 0..120 seconds and steering 0..3 seconds')
    if args.drive_seconds and not args.exercise:parser.error('Driving requires --exercise')
    if args.verify_demolition and not (args.drive_seconds or args.waypoint):parser.error('Demolition acceptance requires exploratory driving or a waypoint')
    if args.waypoint and (args.options or args.controls or args.glyphs or args.instruction_glyphs):parser.error('Waypoint and UI/glyph acceptance require separate mission runs')
    if args.waypoint and not args.controller:parser.error('Waypoint routing requires a private virtual controller')
    if not 1<=args.waypoint_seconds<=120:parser.error('Waypoint observation requires 1..120 seconds')
    if args.waypoint and any(abs(value)>0x800000 for value in args.waypoint):parser.error('Waypoint coordinates exceed the bounded game world')
    if args.drive_direction and not args.drive_seconds:parser.error('Driving direction requires --drive-seconds')
    if args.steer_seconds and not args.drive_seconds:parser.error('Steering requires --drive-seconds')
    if args.capture_motion and not args.exercise:parser.error('Motion capture requires --exercise')
    if (args.retry or args.wait_failure or args.options or args.controls or args.glyphs or args.controller or args.instruction_glyphs) and not args.exercise:parser.error('Retry/failure/options/controls/glyphs/controller/instruction-glyphs needs --exercise')
    if args.tutorial_handover and (not args.controller or args.waypoint):parser.error('Tutorial handover requires a separate virtual-controller run')
    if args.controller and args.glyphs:parser.error('Virtual device detection and manual-family rendering need separate runs')
    try:run(args)
    except (ValueError,subprocess.SubprocessError) as error:parser.exit(1,str(error)+'\n')
