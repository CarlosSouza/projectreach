#!/usr/bin/env python3
"""G3: build and run HaloPad's native core from Halo's PE entry point.

Links the VA-model translation (scripts/va-model.py) with guest memory, dispatch, the
main-thread environment and the Windows services implemented so far, then runs it with
the prepared image and game files. The run stops at the first trap and reports it;
evidence goes to docs/artifacts/<date>/G3/core-<target>-<stamp>/.

Usage: .venv/bin/python scripts/run-core.py [--work RUN_DIR] [--target TRIPLE] [--timeout S]
"""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import xiph  # noqa: E402  (libogg/libvorbis for vorbisfile.dll)
import vabuild  # noqa: E402  (translated DLLs)

ROOT = pathlib.Path(__file__).resolve().parents[1]
SUPPORT = ROOT / 'port' / 'llasm-support'
PROFILE = ROOT / 'generated' / 'srw' / 'custom-en-1.0.10.0621'
IMAGE = ROOT / 'generated' / 'analysis' / 'custom-en-1.0.10.0621' / 'image.bin'
GAME_ROOT = ROOT / 'ref' / 'inputs' / 'custom-original'


def sha(p):
    return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()


def build(work, target, main_src):
    va = work / 'va'
    if not (va / 'haloce.va.ll').exists():
        sys.exit('run scripts/va-model.py first')
    out = work / f'core-{target}'
    out.mkdir(exist_ok=True)
    obj = work / f'slices-va-{target}' / 'haloce.va.o'   # shared with run-slices.py
    obj.parent.mkdir(exist_ok=True)
    if not obj.exists() or obj.stat().st_mtime < (va / 'haloce.va.ll').stat().st_mtime:
        print('compiling translated Halo (about 90 s)...', flush=True)
        subprocess.run(['clang', '-target', target, '-c', '-O1', '-fno-fast-math', '-ffp-contract=off', '-Wno-override-module',
                        str(va / 'haloce.va.ll'), '-o', str(obj)], check=True)
    runtime_ll = sorted(va.glob('halopad-*.ll'))
    mod_objs, mod_lls = vabuild.module_objects(va, target, obj.parent)
    subprocess.run([sys.executable, str(ROOT / 'scripts/gen-import-stubs.py'), str(va / 'haloce.va.ll'), str(out / 'stubs.ll'),
                    str(va / 'dispatch.ll'), *map(str, runtime_ll), *map(str, mod_lls)], check=True, capture_output=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/gen-nls-tables.py')], check=True, capture_output=True)
    exe = out / ('halopad-core' if main_src.name == 'halopad_core_main.c' else main_src.stem)
    cmd = ['clang', '-target', target, '-O2', '-fno-fast-math', '-ffp-contract=off', '-w', '-DPTROFS_64BIT=1', '-std=c2x',
           '-Wno-override-module', '-I', str(SUPPORT), '-I', str(ROOT / 'generated' / 'runtime'), *xiph.include_flags(),
           str(main_src), *map(str, sorted((ROOT / 'port/runtime').glob('*.c'))),
           *map(str, sorted(SUPPORT.glob('llasm_*.c'))), str(va / 'dispatch.ll'), *map(str, runtime_ll),
           str(out / 'stubs.ll'), str(obj), *map(str, mod_objs), *[str(m) for m in sorted((ROOT / 'port/apple').glob('*.m'))], str(xiph.archive(target, os.environ.get('SDKROOT'))), '-fobjc-arc', '-framework', 'CoreGraphics', '-framework', 'Cocoa', '-framework', 'Metal', '-framework', 'QuartzCore', '-framework', 'AudioToolbox', '-o', str(exe)]
    link = subprocess.run(cmd, capture_output=True, text=True)
    (out / 'link.log').write_text(' '.join(cmd) + '\n' + link.stdout + link.stderr)
    if link.returncode:
        sys.exit('link failed:\n' + link.stderr[-2000:])
    return exe, obj


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=pathlib.Path)
    ap.add_argument('--target')
    ap.add_argument('--timeout', type=int, default=120)
    ap.add_argument('--main', type=pathlib.Path, default=ROOT / 'port/core/halopad_core_main.c',
                    help='program to link in place of the core (e.g. tests/halo_d3d9_test.c)')
    a = ap.parse_args()
    work = (a.work or max(PROFILE.glob('run-*/va/haloce.va.ll'), key=lambda p: p.stat().st_mtime).parent.parent).resolve()
    target = a.target or json.loads((ROOT / 'toolchains.lock.json').read_text())['target']
    exe, obj = build(work, target, a.main.resolve())
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / 'G3' / f'core-{target}-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    # each run starts from the reference machine's registry; the final state is evidence
    env = dict(os.environ, HALOPAD_IMAGE=str(IMAGE), HALOPAD_MODULE_IMAGES=str(IMAGE.parent / 'modules'), HALOPAD_GAME_ROOT=str(GAME_ROOT), HALOPAD_STATE_ROOT=str(ROOT / 'generated' / 'halopad-disk'), HALOPAD_REPO_ROOT=str(ROOT),
               HALOPAD_REGISTRY=str(evid / 'registry.txt'))
    acceptance = ROOT / 'generated' / 'runtime-state' / 'eula-acceptance.txt'   # written only by scripts/accept-eula.sh
    if acceptance.exists():
        env['HALOPAD_EULA_ACCEPTANCE'] = str(acceptance)
    try:
        run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=a.timeout, env=env)
        code, out, err = run.returncode, run.stdout, run.stderr
    except subprocess.TimeoutExpired as t:
        code, out, err = 'timeout', (t.stdout or b'').decode(errors='replace'), (t.stderr or b'').decode(errors='replace')
    (evid / 'stdout.txt').write_text(out)
    (evid / 'stderr.txt').write_text(err)
    stop = next((l for l in reversed(err.splitlines()) if l.startswith('HALOPAD TRAP') or l.startswith('HALOPAD FAULT')), None)
    report = {'target': target, 'work': str(work.relative_to(ROOT)), 'exit': code, 'stopped_at': stop,
              'identities': {'image.bin': sha(IMAGE), 'haloce.va.o': sha(obj), 'executable': sha(exe)}}
    (evid / 'result.json').write_text(json.dumps(report, indent=1) + '\n')
    print(err[-1500:])
    print('exit', code, '| stopped at:', stop)
    print('evidence', evid.relative_to(ROOT))


if __name__ == '__main__':
    main()
