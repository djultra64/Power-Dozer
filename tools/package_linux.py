#!/usr/bin/env python3
"""Create an audited private Linux diagnostic archive; never marks an RC ready."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import tarfile
import tempfile

ROOT=Path(__file__).resolve().parents[1]
LIBRARIES={'libSDL2-2.0.so.0','libSDL3.so.0','libdbus-1.so.3','libdxcompiler.so','libdxil.so'}
TOP={'blastcorps','Run Blast Corps.sh','README.txt','BUILD.json','gamecontrollerdb.txt'}
PRIVATE={'.z64','.v64','.n64','.rom','.eep','.pak','.state','.wav','.png','.jpg','.bin'}

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def checked_relative(name):
    relative=PurePosixPath(name)
    if relative.is_absolute() or not relative.parts or '..' in relative.parts or str(relative)!=name or '\\' in name or ':' in name:
        raise ValueError('Unsafe license path: '+name)
    return relative

def verified_licenses(source,destination,inputs_root=None):
    source,destination=Path(source),Path(destination)
    provenance=source/'provenance.json'
    if source.is_symlink() or provenance.is_symlink():raise ValueError('License source/provenance symlink')
    manifest=json.loads(provenance.read_text())
    files=manifest.get('files')
    if not isinstance(files,dict) or not 1<=len(files)<=32:raise ValueError('Invalid license inventory')
    inputs=manifest.get('inputs',{})
    if not isinstance(inputs,dict) or len(inputs)>32 or (inputs and inputs_root is None):raise ValueError('Invalid license input inventory')
    for name,entry in inputs.items():
        relative=checked_relative(name);base=Path(inputs_root);path=base.joinpath(*relative.parts)
        if base.is_symlink() or any(base.joinpath(*relative.parts[:i]).is_symlink() for i in range(1,len(relative.parts)+1)):
            raise ValueError('License input symlink: '+name)
        if not path.is_file() or path.stat().st_size!=entry.get('bytes') or digest(path)!=entry.get('sha256'):
            raise ValueError('License input provenance mismatch: '+name)
    verified=[]
    for name,entry in files.items():
        relative=checked_relative(name)
        path=source.joinpath(*relative.parts)
        if any(source.joinpath(*relative.parts[:i]).is_symlink() for i in range(1,len(relative.parts)+1)):
            raise ValueError('License symlink: '+name)
        size=entry.get('bytes')
        if not isinstance(size,int) or not 0<size<=65536 or not path.is_file() or path.stat().st_size!=size or digest(path)!=entry.get('sha256'):
            raise ValueError('License provenance mismatch: '+name)
        verified.append(relative)
    destination.mkdir(parents=True,exist_ok=True)
    for relative in verified:
        target=destination.joinpath(*relative.parts);target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source.joinpath(*relative.parts),target)
    shutil.copyfile(provenance,destination/'provenance.json')
    return len(verified)

def copy_vendor_notices(source,destination):
    source,destination=Path(source),Path(destination)
    for path in sorted(source.rglob('*')):
        relative=path.relative_to(source)
        if not path.is_file() or '.git' in relative.parts:continue
        name=path.name.lower()
        if not (name.startswith(('license','copying','notice','thirdpartynotice')) or name=='copyright' or 'LICENSES' in relative.parts):continue
        if path.is_symlink() or any(source.joinpath(*relative.parts[:i]).is_symlink() for i in range(1,len(relative.parts)+1)):
            raise ValueError('Vendor notice symlink: '+str(relative))
        target=destination/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)

def audit(stage):
    inventory={}
    for path in sorted(Path(stage).rglob('*')):
        rel=path.relative_to(stage)
        if path.is_symlink():raise ValueError('Package symlinks are forbidden: '+str(rel))
        if path.is_dir():
            if rel.parts[0] not in ('lib','licenses'):raise ValueError('Unexpected package directory: '+str(rel))
            continue
        if not path.is_file() or path.suffix.lower() in PRIVATE:raise ValueError('Private or unsupported package file: '+str(rel))
        allowed=(len(rel.parts)==1 and path.name in TOP) or (len(rel.parts)==2 and rel.parts[0]=='lib' and path.name in LIBRARIES) or rel.parts[0]=='licenses'
        if not allowed:raise ValueError('Unlisted package member: '+str(rel))
        if path.read_bytes()[:4] in (bytes.fromhex('80371240'),bytes.fromhex('37804012'),bytes.fromhex('40123780')):raise ValueError('Disguised ROM data: '+str(rel))
        inventory[rel.as_posix()]=digest(path)
    return inventory

def portable_rpath(path,work):
    dynamic=subprocess.check_output(['readelf','-d',str(path)],text=True)
    matches=re.findall(r'\((?:RPATH|RUNPATH)\).*?\[(.*?)\]',dynamic)
    if not matches:return
    quote=lambda s:s.replace('\\','\\\\').replace('"','\\"')
    script=work/'relocate.cmake'
    script.write_text('file(RPATH_CHANGE FILE "'+quote(str(path))+'" OLD_RPATH "'+quote(matches[0])+'" NEW_RPATH "$ORIGIN")\n')
    subprocess.run(['cmake','-P',str(script)],check=True,stdout=subprocess.DEVNULL)

def build(args):
    output=args.output.resolve();output.parent.mkdir(parents=True,exist_ok=True)
    tetra=args.tetrisphere.resolve();deps=tetra/'.local/deps'
    with tempfile.TemporaryDirectory(prefix='blastcorps-package-',dir=output.parent) as scratch:
        work=Path(scratch);stage=work/'blastcorps-private-linux-x86_64';stage.mkdir()
        subprocess.run(['cmake','--install',str(args.build.resolve()),'--prefix',str(stage),'--strip'],check=True)
        (stage/'blastcorps-prototype').rename(stage/'blastcorps')
        (stage/'lib').mkdir();(stage/'licenses').mkdir()
        sources={name:Path('/home/linuxbrew/.linuxbrew/lib')/name for name in ('libSDL2-2.0.so.0','libSDL3.so.0','libdbus-1.so.3')}
        sources.update({name:deps/'rt64/src/contrib/dxc/lib/x64'/name for name in ('libdxcompiler.so','libdxil.so')})
        for name,source in sources.items():
            shutil.copyfile(source,stage/'lib'/name);portable_rpath(stage/'lib'/name,work)
        vendors={name:deps/name for name in ('N64Recomp','N64ModernRuntime','rt64')}
        for name in ('sdl2-compat','sdl3','dbus'):
            vendors[name]=(Path('/home/linuxbrew/.linuxbrew/opt')/name).resolve()
        for vendor,base in vendors.items():
            copy_vendor_notices(base,stage/'licenses'/vendor)
        # Homebrew's DBus notice points to texts omitted from its installed
        # bottle. Include the exact upstream version with verified provenance.
        verified_licenses(ROOT/'licenses/dbus',stage/'licenses/dbus')
        for vendor in ('dxc','embedded'):
            verified_licenses(ROOT/'licenses'/vendor,stage/'licenses'/vendor,inputs_root=deps)
        controller=tetra/'assets/controllers/gamecontrollerdb.txt'
        provenance=json.loads((tetra/'licenses/controller-db/provenance.json').read_text())
        for name,source in {'gamecontrollerdb.txt':controller,'LICENSE':tetra/'licenses/controller-db/LICENSE'}.items():
            entry=provenance['files'][name]
            if source.stat().st_size!=entry['bytes'] or digest(source)!=entry['sha256']:raise ValueError('Controller database provenance mismatch: '+name)
        shutil.copyfile(controller,stage/'gamecontrollerdb.txt')
        shutil.copytree(tetra/'licenses/controller-db',stage/'licenses/controller-db')
        license_count=sum(1 for path in (stage/'licenses').rglob('*') if path.is_file() and path.suffix.lower()!='.json')
        launcher=stage/'Run Blast Corps.sh'
        launcher.write_text('#!/usr/bin/env bash\nset -euo pipefail\npackage_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"\nexport LD_LIBRARY_PATH="$package_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$package_dir/blastcorps" "$@"\n');launcher.chmod(0o755)
        (stage/'README.txt').write_text('Blast Corps private experimental beta; release acceptance is incomplete.\nRun: ./Run Blast Corps.sh\nSupply your own unmodified USA Rev 1 ROM when prompted, or use --rom PATH.\nProgress and private ROM cache live in data beside the executable.\n--import-data OLD_DIRECTORY copies progress/preferences into a new or empty data folder.\nLinux x86-64 with SSSE3/SSE4.1, glibc >= 2.38, a C++ runtime providing GLIBCXX_3.4.31, zlib (libz.so.1), Vulkan drivers, X11 or XWayland, and a desktop file chooser are required.\nThis archive has not verified other Linux distributions or Windows.\n4K/120-Hz performance, physical presentation and broad mission/vehicle coverage require manual testing.\nKnown graphics/stability defects and final source/license review still block release.\nAlt+Enter toggles fullscreen. Default keyboard: arrows move, Z primary/confirm, X secondary/back, Enter Start.\nUse in-game PC options for graphics, audio, help and action remapping.\nNo ROM, extracted overlays, progress or screenshots are included.\n')
        manifest={'schema':1,'ready_for_release':False,'license_review_complete':False,'platform':'linux-x86_64','commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'worktree_dirty':bool(subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True)),'dependencies':json.loads((ROOT/'config/dependencies.json').read_text()),'controller_database':provenance,'license_files':license_count,'files':audit(stage)}
        (stage/'BUILD.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n');audit(stage)
        with output.open('wb') as raw,gzip.GzipFile(fileobj=raw,mode='wb',mtime=0,filename='') as zipped,tarfile.open(fileobj=zipped,mode='w') as archive:
            for path in sorted(stage.rglob('*')):
                if not path.is_file():continue
                payload=path.read_bytes();info=tarfile.TarInfo(stage.name+'/'+path.relative_to(stage).as_posix());info.size=len(payload);info.mode=0o755 if path.name in ('blastcorps','Run Blast Corps.sh') else 0o644;info.mtime=0;archive.addfile(info,io.BytesIO(payload))
        output.with_suffix(output.suffix+'.sha256').write_text(digest(output)+'  '+output.name+'\n')
        print(json.dumps({'archive':str(output),'sha256':digest(output),'members':len(manifest['files'])+1,'ready_for_release':False}))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--build',type=Path,default=ROOT/'build');parser.add_argument('--tetrisphere',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);build(parser.parse_args())
