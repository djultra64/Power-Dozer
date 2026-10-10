#!/usr/bin/env python3
"""Prepare the pinned SDK timer with immutable host-owned scheduling records."""
import argparse
import hashlib
from pathlib import Path

EXPECTED = '5320da1c67d44425f4f13b58d0d3e7afae3029c8142557f9e734083a2f75617f'

TIMER_THREAD = r'''void timer_thread(RDRAM_ARG1) {
    ultramodern::set_native_thread_name("Timer Thread");
    ultramodern::set_native_thread_priority(ultramodern::ThreadPriority::VeryHigh);

    // Scheduling keys and delivery data belong exclusively to this thread.
    // osSetTimer can reuse its guest object without mutating an ordered key.
    auto timer_sort = [](const AddTimerAction& a, const AddTimerAction& b) {
        return a.timestamp != b.timestamp ? a.timestamp < b.timestamp : a.timer < b.timer;
    };
    std::set<AddTimerAction, decltype(timer_sort)> active_timers{timer_sort};
    auto remove_timer = [&](PTR(OSTimer) address) {
        for (auto it = active_timers.begin(); it != active_timers.end(); ++it) {
            if (it->timer == address) { active_timers.erase(it); break; }
        }
    };
    auto process_timer_action = [&](const Action& action) {
        if (const auto* add = std::get_if<AddTimerAction>(&action)) {
            remove_timer(add->timer);
            active_timers.insert(*add);
        } else if (const auto* remove = std::get_if<RemoveTimerAction>(&action)) {
            remove_timer(remove->timer);
        }
    };
    while (true) {
        Action action;
        while (timer_context.action_queue.try_dequeue(action)) process_timer_action(action);
        while (active_timers.empty()) {
            timer_context.action_queue.wait_dequeue(action);
            process_timer_action(action);
        }
        auto current = *active_timers.begin();
        active_timers.erase(active_timers.begin());
        auto wait_duration = ticks_to_timepoint(current.timestamp) - std::chrono::high_resolution_clock::now();
        if (wait_duration.count() >= 0 && timer_context.action_queue.wait_dequeue_timed(action, wait_duration)) {
            // Restore before processing replacement/cancellation of this address.
            active_timers.insert(current);
            process_timer_action(action);
        } else {
            ultramodern::enqueue_external_message_src(current.mq, current.msg, false, ultramodern::EventMessageSource::Timer);
            if (current.interval != 0) {
                current.timestamp = current.interval + time_now();
                active_timers.insert(current);
            }
        }
    }
}

'''

def prepare(source, destination):
    original = Path(source) / 'ultramodern/src/timer.cpp'
    raw = original.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise ValueError('SDK timer source fingerprint changed')
    text = raw.decode()
    old_action = 'struct AddTimerAction {\n    PTR(OSTimer) timer;\n};'
    new_action = '''struct AddTimerAction {
    PTR(OSTimer) timer;
    OSTime timestamp;
    OSTime interval;
    PTR(OSMesgQueue) mq;
    OSMesg msg;
};'''
    enqueue = 'timer_context.action_queue.enqueue(AddTimerAction{ t_ });'
    start = 'void timer_thread(RDRAM_ARG1) {'
    end = 'void ultramodern::init_timers(RDRAM_ARG1) {'
    for site in (old_action, enqueue, start, end):
        if text.count(site) != 1:
            raise ValueError('SDK timer transform site is not unique')
    text = text.replace(old_action, new_action)
    text = text.replace(enqueue, 'timer_context.action_queue.enqueue(AddTimerAction{ t_, t->timestamp, interval, mq, msg });')
    begin, finish = text.index(start), text.index(end)
    text = text[:begin] + TIMER_THREAD + text[finish:]
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    output = destination / 'blastcorps-timer.cpp'
    if not output.exists() or output.read_text() != text:
        output.write_text(text)
    return output

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
