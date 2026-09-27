"""Compiled objects for the translated DLLs of a VA-model run (va/<module>/<module>.va.ll).

Shared by scripts/run-core.py and scripts/run-slices.py: dispatch.ll names every
module procedure, so every VA link includes these objects.
"""
import pathlib
import os
import subprocess


def module_objects(va: pathlib.Path, target: str, outdir: pathlib.Path):
    """Compile (when stale) and return ([object paths], [.ll paths])."""
    objs, lls = [], []
    for d in sorted(p for p in va.iterdir() if p.is_dir()):
        ll = d / f'{d.name}.va.ll'
        if not ll.exists():
            continue
        obj = outdir / f'{d.name}.va.o'
        if not obj.exists() or obj.stat().st_mtime < ll.stat().st_mtime:
            print(f'compiling translated {d.name}...', flush=True)
            sdk = os.environ.get('SDKROOT') or (subprocess.run(['xcrun', '--sdk', 'iphonesimulator' if 'simulator' in target else 'iphoneos',
                                                                  '--show-sdk-path'], check=True, capture_output=True, text=True).stdout.strip()
                                                   if 'ios' in target else None)
            subprocess.run(['clang', '-target', target, *(['-isysroot', sdk] if sdk else []), '-c', '-O1', '-fno-fast-math', '-ffp-contract=off',
                            '-Wno-override-module', str(ll), '-o', str(obj)], check=True)
        objs.append(obj)
        lls.append(ll)
    return objs, lls
