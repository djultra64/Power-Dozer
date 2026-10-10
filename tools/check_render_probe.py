#!/usr/bin/env python3
"""Require sustained real swapchain submissions and fractional mission renders.

This verifies a submission stream on the test output; it does not certify
physical display scans or visual correctness of interpolated geometry.
"""
import argparse
import json
import math
from pathlib import Path
import re


def check(path, target=120, minimum_windows=10):
    if target not in (60, 120) or minimum_windows < 1:
        raise ValueError('Interpolation acceptance requires target 60/120 and positive coverage')
    mode = None
    state = None
    eligible = False
    primed = False
    windows = []

    def active():
        return mode == 2 and state is not None and not any(state.get(k, 1) for k in ('paused', 'failed', 'complete')) and state.get('window') == -1

    for line in Path(path).read_text(errors='replace').splitlines():
        transition = re.search(r'game mode switch from -?\d+ to (-?\d+)', line)
        if transition:
            mode = int(transition[1])
            eligible &= active()
        try:
            event = json.loads(line)
        except json.JSONDecodeError:
            continue
        if not isinstance(event, dict):
            continue
        if event.get('event') == 'guest_state':
            state = event
            eligible &= active()
        if event.get('event') != 'rt64_actual_frames':
            continue
        if primed and eligible and active():
            windows.append(event)
        primed = True
        eligible = active()

    if len(windows) < minimum_windows:
        raise ValueError('Insufficient unpaused mission coverage for presentation acceptance')
    rates = []
    for window in windows:
        seconds = window.get('seconds', 0)
        rate = window.get('present_submission_fps', float('nan'))
        count = window.get('successful_present_submissions', float('nan'))
        if not all(math.isfinite(v) for v in (seconds, rate, count)) or seconds <= 0:
            raise ValueError('Presentation observation is not finite with positive elapsed time')
        if abs(count / seconds - rate) > 0.1:
            raise ValueError('Presentation rate disagrees with actual submission count')
        if window.get('failed_present_submissions', 1):
            raise ValueError('Swapchain reported failed submissions')
        if not window.get('fractional_renders', 0) or not window.get('mapped_transforms', 0):
            raise ValueError('Missing fractional renders or mapped geometry')
        if rate < target * 0.98:
            raise ValueError(f'Actual presentation rate {rate:.3f} below {target}-FPS acceptance at VI {window.get("vi")}')
        rates.append(rate)
    return {'mission_windows': len(windows), 'seconds': sum(w['seconds'] for w in windows), 'minimum_fps': min(rates), 'maximum_fps': max(rates), 'target_fps': target, 'physical_display_verified': False, 'visual_interpolation_verified': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('--target', type=int, default=120)
    args = parser.parse_args()
    try:
        print(json.dumps(check(args.log, args.target)))
    except ValueError as error:
        parser.exit(1, str(error) + '\n')
