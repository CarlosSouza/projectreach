"""Asset-free depth readback validation, not game/driver acceptance."""
import json
import pathlib
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts/xbox'))
from draw_capture import read_depth, compare_depth_snapshots, compare_native_pixels, validate_raster_input


class DepthCaptureTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.folder = pathlib.Path(self.directory.name)
        self.record = dict(complete=True, calibrated=True, gl_error=0, restore_error=0, framebuffer_status=0x8cd5,
                           encoding='normalized-float32-le', width=640, height=480, texture=2,
                           level=0, framebuffer=3, file='depth.bin', presented_frames=20)
        self.blank = struct.pack('<f', 1) * (640 * 480)
        for label in ('base', 'equal'):
            (self.folder / label).mkdir()
            for phase in ('before', 'after'):
                self.write(label, phase, self.blank)

    def write(self, label, phase, data, **changes):
        record = dict(self.record, file='depth-' + phase + '.bin', **changes)
        folder = self.folder / label
        (folder / ('depth-' + phase + '.json')).write_text(json.dumps(record))
        (folder / record['file']).write_bytes(data)

    def responsive(self):
        pixels = struct.pack('<f', .5) + self.blank[4:]
        for label, phase in (('base', 'after'), ('equal', 'before'), ('equal', 'after')):
            self.write(label, phase, pixels)
        return pixels

    def test_responsive_unchanged_intervening_depth(self):
        self.responsive()
        result = compare_depth_snapshots(self.folder)
        self.assertEqual(result['base_changed'], 1)
        self.assertEqual(result['intervening_changed'], 0)
        self.assertEqual(result['equal_changed'], 0)
        self.assertTrue(result['after_base_equals_before_equal'])

    def test_intervening_write_is_measured_not_suppressed(self):
        self.responsive()
        different = struct.pack('<f', .25) + self.blank[4:]
        self.write('equal', 'before', different)
        self.write('equal', 'after', different)
        self.assertEqual(compare_depth_snapshots(self.folder)['intervening_changed'], 1)
        self.assertEqual(compare_depth_snapshots(self.folder)['base_overwritten_closer'], 1)
        self.assertEqual(compare_depth_snapshots(self.folder)['base_overwritten_farther'], 0)

    def test_presentation_boundary_is_rejected(self):
        self.responsive()
        self.write('equal', 'after', self.blank, presented_frames=21)
        with self.assertRaisesRegex(ValueError, 'presentation boundary'):
            compare_depth_snapshots(self.folder)

    def test_no_base_response_is_inconclusive(self):
        with self.assertRaisesRegex(ValueError, 'no measurable'):
            compare_depth_snapshots(self.folder)

    def test_target_change_is_rejected(self):
        self.responsive()
        self.write('equal', 'after', self.blank, texture=7)
        with self.assertRaisesRegex(ValueError, 'target changed'):
            compare_depth_snapshots(self.folder)

    def test_errors_encoding_and_layout_rejected(self):
        for change in (dict(complete=False), dict(calibrated=False), dict(gl_error=0x502), dict(restore_error=0x502),
                       dict(framebuffer_status=0), dict(width=128), dict(encoding='unknown'), dict(level=1)):
            self.write('base', 'before', self.blank, **change)
            with self.assertRaisesRegex(ValueError, 'Incomplete or unsupported'):
                read_depth(self.folder / 'base', 'before')

    def test_nonfinite_or_outside_depth_rejected(self):
        for value in (float('nan'), float('inf'), -1, 2):
            self.write('base', 'before', struct.pack('<f', value) + self.blank[4:])
            with self.assertRaisesRegex(ValueError, 'Invalid normalized'):
                read_depth(self.folder / 'base', 'before')

    def test_scaled_depth_requires_verified_extent_and_preserves_all_pixels(self):
        self.record.update(capture_schema=2, width=1280, height=960, texture_width=1280, texture_height=960)
        self.blank = struct.pack('<f', 1) * (1280 * 960)
        for label in ('base', 'equal'):
            for phase in ('before', 'after'):
                self.write(label, phase, self.blank)
        self.responsive()
        result = compare_depth_snapshots(self.folder)
        self.assertEqual((result['width'], result['height'], result['pixels']), (1280, 960, 1228800))
        self.assertEqual(result['base_changed'], 1)
        self.assertEqual(result['equal_changed'], 0)

    def test_extent_mismatch_or_missing_is_rejected(self):
        for width, height in ((640, 479), (1280, 960), (None, 480), (640.0, 480)):
            self.write('base', 'before', self.blank, capture_schema=2, texture_width=width, texture_height=height)
            with self.assertRaisesRegex(ValueError, 'extent'):
                read_depth(self.folder / 'base', 'before')

    def test_unverified_scaled_or_unknown_schema_is_rejected(self):
        for changes in (dict(width=1280, height=960), dict(capture_schema=3), dict(capture_schema=2.0)):
            self.write('base', 'before', self.blank, **changes)
            with self.assertRaisesRegex(ValueError, 'schema'):
                read_depth(self.folder / 'base', 'before')

    def test_dimension_bounds_and_types_are_rejected(self):
        for changes in (dict(width=True), dict(width=640.0), dict(width=4097), dict(height=0), dict(height=None)):
            self.write('base', 'before', self.blank, **changes)
            with self.assertRaisesRegex(ValueError, 'Incomplete or unsupported'):
                read_depth(self.folder / 'base', 'before')

    def test_same_pixel_count_different_extent_is_a_target_change(self):
        self.responsive()
        self.write('equal', 'after', self.blank, capture_schema=2, width=1280, height=240,
                   texture_width=1280, texture_height=240)
        with self.assertRaisesRegex(ValueError, 'target changed'):
            compare_depth_snapshots(self.folder)

    def test_scaled_draw_viewport_must_match_and_legacy_replay_stays_strict(self):
        draw = dict(viewport_bits=list(struct.unpack('<4I', struct.pack('<4f', 0, 0, 1280, 960))),
                    depth_range_bits=[0, 0x3f800000])
        validate_raster_input(draw, 1280, 960)
        with self.assertRaisesRegex(ValueError, '640x480'):
            validate_raster_input(draw)
        with self.assertRaisesRegex(ValueError, '1280x480'):
            validate_raster_input(draw, 1280, 480)

    def test_truncated_or_outside_file_rejected(self):
        self.write('base', 'before', b'short')
        with self.assertRaisesRegex(ValueError, 'Truncated'):
            read_depth(self.folder / 'base', 'before')
        record = dict(self.record, file='../outside.bin')
        (self.folder / 'base/depth-before.json').write_text(json.dumps(record))
        with self.assertRaisesRegex(ValueError, 'Depth path'):
            read_depth(self.folder / 'base', 'before')


class NativePixelTests(unittest.TestCase):
    write = DepthCaptureTests.write
    responsive = DepthCaptureTests.responsive

    def setUp(self):
        DepthCaptureTests.setUp(self)
        self.responsive()
        self.write('equal', 'native-copy', struct.pack('<f', .5) + self.blank[4:])
        self.native = dict(complete=True, color_copy_equal=True, framebuffer_status=0x8cd5,
                           gl_error=0, restore_error=0, framebuffer=3, depth_texture=2,
                           program=7, vao=8, presented_frames=20, width=640, height=480,
                           depth_bits=24, stencil_bits=8)
        self.live = dict(complete=True, gl_error=0, framebuffer=3, presented_frames=20)
        self.pixels = b'\x00' * (640 * 480 * 4)
        for label in ('before', 'equal', 'repeat', 'live'):
            self.image(label, self.pixels)
        self.image('always', b'\xff\x00\x00\xff' + self.pixels[4:])
        self.records()
        self.addCleanup(mock.patch.stopall)
        mock.patch('draw_capture.compare_live_depth', return_value={'presented_frames': 20}).start()
        mock.patch('draw_capture.load_draw', return_value=dict(program=7, presented_frames=20,
                                                              framebuffer={'draw': 3})).start()

    def records(self):
        for name, record in (('native-pixels', self.native), ('native-live', self.live)):
            (self.folder / 'equal' / (name + '.json')).write_text(json.dumps(record))

    def image(self, label, data):
        (self.folder / 'equal' / ('native-' + label + '.rgba')).write_bytes(data)

    def test_native_response_with_unchanged_base_depth(self):
        result = compare_native_pixels(self.folder)
        self.assertEqual(result['equal_always_color_different'], 1)
        self.assertEqual(result['different_at_base_depth_unchanged'], 1)
        self.assertEqual(result['different_at_base_later_closer'], 0)
        self.assertEqual(result['always_only_at_base_depth_unchanged'], 1)
        self.assertTrue(result['live_equal'])

    def test_native_difference_at_later_closer_depth(self):
        self.write('equal', 'before', struct.pack('<f', .25) + self.blank[4:])
        self.write('equal', 'native-copy', struct.pack('<f', .25) + self.blank[4:])
        result = compare_native_pixels(self.folder)
        self.assertEqual(result['different_at_base_later_closer'], 1)
        self.assertEqual(result['different_at_base_depth_unchanged'], 0)
        self.assertEqual(result['always_only_at_base_later_closer'], 1)

    def test_color_difference_is_not_a_missing_response(self):
        red = b'\x80\x00\x00\xff' + self.pixels[4:]
        for label in ('equal', 'repeat', 'live'):
            self.image(label, red)
        result = compare_native_pixels(self.folder)
        self.assertEqual(result['equal_always_color_different'], 1)
        self.assertEqual(result['always_only_color_response'], 0)

    def test_native_depth_copy_mismatch_rejected(self):
        self.write('equal', 'native-copy', self.blank)
        with self.assertRaisesRegex(ValueError, 'cloned depth'):
            compare_native_pixels(self.folder)

    def test_native_repeat_and_live_mismatch_rejected(self):
        for label in ('repeat', 'live'):
            self.image(label, b'\xff' * len(self.pixels))
            with self.assertRaisesRegex(ValueError, 'repeat/live'):
                compare_native_pixels(self.folder)
            self.image(label, self.pixels)

    def test_native_empty_response_and_truncation_rejected(self):
        self.image('always', self.pixels)
        with self.assertRaisesRegex(ValueError, 'no measurable'):
            compare_native_pixels(self.folder)
        self.image('always', b'short')
        with self.assertRaisesRegex(ValueError, 'Truncated'):
            compare_native_pixels(self.folder)

    def test_native_metadata_fails_closed(self):
        for key, value in (('complete', False), ('color_copy_equal', False), ('gl_error', 0x502),
                           ('restore_error', 0x502), ('depth_bits', 32), ('stencil_bits', 0)):
            old = self.native[key]
            self.native[key] = value
            self.records()
            with self.assertRaisesRegex(ValueError, 'Incomplete or unsupported'):
                compare_native_pixels(self.folder)
            self.native[key] = old

    def test_native_frame_and_target_rejected(self):
        for key, value, message in (('program', 99, 'program/target'), ('depth_texture', 9, 'program/target'),
                                     ('presented_frames', 21, 'presentation boundary')):
            old = self.native[key]
            self.native[key] = value
            self.records()
            with self.assertRaisesRegex(ValueError, message):
                compare_native_pixels(self.folder)
            self.native[key] = old
