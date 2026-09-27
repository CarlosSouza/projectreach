import importlib.util
import pathlib
import struct
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('x86_oracle', ROOT / 'scripts' / 'x86-oracle.py')
oracle_mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle_mod)

INPUT = ROOT / 'ref' / 'inputs' / 'custom-original' / 'haloce.exe'


@unittest.skipUnless(INPUT.exists(), 'accepted Custom Edition input root not assembled')
class OracleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.oracle = oracle_mod.Oracle()
        cls.slots = {name: s for s, name, _ in cls.oracle.image.imports}

    def snippet(self, code, convention='cdecl'):
        # The first bytes argument is copied to HEAP_BASE, so it can be run as code.
        return self.oracle.call(oracle_mod.HEAP_BASE, [code], convention)

    def test_crc32_check_value(self):
        obs = self.oracle.call(oracle_mod.CRC32, [0, b'123456789', 9], 'stdcall')
        self.assertEqual(obs['eax'], 0xCBF43926)
        self.assertEqual(obs['stack_delta'], 16)

    def test_teb_and_tls_are_visible_through_fs(self):
        # mov eax, fs:[0x18]; ret  -> TEB self pointer
        obs = self.snippet(bytes.fromhex('64a118000000c3'))
        self.assertEqual(obs['eax'], oracle_mod.TEB_BASE)
        # mov eax, fs:[0x2c]; mov eax, [eax]; ret -> TLS slot 0 block
        obs = self.snippet(bytes.fromhex('64a12c0000008b00c3'))
        self.assertEqual(obs['eax'], oracle_mod.TLS_BASE + 0x100)

    def test_static_import_traps_loudly(self):
        slot = self.slots['KERNEL32.dll!GetTickCount']
        code = b'\xff\x15' + struct.pack('<I', slot) + b'\xc3'
        with self.assertRaisesRegex(oracle_mod.OracleError, 'KERNEL32.dll!GetTickCount'):
            self.snippet(code)

    def test_real_halo_import_call_site_traps(self):
        # 0x479440 is the first 'call [GetTickCount]' in .text of the accepted image.
        with self.assertRaisesRegex(oracle_mod.OracleError, 'GetTickCount'):
            self.oracle.call(0x479440, [], 'cdecl')

    def test_delay_import_traps_loudly(self):
        name, slot = next((n, s) for s, n, d in self.oracle.image.imports if d and n.startswith('WSOCK32'))
        code = b'\xff\x15' + struct.pack('<I', slot) + b'\xc3'
        with self.assertRaisesRegex(oracle_mod.OracleError, 'delay:' + name.replace('#', '\\#')):
            self.snippet(code)

    def test_explicit_handler_is_used_only_when_supplied(self):
        handled = oracle_mod.Oracle(self.oracle.image, {'KERNEL32.dll!GetTickCount': lambda uc: (1234, 0)})
        slot = self.slots['KERNEL32.dll!GetTickCount']
        obs = handled.call(oracle_mod.HEAP_BASE, [b'\xff\x15' + struct.pack('<I', slot) + b'\xc3'], 'cdecl')
        self.assertEqual(obs['eax'], 1234)

    def test_unmapped_access_fails(self):
        # mov eax, [0x00000010]; ret
        with self.assertRaisesRegex(oracle_mod.OracleError, 'unmapped'):
            self.snippet(bytes.fromhex('a110000000c3'))

    def test_rejects_unaccepted_image(self):
        other = ROOT / 'generated' / 'inspection' / 'supplied-installer' / 'haloce.exe'
        if other.exists():
            with self.assertRaises(oracle_mod.OracleError):
                oracle_mod.Image(other)


if __name__ == '__main__':
    unittest.main()

