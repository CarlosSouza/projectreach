"""Prepared packages use inert fixtures; the expected candidate identity is external to the ZIP."""
import importlib.util
import json
from pathlib import Path
import stat
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('halopad_package', ROOT / 'scripts/halopad_package.py')
pkg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pkg)


class PreparedPackageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.game = self.root / 'game'
        self.data = self.root / 'app/data'
        self.game.mkdir()
        self.data.mkdir(parents=True)
        self.exe = self.game / 'haloce.exe'
        self.exe.write_bytes(b'inert original input')
        (self.game / 'maps').mkdir()
        (self.game / 'maps/ui.map').write_bytes(b'inert resource' * 100)
        (self.game / 'config.txt').write_text('stock settings')
        (self.game / 'private-player-save').write_text('not in package')
        (self.game / 'unknown.dll').write_text('not in package')
        (self.data / 'image.bin').write_bytes(b'inert initialization')
        native = self.root / 'native.o'
        native.write_bytes(b'inert object fixture, never executed')
        self.profile = {'id': 'test-custom', 'executable': 'haloce.exe', 'accepted_sha256': pkg.digest(self.exe), 'modules': {}}
        self.stock = {'profile': self.profile['id'], 'files': [dict(path=str(p.relative_to(self.game)), **pkg.record(p))
                      for p in [self.exe, self.game / 'maps/ui.map', self.game / 'config.txt']]}
        self.identity = pkg.create_identity(self.profile, 'test-target', {'test.o': native}, self.data, self.stock)
        (self.data / 'core-identity.json').write_bytes(pkg.canonical(self.identity))
        self.output = self.root / 'test.halopad.zip'

    def prepare(self):
        return pkg.prepare(self.game, self.data, self.output)

    def rewrite(self, transform):
        replacement = self.root / 'mutated.zip'
        with zipfile.ZipFile(self.output) as src, zipfile.ZipFile(replacement, 'w') as dst:
            for info in src.infolist():
                body = src.read(info)
                for new_info, data in transform(info, body):
                    dst.writestr(new_info, data)
        return replacement

    def test_roundtrip_deterministic_and_excludes_state_and_native_code(self):
        manifest = self.prepare()
        self.assertEqual(pkg.verify(self.output, self.identity), manifest)
        second = self.root / 'again.halopad.zip'
        pkg.prepare(self.game, self.data, second)
        self.assertEqual(pkg.digest(self.output), pkg.digest(second))
        with zipfile.ZipFile(self.output) as archive:
            self.assertEqual(set(archive.namelist()), {'manifest.json', 'game/haloce.exe', 'game/maps/ui.map',
                                                       'game/config.txt', 'core-data/image.bin'})
        self.assertNotIn(str(self.root).encode(), pkg.canonical(manifest))

    def test_different_core_rejected_even_with_same_profile(self):
        self.prepare()
        other = dict(self.identity)
        other['target'] = 'another-native-core'
        other.pop('id')
        other['id'] = __import__('hashlib').sha256(pkg.canonical(other)).hexdigest()
        with self.assertRaises(pkg.PackageError):
            pkg.verify(self.output, other)

    def test_input_and_inert_data_identity_checked(self):
        for path in (self.exe, self.game / 'maps/ui.map', self.data / 'image.bin'):
            with self.subTest(path=path):
                original = path.read_bytes()
                path.write_bytes(b'changed')
                with self.assertRaises(pkg.PackageError): self.prepare()
                self.assertFalse(self.output.exists())
                path.write_bytes(original)

    def test_case_insensitive_source_mapping(self):
        upper = self.game / 'HALOCE.EXE'
        self.exe.rename(upper)
        self.prepare()
        pkg.verify(self.output, self.identity)

    def test_links_and_special_files_rejected(self):
        link = self.game / 'link'
        link.symlink_to(self.data, target_is_directory=True)
        with self.assertRaises(pkg.PackageError): self.prepare()
        link.unlink()
        __import__('os').mkfifo(link)
        with self.assertRaises(pkg.PackageError): self.prepare()

    def test_corruption_missing_extra_and_symlink_members_rejected(self):
        self.prepare()
        def corrupt(info, body):
            return [(info, b'x' * len(body) if info.filename == 'game/maps/ui.map' else body)]
        def missing(info, body):
            return [] if info.filename == 'game/maps/ui.map' else [(info, body)]
        def extra(info, body):
            return [(info, body), ('game/unexpected', b'bad')] if info.filename == 'manifest.json' else [(info, body)]
        def link(info, body):
            if info.filename == 'game/maps/ui.map': info.external_attr = (stat.S_IFLNK | 0o777) << 16
            return [(info, body)]
        for mutate in (corrupt, missing, extra, link):
            with self.subTest(mutate=mutate.__name__), self.assertRaises(pkg.PackageError):
                pkg.verify(self.rewrite(mutate), self.identity)

    def test_manifest_cannot_choose_its_own_hashes(self):
        self.prepare()
        def change(info, body):
            if info.filename == 'manifest.json':
                manifest = json.loads(body)
                manifest['files']['game/maps/ui.map']['sha256'] = '0' * 64
                body = pkg.canonical(manifest)
            return [(info, body)]
        with self.assertRaises(pkg.PackageError): pkg.verify(self.rewrite(change), self.identity)

    def test_traversal_and_collisions_rejected(self):
        for name in ('../escape', '/absolute', 'C:/drive', 'a\\b', 'a//b', 'a/./b', 'a\x00b', 'a./b'):
            with self.subTest(name=name), self.assertRaises(pkg.PackageError): pkg.safe_name(name)
        for names in (['a', 'A'], ['a', 'a/b'], ['caf\u00e9', 'cafe\u0301'], ['A/one', 'a/two']):
            with self.subTest(names=names), self.assertRaises(pkg.PackageError): pkg.unique_names(names)
        self.prepare()
        def traversal(info, body):
            if info.filename == 'game/maps/ui.map': info.filename = '../escape'
            return [(info, body)]
        with self.assertRaises(pkg.PackageError): pkg.verify(self.rewrite(traversal), self.identity)
        self.assertFalse((self.root.parent / 'escape').exists())

    def test_duplicate_json_keys_rejected(self):
        with self.assertRaises(pkg.PackageError): pkg.load_json(b'{"schema":1,"schema":1}')

    def test_size_limits_and_types(self):
        for size in (-1, pkg.MAX_FILE + 1, True, 1.5):
            with self.subTest(size=size), self.assertRaises(pkg.PackageError):
                pkg.check_records({'x': {'size': size, 'sha256': '0' * 64}})
        self.prepare()
        def oversized(info, body):
            if info.filename == 'manifest.json': body = b' ' * (pkg.MAX_MANIFEST + 1)
            return [(info, body)]
        with self.assertRaises(pkg.PackageError): pkg.verify(self.rewrite(oversized), self.identity)

    def test_no_clobber_and_no_partial_on_write_failure(self):
        self.output.write_text('keep existing')
        with self.assertRaises(pkg.PackageError): self.prepare()
        self.assertEqual(self.output.read_text(), 'keep existing')
        self.output.unlink()
        with patch.object(pkg.zipfile.ZipFile, 'open', side_effect=OSError('disk full')):
            with self.assertRaises(OSError): self.prepare()
        self.assertFalse(self.output.exists())
        self.assertEqual(list(self.root.glob('.halopad-package-*')), [])

    def test_source_changes_during_write_never_publish(self):
        original_sources = pkg.sources_for
        def change_after_preflight(root, expected):
            selected = original_sources(root, expected)
            if root == self.data:
                self.exe.write_bytes(b'changed after the initial hash')
            return selected
        with patch.object(pkg, 'sources_for', side_effect=change_after_preflight):
            with self.assertRaises(pkg.PackageError): self.prepare()
        self.assertFalse(self.output.exists())

    def test_output_inside_source_rejected_without_creating_directories(self):
        with self.assertRaises(pkg.PackageError):
            pkg.prepare(self.game, self.data, self.game / 'new-dir/result.halopad.zip')
        self.assertFalse((self.game / 'new-dir').exists())


if __name__ == '__main__':
    unittest.main()
