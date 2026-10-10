#!/usr/bin/env python3
"""Stage a deterministic private source review archive; never certify licensing."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import subprocess
import tarfile

ROOT=Path(__file__).resolve().parents[1]
SUFFIXES={'.h','.hpp','.hh','.c','.cpp','.cc','.cxx','.s','.asm','.hlsl','.hlsli','.glsl','.vert','.frag','.comp','.py','.sh','.bat','.ps1','.toml','.json','.txt','.md','.yml','.yaml','.natvis','.rc','.manifest','.def','.inc','.map','.csv','.xml','.html','.rst','.fbs','.mk','.cfg','.sln','.vcxproj','.props','.filters','.in','.cmake'}
NAMES={'LICENSE','COPYING','COPYRIGHT','NOTICE','Makefile','meson.build','.gitignore','.gitmodules','.gitattributes'}
SUFFIXES.update({'.inl','.ipp','.tpp'})
HOST_DIRS={'include','src','tools','tests','config','licenses','docs'}
HOST_FILES={'CMakeLists.txt','README.md','.gitignore','Run Blast Corps.sh','Run Blast Corps Beta.sh'}
SOURCE_FIXTURES={'tests/fixtures/legacy-completed.hex'}
ROM_MAGIC={bytes.fromhex(s) for s in ('80371240','37804012','40123780')}

def digest(data):return hashlib.sha256(data).hexdigest()

def source_path(name):
    p=PurePosixPath(name)
    if p.is_absolute() or not p.parts or '..' in p.parts or str(p)!=name or '\\' in name or ':' in name or any(part in {'.local','.git','__pycache__','node_modules','.venv'} for part in p.parts):
        raise ValueError('Unsafe source path: '+name)
    if name not in SOURCE_FIXTURES and p.suffix.lower() not in SUFFIXES and p.name not in NAMES and not p.name.lower().startswith(('license','copying','copyright','notice','thirdpartynotice')):
        raise ValueError('Unsupported source path: '+name)
    return p

def inventory_files(root,names):
    root=Path(root);inventory={}
    for name in sorted(set(names)):
        relative=source_path(name);path=root.joinpath(*relative.parts)
        if any(root.joinpath(*relative.parts[:i]).is_symlink() for i in range(1,len(relative.parts)+1)):
            raise ValueError('Source symlink: '+name)
        data=path.read_bytes()
        if not data or len(data)>64*1024*1024:
            if data:raise ValueError('Source size limit: '+name)
        if data[:4] in ROM_MAGIC or data.startswith((b'\x7fELF',b'MZ',b'PK\x03\x04')) or b'\0' in data:
            raise ValueError('Source contains binary or ROM data: '+name)
        inventory[name]={'bytes':len(data),'sha256':digest(data),'executable':bool(path.stat().st_mode&0o111)}
    return inventory

def write_archive(output,groups,metadata):
    payloads=[];files={}
    for root,inventory,prefix in groups:
        source_path(prefix+'/file.txt')
        for name,entry in sorted(inventory.items()):
            data=(Path(root)/name).read_bytes()
            if len(data)!=entry['bytes'] or digest(data)!=entry['sha256']:raise ValueError('Source changed during snapshot: '+name)
            target=prefix+'/'+name
            if target in files:raise ValueError('Duplicate source member: '+target)
            files[target]=entry;payloads.append((target,data,0o755 if entry['executable'] else 0o644))
    manifest=dict(metadata,files=files)
    payloads.append(('SOURCES.json',(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode(),0o644))
    output=Path(output);output.parent.mkdir(parents=True,exist_ok=True)
    with output.open('wb') as raw,gzip.GzipFile(fileobj=raw,mode='wb',mtime=0,filename='') as zipped,tarfile.open(fileobj=zipped,mode='w') as archive:
        for name,data,mode in sorted(payloads):
            member=tarfile.TarInfo('blastcorps-private-source/'+name);member.size=len(data);member.mode=mode;member.mtime=0;archive.addfile(member,io.BytesIO(data))
    output.with_suffix(output.suffix+'.sha256').write_text(digest(output.read_bytes())+'  '+output.name+'\n')
    return manifest

def git(root,*args):return subprocess.check_output(['git','-C',str(root),*args],text=True).strip()

def repository_inventory(root,commit):
    if git(root,'rev-parse','HEAD')!=commit:raise ValueError('Wrong dependency revision: '+str(root))
    if git(root,'status','--porcelain','--untracked-files=no'):raise ValueError('Modified dependency: '+str(root))
    # check_output.strip removes the first clean marker, so inspect raw output.
    submodules=subprocess.check_output(['git','-C',str(root),'submodule','status','--recursive'],text=True)
    if any(line and line[0]!=' ' for line in submodules.splitlines()):raise ValueError('Unverified dependency submodules: '+str(root))
    repositories=[('',root,commit)]
    for line in submodules.splitlines():
        parts=line.split();repositories.append((parts[1]+'/',root/parts[1],parts[0]))
    names=[];provenance=[];excluded=[]
    for prefix,repository,revision in repositories:
        if git(repository,'status','--porcelain','--untracked-files=no'):raise ValueError('Modified dependency submodule: '+str(repository))
        provenance.append({'path':prefix.rstrip('/'),'commit':revision,'repository':git(repository,'remote','get-url','origin')})
        tracked=subprocess.check_output(['git','-C',str(repository),'ls-files','-z'],text=False).decode().split('\0')
        for local in tracked:
            if not local or not (repository/local).is_file():continue
            name=prefix+local
            try:source_path(name)
            except ValueError:excluded.append(name);continue
            names.append(name)
    return inventory_files(root,names),provenance,sorted(excluded)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--deps',type=Path,required=True,help='Read-only pinned source checkouts (used only for creating this archive)')
    parser.add_argument('--mapping',type=Path,default=ROOT/'.local/upstream/blastcorps')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();lock=json.loads((ROOT/'config/source-build.json').read_text());groups=[];dependencies={}
    names=[]
    candidates=[ROOT/name for name in HOST_FILES if (ROOT/name).is_file()]
    for directory in HOST_DIRS:candidates.extend((ROOT/directory).rglob('*'))
    for path in candidates:
        relative=path.relative_to(ROOT)
        if relative.parts[0] not in HOST_DIRS and relative.as_posix() not in HOST_FILES:continue
        if not path.is_file():continue
        if relative.parts[:2]==('docs','evidence'):continue
        try:source_path(relative.as_posix())
        except ValueError:continue
        names.append(relative.as_posix())
    groups.append((ROOT,inventory_files(ROOT,names),'host'))
    for name,entry in lock['dependencies'].items():
        base=args.deps/name;inventory,provenance,excluded=repository_inventory(base,entry['commit'])
        groups.append((base,inventory,'vendor/'+name));dependencies[name]={'repository':entry['repository'],'commit':entry['commit'],'repositories':provenance,'excluded_non_source':excluded}
    mapping=lock['mapping'];base=args.mapping
    if git(base,'rev-parse','HEAD')!=mapping['commit'] or git(base,'status','--porcelain','--untracked-files=no'):raise ValueError('Unverified mapping source')
    groups.append((base,inventory_files(base,mapping['inputs']),'mapping'))
    metadata={'schema':1,'purpose':'private source and reconstruction review','license_review_complete':False,'corresponding_source_verified':False,'clean_rebuild_verified':False,'host_commit':git(ROOT,'rev-parse','HEAD'),'worktree_dirty':bool(git(ROOT,'status','--porcelain')),'rom_included':False,'rom_derived_cpu_rsp_source_included':False,'rom_regeneration':'host/tools/build.py requires the recipient own validated USA Rev 1 ROM','mapping':mapping,'dependencies':dependencies,'build_recipe':'host/docs/qa/source-rebuild.md','limitations':['Generated CPU/RSP source and mapping licensing/redistribution scope unresolved.','Independent CPU rebuild evidence is recorded separately; exact final binary correspondence and GPU runtime not verified.','DXC binaries are excluded; bootstrap downloads exact pinned build-tool inputs separately.','Source license/review flag remains false; this artifact is not a certified GPL corresponding-source delivery.']}
    manifest=write_archive(args.output,groups,metadata)
    print(json.dumps({'archive':str(args.output.resolve()),'sha256':digest(args.output.read_bytes()),'source_files':len(manifest['files']),'license_review_complete':False,'corresponding_source_verified':False}))

if __name__=='__main__':main()
