"""Exercise touch-binding ownership and rollback without game data or a Simulator."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class TouchBindingPolicy(unittest.TestCase):
    def test_profile_slot_and_rollback_transitions(self):
        with tempfile.TemporaryDirectory(prefix='halopad-touch-policy-') as work:
            exe = pathlib.Path(work) / 'policy'
            build = subprocess.run([
                'clang', '-std=c2x', '-DPTROFS_64BIT=1', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                '-I', str(ROOT / 'port/llasm-support'),
                str(ROOT / 'tests/halo_touch_binding_policy_test.c'),
                str(ROOT / 'port/runtime/halopad_touch_binding.c'), '-o', str(exe),
            ], capture_output=True, text=True, timeout=30)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('25 assertions, 0 failures', result.stdout)


if __name__ == '__main__':
    unittest.main()
