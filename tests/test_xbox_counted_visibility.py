"""Candidate generation is exact-input, opt-in, and never edits ANGLE source."""
import hashlib
import importlib.util
import json
import pathlib
import tempfile
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('counted', ROOT / 'scripts/xbox/angle/counted_visibility.py')
counted = importlib.util.module_from_spec(spec)
spec.loader.exec_module(counted)


class CountedVisibilityTests(unittest.TestCase):
    def fixtures(self):
        return {
            'ContextMtl.mm': b'setVisibilityResultMode(MTLVisibilityResultModeBoolean, resultOffset)\n' * 2,
            'DisplayMtl.mm': b'#include "libANGLE/renderer/metal/shaders/mtl_internal_shaders_src_autogen.h"\n',
            'shaders/mtl_internal_shaders_src_autogen.h': counted.OLD_SUM.encode(),
        }

    def test_generation_leaves_sources_untouched_and_records_exact_outputs(self):
        data = self.fixtures()
        with tempfile.TemporaryDirectory() as tmp:
            source, output = pathlib.Path(tmp) / 'source', pathlib.Path(tmp) / 'output'
            for name, value in data.items():
                path = source / counted.BASE / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(value)
            hashes = {name: hashlib.sha256(value).hexdigest() for name, value in data.items()}
            with patch.object(counted, 'INPUTS', hashes):
                counted.generate(source, output)
            self.assertEqual((output / 'ContextMtl.mm').read_text().count('ModeCounting'), 2)
            self.assertIn('"counted_shaders.h"', (output / 'DisplayMtl.mm').read_text())
            self.assertIn(counted.NEW_SUM, (output / 'counted_shaders.h').read_text())
            identity = json.loads((output / 'identity.json').read_text())
            self.assertEqual(identity['inputs'], hashes)
            for name, value in data.items():
                self.assertEqual((source / counted.BASE / name).read_bytes(), value)
                target = 'counted_shaders.h' if name.endswith('.h') else name
                self.assertEqual(identity['outputs'][name], hashlib.sha256((output / target).read_bytes()).hexdigest())

    def test_unknown_inputs_and_duplicate_anchors_fail_closed(self):
        for name, value in self.fixtures().items():
            with self.subTest(name=name):
                with self.assertRaisesRegex(ValueError, 'input changed'):
                    counted.adapt(name, value)
                bad = value + value
                with patch.object(counted, 'INPUTS', {name: hashlib.sha256(bad).hexdigest()}):
                    with self.assertRaisesRegex(ValueError, 'Expected'):
                        counted.adapt(name, bad)

    def test_normal_ios_builder_resets_cached_candidate_option(self):
        self.assertIn('-DHALOPAD_ANGLE_COUNTED_VISIBILITY=OFF',
                      (ROOT / 'scripts/xbox/build-ios.sh').read_text())


if __name__ == '__main__':
    unittest.main()
