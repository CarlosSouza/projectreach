"""Exercise real Git indexes in disposable repos; never stage project inputs."""
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class RepoSafety(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        (self.root / 'scripts').mkdir()
        shutil.copy(ROOT / 'scripts/check-repo-safety.py', self.root / 'scripts')
        shutil.copy(ROOT / '.gitignore', self.root)
        self.git('init', '-q')
    def git(self, *args):
        subprocess.run(['git', '-C', str(self.root), *args], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    def guard(self):
        return subprocess.run([sys.executable, str(self.root/'scripts/check-repo-safety.py')], stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode
    def test_ignored_input_allowed(self):
        (self.root/'ref').mkdir()
        (self.root/'ref/game.exe').write_bytes(b'MZ synthetic fixture')
        self.assertEqual(self.guard(), 0)
    def test_forced_staging_private_input_fails(self):
        (self.root/'ref').mkdir()
        (self.root/'ref/game.exe').write_bytes(b'MZ synthetic fixture')
        self.git('add', '-f', 'ref/game.exe')
        self.assertNotEqual(self.guard(), 0)
    def test_disguised_binary_in_index_fails_after_worktree_changes(self):
        (self.root/'innocent.txt').write_bytes(b'MZ synthetic fixture')
        self.git('add', 'innocent.txt')
        (self.root/'innocent.txt').write_text('ordinary text')
        self.assertNotEqual(self.guard(), 0)
    def test_untracked_disguised_binary_fails(self):
        (self.root/'innocent.txt').write_bytes(b'MZ synthetic fixture')
        self.assertNotEqual(self.guard(), 0)
