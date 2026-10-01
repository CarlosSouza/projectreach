"""Presentation diagnostics must bracket the real guest blit, not replace it."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class XboxGLGeneratorTests(unittest.TestCase):
    def test_blit_coordinates_and_widened_stack_arguments(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = pathlib.Path(tmp) / 'guest.c'
            output = pathlib.Path(tmp) / 'host.c'
            source.write_text(
                'static void guest_glBlitFramebuffer(GLint x0, GLint y0, GLint x1, GLint y1, '
                'GLint x2, GLint y2, GLint x3, GLint y3, GLbitfield mask, GLenum filter)\n'
                'void hostgl_glBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, long long, long long);\n')
            subprocess.run([sys.executable, str(ROOT / 'scripts/xbox/gen-host-gl.py'),
                            str(source), str(output)], check=True, capture_output=True)
            generated = output.read_text()
            before = generated.index('xg_gl_trace_blit(0, a0, a1, a2, a3);')
            call = generated.index('p_glBlitFramebuffer(a0, a1, a2, a3, a4, a5, a6, a7, (GLbitfield)a8, (GLenum)a9);')
            after = generated.index('xg_gl_trace_blit(1, a4, a5, a6, a7);')
            self.assertLess(before, call)
            self.assertLess(call, after)


if __name__ == '__main__':
    unittest.main()
