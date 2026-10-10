import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
spec=importlib.util.spec_from_file_location('render_hooks',Path(__file__).resolve().parents[1]/'tools/prepare_render_observer.py')
hooks=importlib.util.module_from_spec(spec);spec.loader.exec_module(hooks)

class RenderHookPreparation(unittest.TestCase):
    def setUp(self):
        self.original=hooks.EXPECTED.copy()
        self.scratch=tempfile.TemporaryDirectory();self.root=Path(self.scratch.name)
        self.source=self.root/'source/src/hle';self.source.mkdir(parents=True)
        snippets={'present':'                swapChainValid = ext.swapChain->present(swapChainIndex, &waitSemaphore, 1);\n                RenderTargetKey colorTargetKey(presentFb->addressStart, presentFb->width, presentFb->siz, Framebuffer::Type::Color);\n                    framesToPresent = std::min(int(frameCounters.available), i + 1);','workload':'                    framesRendered++;\n                        displayFrames--;\n                            skippedFrames = true;\n                            displayTicks += workload.viOriginalRate * (displayFrames - (frame + 1));'}
        self.files={}
        for kind,site in snippets.items():
            path=self.source/('rt64_'+kind+'_queue.cpp');path.write_text('// fixture\n'+site+'\n')
            self.files[kind]=path;hooks.EXPECTED[kind]=hashlib.sha256(path.read_bytes()).hexdigest()
        self.output=self.root/'output'
    def tearDown(self):hooks.EXPECTED=self.original;self.scratch.cleanup()
    def test_hooks_are_local_and_reference_is_unchanged(self):
        before={kind:path.read_bytes() for kind,path in self.files.items()}
        hooks.prepare(self.root/'source',self.output)
        self.assertIn('bc_render_present(swapChainValid)',(self.output/'blastcorps_present_queue.cpp').read_text())
        self.assertIn('bc_render_generated(generateInterpolatedFrames, curFrameWeight',(self.output/'blastcorps_workload_queue.cpp').read_text())
        self.assertIn('bc_render_selecting()', (self.output/'blastcorps_present_queue.cpp').read_text())
        self.assertIn('BcRenderSkip::Presentation', (self.output/'blastcorps_present_queue.cpp').read_text())
        self.assertIn('uint32_t(framesToPresent - i)', (self.output/'blastcorps_present_queue.cpp').read_text())
        for reason in ('Reduction', 'Deadline', 'NewWorkload'):
            self.assertIn('BcRenderSkip::'+reason, (self.output/'blastcorps_workload_queue.cpp').read_text())
        self.assertEqual(before,{kind:path.read_bytes() for kind,path in self.files.items()})
    def test_changed_fingerprint_refuses_to_build(self):
        self.files['present'].write_text('changed')
        with self.assertRaisesRegex(ValueError,'fingerprint'):hooks.prepare(self.root/'source',self.output)
        self.assertFalse((self.output/'blastcorps_present_queue.cpp').exists())
    def test_nonunique_site_is_rejected_even_with_matching_hash(self):
        path=self.files['present'];path.write_bytes(path.read_bytes()*2);hooks.EXPECTED['present']=hashlib.sha256(path.read_bytes()).hexdigest()
        with self.assertRaisesRegex(ValueError,'not unique'):hooks.prepare(self.root/'source',self.output)
    def test_reconfigure_does_not_rewrite_identical_generated_files(self):
        hooks.prepare(self.root/'source',self.output)
        before={p.name:(p.read_bytes(),p.stat().st_mtime_ns) for p in self.output.iterdir()}
        hooks.prepare(self.root/'source',self.output)
        self.assertEqual(before,{p.name:(p.read_bytes(),p.stat().st_mtime_ns) for p in self.output.iterdir()})
