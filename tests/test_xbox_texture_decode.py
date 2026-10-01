"""Asset-free independent texture-oracle fixtures, not gameplay acceptance."""
import pathlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts/xbox'))
from texture_decode import cache_symbol, morton_index, decode_rgb565, compare_texture, validate_cache_revision


class TextureDecodeTests(unittest.TestCase):
    def test_unreviewed_cache_revision_rejected(self):
        validate_cache_revision('bfbac35761335c28aac7a47bf0c578ea37764810')
        for revision in ('', 'bfbac357', 'f8937c6179757774c75f4e7d36de446fabd3dcc8'):
            with self.assertRaisesRegex(ValueError, 'reviewed texture-cache ABI'):
                validate_cache_revision(revision)

    def test_cache_symbol_is_unambiguous_data_address(self):
        self.assertEqual(cache_symbol('0000000088c88010 b texture_buckets\n'), 0x88c88010)
        for symbols in ('', '1234 b texture_buckets', '88c88010 T texture_buckets',
                        '88c88010 b texture_buckets\n88c88010 b texture_buckets', '88c88011 b texture_buckets'):
            with self.assertRaises(ValueError):
                cache_symbol(symbols)

    def test_rectangular_known_layout(self):
        # xemu's documented 8x32 layout has masks x=0x15 and y=0xea.
        self.assertEqual(morton_index(7, 0, 8, 32), 0x15)
        self.assertEqual(morton_index(0, 31, 8, 32), 0xea)
        self.assertEqual(morton_index(7, 31, 8, 32), 255)
        # 4x2: row 0 is 0,1,4,5; row 1 is 2,3,6,7.
        self.assertEqual([morton_index(x, y, 4, 2) for y in range(2) for x in range(4)],
                         [0, 1, 4, 5, 2, 3, 6, 7])

    def test_rectangular_and_degenerate_addresses_are_bijective(self):
        for w, h in ((256, 128), (128, 256), (1, 32), (32, 1), (1, 1)):
            self.assertEqual({morton_index(x, y, w, h) for y in range(h) for x in range(w)}, set(range(w*h)))

    def test_rgb565_primary_colors_and_swizzled_order(self):
        raw = struct.pack('<8H', 0xf800, 0x07e0, 0x001f, 0xffff, 0, 0xffff, 0xf800, 0x001f)
        colors = decode_rgb565(raw, 4, 2)
        expected = [(0, 0, 255, 255), (0, 255, 0, 255), (0, 0, 0, 255), (255, 255, 255, 255),
                    (255, 0, 0, 255), (255, 255, 255, 255), (0, 0, 255, 255), (255, 0, 0, 255)]
        self.assertEqual(colors, b''.join(bytes(value) for value in expected))

    def test_linear_pitch_skips_padding(self):
        raw = struct.pack('<4H', 0xf800, 0x5555, 0x07e0, 0x5555)
        self.assertEqual(decode_rgb565(raw, 1, 2, linear=True, pitch=4), bytes((0, 0, 255, 255, 0, 255, 0, 255)))

    def test_invalid_layout_rejected(self):
        for raw, w, h, linear, pitch in ((b'', 0, 1, False, 0), (b'\0'*6, 3, 1, False, 6),
                                        (b'\0', 1, 1, False, 2), (b'\0'*4, 1, 1, False, 4),
                                        (b'\0'*2, 2, 1, True, 2)):
            with self.assertRaises(ValueError):
                decode_rgb565(raw, w, h, linear, pitch)

    def test_comparison_reports_mismatch_and_rejects_incomplete_sources(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory)
            (folder / 'raw.xbox').write_bytes(b'\0\0')
            (folder / 'gpu.rgba').write_bytes(bytes((0, 0, 0, 255)))
            source = dict(supported=True, complete=True, format=5, width=1, height=1, linear=False,
                          pitch=2, length=2, file='raw.xbox')
            image = dict(width=1, height=1, file='gpu.rgba', xbox_source=source)
            self.assertTrue(compare_texture(folder, image)['identical'])
            (folder / 'gpu.rgba').write_bytes(bytes((255, 0, 0, 255)))
            self.assertEqual(compare_texture(folder, image)['changed_pixels'], 1)
            for changes in (dict(complete=False), dict(width=2), dict(file='../raw.xbox'), dict(length=3)):
                original = source.copy()
                source.update(changes)
                with self.assertRaises(ValueError):
                    compare_texture(folder, image)
                source.clear()
                source.update(original)
