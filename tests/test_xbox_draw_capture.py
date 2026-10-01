"""Asset-free fixtures for private draw snapshot completeness and byte identity."""
import json
import pathlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts/xbox'))
from draw_capture import load_draw, position_bytes, compare_clip_positions


class DrawCaptureTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.folder = pathlib.Path(self.directory.name)
        self.vertices = struct.pack('<9f', 0, 0, 0, 1, 0, 0, 0, 1, 0)
        (self.folder / 'vertices.bin').write_bytes(self.vertices)
        (self.folder / 'elements.bin').write_bytes(struct.pack('<3H', 0, 1, 2))
        for name in ('vertex.glsl', 'fragment.glsl'):
            (self.folder / name).write_text('fixture shader source')
        self.draw = {'complete': True, 'gl_error': 0, 'count': 3, 'index_offset': 0,
                     'index_type': 0x1403, 'element_buffer': 2,
                     'buffers': {'1': {'file': 'vertices.bin', 'size': 36},
                                 '2': {'file': 'elements.bin', 'size': 6}},
                     'attributes': [{'enabled': False}] * 16}
        self.draw['attributes'][0] = {'enabled': True, 'buffer': 1, 'type': 0x1406,
                                      'size': 3, 'stride': 12, 'offset': 0}

    def load(self):
        (self.folder / 'draw.json').write_text(json.dumps(self.draw))
        return load_draw(self.folder)

    def test_exact_indexed_positions(self):
        self.assertEqual(b''.join(position_bytes(self.load())), self.vertices)

    def test_incomplete_or_error_capture_rejected(self):
        for complete, error in ((False, 0), (True, 0x502)):
            self.draw.update(complete=complete, gl_error=error)
            with self.assertRaises(ValueError):
                self.load()

    def test_truncated_buffer_rejected(self):
        (self.folder / 'vertices.bin').write_bytes(b'short')
        with self.assertRaises(ValueError):
            self.load()

    def test_index_range_rejected(self):
        self.draw['index_offset'] = 4
        with self.assertRaises(ValueError):
            self.load()

    def test_missing_attribute_buffer_rejected(self):
        self.draw['attributes'][0]['buffer'] = 99
        with self.assertRaises(ValueError):
            self.load()

    def test_outside_path_rejected(self):
        self.draw['buffers']['1']['file'] = '../vertices.bin'
        with self.assertRaises(ValueError):
            self.load()

    def test_position_range_rejected(self):
        self.draw['attributes'][0]['offset'] = 36
        with self.assertRaises(ValueError):
            position_bytes(self.load())

    def test_identical_clip_positions(self):
        positions = struct.pack('<4f', 1, 2, 3, 4)
        for label in ('base', 'equal'):
            (self.folder / (label + '-position.bin')).write_bytes(positions)
        self.assertEqual(compare_clip_positions(self.folder, 1), {
            'vertices': 1, 'identical': True, 'changed_vertices': 0,
            'max_component_delta': 0, 'max_ndc_depth_delta': 0})

    def test_changed_clip_positions(self):
        (self.folder / 'base-position.bin').write_bytes(struct.pack('<4f', 1, 2, 3, 4))
        (self.folder / 'equal-position.bin').write_bytes(struct.pack('<4f', 1, 2, 4, 4))
        result = compare_clip_positions(self.folder, 1)
        self.assertFalse(result['identical'])
        self.assertEqual(result['changed_vertices'], 1)
        self.assertEqual(result['max_component_delta'], 1)
        self.assertEqual(result['max_ndc_depth_delta'], 0.25)

    def test_incomplete_clip_output_rejected(self):
        for label in ('base', 'equal'):
            (self.folder / (label + '-position.bin')).write_bytes(b'short')
        with self.assertRaisesRegex(ValueError, 'Incomplete'):
            compare_clip_positions(self.folder, 1)

    def test_nonfinite_clip_output_rejected(self):
        for label in ('base', 'equal'):
            (self.folder / (label + '-position.bin')).write_bytes(struct.pack('<4f', 1, 2, float('nan'), 4))
        with self.assertRaisesRegex(ValueError, 'Nonfinite'):
            compare_clip_positions(self.folder, 1)


if __name__ == '__main__':
    unittest.main()
