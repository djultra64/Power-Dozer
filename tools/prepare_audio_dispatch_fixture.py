#!/usr/bin/env python3
"""Copy fingerprinted original guest audio/scheduler functions for CPU regression."""
import argparse
import hashlib
from pathlib import Path

EXPECTED = {'func_80267A9C': ('funcs_3.c', '08524ab6e75a51513ce000c87e2c4f1076ef466a52f5399bbe1bbe47817aa171'), 'func_80271C24': ('funcs_4.c', '2d7c6fca4a857cb35cbef4e5a1268a95b6dc07a267db8db121b2d4199c195cf0'), 'func_802712FC': ('funcs_4.c', 'f633981d07e3a67d947ecfff6278272fa982f4e67a69d91da92ec6677ced2103'), 'func_80271CE4': ('funcs_4.c', '470bf4782f98e03387f805551f6762309c0db53f51c159822ecf117cfca2b662'), 'func_802715DC': ('funcs_4.c', '93842f2ad5ef71297c8ef3dd2510a1a0c2e1e65fd3735038dee24cc78a0ead46'), 'func_80271A84': ('funcs_4.c', '8313461b3cf64618357c84aa9e4d297f3ec68ee9b4ad77695b89256c8ec57392'), 'func_802D4BB8': ('funcs_22.c', '0668d1c428a9dbf9e3c7cf0273e178c5eef34c0accb04f3d28a15db2eb52ba2e')}


def extract(text, name):
    site = 'RECOMP_FUNC void ' + name + '('
    if text.count(site) != 1:
        raise ValueError('Original audio fixture function sites changed: ' + name)
    start = text.index(site)
    finish = text.find('\nRECOMP_FUNC void ', start + 1)
    return text[start:len(text) if finish < 0 else finish]


def prepare(source, destination):
    chunks = []
    for name, (filename, expected) in EXPECTED.items():
        raw = (Path(source) / filename).read_text()
        function = extract(raw, name)
        if hashlib.sha256(function.encode()).hexdigest() != expected:
            raise ValueError('Original audio fixture function fingerprint changed: ' + name)
        chunks.append(function)
    text = '#include "support.h"\n#include "funcs.h"\n' + ''.join(chunks)
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    target = destination / 'blastcorps-audio-dispatch-guest.cpp'
    if not target.exists() or target.read_text() != text:
        target.write_text(text)
    return target


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
