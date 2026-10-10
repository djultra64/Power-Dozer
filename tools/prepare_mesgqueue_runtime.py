#!/usr/bin/env python3
"""Prepare pinned SDK message retries without starving other host producers."""
import argparse
import hashlib
import re
from pathlib import Path

EXPECTED = 'b504bd7e4290189e0e7e7b522486cc4b9ec22ae54f73297d4fa2c0e3f1575045'
BATCH_SITE = '''void dequeue_external_messages(RDRAM_ARG1) {
    QueuedMessage to_send;
    std::vector<QueuedMessage> requeued_messages{};'''
BATCH_REPLACEMENT = '''static void dequeue_external_messages_with_retry(RDRAM_ARG const QueuedMessage* first_retry) {
    QueuedMessage to_send;
    std::vector<QueuedMessage> requeued_messages{};
    // Keep the first rejected message ahead of later retries from its producer.
    if (first_retry != nullptr) requeued_messages.push_back(*first_retry);'''
WAIT_SITE = '''void ultramodern::wait_for_external_message(RDRAM_ARG1) {'''
WAIT_REPLACEMENT = '''void dequeue_external_messages(RDRAM_ARG1) {
    dequeue_external_messages_with_retry(PASS_RDRAM nullptr);
}

void ultramodern::wait_for_external_message(RDRAM_ARG1) {'''
RETRY_SITE = 'external_messages.enqueue(to_send);'
RETRY_REPLACEMENT = '''// A retry must not hide older producers behind the queue's first-three
// producer heuristic. Drain once, then requeue all rejected messages.
dequeue_external_messages_with_retry(PASS_RDRAM &to_send);'''


def prepare(source, destination):
    original = Path(source) / 'ultramodern/src/mesgqueue.cpp'
    raw = original.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise ValueError('SDK message queue source fingerprint changed')
    text = raw.decode()
    for site, count in ((BATCH_SITE, 1), (WAIT_SITE, 1), (RETRY_SITE, 2)):
        if text.count(site) != count:
            raise ValueError('SDK message queue transform sites changed')
    text = text.replace(BATCH_SITE, BATCH_REPLACEMENT)
    text = text.replace(WAIT_SITE, WAIT_REPLACEMENT)
    text = re.sub(r'(?m)^([ \t]*)' + re.escape(RETRY_SITE),
                  lambda match: '\n'.join(match.group(1) + line for line in RETRY_REPLACEMENT.splitlines()), text)
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    output = destination / 'blastcorps-mesgqueue.cpp'
    if not output.exists() or output.read_text() != text:
        output.write_text(text)
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
