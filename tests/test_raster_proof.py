import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

tool=Path(__file__).resolve().parents[1]/'tools/raster_proof.py'
import sys
sys.path.insert(0,str(tool.parent))
spec=importlib.util.spec_from_file_location('raster_proof',tool)
proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)

class RasterEvidenceTests(unittest.TestCase):
    def fixture(self,root):
        (root/'runs.json').write_text(json.dumps({'mode':'service'}))
        for name in ('reference','fast'):
            run=root/name;scene=run/'scenes/frame-00000001';scene.mkdir(parents=True)
            for file in ('workram-001.bin','native.wav','state.tsv'): (run/file).write_bytes(b'same')
            (run/'events.txt').write_text('PORT test\nTIME 1\n')
            (run/'raster.tsv').write_text('sequence\ttime\tmode\tskip\tpolygons\n1\t1\t14\t0\t0\n')
            (scene/'metadata.json').write_text(json.dumps({'sequence':1}))
            (scene/'native-argb32.bin').write_bytes(b'same')

    def test_service_rejects_a_raster_bypass(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);self.fixture(root)
            (root/'fast/raster.tsv').write_text('sequence\ttime\tmode\tskip\tpolygons\n1\t1\t14\t1\t0\n')
            with self.assertRaises(ValueError):proof.compare(root)

    def test_service_rejects_native_image_difference(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);self.fixture(root)
            (root/'fast/scenes/frame-00000001/native-argb32.bin').write_bytes(b'wrong')
            with self.assertRaises(ValueError):proof.compare(root)
