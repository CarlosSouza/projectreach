"""Mac smoke evidence must identify the binaries it actually runs."""
import hashlib
import importlib.util
import json
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('xbox_mac_smoke', ROOT / 'scripts/xbox/smoke-mac.py')
smoke = importlib.util.module_from_spec(spec)
spec.loader.exec_module(smoke)


class XboxMacManifestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = pathlib.Path(self.temp.name)
        self.exe = root / 'exe'
        self.image = root / 'image'
        self.path = root / 'build.json'
        self.exe.write_bytes(b'fixture executable')
        self.image.write_bytes(b'fixture image')
        self.manifest = {
            'revision': 'fixture-revision',
            'executable_sha256': hashlib.sha256(self.exe.read_bytes()).hexdigest(),
            'guest_sha256': hashlib.sha256(self.image.read_bytes()).hexdigest(),
        }
        self.path.write_text(json.dumps(self.manifest))

    def check(self):
        return smoke.verify_build(self.exe, self.image, self.path)

    def test_exact_build(self):
        self.assertEqual(self.check(), self.manifest)

    def test_changed_image(self):
        self.image.write_bytes(b'new guest')
        with self.assertRaisesRegex(ValueError, 'Stale Xbox Mac build'):
            self.check()

    def test_changed_executable(self):
        self.exe.write_bytes(b'new executable')
        with self.assertRaisesRegex(ValueError, 'Stale Xbox Mac build'):
            self.check()

    def test_missing_manifest(self):
        self.path.unlink()
        with self.assertRaisesRegex(ValueError, 'no manifest'):
            self.check()

    def test_missing_revision(self):
        del self.manifest['revision']
        self.path.write_text(json.dumps(self.manifest))
        with self.assertRaisesRegex(ValueError, 'no revision'):
            self.check()


if __name__ == '__main__':
    unittest.main()
