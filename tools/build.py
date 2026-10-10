#!/usr/bin/env python3
"""Reproduce the local experimental build; never distribute its ROM-derived files."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run([str(a) for a in args], cwd=ROOT, check=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--tetrisphere', type=Path, required=True)
    args = parser.parse_args()
    tetra = args.tetrisphere.resolve()
    lock = json.loads((ROOT/'config/dependencies.json').read_text())
    upstream = ROOT/'.local/upstream/blastcorps'
    if not upstream.exists():
        upstream.parent.mkdir(parents=True, exist_ok=True)
        run('git', 'clone', lock['sources']['mapping']['url'], upstream)
        run('git', '-C', upstream, 'checkout', '--detach', lock['sources']['mapping']['commit'])
    for path, commit in [(upstream, lock['sources']['mapping']['commit'])] + [
        (tetra/'.local/deps'/('rt64' if name=='RT64' else name), lock['sources'][name])
        for name in ('N64Recomp','N64ModernRuntime','RT64')]:
        actual = subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'], text=True).strip()
        if actual != commit:
            raise SystemExit(f'Dependency revision mismatch: {path}: {actual}')
    run(sys.executable, ROOT/'tools/prepare.py', args.rom.resolve())
    private = ROOT/'.local'
    code = (private/'hd_code.bin').read_bytes()
    text = code[0x802E68F0-0x802447C0:0x802E77B0-0x802447C0]
    data = code[0x8030EB90-0x802447C0:0x8030F390-0x802447C0]
    targets = struct.unpack('>16H', data[16:48])
    if any(not 0x1080 <= address < 0x1F40 for address in targets):
        raise SystemExit('Unexpected audio dispatch table')
    (private/'audio-ucode.bin').write_bytes(text)
    (private/'audio-rsp.toml').write_text(
        'text_offset = 0\ntext_size = 0xEC0\ntext_address = 0x1080\n'
        'rom_file_path = "audio-ucode.bin"\noutput_file_path = "audio-rsp.cpp"\n'
        'output_function_name = "blastcorps_audio_rsp"\n'
        'extra_indirect_branch_targets = ['+','.join(hex(a) for a in targets)+']\n')
    run(tetra/'.local/m1/toolchain/n64recomp-build/RSPRecomp', private/'audio-rsp.toml')
    run(sys.executable, ROOT/'tools/recompile.py', '--tetrisphere', tetra)
    shutil.copyfile(tetra/'assets/controllers/gamecontrollerdb.txt', private/'gamecontrollerdb.txt')
    run('cmake','-S',ROOT,'-B',ROOT/'build','-G','Ninja',
        '-DCMAKE_BUILD_TYPE=RelWithDebInfo',f'-DTETRISPHERE_ROOT={tetra}')
    run('cmake','--build',ROOT/'build','-j','4')

if __name__ == '__main__':
    main()
