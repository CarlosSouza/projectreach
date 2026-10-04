"""Keyboard binding names for the iOS host (upstream build 85 imports)."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class XboxScancodeNameTests(unittest.TestCase):
    def test_names_round_trip_and_unknowns_match_sdl(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'scancode-test'
            subprocess.run(['clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                            '-fsanitize=address,undefined', '-I', str(ROOT / 'port/xbox'),
                            '-o', str(binary)], input=r'''
#include <assert.h>
#include "xg_scancode_names.h"
int main(void) {
    for (size_t i = 0; i < sizeof xg_scancode_names / sizeof *xg_scancode_names; i++)
        assert(xg_scancode_from_name(xg_scancode_names[i].name) == xg_scancode_names[i].code);
    assert(!strcmp(xg_scancode_name(4), "A") && !strcmp(xg_scancode_name(44), "Space"));
    assert(xg_scancode_from_name("left shift") == 225);
    assert(!*xg_scancode_name(0) && !*xg_scancode_name(300));
    assert(xg_scancode_from_name("") == 0 && xg_scancode_from_name(0) == 0 && xg_scancode_from_name("nope") == 0);
    return 0;
}
''', text=True, check=True, capture_output=True)
            subprocess.run([str(binary)], check=True, capture_output=True)

    def test_both_hosts_implement_the_imports(self):
        for name in ('xg_ios.m', 'xg_sdl.c'):
            source = (ROOT / 'port/xbox' / name).read_text()
            self.assertIn('xh_host_sdl_scancode_name(', source)
            self.assertIn('xh_host_sdl_scancode_from_name(', source)


if __name__ == '__main__':
    unittest.main()
