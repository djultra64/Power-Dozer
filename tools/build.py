#!/usr/bin/env python3
"""Reproduce the local experimental build; never distribute its ROM-derived files."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from sdk_paths import sdk_paths, sdk_cmake_options

ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run([str(a) for a in args], cwd=ROOT, check=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    sdk=parser.add_mutually_exclusive_group(required=True)
    sdk.add_argument('--tetrisphere', type=Path)
    sdk.add_argument('--sdk-root',type=Path)
    parser.add_argument('--mapping-source',type=Path)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build')
    args = parser.parse_args()
    paths=sdk_paths(sdk_root=args.sdk_root,tetrisphere=args.tetrisphere)
    lock = json.loads((ROOT/'config/dependencies.json').read_text())
    upstream=(args.mapping_source or (ROOT.parent/'mapping' if args.sdk_root and (ROOT.parent/'mapping').is_dir() else ROOT/'.local/upstream/blastcorps')).resolve()
    if not upstream.exists():
        upstream.parent.mkdir(parents=True, exist_ok=True)
        run('git', 'clone', lock['sources']['mapping']['url'], upstream)
        run('git', '-C', upstream, 'checkout', '--detach', lock['sources']['mapping']['commit'])
    source_lock=json.loads((ROOT/'config/source-build.json').read_text())
    checkouts=[
        (paths['deps']/('rt64' if name=='RT64' else name), lock['sources'][name])
        for name in ('N64Recomp','N64ModernRuntime','RT64')]
    if args.sdk_root:checkouts.append((paths['deps']/'spimdisasm',source_lock['dependencies']['spimdisasm']['commit']))
    for path,commit in [(upstream,lock['sources']['mapping']['commit']),*checkouts]:
        if args.sdk_root and not (path/'.git').exists():
            from bootstrap_sdk import verify_source_snapshot
            source_manifest=paths['deps'].parent/'SOURCE-INPUTS.json'
            verify_source_snapshot(source_manifest,path,'mapping' if path==upstream else 'vendor/'+path.name)
            continue
        actual = subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'], text=True).strip()
        if actual != commit:
            raise SystemExit(f'Dependency revision mismatch: {path}: {actual}')
    if args.sdk_root:
        from bootstrap_sdk import verify_prepared_rt64, verify_sdk_binary_inputs
        verify_prepared_rt64(args.sdk_root,source_lock['rt64_cmake_sha256'])
        verify_sdk_binary_inputs(args.sdk_root,source_lock)
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
    run(paths['tools']/'RSPRecomp', private/'audio-rsp.toml')
    sdk_option=['--sdk-root',args.sdk_root] if args.sdk_root else ['--tetrisphere',args.tetrisphere]
    run(sys.executable, ROOT/'tools/recompile.py', *sdk_option,'--mapping-source',upstream)
    controller=(args.sdk_root/'gamecontrollerdb.txt') if args.sdk_root else (args.tetrisphere/'assets/controllers/gamecontrollerdb.txt')
    shutil.copyfile(controller, private/'gamecontrollerdb.txt')
    cmake_options=sdk_cmake_options(sdk_root=args.sdk_root,tetrisphere=args.tetrisphere)
    run('cmake','-S',ROOT,'-B',args.build_dir.resolve(),'-G','Ninja',
        '-DCMAKE_BUILD_TYPE=RelWithDebInfo',*cmake_options)
    run('cmake','--build',args.build_dir.resolve(),'-j','4')

if __name__ == '__main__':
    main()
