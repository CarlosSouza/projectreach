"""Input acceptance must reject wrong builds independently of hash acceptance."""
import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('inspect_inputs', ROOT / 'scripts/inspect-inputs.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class IdentityGuards(unittest.TestCase):
    def setUp(self):
        self.report = {'machine': 0x14c, 'optional_magic': 0x10b, 'versions': ['1.0.10.0621'], 'sha256': 'a'*64}
        self.profile = {'version': '1.0.10.0621', 'accepted_sha256': 'a'*64}
    def test_unknown_hash_is_never_accepted(self):
        self.profile['accepted_sha256'] = None
        self.assertTrue(module.profile_errors(self.report, self.profile))
    def test_wrong_version_rejected_even_if_hash_matches(self):
        self.report['versions'] = ['1.0.0.609']
        self.assertTrue(module.profile_errors(self.report, self.profile))
    def test_wrong_machine_rejected(self):
        self.report['machine'] = 0x8664
        self.assertTrue(module.profile_errors(self.report, self.profile))
    def test_wrong_hash_rejected(self):
        self.report['sha256'] = 'b'*64
        self.assertTrue(module.profile_errors(self.report, self.profile))
    def test_exact_identity_does_not_assert_gameplay(self):
        self.assertEqual(module.profile_errors(self.report, self.profile), [])
