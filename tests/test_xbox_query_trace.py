"""Run the actual bounded observer with inert GL and a controlled clock."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class XboxQueryTraceTests(unittest.TestCase):
    def test_actual_observer_is_opt_in_bounded_and_preserves_gl(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'query-trace'
            build = subprocess.run(['clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                            '-fsanitize=address,undefined',
                            '-I', str(ROOT / 'port/xbox'), '-o', str(binary)], input=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
void xg_log(const char *format, ...);
enum { GL_ANY_SAMPLES_PASSED = 0x8c2f, GL_QUERY_RESULT = 0x8866, GL_QUERY_RESULT_AVAILABLE = 0x8867 };
static time_t tick = 100;
static int clock_calls, logs;
static char last_log[512];
static time_t test_clock(time_t *out) {
    clock_calls++; if (out) *out = tick; return tick;
}
#define time test_clock
#include "xg_query_trace.h"
#undef time
#define xg_gl_trace_query xg_query_trace
void xg_log(const char *format, ...) {
    va_list args; va_start(args, format);
    vsnprintf(last_log, sizeof(last_log), format, args);
    va_end(args); logs++;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    if (strcmp(argv[1], "unset")) setenv("XG_TRACE_QUERIES", argv[1], 1);
    else unsetenv("XG_TRACE_QUERIES");
    xg_gl_trace_query(1, 7, GL_ANY_SAMPLES_PASSED, 0);
    xg_gl_trace_query(0, 7, GL_QUERY_RESULT_AVAILABLE, 0);
    xg_gl_trace_query(0, 7, GL_QUERY_RESULT_AVAILABLE, 1);
    xg_gl_trace_query(0, 7, GL_QUERY_RESULT, 0);
    xg_gl_trace_query(0, 7, GL_QUERY_RESULT, 1);
    xg_gl_trace_query(0, 7, GL_QUERY_RESULT, 9);
    assert(!logs);
    tick++;
    xg_gl_trace_query(1, 8, GL_ANY_SAMPLES_PASSED, 0);
    if (strcmp(argv[1], "1")) { assert(!logs && !clock_calls); return 0; }
    assert(logs == 1);
    assert(strstr(last_log, "begins 2 ready 1 pending 1 result-zero 1 result-one 1 result-other 1"));
    tick++;
    xg_gl_trace_query(0, 8, GL_QUERY_RESULT, 1);
    assert(logs == 2);
    assert(strstr(last_log, "begins 0 ready 0 pending 0 result-zero 0 result-one 1 result-other 0"));
    for (int i = 0; i < 200; i++) {
        tick++; xg_gl_trace_query(0, 8, GL_QUERY_RESULT, 0);
    }
    assert(logs == 180);
    int previous = clock_calls;
    tick++; xg_gl_trace_query(0, 8, GL_QUERY_RESULT, 1);
    assert(clock_calls == previous && logs == 180);
    return 0;
}
''', text=True, capture_output=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            for setting in ('unset', '', '0', 'true', '1'):
                with self.subTest(setting=setting):
                    subprocess.run([str(binary), setting], check=True, capture_output=True)


if __name__ == '__main__':
    unittest.main()
