from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import recompile

class FrontendHooks(unittest.TestCase):
    def normalize(self,body,code=None,front=None):
        function=getattr(recompile,'normalize_frontend_menu_body',None)
        self.assertIsNotNone(function,'frontend normalizer must validate original sites before hooking')
        return function(body,self.code if code is None else code,self.front if front is None else front)

    def setUp(self):
        self.code=bytearray(0xB5000);self.front=bytearray(0x24000)
        for data,base,address,encoded in (
            (self.code,0x802447C0,0x80244D9C,'0c07a92e000000003c0480360c07a37390844ae80c09abdb3404800a'),
            (self.code,0x802447C0,0x8026BCE0,'3c0f80313c1880378f18bb408def56c427bdfeb03c09803701f8c82300194100'),
            (self.code,0x802447C0,0x8026EF68,'03e0000827bd0150'),
            (self.code,0x802447C0,0x802F8CF4,'0140009d00000055b0000a2800000002000500000000001d00000000'),
            (self.front,0x801E7000,0x801EA4B8,'27bdffe0afbf00140c07a89e00000000'),
            (self.front,0x801E7000,0x801FE990,'3c0e80368dce4a903c0f80368def4a9427bdff20'),
            (self.front,0x801E7000,0x801FEC34,'3c0e803795cebb163401ffff15c1000c000000003c1880003c018036ac384a983c01803624190000ac394a9c240400040c07a3ae2405000110000141'),
            (self.front,0x801E7000,0x801FF884,'3c09803691294ae83c18803027188bdca3a9007f')):
            raw=bytes.fromhex(encoded);data[address-base:address-base+len(raw)]=raw
        self.sites={
            '80244930':['    // 0x80244D9C: jal         0x801EA4B8\n    // 0x80244DA0: nop\n\n    func_801EA4B8(rdram, ctx);'],
            '801FE990':['    // 0x801FE990: lui         $t6, 0x8036','L_801FEC34:\n    // 0x801FEC34: lui         $t6, 0x8037','L_801FF884:\n    // 0x801FF884: lui         $t1, 0x8036'],
            '8026BCE0':['    bc_pc_options_glyphs(rdram,ctx,bc_glyph_frame);']}

    def body(self,name):return f'RECOMP_FUNC void func_{name}(uint8_t* rdram, recomp_context* ctx) {{\n'+ '\n'.join(self.sites[name])+'\n    return;\n;}'

    def test_exact_hooks_idempotent_and_reject_missing_duplicate_or_edited_hooks(self):
        for name,sites in self.sites.items():
            body=self.body(name);changed=self.normalize(body)
            self.assertNotEqual(changed,body)
            self.assertEqual(self.normalize(changed),changed)
            for site in sites:
                with self.assertRaises(ValueError):self.normalize(body.replace(site,''))
                with self.assertRaises(ValueError):self.normalize(body.replace(site,site+'\n'+site))
            with self.assertRaises(ValueError):self.normalize(changed.replace('bc_frontend_menu_','bc_frontend_menu_corrupt_',1))

    def test_fingerprints_checked_even_on_already_hooked_functions(self):
        for name in self.sites:
            body=self.normalize(self.body(name));code=bytearray(self.code);front=bytearray(self.front)
            if name=='80244930':code[0x80244D9C-0x802447C0]^=1
            elif name=='801FE990':front[0x801FEC34-0x801E7000]^=1
            else:code[0x8026EF68-0x802447C0]^=1
            with self.assertRaisesRegex(ValueError,'frontend'):self.normalize(body,code,front)

    def test_other_functions_background_and_ticker_unchanged(self):
        for name in ['801EA4B8','80200BE0','801E9718','801EAA7C']:
            body=f'RECOMP_FUNC void func_{name}(uint8_t* rdram, recomp_context* ctx) {{\n    return;\n;}}'
            self.assertEqual(self.normalize(body),body)

if __name__=='__main__':unittest.main()
