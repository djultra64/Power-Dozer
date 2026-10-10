#!/usr/bin/env python3
"""Independently inspect a checked Controller Pak without changing progress."""
import argparse
import json
from pathlib import Path
import struct
import zlib

LIMIT=16+16*32+123*256

def check(path,required_player=None):
    path=Path(path)
    if path.stat().st_size>LIMIT:raise ValueError('Oversized Pak')
    raw=path.read_bytes()
    if len(raw)<32 or raw[:4]!=b'BCPK':raise ValueError('Invalid Pak framing')
    version,size,checksum=struct.unpack_from('<III',raw,4)
    payload=raw[16:]
    if version!=1 or size!=len(payload) or zlib.crc32(payload)!=checksum:raise ValueError('Invalid Pak version/length/checksum')
    position=0;files=[];used=0;identities=set()
    for index in range(16):
        if position>=len(payload):raise ValueError('Missing directory slot')
        present=payload[position];position+=1
        if present==0:continue
        if present!=1 or position+31>len(payload):raise ValueError('Invalid directory slot')
        company,game,name,extension,occupied,size=struct.unpack_from('<HI16s4sBI',payload,position);position+=31
        identity=(company,game,name,extension)
        if not company or not game or occupied>1 or size==0 or size%256 or size>123*256-used or position+size>len(payload) or identity in identities:raise ValueError('Invalid file metadata')
        data=payload[position:position+size];position+=size;used+=size;identities.add(identity)
        player=data[:16].split(b'\0',1)[0].decode('ascii',errors='replace')
        files.append(dict(index=index,company=f'{company:04X}',game=f'{game:08X}',name=name.hex(),extension=extension.hex(),occupied=bool(occupied),size=size,player=player))
    if position!=len(payload):raise ValueError('Unexpected trailing bytes')
    if required_player and not any(f['occupied'] and f['player']==required_player for f in files):raise ValueError(f'Player {required_player!r} was not persisted')
    return dict(files=files,free_bytes=123*256-used)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path',type=Path)
    parser.add_argument('--require-player')
    args=parser.parse_args()
    try:print(json.dumps(check(args.path,args.require_player)))
    except (OSError,ValueError) as error:parser.exit(1,str(error)+'\n')
