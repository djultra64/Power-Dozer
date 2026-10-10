"""Detect already-defined progress freezes before terminating a private probe."""
import json


def stalled_progress(content):
    previous = None
    for line in content.splitlines():
        try:event = json.loads(line)
        except json.JSONDecodeError:continue
        if not isinstance(event, dict) or event.get('event') not in ('progress', 'probe_end'):continue
        if not all(type(event.get(key)) is int and event[key] >= 0 for key in ('vi', 'gfx', 'audio')):continue
        if event['vi'] < 300:continue
        if previous is not None and event['vi']-previous['vi'] >= 60:
            for label, counter in (('Graphics', 'gfx'), ('Audio', 'audio')):
                if event[counter] <= previous[counter]:
                    return f"{label} stopped: VI {previous['vi']}–{event['vi']}, {event[counter]} tasks"
        previous = event
    return None
