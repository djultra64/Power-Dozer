#!/usr/bin/env python3
"""Prepare pinned SDK threads so external destruction finishes before RAM reuse."""
import argparse
import hashlib
from pathlib import Path

EXPECTED = 'a2fef03027eece8d5ae6346b4f8695ed3fb2b03217069e6b8ef4a80995771468'
CALLBACK_SITE = 'static ultramodern::threads::callbacks_t threads_callbacks;'
DESTROY_SITE = '''        t->context = nullptr;
        cur_context->running.signal();
'''
CLEANUP_SITE = '''void ultramodern::cleanup_thread(UltraThreadContext *cur_context) {
    deleted_threads.enqueue(cur_context);
}'''

CALLBACK_REPLACEMENT = '''static ultramodern::threads::callbacks_t threads_callbacks;
// External osDestroyThread owns join/deletion; normal/self exits retain the
// SDK cleanup worker. Never hold this mutex while waking or joining a thread.
static std::mutex destroyed_context_mutex;
static std::unordered_set<UltraThreadContext*> destroyed_contexts;'''
DESTROY_REPLACEMENT = '''        {
            std::lock_guard lock(destroyed_context_mutex);
            destroyed_contexts.insert(cur_context);
        }
        t->context = nullptr;
        cur_context->running.signal();
        // The cancelled host thread still checks its guest OSThread after
        // waking. Finish those reads before the game reuses/unloads its RAM.
        cur_context->host_thread.join();
        delete cur_context;
'''
CLEANUP_REPLACEMENT = '''void ultramodern::cleanup_thread(UltraThreadContext *cur_context) {
    {
        std::lock_guard lock(destroyed_context_mutex);
        if (destroyed_contexts.erase(cur_context)) return;
    }
    deleted_threads.enqueue(cur_context);
}'''


def prepare(source, destination):
    original = Path(source) / 'ultramodern/src/threads.cpp'
    raw = original.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise ValueError('SDK thread lifecycle source fingerprint changed')
    text = raw.decode()
    replacements = (('#include <string>', '#include <string>\n#include <mutex>\n#include <unordered_set>'),
                    (CALLBACK_SITE, CALLBACK_REPLACEMENT),
                    (DESTROY_SITE, DESTROY_REPLACEMENT),
                    (CLEANUP_SITE, CLEANUP_REPLACEMENT))
    for site, _ in replacements:
        if text.count(site) != 1:
            raise ValueError('SDK thread lifecycle transform site is not unique')
    for site, replacement in replacements:
        text = text.replace(site, replacement)
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    output = destination / 'blastcorps-threads.cpp'
    if not output.exists() or output.read_text() != text:
        output.write_text(text)
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
