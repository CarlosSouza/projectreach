"""Synthetic timeline validation, not visual/game acceptance."""
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts/xbox'))
from draw_capture import compare_color_trace


class ColorTraceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = pathlib.Path(self.temp.name)
        self.record = dict(complete=True, gl_error=0, framebuffer=3, program=7,
                           depth_function=0x202, mode=4, count=144, width=640, height=480,
                           presented_frames=120)
        self.blank = b'\x00' * (640 * 480 * 4)
        for phase in ('before', 'after'):
            self.write(phase)
        (self.folder / '0000-before.rgba').write_bytes(self.blank)
        (self.folder / '0000-after.rgba').write_bytes(b'\xff\x00\x00\xff' + self.blank[4:])
        for stage in ('vertex', 'fragment'):
            (self.folder / f'program-7-{stage}.glsl').write_text('void main() {}')

    def write(self, phase, **changes):
        (self.folder / ('0000-' + phase + '.json')).write_text(json.dumps(dict(self.record, **changes)))

    def test_color_response_counted(self):
        result = compare_color_trace(self.folder, 120)
        self.assertEqual(result['draws'][0]['changed'], 1)
        self.assertEqual(result['draws'][0]['program'], 7)

    def test_incomplete_or_cross_frame_rejected(self):
        for change in (dict(complete=False), dict(gl_error=0x502), dict(presented_frames=121), dict(width=10)):
            self.write('after', **change)
            with self.assertRaisesRegex(ValueError, 'Incomplete or crossed-frame'):
                compare_color_trace(self.folder, 120)

    def test_state_change_rejected(self):
        self.write('after', depth_function=0x207)
        with self.assertRaisesRegex(ValueError, 'draw state changed'):
            compare_color_trace(self.folder, 120)

    def test_invalid_identifier_rejected(self):
        self.write('after', program='../outside')
        with self.assertRaisesRegex(ValueError, 'identifiers'):
            compare_color_trace(self.folder, 120)

    def test_truncated_image_rejected(self):
        (self.folder / '0000-after.rgba').write_bytes(b'short')
        with self.assertRaisesRegex(ValueError, 'Truncated'):
            compare_color_trace(self.folder, 120)

    def test_overflow_rejected(self):
        (self.folder / 'overflow.txt').write_text('Too many draws')
        with self.assertRaisesRegex(ValueError, 'excessive'):
            compare_color_trace(self.folder, 120)

    def material_fixture(self):
        self.record['material_captured'] = True
        self.material = dict(complete=True, gl_error=0, program=7, presented_frames=120,
                            active_texture=0x84c0, uniforms={'alpha_reference': [0],
                            'texture_scale[0]': [0x3f800000] * 4,
                            'texture_scale[1]': [0x3f800000] * 4, 'texture_lod_bias': [0] * 4},
                            textures=[dict(stage=i, unit=i, object=i + 1,
                            level0=dict(complete=True, gl_error=0, framebuffer_status=0x8cd5,
                                        width=1, height=1, file=f'texture-{i + 1}.rgba')) for i in range(2)])
        for phase in ('before', 'after'):
            self.write(phase)
            path = self.folder / ('0000-' + phase + '-material')
            path.mkdir()
            (path / 'material.json').write_text(json.dumps(self.material))
            for texture in self.material['textures']:
                (path / texture['level0']['file']).write_bytes(b'\x10\x20\x30\xff')

    def write_material_after(self):
        (self.folder / '0000-after-material/material.json').write_text(json.dumps(self.material))

    def test_exact_material_pair(self):
        self.material_fixture()
        row = compare_color_trace(self.folder, 120, require_materials=True)['draws'][0]
        self.assertTrue(row['material']['textures_unchanged'])

    def test_requested_material_missing_rejected(self):
        with self.assertRaisesRegex(ValueError, 'No matching material'):
            compare_color_trace(self.folder, 120, require_materials=True)

    def test_material_frame_or_error_rejected(self):
        self.material_fixture()
        for name, value in [('presented_frames', 121), ('gl_error', 0x502), ('complete', False)]:
            old = self.material[name]
            self.material[name] = value
            self.write_material_after()
            with self.assertRaisesRegex(ValueError, 'Incomplete or crossed-frame material'):
                compare_color_trace(self.folder, 120)
            self.material[name] = old

    def test_material_uniform_change_rejected(self):
        self.material_fixture()
        self.material['uniforms']['alpha_reference'] = [0x3f800000]
        self.write_material_after()
        with self.assertRaisesRegex(ValueError, 'Material state changed'):
            compare_color_trace(self.folder, 120)

    def test_material_nonfinite_uniform_rejected(self):
        self.material_fixture()
        self.material['uniforms']['alpha_reference'] = [0x7f800000]
        self.write_material_after()
        with self.assertRaisesRegex(ValueError, 'Invalid material uniform'):
            compare_color_trace(self.folder, 120)

    def test_material_texture_change_or_truncation_rejected(self):
        self.material_fixture()
        path = self.folder / '0000-after-material/texture-1.rgba'
        path.write_bytes(b'\xff' * 4)
        with self.assertRaisesRegex(ValueError, 'Material texture changed'):
            compare_color_trace(self.folder, 120)
        path.write_bytes(b'short')
        with self.assertRaisesRegex(ValueError, 'Truncated texture'):
            compare_color_trace(self.folder, 120)

    def test_material_missing_sampler_or_uniform_rejected(self):
        self.material_fixture()
        self.material['textures'][1]['unit'] = 99
        self.write_material_after()
        with self.assertRaisesRegex(ValueError, 'Invalid material texture binding'):
            compare_color_trace(self.folder, 120)
        self.material['textures'][1]['unit'] = 1
        del self.material['uniforms']['alpha_reference']
        self.write_material_after()
        with self.assertRaisesRegex(ValueError, 'Missing material uniform'):
            compare_color_trace(self.folder, 120)
