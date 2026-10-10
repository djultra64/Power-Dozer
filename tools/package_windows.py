#!/usr/bin/env python3
"""Package Power Dozer Windows binaries with bounded inputs and unresolved review flags."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import tempfile
import zipfile
import package_linux

ROOT=Path(__file__).resolve().parents[1]
DLLS={'SDL2.dll','dxcompiler.dll','dxil.dll'}
TOP={'power-dozer.exe','Run Power Dozer.cmd','README.txt','gamecontrollerdb.txt','BUILD.json'}
ROM_MAGIC={bytes.fromhex(s) for s in ('80371240','37804012','40123780')}

def audit(stage):
    stage=Path(stage);files={}
    for path in sorted(stage.rglob('*')):
        relative=path.relative_to(stage)
        if path.is_symlink():raise ValueError('Package symlink: '+str(relative))
        if path.is_dir():
            if relative.parts[0]!='licenses':raise ValueError('Unexpected package directory: '+str(relative))
            continue
        allowed=(len(relative.parts)==1 and path.name in TOP|DLLS) or relative.parts[0]=='licenses'
        if not allowed or not path.is_file() or path.suffix.lower() in package_linux.PRIVATE:raise ValueError('Unlisted or private Windows member: '+str(relative))
        data=path.read_bytes()
        if data[:4] in ROM_MAGIC:raise ValueError('Disguised ROM data: '+str(relative))
        files[relative.as_posix()]=hashlib.sha256(data).hexdigest()
    return files

def launcher_text():
    return '@echo off\r\n"%~dp0power-dozer.exe" %*\r\n'

def write_archive(stage,output,*,version='0.82',commit):
    if not re.fullmatch(r'[0-9]+\.[0-9]+',version):raise ValueError('Unsafe version')
    if not re.fullmatch(r'[0-9a-f]{40}',commit):raise ValueError('Invalid source commit')
    stage=Path(stage);output=Path(output);files=audit(stage)
    files.pop('BUILD.json',None)
    manifest={'schema':1,'product':'Power Dozer','version':version,'release_tag':'v'+version,'platform':'windows-x86_64',
              'commit':commit,'ready_for_release':False,'license_review_complete':False,'files':files}
    members={name:(stage/name).read_bytes() for name in files}
    members['BUILD.json']=(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name,data in sorted(members.items()):
            info=zipfile.ZipInfo('power-dozer-v'+version+'-windows-x86_64/'+name,date_time=(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=0o100644<<16;archive.writestr(info,data)
    checksum=hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(output.suffix+'.sha256').write_text(checksum+'  '+output.name+'\n')
    return manifest

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True,help='Release directory containing the native executable and DLLs')
    parser.add_argument('--deps',type=Path,required=True,help='Pinned original dependency sources')
    parser.add_argument('--controllers',type=Path,required=True)
    parser.add_argument('--controller-notices',type=Path,required=True)
    parser.add_argument('--version',default='0.82');parser.add_argument('--commit',required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='power-dozer-windows-') as temporary:
        stage=Path(temporary)
        shutil.copyfile(args.build/'blastcorps-prototype.exe',stage/'power-dozer.exe')
        original=args.deps/'rt64/src/contrib'
        pinned={'SDL2.dll':original/'mupen64plus-win32-deps/SDL2-2.26.3/lib/x64/SDL2.dll',
                **{name:original/'dxc/bin/x64'/name for name in ('dxcompiler.dll','dxil.dll')}}
        for name,source in pinned.items():
            binary=args.build/name
            if package_linux.digest(binary)!=package_linux.digest(source):raise ValueError('Windows DLL provenance mismatch: '+name)
            shutil.copyfile(binary,stage/name)
        for vendor in ('N64Recomp','N64ModernRuntime','rt64'):
            package_linux.copy_vendor_notices(args.deps/vendor,stage/'licenses'/vendor)
        for vendor in ('dxc-windows','embedded'):
            package_linux.verified_licenses(ROOT/'licenses'/vendor,stage/'licenses'/vendor,inputs_root=args.deps)
        for name in ('LICENSE','NOTICE','CREDITS.md'):
            source=ROOT/name
            if source.is_symlink() or not source.is_file():raise ValueError('Missing host notice: '+name)
            target=stage/'licenses/Power-Dozer'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
        provenance=json.loads((args.controller_notices/'provenance.json').read_text())
        for name,source in {'gamecontrollerdb.txt':args.controllers,'LICENSE':args.controller_notices/'LICENSE'}.items():
            entry=provenance['files'][name]
            if source.stat().st_size!=entry['bytes'] or package_linux.digest(source)!=entry['sha256']:raise ValueError('Controller provenance mismatch: '+name)
        shutil.copyfile(args.controllers,stage/'gamecontrollerdb.txt');shutil.copytree(args.controller_notices,stage/'licenses/controller-db')
        (stage/'Run Power Dozer.cmd').write_bytes(launcher_text().encode())
        (stage/'README.txt').write_text('Power Dozer v'+args.version+' experimental Windows beta.\nRun "Run Power Dozer.cmd" or power-dozer.exe. Import your own Blast Corps USA Rev 1 ROM through the selector or --rom PATH.\nNormalized ROM SHA-1: 483f7161aea39de8b45c9fbc70a2c3883c4dea8c. No ROM or saves included.\nKeep all DLLs and licenses beside the executable; data defaults to executable/data.\nUse --data-dir PATH for an isolated data folder; --import-data OLD requires a new or empty destination.\nRequires Windows x64, SSSE3/SSE4.1, Vulkan drivers and Microsoft Visual C++ x64 Redistributable.\nOfficial runtime: https://aka.ms/vc14/vc_redist.x64.exe\nDefault keyboard: arrows move, Z confirm, X back, Enter Start. Prompts follow the last active device; family and remapping are configurable.\nIntermittent audio loss remains under investigation; 4K/120 Hz and physical controllers remain manual.\nAttribution and scope: licenses/Power-Dozer/CREDITS.md and NOTICE.\nBUILD.json records file hashes and preserves ready_for_release=false, license_review_complete=false.\n')
        manifest=write_archive(stage,args.output,version=args.version,commit=args.commit)
        print(json.dumps({'archive':str(args.output),'sha256':hashlib.sha256(args.output.read_bytes()).hexdigest(),'members':len(manifest['files'])+1,'ready_for_release':False}))

if __name__=='__main__':main()
