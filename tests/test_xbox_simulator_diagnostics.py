"""Rendering probes must not silently replace combat acceptance."""
import importlib.util
import os
import pathlib
import subprocess
import sys
import tempfile
import time
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

    def test_paired_depth_requires_stationary_match(self):
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--case', 'match', '--render-diagnostics'],
                                capture_output=True, text=True,
                                env=dict(os.environ, XG_DEPTH_COMPARE='paired'))
        self.assertEqual(result.returncode, 2)
        self.assertIn('paired depth requires --stationary-match', result.stderr)

    def test_depth_probe_requires_two_distinct_complete_controls(self):
        same = 'depth probe: same-program covered 6728 failed 0 error 0x0\n'
        other = 'depth probe: separate-program covered 6728 failed 0 error 0x0\n'
        self.assertTrue(smoke.depth_probe_pass(same + other))
        for text in (same, same + same, '', (same + other).replace('covered 6728', 'covered 0'),
                     (same + other).replace('failed 0', 'failed 1'),
                     (same + other).replace('error 0x0', 'error 0x502')):
            with self.subTest(text=text):
                self.assertFalse(smoke.depth_probe_pass(text))

    def test_fresh_complete_pair_is_preserved(self):
        data = b'P6\n1 1\n255\n\xff\x20\x00'
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory)
            for part in ('source', 'destination'):
                (folder / ('presentation.' + part + '.ppm')).write_bytes(data)
            self.assertTrue(smoke.capture_depth_phase(folder, 'equal', time.time() - 10))
            for part in ('source', 'destination'):
                self.assertEqual((folder / ('equal.presentation.' + part + '.ppm')).read_bytes(), data)

    def test_partial_stale_or_missing_pair_is_not_preserved(self):
        complete = b'P6\n1 1\n255\n\xff\x20\x00'
        for problem in ('partial', 'stale', 'missing', 'bad-header'):
            with self.subTest(problem=problem), tempfile.TemporaryDirectory() as directory:
                folder = pathlib.Path(directory)
                started = time.time() - 10
                (folder / 'presentation.source.ppm').write_bytes(complete)
                destination = folder / 'presentation.destination.ppm'
                if problem != 'missing':
                    destination.write_bytes(complete[:-1] if problem == 'partial' else
                                            b'P6\nnot a size\n255\n' if problem == 'bad-header' else complete)
                if problem == 'stale':
                    os.utime(destination, (started, started))
                self.assertFalse(smoke.capture_depth_phase(folder, 'equal', started))
                self.assertEqual(list(folder.glob('equal.*')), [])


if __name__ == '__main__':
    unittest.main()
