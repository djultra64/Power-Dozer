#!/usr/bin/env python3
"""Validate the private ROM and extract the original executable overlays."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_SHA1 = '483f7161aea39de8b45c9fbc70a2c3883c4dea8c'

def validate_rom(data):
    if len(data) != 8388608 or hashlib.sha1(data).hexdigest() != EXPECTED_SHA1:
        raise ValueError('An unmodified Blast Corps USA Rev 1 ROM is required')
    return data

def read_rom(path):
    path = Path(path)
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as archive:
            names = [i for i in archive.infolist() if i.filename.lower().endswith(('.z64', '.v64', '.n64'))]
            if len(names) != 1 or names[0].file_size != 8388608:
                raise ValueError('ZIP must contain exactly one 8 MiB ROM')
            data = archive.read(names[0])
    else:
        data = path.read_bytes()
    if data[:4] == bytes.fromhex('37804012'):
        data = bytes(b for i in range(0, len(data), 2) for b in data[i:i+2][::-1])
    elif data[:4] == bytes.fromhex('40123780'):
        data = bytes(b for i in range(0, len(data), 4) for b in data[i:i+4][::-1])
    return validate_rom(data)

def extract_sections(rom):
    layout = [('hd_code', 0x802447C0, 0x787FD0, 0x7D73B4, 0xC713, 0xA4410, 0x26A90),
              ('hd_front_end', 0x801E7000, 0x7E3AD0, 0x7F63A1, 0x383C, 0x21040, 0x8E50)]
    result = []
    for name, vram, text_start, data_start, data_size, text_size, raw_data_size in layout:
        text = gzip.decompress(rom[text_start:data_start])
        data = gzip.decompress(rom[data_start:data_start+data_size])
        if (len(text), len(data)) != (text_size, raw_data_size):
            raise ValueError('Overlay size mismatch: ' + name)
        result.append(dict(name=name, vram=vram, data=text+data, text_size=text_size))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    rom = read_rom(args.rom)
    private = ROOT / '.local'
    private.mkdir(exist_ok=True)
    (private / 'rom.z64').write_bytes(rom)
    sections = extract_sections(rom)
    image = bytearray(0x1000)
    metadata = []
    for section in sections:
        offset = len(image)
        image.extend(section['data'])
        (private / (section['name']+'.bin')).write_bytes(section['data'])
        metadata.append(dict(name=section['name'], rom=offset, vram=section['vram'],
                             size=len(section['data']), text_size=section['text_size']))
    (private / 'code.bin').write_bytes(image)
    (private / 'sections.json').write_text(json.dumps(metadata, indent=2)+'\n')
    print(json.dumps({'event':'rom_prepared', 'rom_id':'blastcorps-us-rev1', 'sections':metadata}))

if __name__ == '__main__':
    main()
