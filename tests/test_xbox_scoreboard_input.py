"""Roster paging queue: real helper, no guest/UI acceptance claim."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ScoreboardInputTests(unittest.TestCase):
    def test_paging_pairs_cancel_and_bounds(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'scoreboard'
            subprocess.run(['clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                            '-fsanitize=address,undefined', '-I', str(ROOT / 'port/xbox'),
                            '-o', str(binary)], input=r'''
#include <assert.h>
#include "xg_scoreboard_input.h"
int main(void) {
    struct xg_scoreboard_input s = {0}; int down = 0;
    xg_scoreboard_drag(&s, 39.5f);
    xg_scoreboard_drag(&s, 40.0f);
    assert(!xg_scoreboard_next(&s, &down));
    xg_scoreboard_drag(&s, .5f);
    assert(xg_scoreboard_next(&s, &down) == 1 && down);
    assert(xg_scoreboard_next(&s, &down) == 1 && !down);
    assert(!xg_scoreboard_next(&s, &down));
    xg_scoreboard_drag(&s, -160);
    assert(xg_scoreboard_next(&s, &down) == -1 && down);
    xg_scoreboard_clear(&s);
    assert(xg_scoreboard_next(&s, &down) == -1 && !down);
    assert(!xg_scoreboard_next(&s, &down)); /* second down canceled */
    xg_scoreboard_drag(&s, 79);
    xg_scoreboard_clear(&s);
    xg_scoreboard_drag(&s, 1);
    assert(!xg_scoreboard_next(&s, &down)); /* no stale fraction */
    xg_scoreboard_drag(&s, NAN); xg_scoreboard_drag(&s, INFINITY);
    assert(s.points == 1 && !s.pages);
    xg_scoreboard_drag(&s, 1e30f);
    xg_scoreboard_drag(&s, 1e30f);
    for (int i = 0; i < 8; i++) {
        assert(xg_scoreboard_next(&s, &down) == 1 && down);
        assert(xg_scoreboard_next(&s, &down) == 1 && !down);
    }
    assert(!xg_scoreboard_next(&s, &down));
    xg_scoreboard_drag(&s, 80); xg_scoreboard_drag(&s, -80);
    assert(!xg_scoreboard_next(&s, &down));
    return 0;
}
''', text=True, capture_output=True, check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
