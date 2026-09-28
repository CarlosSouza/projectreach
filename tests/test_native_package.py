"""Native ZIP import contract. Run via scripts/test-native-package.py (inert fixtures only)."""
import copy
import io
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import tempfile
import unittest
import zipfile
import zlib
import test_game_package as package_tests
pkg = package_tests.pkg


# Independently construct ZIP64 and streamed descriptors; also checked by Python's reader.
def archive_bytes(files, method=8, descriptor=False, zip64=False, signature=True):
    local = bytearray()
    central = bytearray()
    for name, raw in files:
        name = name.encode('utf-8')
        offset = len(local)
        compressor = zlib.compressobj(wbits=-15)
        body = compressor.compress(raw) + compressor.flush() if method == 8 else raw
        crc = zlib.crc32(raw)
        flags = 0x800 | (8 if descriptor else 0)
        version = 45 if zip64 else 20
        extra = struct.pack('<HHQQ', 1, 16, len(raw), len(body)) if zip64 else b''
        compressed, expanded = (0xffffffff, 0xffffffff) if zip64 else ((0, 0) if descriptor else (len(body), len(raw)))
        local += struct.pack('<I5H3I2H', 0x04034b50, version, flags, method, 0, 0,
                             0 if descriptor else crc, compressed, expanded, len(name), len(extra)) + name + extra + body
        if descriptor:
            local += (struct.pack('<I', 0x08074b50) if signature else b'')
            local += struct.pack('<IQQ' if zip64 else '<III', crc, len(body), len(raw))
        extra = struct.pack('<HHQQQ', 1, 24, len(raw), len(body), offset) if zip64 else b''
        central += struct.pack('<I6H3I5H2I', 0x02014b50, (3 << 8) | version, version, flags, method, 0, 0,
                               crc, 0xffffffff if zip64 else len(body), 0xffffffff if zip64 else len(raw),
                               len(name), len(extra), 0, 0, 0, (stat.S_IFREG | 0o600) << 16,
                               0xffffffff if zip64 else offset) + name + extra
    offset = len(local)
    local += central
    count = len(files)
    if zip64:
        end64 = len(local)
        local += struct.pack('<IQ2H2I4Q', 0x06064b50, 44, 45, 45, 0, 0, count, count, len(central), offset)
        local += struct.pack('<IIQI', 0x07064b50, 0, end64, 1)
    local += struct.pack('<I4H2IH', 0x06054b50, 0, 0, 0xffff if zip64 else count, 0xffff if zip64 else count,
                         0xffffffff if zip64 else len(central), 0xffffffff if zip64 else offset, 0)
    return bytes(local)


@unittest.skipUnless(os.environ.get("HALOPAD_PACKAGE_TEST_COMMAND"), "run scripts/test-native-package.py")
class NativePackageTests(unittest.TestCase):
    def setUp(self):
        self.fixture = package_tests.PreparedPackageTests()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        f = self.fixture
        # More than one inflate output chunk, including a zero-length regular file.
        (f.game / 'maps/ui.map').write_bytes(b'inert resource' * 20000)
        f.identity['stock_files']['maps/ui.map'] = pkg.record(f.game / 'maps/ui.map')
        (f.game / 'maps/empty.map').write_bytes(b'')
        f.identity['stock_files']['maps/empty.map'] = pkg.record(f.game / 'maps/empty.map')
        f.identity.pop('id')
        f.identity['id'] = __import__('hashlib').sha256(pkg.canonical(f.identity)).hexdigest()
        self.identity = f.data / 'core-identity.json'
        self.identity.write_bytes(pkg.canonical(f.identity))
        f.prepare()
        self.destination = f.root / 'installed'
        self.destination.mkdir()
        (self.destination / 'keep').write_bytes(b'old installation and player state')
        with zipfile.ZipFile(f.output) as archive:
            self.files = [(n, archive.read(n)) for n in archive.namelist()]
        self.source_hash = pkg.digest(f.output)

    def run_native(self, path=None, mode=None, success=True):
        command = json.loads(os.environ['HALOPAD_PACKAGE_TEST_COMMAND'])
        result = subprocess.run(command + [str(self.identity), str(path or self.fixture.output), str(self.destination)] + ([mode] if mode else []),
                                capture_output=True, text=True, timeout=30)
        self.assertIn(result.returncode, (0, 1), result.stderr)
        response = json.loads(result.stdout)
        self.assertEqual(response['ok'], success, response)
        self.assertEqual(result.returncode, 0 if success else 1)
        self.assertEqual(pkg.digest(self.fixture.output), self.source_hash)
        if not success:
            self.assertTrue(response['error'])
            self.assertEqual((self.destination / 'keep').read_bytes(), b'old installation and player state')
            self.assertFalse(list(self.fixture.root.glob('.halopad-import-*')))
        else:
            expected = self.fixture.identity['stock_files']
            self.assertEqual(set(pkg.inventory(self.destination)), set(expected))
            for name, record in expected.items():
                self.assertEqual(pkg.record(self.destination / name), record)
            if response['backup']:
                self.assertEqual((Path(response['backup']) / 'keep').read_bytes(), b'old installation and player state')
        return response

    def save(self, raw):
        path = self.fixture.root / 'variation.zip'
        path.write_bytes(raw)
        return path

    def rejected_bytes(self, raw):
        return self.run_native(self.save(raw), success=False)

    def reset_destination(self):
        __import__('shutil').rmtree(self.destination)
        self.destination.mkdir()
        (self.destination / 'keep').write_bytes(b'old installation and player state')

    def test_producer_roundtrip_retains_previous_installation(self):
        response = self.run_native()
        self.assertTrue(response['backup'])
        self.assertFalse(list(self.destination.glob('core-data')))

    def test_first_import(self):
        (self.destination / 'keep').unlink()
        self.destination.rmdir()
        self.assertEqual(self.run_native()['backup'], '')

    def test_stored_deflate_zip64_descriptors_and_empty_files(self):
        for method in (0, 8):
            for zip64 in (False, True):
                for descriptor, signature in ((False, False), (True, False), (True, True)):
                    with self.subTest(method=method, zip64=zip64, descriptor=descriptor, signature=signature):
                        raw = archive_bytes(self.files, method, descriptor, zip64, signature)
                        with zipfile.ZipFile(io.BytesIO(raw)) as archive:
                            self.assertEqual([(n, archive.read(n)) for n in archive.namelist()], self.files)
                        self.run_native(self.save(raw))
                        self.reset_destination()

    def test_python_forced_local_zip64(self):
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in self.files:
                with archive.open(name, 'w', force_zip64=True) as dest:
                    dest.write(data)
        self.run_native(self.save(buffer.getvalue()))

    def test_manifest_cannot_choose_core_hashes_types_or_duplicate_keys(self):
        raw_manifest = self.files[0][1]
        value = json.loads(raw_manifest)
        changes = []
        for key, replacement in [('core_id', '0'*64), ('profile', 'retail'), ('schema', True), ('schema', 1.0)]:
            changed = copy.deepcopy(value); changed[key] = replacement
            changes.append(json.dumps(changed).encode())
        changed = copy.deepcopy(value); changed['files']['game/haloce.exe']['sha256'] = '0'*64
        changes += [pkg.canonical(changed), b'{"schema":99,' + raw_manifest[1:], b'{"sch\\u0065ma":99,' + raw_manifest[1:],
                    raw_manifest.replace(b'"size":20', b'"size":true'), b'[' * 100 + b'0' + b']' * 100, b'\xff', raw_manifest+b'junk']
        # A duplicate file-record property, after the real value, must also fail.
        changes.append(raw_manifest.replace(b'"sha256":', b'"size":42,"sha256":', 1))
        for changed in changes:
            if changed == raw_manifest:
                continue
            with self.subTest(manifest=changed[:80]):
                self.rejected_bytes(archive_bytes([('manifest.json', changed)] + self.files[1:]))

    def test_noncanonical_but_equivalent_manifest_is_accepted(self):
        manifest = json.dumps(json.loads(self.files[0][1]), indent=3).encode()
        self.run_native(self.save(archive_bytes([('manifest.json', manifest)] + self.files[1:])))

    def test_content_mutations_including_inert_core_data(self):
        for target in ['game/haloce.exe', 'game/maps/ui.map', 'core-data/image.bin']:
            with self.subTest(target=target):
                changed = [(n, (b'X' + data[1:]) if n == target else data) for n, data in self.files]
                self.rejected_bytes(archive_bytes(changed))

    def test_names_missing_extra_duplicate_traversal_and_aliases(self):
        for name in ['../escape', '/absolute', 'game/../escape', 'game//bad', 'game/back\\slash', 'game/a:b',
                     'game/trailing.', 'game/trailing ', 'game/./bad', 'game/\x00bad', 'game/HALOCE.EXE',
                     'manifest.json', 'core-data/../escape', 'game/' + 'x'*513]:
            with self.subTest(name=name):
                changed = self.files[:1] + [(name, self.files[1][1])] + self.files[2:]
                self.rejected_bytes(archive_bytes(changed))
        self.rejected_bytes(archive_bytes(self.files[:-1]))
        self.rejected_bytes(archive_bytes(self.files + [('extra', b'not allowed')]))

    def test_archive_symlink_fifo_and_destination_link_rejected(self):
        link = self.fixture.root / 'link.zip'; link.symlink_to(self.fixture.output)
        self.run_native(link, success=False)
        fifo = self.fixture.root / 'fifo.zip'; os.mkfifo(fifo)
        self.run_native(fifo, success=False)
        original = self.fixture.root / 'original'
        self.destination.rename(original); self.destination.symlink_to(original, target_is_directory=True)
        self.run_native(success=False)

    def test_write_and_commit_failures_preserve_old_tree(self):
        for mode in ('write-first', 'write', 'publish'):
            with self.subTest(mode=mode): self.run_native(mode=mode, success=False)

    def test_backup_naming_failure_preserves_old_tree(self):
        response = self.run_native(mode='backup')
        self.assertTrue(Path(response['backup']).name.startswith('.halopad-import-'))

    def test_bad_central_and_local_metadata(self):
        raw = archive_bytes(self.files)
        cd = raw.index(b'PK\x01\x02')
        # Each malformed archive starts from an otherwise valid producer-equivalent ZIP.
        patches = [(cd+8, '<H', 1), (cd+10, '<H', 99), (cd+34, '<H', 1),
                   (cd+38, '<I', (stat.S_IFLNK | 0o777) << 16),
                   (cd+38, '<I', (stat.S_IFIFO | 0o600) << 16), (cd+38, '<I', 0x10),
                   (cd+24, '<I', 0x7fffffff), (cd+20, '<I', 0xffffffff),
                   (cd+42, '<I', 1), (cd+28, '<H', 0xffff), (cd+30, '<H', 0xffff),
                   (6, '<H', 1), (8, '<H', 0), (14, '<I', 0), (18, '<I', 1), (22, '<I', 1),
                   (26, '<H', 0), (28, '<H', 0xffff), (30, '<B', ord('X'))]
        for at, fmt, value in patches:
            with self.subTest(at=at, fmt=fmt, value=value):
                bad = bytearray(raw); struct.pack_into(fmt, bad, at, value); self.rejected_bytes(bad)
        # Truncated end/directory/data and trailing bytes, multi-volume EOCD.
        for cutoff in (0, 1, 22, len(raw)//2, len(raw)-1, len(raw)-22): self.rejected_bytes(raw[:cutoff])
        self.rejected_bytes(raw+b'extra')
        bad = bytearray(raw); struct.pack_into('<H', bad, len(raw)-18, 1); self.rejected_bytes(bad)

    def test_corrupt_zip64_and_descriptors(self):
        raw = archive_bytes(self.files, descriptor=True, zip64=True)
        record = raw.index(b'PK\x06\x06'); locator = raw.index(b'PK\x06\x07'); cd = raw.index(b'PK\x01\x02')
        descriptor = raw.index(b'PK\x07\x08')
        for at, fmt, value in [(record+4, '<Q', 1), (record+48, '<Q', 2**64-1),
                               (locator+8, '<Q', 2**64-1), (locator+16, '<I', 2),
                               (30+len('manifest.json')+2, '<H', 1),
                               (cd+46+len('manifest.json')+4, '<Q', 2**64-1),
                               (descriptor+4, '<I', 0), (descriptor+8, '<Q', 2**64-1)]:
            with self.subTest(at=at):
                bad = bytearray(raw); struct.pack_into(fmt, bad, at, value); self.rejected_bytes(bad)

    def test_descriptor_crc_can_equal_optional_signature(self):
        target = 0x08074b50
        base = zlib.crc32(bytes(4))
        basis = {}
        for bit in range(32):
            value = zlib.crc32((1 << bit).to_bytes(4, 'little')) ^ base
            mask = 1 << bit
            while value:
                pivot = value.bit_length()-1
                if pivot not in basis:
                    basis[pivot] = (value, mask)
                    break
                vector, combination = basis[pivot]
                value ^= vector; mask ^= combination
        value, mask = target ^ base, 0
        while value:
            vector, combination = basis[value.bit_length()-1]
            value ^= vector; mask ^= combination
        raw = mask.to_bytes(4, 'little')
        self.assertEqual(zlib.crc32(raw), target)
        record = {'size': 4, 'sha256': __import__('hashlib').sha256(raw).hexdigest()}
        identity = self.fixture.identity
        identity['stock_files']['config.txt'] = record
        identity.pop('id')
        identity['id'] = __import__('hashlib').sha256(pkg.canonical(identity)).hexdigest()
        self.identity.write_bytes(pkg.canonical(identity))
        files = [(n, pkg.canonical(pkg.manifest_for(identity)) if n == 'manifest.json' else raw if n == 'game/config.txt' else body)
                 for n, body in self.files]
        for zip64 in (False, True):
            for signature in (False, True):
                with self.subTest(zip64=zip64, signature=signature):
                    self.run_native(self.save(archive_bytes(files, descriptor=True, zip64=zip64, signature=signature)))
                    self.reset_destination()

    def test_invalid_bundled_identity_fails_closed(self):
        original = self.identity.read_bytes()
        values = [None, [], {}, dict(self.fixture.identity, schema=True), dict(self.fixture.identity, id='invalid')]
        for size in (-1, 1.5, True, 2**31+1):
            value = copy.deepcopy(self.fixture.identity)
            value['stock_files']['haloce.exe']['size'] = size
            values.append(value)
        value = copy.deepcopy(self.fixture.identity)
        value['stock_files']['MAPS/other'] = value['stock_files']['maps/ui.map']
        values.append(value)
        value = copy.deepcopy(self.fixture.identity)
        value['stock_files']['maps'] = value['stock_files']['maps/ui.map']
        values.append(value)
        for value in values:
            with self.subTest(identity=value):
                self.identity.write_text(json.dumps(value))
                self.run_native(success=False)
        self.identity.write_bytes(original)

    def test_payload_corruption_and_expansion_bomb(self):
        raw = bytearray(archive_bytes(self.files))
        at = 30+len('manifest.json'); raw[at] ^= 0xff
        self.rejected_bytes(raw)
        changed = [(n, b'X'*4000000 if n == 'game/maps/ui.map' else body) for n, body in self.files]
        raw = bytearray(archive_bytes(changed))
        # Lie consistently in both headers; streaming output still must stop at the trusted bound.
        with zipfile.ZipFile(io.BytesIO(raw)) as archive:
            info = archive.getinfo('game/maps/ui.map')
        struct.pack_into('<I', raw, info.header_offset+22, self.fixture.identity['stock_files']['maps/ui.map']['size'])
        cd = raw.index(b'PK\x01\x02')
        while raw[cd+46:cd+46+struct.unpack_from('<H', raw, cd+28)[0]] != b'game/maps/ui.map':
            cd += 46 + sum(struct.unpack_from('<3H', raw, cd+28))
        struct.pack_into('<I', raw, cd+24, self.fixture.identity['stock_files']['maps/ui.map']['size'])
        self.rejected_bytes(raw)
