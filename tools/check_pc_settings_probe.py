#!/usr/bin/env python3
"""Validate complete PC settings snapshot evidence; captures still need review."""
import json
from pathlib import Path

SECTIONS={
    'DISPLAY':['INTERNAL RESOLUTION','ASPECT RATIO','FRAME RATE','WINDOW MODE'],
    'GRAPHICS':['ANTI-ALIASING','SCREEN FILTER','TEXTURE FILTER'],
    'AUDIO':['MASTER VOLUME','MUTE'],
    'CONTROLS':['ACCEL/OK','BACK/REVERSE','PAUSE','BRAKE/EXIT','CAM LEFT','CAM DOWN','CAM UP','CAM RIGHT','SPECIAL L','SPECIAL R','MOVE LEFT','MOVE RIGHT','MOVE UP','MOVE DOWN']}

def check(path):
    events=[]
    for line in Path(path).read_text(errors='replace').splitlines():
        try:event=json.loads(line)
        except json.JSONDecodeError:continue
        if isinstance(event,dict) and event.get('event')=='pc_settings_view':events.append(event)
    for width in (960,1280):
        for section,labels in SECTIONS.items():
            if not any(event.get('open') and event.get('section')==section and event.get('modal')=='none' and
                       event.get('width')==width and event.get('height')==720 and
                       [row.get('label') for row in event.get('rows',[])]==labels for event in events):
                raise ValueError(f'Missing complete {section} screen at {width}x720')
        for modal in ('capture','defaults'):
            if not any(event.get('open') and event.get('modal')==modal and event.get('width')==width and event.get('height')==720 for event in events):
                raise ValueError(f'Missing {modal} modal at {width}x720')
    opened=closed=reopened=False
    for event in events:
        if event.get('open'):
            reopened|=closed;opened=True
        elif opened:closed=True
    if not reopened:raise ValueError('Missing close and reopen transition')
    return {'complete_sections_both_720p_aspects':True,'capture_and_defaults_modals':True,
            'closed_and_reopened':True,'native_capture_review_required':True}

def check_reload(path, expected, remapped_confirm=False):
    """Check a second native launch without settings overrides or controller input."""
    events=[]
    for line in Path(path).read_text(errors='replace').splitlines():
        try:event=json.loads(line)
        except json.JSONDecodeError:continue
        if isinstance(event,dict):events.append(event)
    effective=next((event for event in events if event.get('event')=='effective_settings'),{})
    for field,value in expected.items():
        if field!='schema' and effective.get(field)!=value:
            raise ValueError(f'Saved {field} did not survive the native restart')
    if remapped_confirm and not any(event.get('event')=='input' and event.get('buttons')==32768 for event in events):
        raise ValueError('Reloaded remapped key did not activate Confirm')
    return {'saved_settings_reloaded':True,'remapped_confirm_reloaded':remapped_confirm}

def filters_applied(content, presentation, texture):
    for line in content.splitlines():
        try:event=json.loads(line)
        except json.JSONDecodeError:continue
        if isinstance(event,dict) and event.get('event')=='pc_filters_applied' and event.get('presentation')==presentation and event.get('texture')==texture:return True
    return False

def check_live_filters(content):
    expected=iter([('pixel','three_point'),('linear','three_point'),('linear','linear'),('linear','three_point')])
    target=next(expected)
    for line in content.splitlines():
        if filters_applied(line,*target):
            target=next(expected,None)
            if target is None:return {'live_filter_changes_applied':True}
    raise ValueError('Missing ordered live filter application evidence')
