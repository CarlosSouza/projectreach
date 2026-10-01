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
