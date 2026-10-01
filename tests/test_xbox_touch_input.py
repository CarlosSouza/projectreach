"""Exercise the actual native touch buffer without UIKit or game inputs."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class XboxTouchInputTests(unittest.TestCase):
    def test_short_gestures_release_and_cancel(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'touch-test'
            subprocess.run(['clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                            '-I', str(ROOT / 'port/xbox'), '-o', str(binary)],
                           input=r'''
#include <assert.h>
#include "xg_touch_input.h"
int main(void) {
    struct xg_touch_input input = {0};
    struct xg_touch_pad state = {0}, neutral = {0};
    state.buttons = 1u | (1u << 6);
    state.axes[1] = 1; state.axes[2] = -.8f; state.axes[5] = 1;
    xg_touch_publish(&input, &state);
    xg_touch_publish(&input, &neutral); /* released before guest frame */
    assert(xg_touch_axis(&input, 1) == 1);
    assert(xg_touch_axis(&input, 1) == 0);
    assert(xg_touch_axis(&input, 2) == -.8f); /* other axis not consumed */
    assert(xg_touch_axis(&input, 5) == 1); /* quick trigger tap */
    assert(xg_touch_axis(&input, 5) == 0);
    assert(xg_touch_button(&input, 0) == 1);
    assert(xg_touch_button(&input, 0) == 0);
    assert(xg_touch_button(&input, 6) == 1);
    xg_touch_publish(&input, &state);
    assert(xg_touch_axis(&input, 1) == 1);
    assert(xg_touch_axis(&input, 1) == 1); /* held input stays live */
    assert(xg_touch_button(&input, 0) == 1);
    assert(xg_touch_button(&input, 0) == 1);
    state.axes[1] = -.3f;
    xg_touch_publish(&input, &state);
    xg_touch_publish(&input, &neutral);
    assert(xg_touch_axis(&input, 1) == -.3f); /* newest direction */
    assert(xg_touch_stronger(.9f, -.3f) == .9f); /* hardware merge */
    assert(xg_touch_stronger(.2f, -.3f) == -.3f);
    xg_touch_publish(&input, &state);
    xg_touch_clear(&input);
    for (int axis = -1; axis <= 6; axis++) assert(xg_touch_axis(&input, axis) == 0);
    for (int button = -1; button <= 32; button++) assert(xg_touch_button(&input, button) == 0);
    return 0;
}
''', text=True, check=True, capture_output=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
