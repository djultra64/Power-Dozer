#!/usr/bin/env python3
"""Prepare private guest sources for the sound heartbeat hook and regression.

Only fingerprints and extraction logic are distributed. The user supplies the
original locally generated functions; none of their bodies live in this tool.
"""
import argparse
import hashlib
from pathlib import Path
import re


EXPECTED = {
    '8025F044': ('funcs_1.c', '34b04c654c1210b3fcb9c1403906f74b1f45763b2d3d71a8144a31750ed3180a'),
    '8025F0F0': ('funcs_1.c', 'b64867f06a852a48625bc649bfe2190c01c9fe53c2f3a79d88282bb01e8378b9'),
    '80260148': ('funcs_1.c', '035f03c38511c2f4de607cdfcc500d73a56b573cb5c0fac7875f47463b0e3345'),
    '802D6C8C': ('funcs_23.c', 'bf18ee1be1a5fa30a91e37666d2242ab213ce1e60113eabae034349852198b3a'),
    '802D6DB0': ('funcs_23.c', '33541b873176621a79f9375301677a297f263f067820e0e4481c4efc6bfc5d2b'),
    '802D6E3C': ('funcs_23.c', '3637a92d26253fb6cca04fe632eb5112d9dc62e1c1a95c78b48720e8ac52e5e9'),
    '802D6EB0': ('funcs_23.c', 'd414a5bb478af771ae61cfd03efb083f8751cd0e00726b84ccb996f02aa7423a'),
    '802D6EE0': ('funcs_23.c', '53967d9b51a03a9c3aed3311fadc3bb2fd9bfb77a897a52241a91f8d23372cbe'),
}
POST = 'func_802D6C8C'
ORIGINAL_POST = 'bc_original_sound_event_post'


def extract(text, name):
    marker = 'RECOMP_FUNC void ' + name + '('
    if text.count(marker) != 1:
        raise ValueError('Guest definition is not unique: ' + name)
    start = text.index(marker)
    end = text.find('RECOMP_FUNC void ', start + len(marker))
    return text[start:len(text) if end < 0 else end]


def rename_definition(text):
    marker = 'RECOMP_FUNC void ' + POST + '('
    if text.count(marker) != 1:
        raise ValueError('Guest post definition is not unique')
    return text.replace(marker, 'RECOMP_FUNC void ' + ORIGINAL_POST + '(', 1)


def write_changed(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def prepare(source, destination):
    chunks = []
    for address, (filename, expected) in EXPECTED.items():
        chunk = extract((source / filename).read_text(), 'func_' + address)
        if hashlib.sha256(chunk.encode()).hexdigest() != expected:
            raise ValueError('Guest heartbeat function fingerprint changed: ' + address)
        chunks.append(chunk)
    original = '\n'.join(chunks)
    calls = set(re.findall(r'\b(func_[A-F0-9]+|osSetIntMask_recomp|bc_rmonPrintf)\(rdram, ctx\)', original))
    functions = {'func_' + address for address in EXPECTED}
    supplied = {'func_802DDDE0', 'osSetIntMask_recomp', 'bc_rmonPrintf'}
    declarations = '\n'.join('void ' + name + '(uint8_t*, recomp_context*);'
                             for name in sorted(calls | functions | {ORIGINAL_POST}))
    destination.mkdir(parents=True, exist_ok=True)
    write_changed(destination / 'sound-heartbeat-declarations.inc',
                  'extern "C" {\n' + declarations + '\n}\n')
    write_changed(destination / 'sound-heartbeat-unreachable.inc', '\n'.join(
        'void ' + name + '(uint8_t*, recomp_context*) { throw std::runtime_error("Unexpected guest dependency: ' + name + '"); }'
        for name in sorted(calls - functions - supplied)))
    write_changed(destination / 'sound-heartbeat-original.inc', rename_definition(original))
    # All call sites, including those in this same translation unit, retain the
    # public name. This works with both ELF and COFF, without linker --wrap.
    runtime = 'extern "C" void ' + ORIGINAL_POST + '(uint8_t*, recomp_context*);\n'
    full_source = (source / 'funcs_23.c').read_text()
    runtime = '#include "recomp.h"\n' + runtime + rename_definition(full_source)
    write_changed(destination / 'blastcorps-sound-heartbeat-guest.cpp', runtime)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
