import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib
spec=importlib.util.spec_from_file_location('pak_checker',Path(__file__).resolve().parents[1]/'tools/check_pak_save.py')
probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)

def framing(payload):return b'BCPK'+struct.pack('<III',1,len(payload),zlib.crc32(payload))+payload

def file_slot(extension=1,size=3584,occupied=1):
    data=b'AAA\0'+bytes(size-4)
    return b'\1'+struct.pack('<HI16s4sBI',0x3031,0x4E424345,b'GAME',bytes([extension,0,0,0]),occupied,size)+data

class PakAcceptance(unittest.TestCase):
    def inspect(self,data,player=None):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'pak.bin';path.write_bytes(data)
            return probe.check(path,player)
    def test_actual_file_metadata_and_player(self):
        result=self.inspect(framing(file_slot()+bytes(15)),'AAA')
        self.assertEqual(result['free_bytes'],27904)
        self.assertEqual(result['files'][0]['game'],'4E424345')
    def test_checksum_corruption_rejected(self):
        raw=bytearray(framing(file_slot()+bytes(15)));raw[50]^=1
        with self.assertRaisesRegex(ValueError,'checksum'):self.inspect(raw)
    def test_framed_invalid_directory_rejected(self):
        for payload in (bytes([2])+bytes(15),file_slot(occupied=2)+bytes(15),file_slot()+file_slot()+bytes(14)):
            with self.subTest(payload_size=len(payload)),self.assertRaises(ValueError):self.inspect(framing(payload))
    def test_capacity_overrun_rejected_even_valid_crc(self):
        with self.assertRaisesRegex(ValueError,'metadata'):self.inspect(framing(file_slot(1,16384)+file_slot(2,15360)+bytes(14)))
    def test_requested_player_must_exist_and_be_occupied(self):
        with self.assertRaisesRegex(ValueError,'persisted'):self.inspect(framing(file_slot()+bytes(15)),'OTHER')
        with self.assertRaisesRegex(ValueError,'persisted'):self.inspect(framing(file_slot(occupied=0)+bytes(15)),'AAA')
    def test_truncation_and_trailing_data_rejected(self):
        with self.assertRaises(ValueError):self.inspect(framing(bytes(15)))
        with self.assertRaises(ValueError):self.inspect(framing(bytes(17)))
