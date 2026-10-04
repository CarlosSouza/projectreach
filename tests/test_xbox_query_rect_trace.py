"""Compile the actual CPU-only rectangle observer; no private guest inputs."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class QueryRectangleTests(unittest.TestCase):
    def test_observer_opt_in_association_rejections_and_cap(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'rects'
            result = subprocess.run([
                'clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                '-fsanitize=address,undefined', '-I', str(ROOT / 'port/xbox'),
                '-o', str(binary)], input=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
static time_t tick = 100;
static int logs;
static char message[512];
static time_t test_clock(time_t *out) { if (out) *out = tick; return tick; }
static void xg_log(const char *format, ...) {
    va_list args; va_start(args, format);
    vsnprintf(message, sizeof(message), format, args); va_end(args); logs++;
}
#define time test_clock
#include "xg_query_rect_trace.h"
#undef time
static float vertices[4][64];
static void quad(unsigned id) {
    xg_query_rect_begin(0x8c2f, id);
    xg_query_rect_upload(0x8892, sizeof(vertices), vertices);
    xg_query_rect_draw(6, 0, 4);
    xg_query_rect_end(0x8c2f);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    if (strcmp(argv[1], "unset")) setenv("XG_TRACE_QUERY_RECTS", argv[1], 1);
    else unsetenv("XG_TRACE_QUERY_RECTS");
    for (int i = 0; i < 4; i++) {
        vertices[i][0] = i == 0 || i == 3 ? -5 : 15;
        vertices[i][1] = i < 2 ? 10 : 20;
        vertices[i][2] = 0.9f; vertices[i][3] = 1;
    }
    quad(7); xg_query_rect_result(7, 0x8866, 1);
    if (strcmp(argv[1], "1")) { assert(logs == 0); return 0; }
    assert(logs == 1 && strstr(message, "id 7 rect -5 10 15 20") &&
           strstr(message, "area 200 result 1"));
    xg_query_rect_result(7, 0x8866, 1); assert(logs == 1);
    tick++; xg_query_rect_result(7, 0x8867, 1); assert(logs == 1);
    xg_query_rect_result(8, 0x8866, 1); assert(logs == 1);
    xg_query_rect_result(7, 0x8866, 0); assert(logs == 2);
    quad(263);
    xg_query_rect_result(263, 0x8866, 1); assert(logs == 2);
    tick++;
    xg_query_rect_result(7, 0x8866, 1); assert(logs == 2);
    xg_query_rect_result(263, 0x8866, 0); assert(logs == 3);
    for (int failure = 0; failure < 9; failure++) {
        tick++; xg_query_rect_begin(0x8c2f, 263);
        float saved = vertices[2][0];
        if (failure == 0) vertices[2][0] = NAN;
        if (failure == 1) vertices[2][0] = 17;
        if (failure == 2) vertices[2][3] = 2;
        xg_query_rect_upload(0x8892, failure == 3 ? 1023 : sizeof(vertices), vertices);
        if (failure == 4) xg_query_rect_upload(0x8892, sizeof(vertices), vertices);
        if (failure == 5) xg_query_rect_draw(6, 0, 4);
        xg_query_rect_draw(failure == 6 ? 4 : 6, failure == 7 ? 1 : 0, failure == 8 ? 3 : 4);
        xg_query_rect_end(0x8c2f);
        xg_query_rect_result(263, 0x8866, 1); assert(logs == 3);
        vertices[2][0] = saved; vertices[2][3] = 1;
    }
    for (int i = 0; i < 4200; i++) {
        tick++; quad(7); xg_query_rect_result(7, 0x8866, 1);
    }
    assert(logs == 4096 && !xg_query_rect_enabled());
    return 0;
}
''', text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            for value in ('unset', '', '0', 'true', '1'):
                with self.subTest(value=value):
                    subprocess.run([str(binary), value], check=True, capture_output=True)


if __name__ == '__main__':
    unittest.main()
