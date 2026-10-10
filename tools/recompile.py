#!/usr/bin/env python3
"""Experimental static recompilation using upstream boundaries and spimdisasm."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from sdk_paths import sdk_paths

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sdk=parser.add_mutually_exclusive_group(required=True)
    sdk.add_argument('--tetrisphere',type=Path)
    sdk.add_argument('--sdk-root',type=Path)
    parser.add_argument('--mapping-source',type=Path)
    args = parser.parse_args()
    paths=sdk_paths(sdk_root=args.sdk_root,tetrisphere=args.tetrisphere)
    deps = paths['deps']
    sys.path.insert(0, str(deps / 'spimdisasm'))
    for packages in sorted(paths['analysis_venv'].glob('lib/python*/site-packages')):
        sys.path.insert(0,str(packages))
    import yaml
    from spimdisasm import common, mips
    common.GlobalConfig.COMPILER = common.Compiler.IDO
    common.GlobalConfig.QUIET = True
    common.GlobalConfig.ASM_COMMENT = False
    upstream=(args.mapping_source or ROOT/'.local/upstream/blastcorps').resolve()
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
              'osPfsFreeBlocks', 'osPfsChecker', 'osPfsAllocateFile', 'osPfsFindFile',
              'osPfsDeleteFile', 'osPfsReadWriteFile', 'osPfsFileState', 'osContInit', 'osContGetQuery',
              '__d_to_ll', '__f_to_ll', '__d_to_ull', '__f_to_ull',
              '__ll_to_d', '__ll_to_f', '__ull_to_d', '__ull_to_f'}
    # Keep controller sampling in the established runtime and extend only its
    # card-presence status for the host Controller Pak services.
    wrappers.difference_update({'osContInit','osContGetQuery'})
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
    tool = paths['tools']/'N64Recomp'
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

    native = '\n'.join(path.read_text() for path in (ROOT/'src').glob('*.cpp'))
    implemented = set(re.findall(r'void (bc_\w+)\(', native))
    fallback = ['#include "support.h"', '#include "prototype.h"', 'extern "C" {']
    for name, binding in bindings.items():
        if not binding['delegated'] and name not in implemented:
            fallback.append(f'void {name}(uint8_t*, recomp_context*) {{ bc_fatal("{binding["original"]}", 0x{binding["address"]:08X}u); }}')
    (out / 'fallbacks.cpp').write_text('\n'.join(fallback+['}'])+'\n')

def normalize_widescreen_body(body):
    """Scope original 4:3 geometry changes to known fullscreen/ticker producers."""
    def once(before,after,label):
        nonlocal body
        if body.count(before)!=1:raise ValueError('Original widescreen '+label+' layout changed')
        body=body.replace(before,after)
    if body.startswith('RECOMP_FUNC void func_80271FD0(') and 'bc_widescreen_sky' not in body:
        site='    MEM_B(-0X403B, ctx->r1) = ctx->r12;'
        once(site,'    bc_widescreen_sky(rdram);\n'+site,'sky buffer flip')
    if body.startswith('RECOMP_FUNC void func_8025E67C(') and 'bc_widescreen_whiteout' not in body:
        at=body.index('\n')+1;body=body[:at]+'    bc_widescreen_whiteout(rdram);\n'+body[at:]
    if body.startswith('RECOMP_FUNC void func_801E9718(') and 'bc_widescreen_ticker(rdram)' not in body:
        for name in ('func_80259CCC','func_80259DC8'):
            site='    '+name+'(rdram, ctx);'
            once(site,'    bc_widescreen_ticker_text(rdram,ctx);\n'+site,'ticker text call')
        at=body.index('\n')+1;body=body[:at]+'    bc_widescreen_ticker(rdram);\n'+body[at:]
    if body.startswith('RECOMP_FUNC void func_801E8DCC(') and '(bc_wide ? 0X24 : 0X1B)' not in body:
        site='    ctx->r1 = SIGNED(ctx->r10) < 0X1B ? 1 : 0;'
        once(site,'    ctx->r1 = SIGNED(ctx->r10) < (bc_wide ? 0X24 : 0X1B) ? 1 : 0;','ticker FIFO length')
    return body


def normalize_map_vehicle_body(body):
    """Repair only the map's two mirrored RGBA16 vehicle sprite producers."""
    if not body.startswith('RECOMP_FUNC void func_801F9258(') or 'bc_map_vehicle_sprite_rewrite' in body:
        return body
    site='    func_80272ED8(rdram, ctx);'
    if body.count(site)!=2:raise ValueError('Original map vehicle sprite caller layout changed')
    at=body.index('\n')+1
    body=body[:at]+'    uint32_t bc_map_sprite_begin=0;\n    bool bc_map_sprite_eligible=false;\n'+body[at:]
    hook=('    bc_map_sprite_begin=uint32_t(ctx->r4);\n'
          '    bc_map_sprite_eligible=bc_map_vehicle_sprite_eligible(rdram,uint32_t(ctx->r5));\n'+site+'\n'
          '    bc_map_vehicle_sprite_rewrite(rdram,bc_map_sprite_begin,uint32_t(ctx->r2),bc_map_sprite_eligible);')
    return body.replace(site,hook)


def normalize(folder):
    # These hooks depend on the original pause layout and callable entries.
    rom=(ROOT/'.local/hd_code.bin').read_bytes()
    prompt_layout=(ROOT/'include/game_prompts.h').read_text()
    for encoded,text in re.findall(r'\{0x([0-9A-F]{8}),"([^"]+)"\}',prompt_layout):
        at=int(encoded,16)-0x802447C0
        if rom[at:at+len(text)+1]!=text.encode('ascii')+b'\0':
            raise ValueError(f'Original game prompt changed at {encoded}')
    layout=(ROOT/'include/blast_save_format.h').read_text()
    for name,address,count,stride in [('blast_level_types',0x802E8F94,60,0x44),('blast_vehicle_time_slots',0x802E8C44,16,1)]:
        encoded=re.search(r'\b'+name+r'=\{([^}]+)\}',layout,re.S)
        expected=bytes(int(value,16) for value in re.findall(r'0x([0-9A-Fa-f]+)',encoded.group(1)))
        actual=bytes(rom[address-0x802447C0+i*stride] for i in range(count))
        if expected!=actual:
            raise ValueError(f'Original EEPROM migration layout changed: {name}')
    for address,expected in (
        (0x802F8BDC,'010000d000200010f1000039000000000005001b001c001d00000000'),
        (0x8024BDA4,'27bdffd03c0e8036'),(0x8026BCE0,'3c0f80313c188037'),
        (0x80259EC4,'27bdffc0afa500448fae0044'),
        (0x80259450,'3c01802fac208c703c01802fac208c743c01802f03e00008ac208c78'),
        (0x80259824,'27bdffa83c18802f'),(0x802E8C84,'3f19999a3f733333'),(0x80259BC8,'27bd005803e0000800000000'),
        (0x8026EF68,'03e0000827bd0150')):
        at=address-0x802447C0
        if rom[at:at+len(expected)//2].hex()!=expected:
            raise ValueError(f'Original PC options hook layout changed at {address:08X}')
    # Pin both overlays: these data layouts and original instruction sites
    # must match the user's validated USA Rev 1 image before applying hooks.
    front=(ROOT/'.local/hd_front_end.bin').read_bytes()
    for data,base,address,expected in (
        (rom,0x802447C0,0x802FA820,'04ff0000fc18000000000000ffffffff'),
        (rom,0x802447C0,0x802FA8B0,'00000000fff600000000000000000000'),
        (front,0x801E7000,0x80208380,'ff6000ae00b400000000000000810028'),
        (front,0x801E7000,0x801E8E1C,'2941001b1420fff1'),
        (front,0x801E7000,0x801E9FB4,'0c096733afb00034'),
        (front,0x801E7000,0x801EA0C0,'0c096772afad0044'),
        (front,0x801E7000,0x801F968C,'0c09cbb6e7a40018'),
        (front,0x801E7000,0x801F96F4,'0c09cbb6e7a60018')):
        at=address-base
        if data[at:at+len(expected)//2].hex()!=expected:
            raise ValueError(f'Original widescreen hook layout changed at {address:08X}')
    pattern = re.compile(r'RECOMP_FUNC void [^(]+\([^)]*\) \{.*?^;\}', re.M | re.S)
    temporary = re.compile(r'^(?P<indent>\s*)gpr (?P<name>jr_addend_[0-9A-F]{8}) = (?P<value>[^;]+);$', re.M)
    def fix(match):
        body = normalize_map_vehicle_body(normalize_widescreen_body(match.group(0)))
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
        if body.startswith('RECOMP_FUNC void func_8026BCE0(') and 'bc_pc_options_draw' not in body:
            at=body.index('\n')+1;body=body[:at]+'    bc_pc_options_draw(rdram);\n'+body[at:]
        if body.startswith('RECOMP_FUNC void func_8026BCE0(') and 'bc_pc_options_glyphs' not in body:
            at=body.index('\n')+1;body=body[:at]+'    const uint32_t bc_glyph_frame = uint32_t(ctx->r5);\n'+body[at:]
            final='    ctx->r29 = ADD32(ctx->r29, 0X150);\n    return;'
            if body.count(final)!=1:raise ValueError('Original PC glyph return layout changed')
            body=body.replace(final,'    ctx->r29 = ADD32(ctx->r29, 0X150);\n    bc_pc_options_glyphs(rdram,ctx,bc_glyph_frame);\n    return;')
        if body.startswith('RECOMP_FUNC void func_80259450(') and 'bc_game_prompt_reset' not in body:
            at=body.index('\n')+1;body=body[:at]+'    bc_game_prompt_reset();\n'+body[at:]
        if body.startswith('RECOMP_FUNC void func_80259824(') and 'bc_game_prompt_flush' not in body:
            at=body.index('\n')+1
            body=body[:at]+'    const uint32_t bc_prompt_reference = uint32_t(ctx->r4), bc_prompt_frame = uint32_t(ctx->r5);\n'+body[at:]
            if body.count('    return;')!=1 or 'ctx->r29 = ADD32(ctx->r29, 0X58);' not in body:raise ValueError('Original instruction glyph flush return layout changed')
            body=body.replace('    return;','    bc_game_prompt_flush(rdram,bc_prompt_reference,bc_prompt_frame);\n    return;')
        if body.startswith('RECOMP_FUNC void func_80259EC4(') and 'bc_game_prompt_text' not in body:
            at=body.index('\n')+1;body=body[:at]+'    bc_game_prompt_text(rdram,ctx);\n'+body[at:]
        if body.startswith('RECOMP_FUNC void func_8024BDA4(') and 'bc_pc_options_selection' not in body:
            at=body.index('\n')+1;body=body[:at]+'    if(bc_pc_options_selection(rdram,ctx))return;\n'+body[at:]
        if body.startswith('RECOMP_FUNC void func_8026BCE0(') and 'bc_original_window_off' not in body:
            rom=(ROOT/'.local/hd_code.bin').read_bytes();at=0x8026BF7C-0x802447C0
            assert rom[at:at+8].hex()=='0c09abdb24044000', 'Original mission-failure cancellation changed'
            call='    ctx->r4 = ADD32(0, 0X4000);\n    func_8026AF6C(rdram, ctx);'
            assert body.count(call)==1
            body=body.replace(call,'    ctx->r4 = ADD32(0, 0X4000);\n    bc_original_window_off(rdram,0);\n    func_8026AF6C(rdram, ctx);\n    bc_original_window_off(rdram,1);')
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
