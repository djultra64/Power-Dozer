import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import recompile

class VehiclePivotProducer(unittest.TestCase):
    def body(self,name,pc):
        return f'RECOMP_FUNC void {name}(uint8_t* rdram, recomp_context* ctx) {{\n    // 0x{pc}: addiu       $sp, $sp, -0x18\n    return;\n;}}'
    def test_shared_vehicle_animation_registers_both_buffers_without_guest_writes(self):
        body=self.body('func_8029E558','8029E558')
        result=recompile.normalize_vehicle_pivot_body(body)
        self.assertIn('bc_register_vehicle_pivots(rdram,uint32_t(ctx->r2),uint32_t(ctx->r3),uint32_t(ctx->r8));',result)
        self.assertNotIn('MEM_W(',result)
        self.assertEqual(result.count('ctx->'),3)
        self.assertEqual(recompile.normalize_vehicle_pivot_body(result),result)
    def test_loading_reused_vehicle_buffers_clears_previous_annotations(self):
        result=recompile.normalize_vehicle_pivot_body(self.body('func_802A1388','802A1388'))
        self.assertIn('bc_register_vehicle_pivots(rdram,0,uint32_t(ctx->r2),0);',result)
        self.assertIn('bc_register_vehicle_pivots(rdram,0,uint32_t(ctx->r3),0);',result)
    def test_building_draw_and_unrelated_producers_are_untouched(self):
        for name in ('func_802BD1F8','func_802BE574','func_8024FC2C'):
            body=self.body(name,'802BD1F8')
            self.assertEqual(recompile.normalize_vehicle_pivot_body(body),body)
    def test_refuses_changed_entry_layout(self):
        with self.assertRaisesRegex(ValueError,'skin entry layout'):
            recompile.normalize_vehicle_pivot_body(self.body('func_8029E558','8029E55C'))
