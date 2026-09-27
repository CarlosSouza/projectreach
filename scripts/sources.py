#!/usr/bin/env python3
"""Fetch explicit source pins, refusing to modify pre-existing checkouts."""
import argparse
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DISABLED = 'disabled://halopad-reference-push'

def git(path, *args):
    return subprocess.check_output(['git', '-C', str(path), *args], text=True).rstrip('\n')

def verify(path, rev):
    if path.is_symlink() or not (path / '.git').is_dir():
        raise RuntimeError('reference is missing, symlinked, or not a standalone clone')
    if git(path, 'rev-parse', 'HEAD') != rev:
        raise RuntimeError('revision mismatch; existing checkout left unchanged')
    if git(path, 'status', '--porcelain', '--untracked-files=all'):
        raise RuntimeError('dirty reference; existing work left unchanged')
    repos = [path]
    for line in git(path, 'submodule', 'status', '--recursive').splitlines():
        if not line.startswith(' '):
            raise RuntimeError('uninitialized or mismatched submodule')
        repos.append(path / line.split()[1])
    for repo in repos:
        for remote in git(repo, 'remote').splitlines():
            if git(repo, 'remote', 'get-url', '--push', '--all', remote) != DISABLED:
                raise RuntimeError('reference push not disabled: ' + str(repo.relative_to(ROOT)))

def bootstrap(source):
    path = ROOT / source['path']
    if path.exists() or path.is_symlink():
        verify(path, source['revision'])
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['git', 'init', str(path)], check=True)
    subprocess.run(['git', '-C', str(path), 'remote', 'add', 'origin', source['url']], check=True)
    subprocess.run(['git', '-C', str(path), 'config', 'remote.origin.pushurl', DISABLED], check=True)
    subprocess.run(['git', '-C', str(path), 'fetch', '--depth=1', 'origin', source['revision']], check=True)
    subprocess.run(['git', '-C', str(path), 'checkout', '--detach', source['revision']], check=True)
    subprocess.run(['git', '-C', str(path), 'submodule', 'update', '--init', '--recursive'], check=True)
    # Git submodule paths originate from the pinned source graph, never shell expansion.
    for line in git(path, 'submodule', 'status', '--recursive').splitlines():
        repo = path / line.split()[1]
        for remote in git(repo, 'remote').splitlines():
            subprocess.run(['git', '-C', str(repo), 'config', '--replace-all', 'remote.' + remote + '.pushurl', DISABLED], check=True)
    verify(path, source['revision'])

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['bootstrap', 'verify'])
    args = parser.parse_args()
    failures = []
    for source in json.loads((ROOT / 'dependencies.lock.json').read_text())['sources']:
        try:
            if args.mode == 'bootstrap':
                bootstrap(source)
            verify(ROOT / source['path'], source['revision'])
            print('PASS', source['name'], source['revision'], flush=True)
        except (RuntimeError, subprocess.CalledProcessError) as exc:
            failures.append(source['name'])
            print('FAIL', source['name'], str(exc), file=sys.stderr, flush=True)
    return bool(failures)

if __name__ == '__main__':
    sys.exit(main())
