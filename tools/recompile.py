#!/usr/bin/env python3
"""Experimental static recompilation using upstream boundaries and spimdisasm."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tetrisphere', type=Path, required=True)
    args = parser.parse_args()
    tetra = args.tetrisphere.resolve()
    deps = tetra / '.local/deps'
    sys.path.insert(0, str(deps / 'spimdisasm'))
    sys.path.insert(0, str(tetra / '.local/m1/analysis-venv/lib/python3.14/site-packages'))
    import yaml
    from spimdisasm import common, mips
    common.GlobalConfig.COMPILER = common.Compiler.IDO
    common.GlobalConfig.QUIET = True
    common.GlobalConfig.ASM_COMMENT = False
    upstream = ROOT / '.local/upstream/blastcorps'
    spec = yaml.safe_load((upstream / 'game/game.us.v11.yaml').read_text())
    meta = json.loads((ROOT / '.local/sections.json').read_text())
    image = (ROOT / '.local/code.bin').read_bytes()
    names = {}
    for file in [upstream / 'game/symbol_addrs.us.v11.txt', upstream / 'game/symbol_addrs.hd_code.us.v11.txt']:
        for name, address in re.findall(r'^([\w]+)\s*=\s*(0x[\da-fA-F]+);', file.read_text(), re.M):
            names[int(address, 16)] = name
    wrappers = set()
    runtime = deps / 'N64ModernRuntime'
    for name in ['ultra_translation', 'cont', 'vi']:
        wrappers.update(re.findall(r'void\s+(\w+)_recomp\(', (runtime / f'librecomp/src/{name}.cpp').read_text()))
    custom = {'osDpSetStatus', 'osInitialize', 'osCreatePiManager', 'osPiStartDma', 'osEPiStartDma',
              'osEPiLinkHandle', 'osCartRomInit', 'osPiRawReadIo', 'osEPiRawReadIo',
              'osAiSetNextBuffer', 'osAiGetLength', 'osAiSetFrequency', 'osSpTaskLoad',
              'osSpTaskStartGo', 'osSpTaskYield', 'osSpTaskYielded',
              'osEepromProbe', 'osEepromRead', 'osEepromWrite', 'osEepromLongRead', 'osEepromLongWrite',
              'osSyncPrintf', 'rmonPrintf', 'osPfsInit', 'osPfsIsPlug', 'osDpGetStatus',
              '__d_to_ll', '__f_to_ll', '__d_to_ull', '__f_to_ull',
              '__ll_to_d', '__ll_to_f', '__ull_to_d', '__ull_to_f'}
    bindings = {}
    sections = []
    for entry in meta:
        segment = next(s for s in spec['segments'] if isinstance(s, dict) and s['name'] == entry['name'])
        begin = segment['start']
        end = begin + entry['size']
        chunks = sorted([s for s in segment['subsegments'] if isinstance(s, list) and len(s)>1], key=lambda s:s[0])
        ctx = common.Context()
        ctx.changeGlobalSegmentRanges(0, len(image), 0x80000000, 0x80800000)
        functions = {}
        for index, chunk in enumerate(chunks):
            if chunk[1] not in ('c', 'asm', 'hasm') or chunk[2] == 'hd_code/8E910' or chunk[2].endswith('_data'):
                continue
            start = chunk[0] + (0x804 if chunk[2]=='hd_code/68810' else 0)
            stop = next((c[0] for c in chunks[index+1:] if c[0]>start), end)
            if chunk[2]=='hd_code/68810':
                stop = chunk[0] + 0x8B0
            if stop <= start or start < begin or stop > end:
                continue
            vram = entry['vram'] + start - begin
            rom = entry['rom'] + start - begin
            for address, name in names.items():
                if vram <= address < vram+stop-start:
                    sym = ctx.globalSegment.addFunction(address, vromAddress=rom+address-vram)
                    sym.name = f'func_{address:08X}'
                    sym.isUserDeclared = True
            part = mips.sections.SectionText(ctx, rom, rom+stop-start, vram,
                                             chunk[2], image, 0, None)
            part.isHandwritten = chunk[1]=='hasm'
            part.analyze()
            for function in part.symbolList:
                address = function.vram
                original = names.get(address, '')
                privileged = any(((w >> 26) == 0x10 and ((w >> 21) & 31) not in (0,4)) or ((w >> 26) == 0x10 and ((w >> 21) & 31)==4 and ((w>>11)&31)!=12) for w in (i.getRaw() for i in function.instructions))
                name = f'func_{address:08X}'
                # Runtime services replace original hardware OS bodies.
                if address in (0x802DBFB8,0x80202024):
                    original = 'internal_'+f'{address:08X}'
                    custom.add(original)
                if privileged and not original.startswith('__os') and original not in wrappers | custom:
                    original = 'privileged_'+f'{address:08X}'
                # CRC helpers have no hardware access; keep their original CPU code.
                pure_os = original in {'__osContDataCrc', '__osContAddressCrc'}
                if original in wrappers | custom or (original.startswith(('__os','privileged_')) and not pure_os):
                    name = original+'_recomp' if original in wrappers else 'bc_'+original
                    bindings[name] = dict(address=address, original=original,
                                          delegated=original in wrappers)
                size = {0x802C61F0:0xCDC, 0x80201E80:0x280}.get(address,function.sizew*4)
                functions[address] = dict(name=name, vram=address, size=size)
        sections.append(dict(**entry, functions=sorted(functions.values(), key=lambda x:x['vram'])))
    # An address can be inferred by more than one chunk; dictionary deduplication is deliberate.
    out = ROOT / '.local/generated'
    out.mkdir(exist_ok=True)
    lines = []
    for section in sections:
        lines += ['[[section]]', f'name = "{section["name"]}"',
                  f'rom = 0x{section["rom"]:X}', f'vram = 0x{section["vram"]:X}',
                  f'size = 0x{section["size"]:X}', 'functions = [']
        lines += [f'  {{ name = "{f["name"]}", vram = 0x{f["vram"]:X}, size = 0x{f["size"]:X} }},' for f in section['functions']]
        lines += [']']
    (out / 'symbols.toml').write_text('\n'.join(lines)+'\n')
    (out / 'bindings.json').write_text(json.dumps(bindings, indent=2)+'\n')
    (out / 'map.json').write_text(json.dumps(sections, indent=2)+'\n')
    (out / 'recomp.toml').write_text('[input]\nentrypoint = 0x802447C0\nsymbols_file_path = "symbols.toml"\n'
        f'rom_file_path = "{ROOT}/.local/code.bin"\noutput_func_path = "{out}/functions"\n'
        'single_file_output = false\nfunctions_per_output_file = 50\nuses_mips3_float_mode = false\n'
        'recomp_include = "#include \\\"support.h\\\""\n[patches]\nignored = '+json.dumps(sorted(bindings))+'\n')
    count = sum(len(s['functions']) for s in sections)
    print(json.dumps(dict(event='symbols_analyzed', functions=count, bindings=len(bindings))), flush=True)
    tool = tetra / '.local/m1/toolchain/n64recomp-build/N64Recomp'
    for stale in (out / 'functions').glob('funcs*'):
        stale.unlink()
    result = subprocess.run([str(tool), str(out / 'recomp.toml')])
    if result.returncode:
        sys.exit(result.returncode)
    normalize(out / 'functions')
    header = (out / 'functions/funcs.h').read_text()
    all_funcs = {f['vram']:f['name'] for s in sections for f in s['functions']}
    for name, address in re.findall(r'void (func_([0-9A-Fa-f]{8}))\(', header):
        all_funcs[int(address,16)] = name
    all_funcs[0x802447C0] = 'recomp_entrypoint'
    (out / 'functions/dispatch.inc').write_text('\n'.join(f'case 0x{a:08X}u: return {n};' for a,n in sorted(all_funcs.items()))+'\n')
    (out / 'functions/bindings.h').write_text('\n'.join(f'extern "C" void {n}(uint8_t*, recomp_context*);' for n in bindings)+'\n')

    native = (ROOT / 'src/runtime.cpp').read_text()
    implemented = set(re.findall(r'void (bc_\w+)\(', native))
    fallback = ['#include "support.h"', '#include "prototype.h"', 'extern "C" {']
    for name, binding in bindings.items():
        if not binding['delegated'] and name not in implemented:
            fallback.append(f'void {name}(uint8_t*, recomp_context*) {{ bc_fatal("{binding["original"]}", 0x{binding["address"]:08X}u); }}')
    (out / 'fallbacks.cpp').write_text('\n'.join(fallback+['}'])+'\n')

def normalize(folder):
    pattern = re.compile(r'RECOMP_FUNC void [^(]+\([^)]*\) \{.*?^;\}', re.M | re.S)
    temporary = re.compile(r'^(?P<indent>\s*)gpr (?P<name>jr_addend_[0-9A-F]{8}) = (?P<value>[^;]+);$', re.M)
    def fix(match):
        body = match.group(0)
        names = list(dict.fromkeys(m['name'] for m in temporary.finditer(body) if m['value'].strip()!='0'))
        body = temporary.sub(lambda m:m.group(0) if m['value'].strip()=='0' else f"{m['indent']}{m['name']} = {m['value']};", body)
        if names:
            at = body.index('\n')+1
            body = body[:at]+''.join(f'    gpr {n} = 0;\n' for n in names)+body[at:]
        if body.startswith('RECOMP_FUNC void func_80244930('):
            body=body.replace('    bc_guest_state(rdram);\n','')
            body=body.replace('    yield_self_1ms(rdram);\n    yield_self_1ms(rdram);','    yield_self_1ms(rdram);')
            for label in ['L_80244CE8','L_80245E70']:
                if label+':\n    yield_self_1ms(rdram);' not in body:
                    body = body.replace(label+':', label+':\n    yield_self_1ms(rdram);')
            body=body.replace('L_802473F0:', 'L_802473F0:\n    bc_guest_state(rdram);')
        if body.startswith('RECOMP_FUNC void func_80278BF0(') and 'bc_guest_render_mode' not in body:
            body=body.replace('    // 0x80278D38: lui', '    bc_guest_render_mode(rdram,ctx);\n    // 0x80278D38: lui')
        if body.startswith('RECOMP_FUNC void func_80278BF0(') and 'bc_guest_translucent_mode' not in body:
            rom=(ROOT/'.local/hd_code.bin').read_bytes()
            at=0x802FC060-0x802447C0
            assert rom[at:at+24].hex()=='0055207800552230005049d800504a500055307800553078', 'Original translucent table changed'
            body=body.replace('L_80278D2C:', 'L_80278D2C:\n    bc_guest_translucent_mode(rdram,ctx,0);')
            lookup='    ctx->r15 = MEM_W(ctx->r15, -0X3F9C);'
            assert body.count(lookup)==1
            body=body.replace(lookup,lookup+'\n    bc_guest_translucent_mode(rdram,ctx,1);')
        if body.startswith('RECOMP_FUNC void func_8028FCD4(') and 'yield_self_1ms(rdram)' not in body:
            body = body.replace('L_8028FD04:', 'L_8028FD04:\n    yield_self_1ms(rdram);')
        for name,draw in [('func_8026AF6C','false'),('func_8026BCE0','true')]:
            if body.startswith('RECOMP_FUNC void '+name+'(') and 'bc_guest_window_trace' not in body:
                at=body.index('\n')+1
                body=body[:at]+f'    bc_guest_window_trace(rdram,ctx,{draw});\n'+body[at:]
        if body.startswith('RECOMP_FUNC void func_80274BF0(') and 'bc_original_fade_window' not in body:
            rom=(ROOT/'.local/hd_code.bin').read_bytes()
            at=0x80274CB8-0x802447C0
            assert rom[at:at+8].hex()=='0c09abdb00002025', 'Original fade cancellation changed'
            cancel='    ctx->r4 = 0 | 0;\n    func_8026AF6C(rdram, ctx);'
            assert body.count(cancel)==1
            body=body.replace(cancel,'    ctx->r4 = 0 | 0;\n    bc_original_fade_window(rdram,ctx,0);\n    func_8026AF6C(rdram, ctx);\n    bc_original_fade_window(rdram,ctx,1);')
            restore='    ctx->r4 = MEM_HU(ctx->r4, -0X3890);\n    func_8026AF6C(rdram, ctx);'
            assert body.count(restore)==1
            body=body.replace(restore,'    ctx->r4 = MEM_HU(ctx->r4, -0X3890);\n    bc_original_fade_window(rdram,ctx,2);\n    func_8026AF6C(rdram, ctx);')
        if body.startswith('RECOMP_FUNC void func_8029CB04('):
            rom = (ROOT / '.local/hd_code.bin').read_bytes()
            a,b = 0x8029CB40-0x802447C0,0x8029CF40-0x802447C0
            assert rom[a:a+20] == rom[b:b+20], 'Shared epilogue changed'
            if 'L_8029CF40:' not in body:
                body = body.replace('L_8029CB40:', 'L_8029CF40:\nL_8029CB40:')
        return re.sub(r'(DDIV|DMULT)\(([^,]+), ([^,]+), &(\w+), &(\w+)\);', r'\1(\2, \3, reinterpret_cast<int64_t*>(&\4), reinterpret_cast<int64_t*>(&\5));',body)
    for path in folder.glob('funcs_*.c'):
        source = path.read_text()
        fixed = pattern.sub(fix,source)
        if fixed != source: path.write_text(fixed)

if __name__ == '__main__': main()
