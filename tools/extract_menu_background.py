#!/usr/bin/env python3
"""Extract the original USA Rev1 main-menu background without drawing its UI.

The codec follows ROM functions 0x802C41C0 and 0x802C42CC. The image is
attract2, selected by func_hd_front_end_80200714(1); no screenshot is used.
Only Python's standard library is required.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

ROM_SHA1 = "483f7161aea39de8b45c9fbc70a2c3883c4dea8c"
START, END = 0x6AD3F0, 0x6BF2F0
WIDTH, HEIGHT = 320, 240


def decode_stream(data, expected_size, window_bits=13):
    """MSB-first literal/absolute-ring LZ stream, with an offset-zero terminator."""
    bit_position = 0
    window = bytearray(1 << window_bits)
    initialized = bytearray(len(window))
    write_position = 1
    output = bytearray()

    def read_bits(count):
        nonlocal bit_position
        if bit_position + count > len(data) * 8:
            raise ValueError("Truncated compressed stream")
        value = 0
        for _ in range(count):
            value = (value << 1) | ((data[bit_position // 8] >>
                                     (7 - bit_position % 8)) & 1)
            bit_position += 1
        return value

    def append(value):
        nonlocal write_position
        if len(output) >= expected_size:
            raise ValueError("Decoded stream exceeds expected image size")
        output.append(value)
        window[write_position] = value
        initialized[write_position] = 1
        write_position = (write_position + 1) & (len(window) - 1)

    while True:
        if read_bits(1):
            append(read_bits(8))
        else:
            read_position = read_bits(window_bits)
            if read_position == 0:
                break
            length = read_bits(16 - window_bits) + 3
            for offset in range(length):
                index = (read_position + offset) & (len(window) - 1)
                if not initialized[index]:
                    raise ValueError("Reference to uninitialized original-code window")
                append(window[index])
    if len(output) != expected_size:
        raise ValueError(f"Expected {expected_size} decoded bytes, got {len(output)}")
    # The original function rounds its advanced input pointer to a halfword.
    source_consumed = ((bit_position + 7) // 8 + 1) & ~1
    return bytes(output), bit_position, source_consumed


def rgba5551_to_rgba8(data):
    pixels = bytearray()
    for (pixel,) in struct.iter_unpack(">H", data):
        for shift in (11, 6, 1):
            component = (pixel >> shift) & 31
            pixels.append((component << 3) | (component >> 2))
        # The original loader preserves RGB and always sets alpha to one.
        pixels.append(255)
    return bytes(pixels)


def png_bytes(pixels, width, height):
    def chunk(kind, payload):
        return (struct.pack(">I", len(payload)) + kind + payload +
                struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    stride = width * 4
    rows = b"".join(b"\0" + pixels[y * stride:(y + 1) * stride]
                    for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b""))


def extract(rom_path, output_directory, original_decoder_output=None):
    rom = Path(rom_path).read_bytes()
    actual_sha1 = hashlib.sha1(rom).hexdigest()
    if actual_sha1 != ROM_SHA1:
        raise ValueError(f"This extractor requires USA Rev1 ROM SHA1 {ROM_SHA1}; "
                         f"received {actual_sha1}")
    compressed = rom[START:END]
    raw, used_bits, source_consumed = decode_stream(compressed, WIDTH * HEIGHT * 2)
    if original_decoder_output is not None:
        if Path(original_decoder_output).read_bytes() != raw:
            raise ValueError("Output differs from the original ROM decoder")
    pixels = rgba5551_to_rgba8(raw)
    # RGB must survive PNG's 8-bit representation with no loss of the 5-bit asset.
    for index, (pixel,) in enumerate(struct.iter_unpack(">H", raw)):
        reconstructed = ((pixels[index * 4] >> 3) << 11 |
                         (pixels[index * 4 + 1] >> 3) << 6 |
                         (pixels[index * 4 + 2] >> 3) << 1 | 1)
        if reconstructed != pixel | 1:
            raise AssertionError("RGB5551 round trip failed")
    png = png_bytes(pixels, WIDTH, HEIGHT)
    output_directory = Path(output_directory)
    output_directory.mkdir(parents=True, exist_ok=True)
    image_path = output_directory / "blast-corps-main-menu-background.png"
    metadata_path = image_path.with_suffix(".json")
    metadata = {
        "asset": "attract2",
        "description": "Original main-menu background; menu UI is separate.",
        "rom": {"edition": "Blast Corps USA Rev1", "sha1": actual_sha1,
                "size_bytes": len(rom), "byte_order": "z64 / big endian"},
        "rom_range": {"start": hex(START), "end_exclusive": hex(END),
                      "compressed_size": len(compressed),
                      "compressed_sha256": hashlib.sha256(compressed).hexdigest()},
        "codec": {"entry_point": "0x802C4070", "decoder": "0x802C41C0",
                  "bit_reader": "0x802C42CC", "window_bits": 13,
                  "bit_order": "MSB first", "initial_write_index": 1,
                  "tokens": "1: literal8; 0: offset13, length3+3; offset0: end",
                  "consumed_bits": used_bits, "aligned_consumed_bytes": source_consumed,
                  "uninitialized_window_reads": 0},
        "image": {"width": WIDTH, "height": HEIGHT,
                  "original_format": "RGBA16 / big-endian RGB5551",
                  "decoded_size_bytes": len(raw),
                  "decoded_sha256": hashlib.sha256(raw).hexdigest(),
                  "loader_argument": 1, "rgb_tint": "none",
                  "loader_alpha": "set low bit to one for every pixel",
                  "png_format": "RGBA8",
                  "channel_expansion": "(v5 << 3) | (v5 >> 2)",
                  "orientation": "original row-major, no crop or scaling",
                  "rgba8_sha256": hashlib.sha256(pixels).hexdigest(),
                  "png_sha256": hashlib.sha256(png).hexdigest()},
        "sources": [
            {"file": ".local/upstream/blastcorps/game/symbol_addrs.us.v11.txt",
             "symbols": "attract2_ROM_START, attract3_ROM_START"},
            {"file": ".local/upstream/blastcorps/game/src/hd_front_end/E4590.c",
             "functions": ["func_hd_front_end_80200714", "func_hd_front_end_80200BE0"]},
            {"file": ".local/upstream/blastcorps/game/src/hd_code/hd.c",
             "detail": "Main menu transitions call func_hd_front_end_80200714(1)."},
            {"file": ".local/generated/functions/funcs_19.c",
             "functions": ["func_802C41C0", "func_802C42CC"],
             "detail": "Instruction-preserving ROM recompilation used to derive codec."}
        ],
        "verification": {"expected_size": True, "rom_sha1": True,
                         "rgb5551_round_trip": True,
                         "screenshot_pixels_used": False,
                         "generated_image_used": False}
    }
    if original_decoder_output is not None:
        metadata["verification"]["original_rom_decoder_byte_comparison"] = {
            "matched_bytes": len(raw),
            "reference": str(original_decoder_output),
            "fixture": ".local/qa/menu-background-codec-fixture.c",
            "detail": "CPU execution of unchanged func_802C41C0/func_802C42CC "
                      "from generated ROM instruction translations; no game/GPU run."
        }
    image_path.write_bytes(png)
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n")
    return image_path, metadata_path


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--output-directory", type=Path,
                        default=Path(".local/evidence/assets"))
    parser.add_argument("--original-decoder-output", type=Path,
                        help="Optional raw output of the original decoder for byte comparison")
    arguments = parser.parse_args()
    for output_path in extract(arguments.rom, arguments.output_directory,
                               arguments.original_decoder_output):
        print(output_path.resolve())
