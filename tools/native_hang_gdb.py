"""Bounded read-only GDB dump; executed by GDB, never calls inferior functions."""
import gdb


def atomic_value(value):
    for field in ('_M_i', '_M_p', '_M_b', '_M_base'):
        try:
            child = value[field]
        except gdb.error:
            continue
        if child.type.strip_typedefs().code in (gdb.TYPE_CODE_INT, gdb.TYPE_CODE_BOOL, gdb.TYPE_CODE_PTR):
            return child
        return atomic_value(child)
    raise ValueError('Unsupported atomic debug layout')


def report(label, expression):
    try:
        print(label + ': ' + str(gdb.parse_and_eval(expression)))
    except (gdb.error, ValueError) as error:
        print(label + ': unavailable: ' + str(error))


for name in ('running_queue_impl', 'rsp_scheduler', 'dp_status', 'pending_gfx', 'bc_dp_sync'):
    report(name, name)
report('host_vi', 'events_context.vi')
report('guest_current_rdp', '*(unsigned int*)(events_context.rdram+0x36bf1c)')
report('guest_next_retrace', '*(unsigned int*)(events_context.rdram+0x36bf14)')
report('sound_player', '*(unsigned int*)(events_context.rdram+0x2e8cec)')
report('sound_free_head', '*(unsigned int*)(events_context.rdram+0x366be4)')
report('sound_alloc_head', '*(unsigned int*)(events_context.rdram+0x366bec)')
report('sound_next_type', '*(unsigned short*)(events_context.rdram+0x366bf8+2)')
report('sound_next_delta', '*(unsigned int*)(events_context.rdram+0x366c20)')

for name, offset in (('idle1', 0x310820), ('main3', 0x310BD0),
                     ('scheduler5', 0x3154F0), ('audio4', 0x368088),
                     ('frontend2', 0x218D30)):
    report('guest_thread_' + name, '*(OSThread*)(events_context.rdram+' + hex(offset) + ')')
    report('guest_resume_' + name, '((OSThread*)(events_context.rdram+' + hex(offset) + '))->context->running')

try:
    queue = gdb.parse_and_eval('external_messages')
    inner = queue['inner']
    print('external_producer_count: ' + str(atomic_value(inner['producerCount'])))
    visualizer = gdb.default_visualizer(queue['sema'])
    if visualizer:
        for _, child in visualizer.children():
            if child.type.strip_typedefs().code == gdb.TYPE_CODE_PTR and 'LightweightSemaphore' in str(child.type.target()):
                if int(child):
                    print('external_semaphore: ' + str(child.dereference()))
                break
    producer = atomic_value(inner['producerListTail'])
    seen = set()
    for index in range(16):
        address = int(producer)
        if not address:
            print('external_producer_end')
            break
        if address in seen:
            print('external_producer_cycle: ' + hex(address))
            break
        seen.add(address)
        value = producer.dereference()
        fields = {name: str(atomic_value(value[name])) for name in
                  ('tailIndex', 'headIndex', 'dequeueOptimisticCount', 'dequeueOvercommit')}
        fields['inactive'] = str(atomic_value(value['inactive']))
        fields['parent'] = str(value['parent'])
        print('external_producer_' + str(index) + ' ' + hex(address) + ': ' + str(fields))
        producer = value['next'].cast(producer.type)
    else:
        print('external_producer_limit_16')
except (gdb.error, ValueError) as error:
    print('external_queue_dump_unavailable: ' + str(error))
