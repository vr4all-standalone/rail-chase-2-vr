"""Overlapping patch upgrades must be repeatable and preserve unknown edits."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest


SPEC = importlib.util.spec_from_file_location(
    'rchase2_patch_sync', Path(__file__).resolve().parents[1]/'tools/sync_engine_patches.py')
SYNC = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SYNC)


class PatchSyncTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.engine = self.root/'engine'
        self.engine.mkdir()
        self.patches = self.root/'patches'
        self.patches.mkdir()
        self.git('init', '-q')
        for name in ('driver.cpp', 'video.cpp'):
            (self.engine/name).write_bytes(b'first\nsecond\nthird\n')
        self.git('add', '.')
        self.git('-c', 'user.name=Gunblade tests', '-c', 'user.email=local@example.invalid',
                 'commit', '-qm', 'fixture')
        self.commit = self.git('rev-parse', 'HEAD').strip()
        self.patch('0001.patch', 'second', 'capture')
        self.patch('0002.patch', 'capture', 'live')

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.engine), *args], text=True,
                                       stderr=subprocess.STDOUT)

    def patch(self, name, before, after):
        (self.patches/name).write_text(
            'diff --git a/driver.cpp b/driver.cpp\n'
            '--- a/driver.cpp\n+++ b/driver.cpp\n@@ -1,3 +1,3 @@\n first\n'
            f'-{before}\n+{after}\n third\n')

    def synchronize(self):
        SYNC.synchronize(self.engine, self.patches, self.commit)

    def test_overlapping_stack_upgrades_original_and_is_repeatable(self):
        self.synchronize()
        expected = b'first\nlive\nthird\n'
        self.assertEqual((self.engine/'driver.cpp').read_bytes(), expected)
        self.synchronize()
        self.assertEqual((self.engine/'driver.cpp').read_bytes(), expected)

    def test_recognizes_intermediate_patch_with_crlf(self):
        (self.engine/'driver.cpp').write_bytes(b'first\r\ncapture\r\nthird\r\n')
        self.synchronize()
        self.assertEqual((self.engine/'driver.cpp').read_bytes(), b'first\r\nlive\r\nthird\r\n')

    def test_unknown_edit_refuses_whole_upgrade_without_writing(self):
        # A second recognized file must remain untouched when another is unknown.
        (self.patches/'0003.patch').write_text(
            'diff --git a/video.cpp b/video.cpp\n--- a/video.cpp\n+++ b/video.cpp\n'
            '@@ -1,3 +1,3 @@\n first\n-second\n+renderer\n third\n')
        (self.engine/'video.cpp').write_text('first\nprivate edit\nthird\n')
        before = {p.name: p.read_bytes() for p in self.engine.glob('*.cpp')}
        with self.assertRaisesRegex(ValueError, 'unrecognized edits'):
            self.synchronize()
        self.assertEqual({p.name: p.read_bytes() for p in self.engine.glob('*.cpp')}, before)


if __name__ == '__main__':
    unittest.main()
