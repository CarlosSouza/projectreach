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
    def test_renderer_must_match_manifest(self):
        apple = 'OpenGL ES 3.0 APPLE on Apple Software Renderer'
        angle = 'OpenGL ES 3.0 ANGLE on ANGLE Metal Renderer: Apple iOS simulator GPU'
        self.assertTrue(smoke.renderer_matches(apple, 'apple-gles'))
        self.assertTrue(smoke.renderer_matches(angle, 'angle-metal'))
        for text, renderer in [(apple, 'angle-metal'), (angle, 'apple-gles'),
                               ('OpenGL ES 3.0 ANGLE Vulkan', 'angle-metal'),
                               ('', 'angle-metal'), (apple, 'unknown')]:
            with self.subTest(text=text, renderer=renderer):
                self.assertFalse(smoke.renderer_matches(text, renderer))

    def test_default_combat_unchanged(self):
        self.assertEqual(smoke.match_environment(), {
            'HALO_NETWORK_TEST': 'host:bloodgulch', 'HALO_NETWORK_TEST_START': '8',
            'HALO_TEST_INPUT': 'bot:7', 'HALO_NETWORK_TEST_SHOOT': '4'})

    def test_campaign_init_and_gate_use_requested_map(self):
        for map_name in ('a10', 'a30'):
            with self.subTest(map_name=map_name):
                self.assertEqual(smoke.campaign_init(map_name),
                                 'map_name levels\\' + map_name + '\\' + map_name + '\n')
                self.assertTrue(smoke.campaign_load_requested(
                    "starting precaching of map '" + map_name + "'", map_name))
                self.assertFalse(smoke.campaign_load_requested("starting precaching of map 'ui'", map_name))
        self.assertFalse(smoke.campaign_load_requested("starting precaching of map 'a10'", 'a30'))

    def test_normal_campaign_clears_scripted_input_and_network_test(self):
        settings = smoke.campaign_environment()
        self.assertEqual(settings['HALO_TEST_INPUT'], '')
        self.assertEqual(settings['HALO_NETWORK_TEST'], '')
        self.assertEqual(settings['HALO_NETWORK_TEST_START'], '')
        self.assertEqual(settings['HALO_NETWORK_TEST_SHOOT'], '0')
        self.assertEqual(smoke.campaign_environment(True)['HALO_TEST_INPUT'], 'bot:7')

    def test_scripted_campaign_requires_explicit_diagnostic_case(self):
        for args in ([], ['--case', 'campaign'], ['--render-diagnostics'],
                     ['--case', 'match', '--render-diagnostics']):
            with self.subTest(args=args):
                result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                         '--scripted-campaign', *args], capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertIn('--scripted-campaign requires', result.stderr)

    def test_later_campaign_map_requires_targeted_case(self):
        for args in ([], ['--case', 'menu'], ['--case', 'match']):
            with self.subTest(args=args):
                result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                         '--campaign-map', 'a30', *args], capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertIn('--campaign-map a30 requires --case campaign', result.stderr)

    def test_unknown_campaign_map_rejected_before_launch(self):
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--case', 'campaign', '--campaign-map', '../outside'],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn('invalid choice', result.stderr)

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

    def test_missing_replay_rejected_before_simulator_tools(self):
        with tempfile.TemporaryDirectory() as directory:
            env = dict(os.environ, XG_DRAW_REPLAY=directory)
            env.pop('XG_DEPTH_COMPARE', None)
            result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                     '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('Invalid replay input:', result.stderr)

    def test_raster_requires_replay_before_simulator_tools(self):
        env = dict(os.environ, XG_DRAW_RASTER='1')
        env.pop('XG_DRAW_REPLAY', None)
        env.pop('XG_DEPTH_COMPARE', None)
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_DRAW_RASTER requires XG_DRAW_REPLAY', result.stderr)

    def test_unknown_raster_attachment_rejected_before_launch(self):
        env = dict(os.environ, XG_DRAW_RASTER='typo')
        env.pop('XG_DEPTH_COMPARE', None)
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_DRAW_RASTER must be', result.stderr)

    def test_capture_minimum_requires_valid_bounds_and_shader_sources(self):
        for minimum in ('typo', '2', '100001', '1000'):
            env = dict(os.environ, XG_CAPTURE_MIN_INDICES=minimum)
            for key in ('XG_DEPTH_COMPARE', 'XG_CAPTURE_SHADER_DIR'):
                env.pop(key, None)
            result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                     '--render-diagnostics'], capture_output=True, text=True, env=env)
            self.assertEqual(result.returncode, 2)
            self.assertIn('XG_CAPTURE_MIN_INDICES', result.stderr)

    def test_texture_readback_requires_shader_sources_before_launch(self):
        env = dict(os.environ, XG_CAPTURE_TEXTURES='1')
        for key in ('XG_DEPTH_COMPARE', 'XG_CAPTURE_SHADER_DIR', 'XG_CAPTURE_MIN_INDICES'):
            env.pop(key, None)
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_CAPTURE_TEXTURES requires XG_CAPTURE_SHADER_DIR', result.stderr)

    def test_xbox_texture_source_requires_pixels_before_launch(self):
        env = dict(os.environ, XG_CAPTURE_XBOX_TEXTURES='1')
        for key in ('XG_DEPTH_COMPARE', 'XG_CAPTURE_SHADER_DIR', 'XG_CAPTURE_MIN_INDICES', 'XG_CAPTURE_TEXTURES'):
            env.pop(key, None)
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_CAPTURE_XBOX_TEXTURES requires', result.stderr)

    def test_live_depth_requires_shader_capture_before_launch(self):
        env = dict(os.environ, XG_CAPTURE_DEPTH='1')
        for key in ('XG_DEPTH_COMPARE', 'XG_CAPTURE_SHADER_DIR', 'XG_CAPTURE_MIN_INDICES', 'XG_CAPTURE_TEXTURES', 'XG_CAPTURE_XBOX_TEXTURES'):
            env.pop(key, None)
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_CAPTURE_DEPTH requires', result.stderr)

    def test_paired_depth_requires_stationary_match(self):
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--case', 'match', '--render-diagnostics'],
                                capture_output=True, text=True,
                                env=dict(os.environ, XG_DEPTH_COMPARE='paired'))
        self.assertEqual(result.returncode, 2)
        self.assertIn('paired depth requires --stationary-match', result.stderr)

    def test_native_pixels_require_live_depth_before_launch(self):
        env = {key: value for key, value in os.environ.items() if not key.startswith('XG_')}
        env['XG_CAPTURE_NATIVE_PIXELS'] = '1'
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_CAPTURE_NATIVE_PIXELS requires XG_CAPTURE_DEPTH', result.stderr)

    def test_base_skip_validated_before_launch(self):
        for skip, message in (('-1', 'must be 0..64'), ('65', 'must be 0..64'),
                              ('no', 'must be 0..64'), ('1', 'requires XG_CAPTURE_SHADER_DIR')):
            env = {key: value for key, value in os.environ.items() if not key.startswith('XG_')}
            env['XG_CAPTURE_BASE_SKIP'] = skip
            result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                     '--render-diagnostics'], capture_output=True, text=True, env=env)
            self.assertEqual(result.returncode, 2)
            self.assertIn('XG_CAPTURE_BASE_SKIP ' + message, result.stderr)

    def test_equal_shader_validated_before_launch(self):
        for shader, message in (('../outside.glsl', 'must be vs007_0.glsl or vs041_0.glsl'),
                                 ('vs007_0.glsl', 'requires XG_CAPTURE_SHADER_DIR')):
            env = {key: value for key, value in os.environ.items() if not key.startswith('XG_')}
            env['XG_CAPTURE_EQUAL_SHADER'] = shader
            result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                     '--render-diagnostics'], capture_output=True, text=True, env=env)
            self.assertEqual(result.returncode, 2)
            self.assertIn('XG_CAPTURE_EQUAL_SHADER ' + message, result.stderr)

    def test_color_trace_frame_validated_before_launch(self):
        for frame, message in (('-1', 'must be 0..10000'), ('10001', 'must be 0..10000'),
                               ('bad', 'must be 0..10000'), ('120', 'requires XG_CAPTURE_SHADER_DIR')):
            env = {key: value for key, value in os.environ.items() if not key.startswith('XG_')}
            env['XG_TRACE_COLOR_FRAME'] = frame
            result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                     '--render-diagnostics'], capture_output=True, text=True, env=env)
            self.assertEqual(result.returncode, 2)
            self.assertIn('XG_TRACE_COLOR_FRAME ' + message, result.stderr)

    def test_material_capture_requires_color_trace_and_textures(self):
        env = {key: value for key, value in os.environ.items() if not key.startswith('XG_')}
        env['XG_TRACE_MATERIALS'] = '1'
        result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                 '--render-diagnostics'], capture_output=True, text=True, env=env)
        self.assertEqual(result.returncode, 2)
        self.assertIn('XG_TRACE_MATERIALS requires', result.stderr)

    def test_index_count_validated_before_launch(self):
        for count, message in [('2', 'must be 3..100000'), ('100001', 'must be 3..100000'),
                               ('bad', 'must be 3..100000'), ('402', 'requires XG_CAPTURE_SHADER_DIR')]:
            env = {key: value for key, value in os.environ.items() if not key.startswith('XG_')}
            env['XG_CAPTURE_INDEX_COUNT'] = count
            result = subprocess.run([sys.executable, str(SCRIPT), '--device', 'unused',
                                     '--render-diagnostics'], capture_output=True, text=True, env=env)
            self.assertEqual(result.returncode, 2)
            self.assertIn('XG_CAPTURE_INDEX_COUNT ' + message, result.stderr)

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
