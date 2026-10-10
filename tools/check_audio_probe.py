#!/usr/bin/env python3
"""Require ongoing RSP audio and a non-silent PCM signal in a native probe log."""
import json
import sys
from pathlib import Path

def check(path):
    events = []
    for line in Path(path).read_text(errors="replace").splitlines():
        try:
            events.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    progress = [e for e in events if e.get("event") in ("progress", "probe_end") and e.get("vi", 0) >= 300]
    if not any(e.get("event") == "probe_end" for e in events):
        raise ValueError("Native run did not finish")
    if len(progress) < 4 or progress[-1]["vi"] - progress[0]["vi"] < 900:
        raise ValueError("Capture at least 20 seconds of native execution")
    checked_intervals = 0
    for before, after in zip(progress, progress[1:]):
        if after["vi"] < before["vi"]:
            raise ValueError("VI counter moved backwards")
        if after["vi"] - before["vi"] < 60:
            continue # The final report can coincide with a periodic report.
        checked_intervals += 1
        if after["audio"] <= before["audio"]:
            raise ValueError(f"Audio stopped: VI {before['vi']}–{after['vi']}, {after['audio']} RSP tasks")
    if checked_intervals < 3:
        raise ValueError("Insufficient advancing audio observations")
    if not any(e.get("event") == "audio_signal" and e.get("peak", 0) > 0 for e in events):
        raise ValueError("No non-silent PCM signal")
    if any(e.get("event") == "fatal" for e in events):
        raise ValueError("Native execution failed")
    print(f"Audio continued through {progress[-1]['vi']} VI / {progress[-1]['audio']} tasks")

if __name__ == "__main__":
    try:
        check(sys.argv[1])
    except ValueError as error:
        sys.exit(str(error))
