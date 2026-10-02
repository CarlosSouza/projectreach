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

    def test_quality_recipe_has_separate_identity_and_checks_both_anchors(self):
        self.assertNotEqual(adapter.identity('render-scale-v1'), adapter.identity('render-quality-v1'))
        for suffix in (b'', adapter.FILTER_ANCHOR * 2):
            original = self.original + suffix
            with patch.object(adapter, 'SOURCE_SHA256', hashlib.sha256(original).hexdigest()):
                with self.assertRaisesRegex(ValueError, 'filtering input changed'):
                    adapter.adapted_source(original, 'render-quality-v1')
        original = self.original + adapter.FILTER_ANCHOR
        with patch.object(adapter, 'SOURCE_SHA256', hashlib.sha256(original).hexdigest()):
            modified = adapter.adapted_source(original, 'render-quality-v1')
        self.assertIn(adapter.INSERT, modified)
        self.assertIn(adapter.FILTER_INSERT, modified)

    def test_filter_fragment_exclusions_cap_and_explicit_game_request(self):
        # Compile the exact original fragment inserted by the adapter, with
        # inert GL boundaries. Each process gets fresh one-time configuration.
        source = '''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef int GLint;
#define D3DTEXF_POINT 1
#define D3DTEXF_ANISOTROPIC 3
#define D3DTEXF_NONE 0
#define D3DTSS_MAXANISOTROPY 0
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84fe
static int maximum, queries, calls;
static float observed;
static void glGetIntegerv(int name, int *value) { assert(name == 0x84ff); queries++; *value = maximum; }
static void glSamplerParameterf(int sampler, int name, float value) { assert(sampler == 7 && name == 0x84fe); calls++; observed = value; }
static void platform_log(const char *format, ...) { (void)format; }
int main(int argc, char **argv) {
    assert(argc == 11);
    setenv("HALO_TEST_ANISOTROPY", argv[1], 1);
    maximum = atoi(argv[2]);
    struct { int anisotropy; } xgpu_capabilities = {atoi(argv[3])};
    int hires=atoi(argv[4]), mipmapped=atoi(argv[5]), min_filter=atoi(argv[6]);
    int mip_filter=atoi(argv[7]), state[1]={atoi(argv[8])}, sampler=7;
''' + adapter.FILTER_INSERT.decode() + '''
    assert(calls == (atoi(argv[9]) > 0));
    assert(observed == atoi(argv[9]));
    assert(queries == atoi(argv[10]));
    return 0;
}
'''
        executable = self.root / 'filter-test'
        subprocess.run(['clang', '-x', 'c', '-Wall', '-Wextra', '-Werror', '-o', str(executable), '-'],
                       input=source, text=True, capture_output=True, check=True)
        # request, cap, extension, hires, mipmapped, min, mip, game's AF, applied, queries
        cases = [('4',16,1,0,1,2,2,1,4,1), ('16',8,1,0,1,2,2,1,8,1),
                 ('16',0,1,0,1,2,2,1,0,1), ('1',16,1,0,1,2,2,1,0,1),
                 ('invalid',16,1,0,1,2,2,1,0,1), ('4x',16,1,0,1,2,2,1,0,1),
                 ('16',16,0,0,1,2,2,1,0,0), ('16',16,1,1,1,2,2,1,0,0),
                 ('16',16,1,0,0,2,2,1,0,0), ('16',16,1,0,1,1,2,1,0,0),
                 ('16',16,1,0,1,2,0,1,0,0), ('4',16,1,0,1,3,2,16,0,1),
                 ('16',16,1,0,1,3,2,4,16,1)]
        for case in cases:
            with self.subTest(case=case):
                subprocess.run([str(executable), *map(str, case)], check=True)

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
