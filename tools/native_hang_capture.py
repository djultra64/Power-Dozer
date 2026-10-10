"""Capture two bounded native snapshots, resuming briefly between them."""
from pathlib import Path
import subprocess
import time


def capture(debugger, pid, dump):
    script = Path(__file__).with_name('native_hang_gdb.py')
    commands = ['set pagination off', 'set debuginfod enabled off',
                'set print elements 8', 'set print max-depth 6',
                'thread apply all bt 24', 'p/x events_context.rdram',
                'x/6wx events_context.rdram+0x315440',
                'x/6wx events_context.rdram+0x368238',
                'x/6wx events_context.rdram+0x368270',
                'x/9wx events_context.rdram+0x3156a4',
                'source ' + str(script)]
    arguments = [debugger, '--batch']
    for command in commands:
        arguments.extend(['-ex', command])
    arguments.extend(['-p', str(pid)])
    for snapshot in range(2):
        dump.write('Native snapshot ' + str(snapshot + 1) + '\n')
        dump.flush()
        try:
            result = subprocess.run(arguments, stdout=dump, stderr=subprocess.STDOUT,
                                    timeout=10, check=False)
            dump.write('Debugger exit: ' + str(result.returncode) + '\n')
        except subprocess.TimeoutExpired:
            dump.write('Debugger timed out; native transition still failed\n')
            break
        dump.flush()
        if snapshot == 0:
            time.sleep(0.15)
