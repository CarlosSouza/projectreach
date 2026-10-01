"""Rendering probes must not silently replace combat acceptance."""
import importlib.util
import os
import pathlib
import subprocess
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'scripts/xbox/smoke-simulator.py'
spec = importlib.util.spec_from_file_location('xbox_simulator_smoke', SCRIPT)
smoke = importlib.util.module_from_spec(spec)
spec.loader.exec_module(smoke)


class SimulatorDiagnosticsTests(unittest.TestCase):
    def test_default_combat_unchanged(self):
        self.assertEqual(smoke.match_environment(), {
            'HALO_NETWORK_TEST': 'host:bloodgulch', 'HALO_NETWORK_TEST_START': '8',
            'HALO_TEST_INPUT': 'bot:7', 'HALO_NETWORK_TEST_SHOOT': '4'})

    def test_stationary_has_no_scripted_input_or_gathering(self):
        settings = smoke.match_environment(True)
        self.assertEqual(settings['HALO_TEST_INPUT'], '')
        self.assertEqual(settings['HALO_NETWORK_TEST_SHOOT'], '0')

    def test_stationary_requires_explicit_diagnostic_match(self):
        for args in ([], ['--case', 'match'], ['--render-diagnostics'],
                     ['--case', 'campaign', '--render-diagnostics']):
            with self.subTest(args=args):
                result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                         '--stationary-match', *args], capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertIn('--stationary-match requires', result.stderr)

    def test_unknown_depth_mode_rejected_before_launch(self):
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True,
                                env=dict(os.environ, XG_DEPTH_COMPARE='typo'))
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_DEPTH_COMPARE must be', result.stderr)


if __name__ == '__main__':
    unittest.main()
