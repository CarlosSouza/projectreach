#!/usr/bin/env python3
"""Local current-tree guard, not a complete publication/history audit."""
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PROTECTED = ('ref/', 'generated/', 'docs/artifacts/', '.venv/', 'build/')
SUFFIXES = {'.exe', '.dll', '.xbe', '.map', '.iso', '.dmg', '.ipa', '.dmp', '.pcap', '.pcapng', '.p12', '.pfx', '.mobileprovision'}

def forbidden(name):
    p = pathlib.PurePosixPath(name)
    return (name.startswith(PROTECTED) or name == 'INPUTS.lock.json' or name.lower().endswith('.halopad.zip') or
            p.suffix.lower() in SUFFIXES or '.app' in [pathlib.PurePosixPath(x).suffix for x in p.parts] or
            p.name == '.env' or p.name.startswith('.env.'))

def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args])

def main():
    errors = []
    tracked = [x.decode() for x in git('ls-files', '-z').split(b'\0') if x]
    untracked = [x.decode() for x in git('ls-files', '--others', '--exclude-standard', '-z').split(b'\0') if x]
    for name in sorted(set(tracked + untracked)):
        if forbidden(name):
            errors.append('protected payload is tracked or unignored: ' + name)
            continue
        # Check the index too, so staging a binary then changing the worktree cannot hide it.
        contents = []
        if name in tracked:
            contents.append(git('show', ':' + name))
        path = ROOT / name
        if path.is_symlink():
            errors.append('review symlink before commit: ' + name)
        elif path.is_file():
            contents.append(path.read_bytes())
        for data in contents:
            if data[:2] == b'MZ' or data[:4] in (b'\x7fELF', b'\xcf\xfa\xed\xfe', b'\xca\xfe\xba\xbe'):
                errors.append('executable content outside private paths: ' + name)
                break
            if b'-----BEGIN ' + b'PRIVATE KEY-----' in data or b'-----BEGIN RSA ' + b'PRIVATE KEY-----' in data:
                errors.append('private-key marker: ' + name)
                break
    probes = ['ref/probe.txt', 'generated/probe.c', 'docs/artifacts/probe.txt', 'INPUTS.lock.json', '.venv/probe']
    for probe in probes:
        if subprocess.run(['git', '-C', str(ROOT), 'check-ignore', '--no-index', '-q', probe]).returncode:
            errors.append('missing ignore protection: ' + probe)
    for error in sorted(set(errors)):
        print('FAIL:', error, file=sys.stderr)
    if not errors:
        print('PASS: current tree/index path and executable guards; private paths ignored.')
        print('Publication NOT authorized. History, provenance, arbitrary secrets and package audit remain separate gates.')
    return bool(errors)

if __name__ == '__main__':
    sys.exit(main())
