"""Cold FBO creation must not replace the presentation destination."""
import hashlib
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts/xbox'))
import guest_adaptation as guest


class PresentOrderTests(unittest.TestCase):
    def test_unique_guard_and_cumulative_identity(self):
        base = b'\n'.join((guest.ANCHOR, guest.FILTER_ANCHOR, guest.COUNT_ANCHOR,
            guest.ATOMIC_ANCHOR, guest.WATER_SAVE_ANCHOR, guest.WATER_RESTORE_ANCHOR,
            *(a for a, _ in guest.border_sampling.RENDERER_EDITS)))
        for count in (0, 1, 2):
            original = base + guest.PRESENT_ANCHOR * count
            with patch.dict('os.environ', {'XBOX_REV': 'synthetic-fixture'}), \
                 patch.object(guest, 'SOURCE_SHA256', hashlib.sha256(original).hexdigest()):
                if count != 1:
                    with self.assertRaisesRegex(ValueError, 'presentation input changed'):
                        guest.adapted_source(original, 'render-present-v1')
                    continue
                previous = guest.adapted_source(original, 'shared-input-v1')
                fixed = guest.adapted_source(original, 'render-present-v1')
                self.assertEqual(fixed, previous.replace(guest.PRESENT_ANCHOR, guest.PRESENT_REPLACE))
                self.assertEqual(fixed.count(guest.PRESENT_READ), 1)
        self.assertIn('render-present-v1', guest.INPUT_ADAPTATIONS)
        self.assertIn('render-present-v1', guest.COUNTED_ADAPTATIONS)
        self.assertNotEqual(guest.identity('render-present-v1'), guest.identity('shared-input-v1'))

    def test_actual_replacement_orders_cold_and_cached_targets(self):
        source = r'''
#include <assert.h>
enum { GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER,
       GL_SCISSOR_TEST, GL_COLOR_BUFFER_BIT, GL_TRUE };
static int read_fb, draw_fb, cleared, cached;
static struct { struct { unsigned texture; } target; } buffer;
static void glBindFramebuffer(int target, unsigned id) {
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) read_fb = id;
    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) draw_fb = id;
}
static unsigned framebuffer_get(unsigned color, unsigned depth) {
    assert(color == 23 && depth == 0);
    if (!cached) { glBindFramebuffer(GL_FRAMEBUFFER, 7); cached = 1; }
    return 7;
}
static void glDisable(int value) { assert(value == GL_SCISSOR_TEST); }
static void glColorMask(int r, int g, int b, int a) {
    assert(r == GL_TRUE && g == GL_TRUE && b == GL_TRUE && a == GL_TRUE);
}
static void glClearColor(float r, float g, float b, float a) {
    assert(r == 0 && g == 0 && b == 0 && a == 1);
}
static void glClear(int mask) { assert(mask == GL_COLOR_BUFFER_BIT); cleared = draw_fb; }
static void before(void) {
    __typeof__(buffer) *back_buffer = &buffer;
''' + guest.PRESENT_ANCHOR.decode() + r'''
}
static void after(void) {
    __typeof__(buffer) *back_buffer = &buffer;
''' + guest.PRESENT_REPLACE.decode() + r'''
}
int main(void) {
    buffer.target.texture = 23;
    for (int warm = 0; warm < 2; warm++) {
        cached = warm; read_fb = 41; draw_fb = 42; before();
        assert(read_fb == 7 && draw_fb == (warm ? 0 : 7) && cleared == 0);
        cached = warm; read_fb = 41; draw_fb = 42; after();
        assert(read_fb == 7 && draw_fb == 0 && cleared == 0);
        after(); assert(read_fb == 7 && draw_fb == 0 && cleared == 0);
    }
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'present'
            build = subprocess.run(['clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                '-fsanitize=address,undefined', '-o', str(binary)], input=source,
                text=True, capture_output=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            subprocess.run([str(binary)], check=True, capture_output=True)


if __name__ == '__main__':
    unittest.main()
