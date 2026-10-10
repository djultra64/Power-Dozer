#!/usr/bin/env python3
"""Require an observed increase in original mission damage and destruction.

Counters come from read-only USA Rev1 g_statsNew telemetry. This verifies
new destruction during active gameplay, not objective completion or saving.
"""
import argparse
import json
from pathlib import Path


def check(path):
    baseline = None
    previous_vi = -1
    evidence = None
    for line in Path(path).read_text(errors='replace').splitlines():
        try:event = json.loads(line)
        except json.JSONDecodeError:continue
        if not isinstance(event, dict) or event.get('event') != 'guest_state':continue
        for field, maximum in (('vi', 2**64-1), ('money', 2**32-1), ('buildings_destroyed', 255)):
            value = event.get(field)
            if type(value) is not int or not 0 <= value <= maximum:
                raise ValueError('Invalid destruction trace '+field)
        if event['vi'] <= previous_vi:raise ValueError('Destruction trace time did not advance')
        previous_vi = event['vi']
        active = event.get('state_hi') == 0 and event.get('state_lo') == 4 and event.get('window') == -1
        active = active and all(event.get(field) == 0 for field in ('paused', 'failed', 'complete'))
        if not active:
            baseline = None
            continue
        if baseline is None or event.get('level') != baseline.get('level') or any(event[field] < baseline[field] for field in ('money', 'buildings_destroyed')):
            baseline = event
            continue
        if event['money'] > baseline['money'] and event['buildings_destroyed'] > baseline['buildings_destroyed']:
            evidence = dict(level=event['level'], first_vi=baseline['vi'], observed_vi=event['vi'], money_delta=event['money']-baseline['money'], buildings_destroyed_delta=event['buildings_destroyed']-baseline['buildings_destroyed'])
    if evidence is None:raise ValueError('No new building destruction with damage observed during active mission')
    return evidence


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    try:print(json.dumps(check(args.log)))
    except ValueError as error:parser.exit(1,str(error)+'\n')
