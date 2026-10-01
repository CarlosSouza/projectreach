"""A stale or rejected Xbox candidate must not silently enter an app build."""
import hashlib
import importlib.util
import json
import os
import pathlib
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
spec = importlib.util.spec_from_file_location('ios_builder', ROOT / 'scripts/build-ios-app.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
PIN = json.loads((ROOT / 'config/xbox-engine.lock.json').read_text())['revision']


class XboxManifestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.out = pathlib.Path(self.temp.name)
        self.lib = self.out / 'iphonesimulator/libhalopad-xbox.a'
        self.lib.parent.mkdir()
        self.lib.write_bytes(b'fixture library, not game code')
        self.guest = self.out / 'halo_guest.elf'
        self.guest.write_bytes(b'fixture guest, not game code')
        self.manifest = {
            'revision': PIN,
            'guest_sha256': hashlib.sha256(self.guest.read_bytes()).hexdigest(),
            'library_sha256': hashlib.sha256(self.lib.read_bytes()).hexdigest(),
        }
        self.save_manifest()
        self.addCleanup(patch.stopall)
        patch.object(builder, 'XBOX_OUT', self.out).start()
        patch.dict(os.environ, {}, clear=True).start()

    def save_manifest(self):
        (self.lib.parent / 'build.json').write_text(json.dumps(self.manifest))

    def test_pc_only_without_local_library(self):
        self.assertEqual(builder.xbox_parts(builder.DEVICE_TARGET), [])

    def test_exact_pin(self):
        self.assertIn(self.lib, builder.xbox_parts(builder.TARGET))

    def test_rejected_revision(self):
        self.manifest['revision'] = 'candidate'
        self.save_manifest()
        with self.assertRaisesRegex(ValueError, 'revision differs'):
            builder.xbox_parts(builder.TARGET)

    def test_explicit_candidate_override(self):
        self.manifest['revision'] = 'candidate'
        self.save_manifest()
        with patch.dict(os.environ, {'XBOX_REV': 'candidate'}):
            self.assertIn(self.lib, builder.xbox_parts(builder.TARGET))

    def test_mismatched_guest(self):
        self.guest.write_bytes(b'different guest')
        with self.assertRaisesRegex(ValueError, 'Stale Xbox build'):
            builder.xbox_parts(builder.TARGET)

    def test_mismatched_library(self):
        self.lib.write_bytes(b'different library')
        with self.assertRaisesRegex(ValueError, 'Stale Xbox build'):
            builder.xbox_parts(builder.TARGET)

    def test_legacy_library_requires_rebuild(self):
        (self.lib.parent / 'build.json').unlink()
        with self.assertRaisesRegex(ValueError, 'no build manifest'):
            builder.xbox_parts(builder.TARGET)

    def angle_fixture(self):
        self.lib = self.out / 'iphonesimulator-angle/libhalopad-xbox.a'
        self.lib.parent.mkdir()
        self.lib.write_bytes(b'fixture library, not game code')
        self.manifest['renderer'] = 'angle-metal'
        self.manifest['angle_source'] = json.loads((ROOT / 'config/xbox-angle.lock.json').read_text())
        self.save_manifest()
        patch.dict(os.environ, {'HALOPAD_XBOX_RENDERER': 'angle-metal'}).start()

    def test_angle_links_only_requested_renderer(self):
        self.angle_fixture()
        parts = builder.xbox_parts(builder.TARGET)
        self.assertIn(self.lib, parts)
        self.assertIn('Metal', parts)
        self.assertNotIn('OpenGLES', parts)

    def test_angle_rejects_physical_device(self):
        self.angle_fixture()
        with self.assertRaisesRegex(ValueError, 'Simulator-only'):
            builder.xbox_parts(builder.DEVICE_TARGET)

    def test_unknown_renderer_rejected(self):
        with patch.dict(os.environ, {'HALOPAD_XBOX_RENDERER': 'unknown'}):
            with self.assertRaisesRegex(ValueError, 'Unknown'):
                builder.xbox_parts(builder.TARGET)

    def test_missing_angle_does_not_silently_build_pc_only(self):
        with patch.dict(os.environ, {'HALOPAD_XBOX_RENDERER': 'angle-metal'}):
            with self.assertRaisesRegex(ValueError, 'library is missing'):
                builder.xbox_parts(builder.TARGET)

    def test_wrong_angle_source_rejected(self):
        self.angle_fixture()
        self.manifest['angle_source']['revision'] = 'different'
        self.save_manifest()
        with self.assertRaisesRegex(ValueError, 'source differs'):
            builder.xbox_parts(builder.TARGET)

    def test_wrong_renderer_manifest_rejected(self):
        self.angle_fixture()
        self.manifest['renderer'] = 'apple-gles'
        self.save_manifest()
        with self.assertRaisesRegex(ValueError, 'renderer differs'):
            builder.xbox_parts(builder.TARGET)


if __name__ == '__main__':
    unittest.main()
