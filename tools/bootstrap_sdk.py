#!/usr/bin/env python3
"""Build an independent Linux SDK from an audited private source snapshot."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import shlex
import subprocess
import sys
import sysconfig
import urllib.request

ROOT=Path(__file__).resolve().parents[1]

def verify_source_snapshot(manifest,root,prefix):
    data=json.loads(Path(manifest).read_text());root=Path(root);count=0
    for name,entry in data['files'].items():
        if not name.startswith(prefix+'/'):continue
        relative=PurePosixPath(name[len(prefix)+1:])
        if relative.is_absolute() or '..' in relative.parts or '\\' in str(relative) or ':' in str(relative):raise ValueError('Unsafe source manifest path')
        path=root.joinpath(*relative.parts)
        if any(root.joinpath(*relative.parts[:i]).is_symlink() for i in range(1,len(relative.parts)+1)):raise ValueError('Source symlink')
        payload=path.read_bytes()
        if len(payload)!=entry['bytes'] or hashlib.sha256(payload).hexdigest()!=entry['sha256']:raise ValueError('Source snapshot mismatch: '+name)
        count+=1
    if not count:raise ValueError('Missing source snapshot prefix: '+prefix)
    return count

def build_commands(sdk_root,python,jobs,host_cflags=''):
    sdk=Path(sdk_root).resolve();deps=sdk/'deps';tools=sdk/'toolchain/n64recomp-build';rt64=sdk/'rt64-build'
    python_inputs=json.loads((ROOT/'config/source-build.json').read_text())['python_source_inputs']
    python_archives=[str(sdk/'build-inputs'/Path(entry['url']).name) for entry in python_inputs.values()]
    return [
        ['cmake','-S',str(deps/'N64Recomp'),'-B',str(tools),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],
        ['cmake','--build',str(tools),'--target','N64RecompCLI','RSPRecomp','-j',str(jobs)],
        ['cmake','-S',str(sdk/'rt64-source'),'-B',str(rt64),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DRT64_STATIC=ON','-DRT64_SDL_WINDOW_VULKAN=ON','-DNFD_PORTAL=ON','-DCMAKE_C_FLAGS='+host_cflags,'-DCMAKE_CXX_FLAGS='+host_cflags],
        ['cmake','--build',str(rt64),'--target','rt64','re-spirv','nfd','libzstd_static','plume','-j',str(jobs)],
        [python,'-m','venv',str(sdk/'analysis-venv')],
        [str(sdk/'analysis-venv/bin/python'),'-m','pip','install','--no-deps',*python_archives],
    ]

def rt64_source_patch(original,expected_cmake):
    if hashlib.sha256(original).hexdigest()!=expected_cmake:raise ValueError('RT64 CMake fingerprint mismatch')
    old=b'set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/src/contrib/dxc/bin/x64/dxc-linux")'
    new=b'set (DXC "${CMAKE_COMMAND}" "-E" "env" "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/src/contrib/dxc/bin/x64/dxc-linux")'
    if original.count(old)!=1:raise ValueError('RT64 CMake shader environment site is not unique')
    modified=original.replace(old,new)
    metadata={'source_sha256':expected_cmake,'modified_sha256':hashlib.sha256(modified).hexdigest(),'change':'Use cmake -E env for the Linux x64 DXC invocation so paths with spaces work; original pinned sources unchanged.'}
    return modified,metadata

def prepare_rt64_source(sdk_root,expected_cmake):
    sdk=Path(sdk_root);source=sdk/'deps/rt64'
    modified,metadata=rt64_source_patch((source/'CMakeLists.txt').read_bytes(),expected_cmake)
    target=sdk/'rt64-source';shutil.copytree(source,target)
    (target/'CMakeLists.txt').write_bytes(modified)
    (sdk/'RT64-SOURCE-PATCH.json').write_text(json.dumps(metadata,indent=2)+'\n')
    return target

def verify_prepared_rt64(sdk_root,expected_cmake):
    sdk=Path(sdk_root);manifest=sdk/'SOURCE-INPUTS.json';original=sdk/'deps/rt64';prepared=sdk/'rt64-source'
    verify_source_snapshot(manifest,original,'vendor/rt64')
    modified,metadata=rt64_source_patch((original/'CMakeLists.txt').read_bytes(),expected_cmake)
    if json.loads((sdk/'RT64-SOURCE-PATCH.json').read_text())!=metadata:raise ValueError('RT64 source patch provenance mismatch')
    count=0
    for name,entry in json.loads(manifest.read_text())['files'].items():
        if not name.startswith('vendor/rt64/'):continue
        relative=PurePosixPath(name[len('vendor/rt64/'):]);path=prepared.joinpath(*relative.parts)
        if any(prepared.joinpath(*relative.parts[:i]).is_symlink() for i in range(1,len(relative.parts)+1)):raise ValueError('Prepared RT64 source symlink')
        payload=path.read_bytes();expected=modified if str(relative)=='CMakeLists.txt' else original.joinpath(*relative.parts).read_bytes()
        if payload!=expected:raise ValueError('Prepared RT64 source mismatch: '+name)
        count+=1
    return count

def verify_sdk_binary_inputs(sdk_root,lock):
    sdk=Path(sdk_root)
    inputs=[(sdk/'deps/rt64/src/contrib/dxc'/relative,entry) for relative,entry in lock['dxc_build_inputs'].items()]
    inputs.append((sdk/'gamecontrollerdb.txt',lock['controller_database']['files']['gamecontrollerdb.txt']))
    for path,entry in inputs:
        if path.is_symlink():raise ValueError('SDK binary input symlink: '+str(path))
        payload=path.read_bytes()
        if len(payload)!=entry['bytes'] or hashlib.sha256(payload).hexdigest()!=entry['sha256']:raise ValueError('SDK binary input mismatch: '+str(path))

def verified_download(entry,path):
    path=Path(path)
    payload=path.read_bytes() if path.is_file() else urllib.request.urlopen(entry['url'],timeout=60).read(entry['bytes']+1)
    if len(payload)!=entry['bytes'] or hashlib.sha256(payload).hexdigest()!=entry['sha256']:raise ValueError('Downloaded input mismatch: '+path.name)
    if not path.exists():path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(payload)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources',type=Path,required=True,help='vendor directory from package_source.py')
    parser.add_argument('--sdk-root',type=Path,required=True)
    parser.add_argument('--jobs',type=int,default=2)
    parser.add_argument('--plan',action='store_true',help='Verify source inputs and show commands without writes/downloads/builds')
    args=parser.parse_args()
    if not 1<=args.jobs<=16:raise ValueError('jobs must be 1..16')
    source=args.sources.resolve();manifest=source.parent/'SOURCES.json';sdk=args.sdk_root.resolve();lock=json.loads((ROOT/'config/source-build.json').read_text())
    snapshot=json.loads(manifest.read_text())
    for name,entry in lock['dependencies'].items():
        if snapshot['dependencies'][name]['commit']!=entry['commit']:raise ValueError('Dependency revision mismatch: '+name)
        verify_source_snapshot(manifest,source/name,'vendor/'+name)
    # SDL_syswm.h exposes X11 types even with an external SDL Vulkan window.
    cflags=shlex.join(shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2','dbus-1','x11'],text=True)))
    commands=build_commands(sdk,sys.executable,args.jobs,cflags)
    if args.plan:
        print(json.dumps({'sdk_root':str(sdk),'commands':commands,'fetch_inputs':[e['url'] for e in lock['dxc_build_inputs'].values()]+[lock['controller_database']['files']['gamecontrollerdb.txt']['url']]+[entry['url'] for entry in lock['python_source_inputs'].values()],'game_binary_built':False,'license_review_complete':False},indent=2));return
    if not Path(sysconfig.get_path('include'),'Python.h').is_file():raise ValueError('The selected Python needs development headers (Python.h) to build pinned Rabbitizer; select a complete Python installation.')
    if sdk.exists() and any(sdk.iterdir()):raise ValueError('Independent SDK destination must be empty; do not merge existing installations')
    sdk.mkdir(parents=True,exist_ok=True);shutil.copytree(source,sdk/'deps');shutil.copyfile(manifest,sdk/'SOURCE-INPUTS.json')
    for relative,entry in lock['dxc_build_inputs'].items():
        target=sdk/'deps/rt64/src/contrib/dxc'/relative;verified_download(entry,target)
        if relative.startswith('bin/'):target.chmod(0o755)
    prepare_rt64_source(sdk,lock['rt64_cmake_sha256'])
    verified_download(lock['controller_database']['files']['gamecontrollerdb.txt'],sdk/'gamecontrollerdb.txt')
    for entry in lock['python_source_inputs'].values():verified_download(entry,sdk/'build-inputs'/Path(entry['url']).name)
    build_env=dict(os.environ)
    # Python's build-time compiler may no longer be installed on the recipient.
    build_env.setdefault('CC',shutil.which('cc') or 'cc');build_env.setdefault('CXX',shutil.which('c++') or 'c++')
    for command in commands:subprocess.run(command,check=True,env=build_env)
    tools={}
    for name,command in {'python':[sys.executable,'--version'],'cmake':['cmake','--version'],'compiler':['c++','--version'],'ninja':['ninja','--version']}.items():
        tools[name]=subprocess.check_output(command,text=True).splitlines()[0]
    (sdk/'SDK-BUILD.json').write_text(json.dumps({'schema':1,'sources_manifest_sha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),'commands':commands,'tools':tools,'license_review_complete':False,'host_binary_rebuild_verified':False},indent=2,sort_keys=True)+'\n')
    print(json.dumps({'sdk_root':str(sdk),'sdk_build_complete':True,'host_binary_rebuild_verified':False,'license_review_complete':False}))

if __name__=='__main__':main()
