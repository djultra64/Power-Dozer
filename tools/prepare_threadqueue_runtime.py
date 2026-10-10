#!/usr/bin/env python3
"""Prepare the pinned SDK thread queue with terminating linked-list removal."""
import argparse
import hashlib
from pathlib import Path

EXPECTED = 'b9bb55edeb9cb99a153d00d80a13c5a09140113403499af7656fd9489d38f305'
REMOVE_BEGIN = 'bool ultramodern::thread_queue_remove(RDRAM_ARG PTR(PTR(OSThread)) queue_, PTR(OSThread) t_) {'
REMOVE_END = 'bool ultramodern::thread_queue_empty(RDRAM_ARG PTR(PTR(OSThread)) queue_) {'
REMOVAL = r'''bool ultramodern::thread_queue_remove(RDRAM_ARG PTR(PTR(OSThread)) queue_, PTR(OSThread) t_) {
    // Walk links, including the SDK's host-owned running queue head.
    // The original repeatedly inspected the head and could loop forever
    // removing a non-head thread; an empty queue dereferenced guest NULL.
    PTR(OSThread)* link = queue_to_ptr(PASS_RDRAM queue_);
    while (*link != NULLPTR) {
        OSThread* current = TO_PTR(OSThread, *link);
        if (*link == t_) {
            *link = current->next;
            return true;
        }
        link = &current->next;
    }
    return false;
}

'''


def prepare(source, destination):
    original = Path(source) / 'ultramodern/src/threadqueue.cpp'
    raw = original.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise ValueError('SDK thread queue source fingerprint changed')
    text = raw.decode()
    for site in (REMOVE_BEGIN, REMOVE_END):
        if text.count(site) != 1:
            raise ValueError('SDK thread queue transform site is not unique')
    begin, end = text.index(REMOVE_BEGIN), text.index(REMOVE_END)
    if end <= begin:
        raise ValueError('SDK thread queue transform sites are out of order')
    text = text[:begin] + REMOVAL + text[end:]
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    output = destination / 'blastcorps-threadqueue.cpp'
    if not output.exists() or output.read_text() != text:
        output.write_text(text)
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
