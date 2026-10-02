import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts/xbox'))
from mip_capture import inspect_mips


class MipCaptureTests(unittest.TestCase):
    def fixture(self, root, mutate=None):
        for frame in (120, 180):
            folder = root / f'frame-{frame}'
            folder.mkdir()
            row = {'complete': True, 'state_restored': True, 'prior_gl_error': 0,
                   'gl_error': 0, 'texture': 4, 'presented_frames': frame,
                   'base_level': 0, 'maximum_level': 2, 'levels': []}
            for i, size in enumerate((4, 2, 1)):
                name = f'level-{i}.rgba'
                (folder / name).write_bytes(bytes([frame, 0, 0, 255]) * size * size)
                row['levels'].append({'level': i, 'pixels': {'complete': True,
                    'gl_error': 0, 'framebuffer_status': 0x8cd5, 'width': size,
                    'height': size, 'file': name}})
            if mutate and frame == 180:
                mutate(row, folder)
            (folder / 'mips.json').write_text(json.dumps(row))

    def test_complete_chain_and_rgb_change(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.fixture(pathlib.Path(tmp))
            result = inspect_mips(tmp)
            self.assertEqual(result['frames'], [120, 180])
            self.assertEqual([x['changed_rgb_bytes'] for x in result['levels']], [16, 4, 1])

    def test_fail_closed(self):
        mutations = [
            lambda r, p: r.update(state_restored=False),
            lambda r, p: r.update(prior_gl_error=0x502),
            lambda r, p: r.update(texture=9),
            lambda r, p: r.update(presented_frames=121),
            lambda r, p: r['levels'].pop(),
            lambda r, p: r['levels'][1]['pixels'].update(width=3),
            lambda r, p: r['levels'][1]['pixels'].update(file='../escape'),
            lambda r, p: (p / 'level-1.rgba').write_bytes(b'bad'),
        ]
        for mutation in mutations:
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as tmp:
                self.fixture(pathlib.Path(tmp), mutation)
                with self.assertRaises(ValueError):
                    inspect_mips(tmp)

    def test_missing_snapshots(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError):
                inspect_mips(tmp)
