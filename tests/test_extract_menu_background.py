"""Small CPU fixtures for the ROM's absolute-ring, overlapping LZ copies."""

import importlib.util
from pathlib import Path
import unittest

SPEC = importlib.util.spec_from_file_location(
    "extract_menu_background", Path(__file__).resolve().parents[1] /
    "tools/extract_menu_background.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def bitstream(bits):
    bits += "0" * (-len(bits) % 8)
    return int(bits, 2).to_bytes(len(bits) // 8, "big")


class MenuBackgroundCodecTests(unittest.TestCase):
    def test_literal_and_overlapping_absolute_ring_copy(self):
        # Literal A at window[1], then copy 10 bytes from [1..10].
        # Every newly copied byte becomes the following source byte.
        bits = "1" + format(ord("A"), "08b")
        bits += "0" + format(1, "013b") + "111"
        bits += "0" + "0" * 13
        output, consumed_bits, consumed_bytes = MODULE.decode_stream(bitstream(bits), 11)
        self.assertEqual(output, b"A" * 11)
        self.assertEqual((consumed_bits, consumed_bytes), (40, 6))

    def test_uninitialized_reference_is_rejected(self):
        bits = "0" + format(1, "013b") + "000"
        with self.assertRaisesRegex(ValueError, "uninitialized"):
            MODULE.decode_stream(bitstream(bits), 3)

    def test_missing_end_marker_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "Truncated"):
            MODULE.decode_stream(bitstream("1" + "01000001"), 1)

    def test_five_bit_channels_survive_png_conversion(self):
        raw = b"".join(((v << 11) | (v << 6) | (v << 1)).to_bytes(2, "big")
                       for v in range(32))
        rgba = MODULE.rgba5551_to_rgba8(raw)
        for v in range(32):
            self.assertEqual(tuple(c >> 3 for c in rgba[v * 4:v * 4 + 3]), (v, v, v))
            self.assertEqual(rgba[v * 4 + 3], 255)


if __name__ == "__main__":
    unittest.main()
