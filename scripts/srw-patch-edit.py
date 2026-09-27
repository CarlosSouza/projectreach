#!/usr/bin/env python3
"""Maintain port/patches/srw-macos-llasm.patch one file at a time.

Applies literal (old -> new) replacements to the current patched copy of one SRW
source file from a tool build, then regenerates that file's section of the patch
as a unified diff against the pinned upstream source in ref/sr/SRW.

Usage: srw-patch-edit.py --build <tool-build> --file SR_main.c --edits edits.json
edits.json: [["old text", "new text"], ...]  (each old text must occur exactly once)
"""
import argparse
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PATCH = ROOT / 'port/patches/srw-macos-llasm.patch'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=pathlib.Path, required=True)
    ap.add_argument('--file', required=True)
    ap.add_argument('--edits', type=pathlib.Path, required=True)
    a = ap.parse_args()
    src = (a.build / 'SRW' / a.file).read_text()
    for old, new in json.loads(a.edits.read_text()):
        if src.count(old) != 1:
            sys.exit(f'FAIL: edit anchor occurs {src.count(old)} times in {a.file}: {old[:60]!r}')
        src = src.replace(old, new)
    tmp = ROOT / 'generated' / 'patchwork' / (a.file + '.new')
    tmp.parent.mkdir(parents=True, exist_ok=True)
    tmp.write_text(src)
    diff = subprocess.run(['diff', '-u', '--label', 'a/' + a.file, '--label', 'b/' + a.file,
                           str(ROOT / 'ref/sr/SRW' / a.file), str(tmp)], capture_output=True, text=True).stdout
    lines = PATCH.read_text().rstrip('\n').split('\n')
    header = '--- a/' + a.file
    if header in lines:
        s = lines.index(header)
        e = next((i for i in range(s + 1, len(lines)) if lines[i].startswith('--- a/')), len(lines))
        lines = lines[:s] + diff.rstrip('\n').split('\n') + lines[e:]
    else:
        lines += diff.rstrip('\n').split('\n')
    PATCH.write_text('\n'.join(lines) + '\n')
    print(diff)


if __name__ == '__main__':
    main()

