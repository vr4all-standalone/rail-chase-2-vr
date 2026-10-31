import importlib.util
from pathlib import Path
import unittest

SPEC = importlib.util.spec_from_file_location(
    'rchase2_replay', Path(__file__).resolve().parents[1]/'tools/replay_model2_trace.py')
REPLAY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(REPLAY)


class ReplayEvidenceTests(unittest.TestCase):
    def test_geometry_retains_captured_matrix_for_native_source_validation(self):
        data = dict(sequence=7, width=496, height=384, xoffset=0, yoffset=128, polygons=[dict(
            vertices=[[1, 2, 6, 0, 0]]*3, attr=1282, window=1, sort=5, luma=255,
            lod=0, object=0x835f85, culled=False, header=[16384, 0, 3528, 7936],
            focus=[768, 768], center=[248, 320], viewport=[0, 128, 496, 512],
            matrix=[.4, 0, 0, 0, .4, 0, 0, 0, 1, -.5, -.25, 6])])
        rows = REPLAY.geometry(data).splitlines()
        self.assertEqual(rows[0], '7 496 384 0 128 1 1')
        self.assertEqual([float(x) for x in rows[1].split()[20:32]],
                         [.4, 0, 0, 0, .4, 0, 0, 0, 1, -.5, -.25, 6])

    def test_flat_or_panel_stereo_does_not_claim_world_geometry(self):
        for world, panel, kind in [(0, 0, 'native_raster'), (0, 8, 'panel_geometry'), (12, 2, 'world_geometry')]:
            with self.subTest(kind=kind):
                evidence = REPLAY.geometry_evidence(f'Geometry evidence: {world} world triangles, {panel} panel triangles\nGL test\n')
                self.assertEqual(evidence['evidence_kind'], kind)
                self.assertEqual(evidence['world_triangles'], world)
                self.assertEqual(evidence['panel_triangles'], panel)

    def test_old_replay_binary_without_geometry_evidence_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'rebuild'):
            REPLAY.geometry_evidence('Replayed 100 native polygons in three independent eye passes\n')


if __name__ == '__main__':
    unittest.main()
