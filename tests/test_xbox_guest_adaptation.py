"""Asset-free checks for the small private guest adaptation boundary."""
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts/xbox'))
import guest_adaptation as adapter


class GuestAdaptationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        self.source = self.root / adapter.RENDERER
        self.source.parent.mkdir(parents=True)
        self.original = b'/* synthetic fixture */\n' + adapter.ANCHOR
        self.source.write_bytes(self.original)
        self.addCleanup(patch.stopall)
        patch.object(adapter, 'SOURCE_SHA256', hashlib.sha256(self.original).hexdigest()).start()
        self.identity = adapter.identity('render-scale-v1')

    def test_restore_after_success_and_force_next_rebuild(self):
        before = self.source.stat().st_mtime_ns
        with adapter.renderer_adaptation(self.root, self.identity):
            self.assertIn(adapter.INSERT, self.source.read_bytes())
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertGreater(self.source.stat().st_mtime_ns, before)

    def test_restore_after_build_failure_and_interrupt(self):
        for exception in (subprocess.CalledProcessError(1, 'ninja'), KeyboardInterrupt()):
            with self.assertRaises(type(exception)):
                with adapter.renderer_adaptation(self.root, self.identity):
                    raise exception
            self.assertEqual(self.source.read_bytes(), self.original)

    def test_preserve_concurrent_edits(self):
        with self.assertRaisesRegex(RuntimeError, 'preserving'):
            with adapter.renderer_adaptation(self.root, self.identity):
                self.source.write_bytes(b'unrelated new edit')
        self.assertEqual(self.source.read_bytes(), b'unrelated new edit')

    def test_changed_upstream_refuses_without_mutation(self):
        changed = self.original + b'new upstream changes'
        self.source.write_bytes(changed)
        with self.assertRaisesRegex(ValueError, 'input changed'):
            with adapter.renderer_adaptation(self.root, self.identity):
                self.fail('must not run build')
        self.assertEqual(self.source.read_bytes(), changed)

    def test_missing_or_ambiguous_anchor_refuses_even_with_valid_hash(self):
        for original in (b'no anchor', adapter.ANCHOR * 2):
            with patch.object(adapter, 'SOURCE_SHA256', hashlib.sha256(original).hexdigest()):
                with self.assertRaisesRegex(ValueError, 'input changed'):
                    adapter.adapted_source(original)

    def test_default_does_not_write_renderer(self):
        before = self.source.stat().st_mtime_ns
        with adapter.renderer_adaptation(self.root, adapter.identity('none')):
            self.assertEqual(self.source.read_bytes(), self.original)
        self.assertEqual(self.source.stat().st_mtime_ns, before)

    def test_unknown_option_refuses(self):
        with self.assertRaisesRegex(ValueError, 'Unknown'):
            adapter.identity('render-scale-v2')

    def test_failed_build_does_not_publish_new_identity(self):
        out = self.root / 'out'
        out.mkdir()
        manifest = out / 'guest-adaptation.json'
        manifest.write_text(json.dumps({'name': 'previous'}))
        with patch.dict('os.environ', {'HALOPAD_XBOX_GUEST_ADAPTATION': 'render-scale-v1'}), \
             patch.object(adapter.subprocess, 'check_output', return_value=''), \
             patch.object(adapter.subprocess, 'run', side_effect=[None, subprocess.CalledProcessError(1, 'ninja')]):
            with self.assertRaises(subprocess.CalledProcessError):
                adapter.build(self.root, 'ndk', 'compiler', out)
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertEqual(json.loads(manifest.read_text()), {'name': 'previous'})


class SaveIdentityTests(unittest.TestCase):
    def test_exact_guest_and_legacy_transition(self):
        with tempfile.TemporaryDirectory() as folder:
            executable = pathlib.Path(folder) / 'save-identity'
            subprocess.run(['xcrun', 'clang', '-x', 'objective-c', '-fobjc-arc',
                            '-framework', 'Foundation', '-I', str(ROOT / 'port/ios'),
                            '-o', str(executable), '-'], input='''
#import "HaloPadXboxSaveIdentity.h"
#include <assert.h>
int main(void) { @autoreleasepool {
    NSString *a = [@"a" stringByPaddingToLength:64 withString:@"a" startingAtIndex:0];
    NSString *b = [@"b" stringByPaddingToLength:64 withString:@"b" startingAtIndex:0];
    NSString *old = HPXboxSaveIdentity(@{@"revision": @"same-pin", @"guest_sha256": a});
    assert([old isEqualToString:HPXboxSaveIdentity(@{@"revision": @"same-pin", @"guest_sha256": a})]);
    assert(![old isEqualToString:HPXboxSaveIdentity(@{@"revision": @"same-pin", @"guest_sha256": b})]);
    assert(![old isEqualToString:HPXboxSaveIdentity(@{@"revision": @"new-pin", @"guest_sha256": a})]);
    assert(![old isEqualToString:@"same-pin"]); // Legacy markers trigger one backup.
    assert(HPXboxSaveIdentity(@{}) == nil);
    assert(HPXboxSaveIdentity(@{@"revision": @1, @"guest_sha256": a}) == nil);
    assert(HPXboxSaveIdentity(@{@"revision": @"same-pin", @"guest_sha256": @"bad"}) == nil);
} return 0; }
''', text=True, check=True, capture_output=True)
            subprocess.run([str(executable)], check=True)


if __name__ == '__main__':
    unittest.main()
