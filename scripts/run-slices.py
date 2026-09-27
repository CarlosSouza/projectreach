#!/usr/bin/env python3
"""G2d slices: translated Halo functions (SRW/llasm -> ARM64) versus the x86 oracle.

Each slice names an original function, its calling convention, a case generator and
what to compare (return value, stack effect, pointer results as buffer offsets, and
final buffer contents). x87 slices run the oracle in Halo's actual precision mode
(control word 0x007F, single precision: Direct3D 9 default; Halo itself loads 0x007E
when it asks Direct3D to preserve the FPU) and report 53-bit agreement separately.

Usage: .venv/bin/python scripts/run-slices.py [--only NAME ...] [--work DIR]
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
import pathlib
import random
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SUPPORT = ROOT / 'port' / 'llasm-support'  # HaloPad copy of SR llasm-support (x87 precision control)
spec = importlib.util.spec_from_file_location('x86_oracle', ROOT / 'scripts' / 'x86-oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)

HALO_FPCW = 0x007F


def rnd_floats(rng, n):
    out = []
    for _ in range(n):
        e = rng.choice([1e-3, 1e-1, 1.0, 10.0, 1000.0])
        out.append(rng.uniform(-e, e))
    return struct.pack('<%df' % n, *out)


def cases_crc32(rng):
    lengths = list(range(0, 18)) + [255, 256, 257, 4096]
    while len(lengths) < 120:
        lengths.append(rng.randrange(0, 4096))
    out = [[rng.getrandbits(32) if i % 3 else 0, bytes(rng.getrandbits(8) for _ in range(n)), n] for i, n in enumerate(lengths)]
    out.append([0, b'123456789', 9])
    return out


def cases_memmove(rng):
    out = []
    for _ in range(300):
        size = 700
        buf = bytes(rng.getrandbits(8) for _ in range(size))
        n = rng.choice([0, 1, 2, 3, 4, 5, 7, 8, 15, 16, 31, 32, 33, 63, 64, 100, 255, 256, rng.randrange(0, 300)])
        d = rng.randrange(0, size - n)
        s = rng.choice([rng.randrange(0, size - n), max(0, min(size - n, d + rng.randrange(-8, 9)))])
        # memmove(dst, src, n); the shared buffer rides along as an unused 4th argument.
        out.append([('ptr', 3, d), ('ptr', 3, s), n, buf])
    return out


def cases_strrchr(rng):
    out = []
    for _ in range(200):
        n = rng.randrange(0, 80)
        s = bytes(rng.choice(b'abcdefgh/\\.') for _ in range(n)) + b'\0'
        c = rng.choice(b'abcdefgh/\\.xz\0')
        out.append([s, c])
    return out


def cases_vec3(rng):
    return [[b'\0' * 12, rnd_floats(rng, 3), rnd_floats(rng, 16)] for _ in range(300)]


def cases_vec4(rng):
    return [[b'\0' * 16, rnd_floats(rng, 4), rnd_floats(rng, 16)] for _ in range(300)]


SLICES = {
    'crc32': {'address': 0x59F2A2, 'alias': 'halo_crc32', 'conv': 'stdcall', 'cases': cases_crc32, 'x87': False},
    'memmove': {'address': 0x5C83F0, 'alias': 'halo_memmove', 'conv': 'cdecl', 'cases': cases_memmove, 'x87': False},
    'strrchr': {'address': 0x5C88C0, 'alias': 'halo_strrchr', 'conv': 'cdecl', 'cases': cases_strrchr, 'x87': False},
    'vec3_transform_coord': {'address': 0x5834D7, 'alias': 'halo_vec3_transform_coord', 'conv': 'stdcall', 'cases': cases_vec3, 'x87': True},
    'vec4_transform': {'address': 0x583B65, 'alias': 'halo_vec4_transform', 'conv': 'stdcall', 'cases': cases_vec4, 'x87': True},
}


def sha(p):
    return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()


def encode(fn_index, fpcw, args):
    out = struct.pack('<III', fn_index, fpcw, len(args))
    for a in args:
        if isinstance(a, int):
            out += struct.pack('<II', 0, a & 0xFFFFFFFF)
        elif isinstance(a, tuple):
            out += struct.pack('<III', 2, a[1], a[2])
        else:
            out += struct.pack('<II', 1, len(a)) + a
    return out


def oracle_observe(orc, s, args):
    """Run the original function with arguments in call order."""
    call_args = list(args)
    reads = [{'arg': i} for i, a in enumerate(args) if isinstance(a, (bytes, bytearray))]
    obs = orc.call(s['address'], call_args, s['conv'], reads=reads)
    vals = obs['arg_values']
    ref = None
    for i, a in enumerate(args):
        if isinstance(a, (bytes, bytearray)) and vals[i] <= obs['eax'] <= vals[i] + len(a):
            ref = f'B{i}+{obs["eax"] - vals[i]}'
            break
    return {'eax': obs['eax'], 'stack_delta': obs['stack_delta'], 'ref': ref or f'V{obs["eax"]:08x}',
            'buffers': [obs['buffers'][i] or '-' for i in sorted(obs['buffers'])]}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--work', type=pathlib.Path)
    ap.add_argument('--seed', type=int, default=0x48414C4F)
    a = ap.parse_args()
    names = a.only or list(SLICES)
    runs = sorted((ROOT / 'generated/srw/custom-en-1.0.10.0621').glob('run-*/haloce.o'), key=lambda p: p.stat().st_mtime)
    work = a.work or runs[-1].parent
    target = json.loads((ROOT / 'toolchains.lock.json').read_text())['target']
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / 'G2d' / f'slices-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    build = work / 'slices'
    build.mkdir(exist_ok=True)
    table = ['#define PTROFS_64BIT 1', '#include "llasm_cpu.h"']
    table += [f'extern void c_{SLICES[n]["alias"]}(_cpu *);' for n in SLICES]
    table += ['void (*const halopad_slice_fns[])(_cpu *) = {' + ', '.join(f'c_{SLICES[n]["alias"]}' for n in SLICES) + '};',
              f'const unsigned halopad_slice_count = {len(SLICES)};']
    (build / 'slice_table.c').write_text('\n'.join(table) + '\n')
    subprocess.run([sys.executable, str(ROOT / 'scripts/gen-import-stubs.py'), str(work / 'haloce.target.ll'), str(build / 'stubs.ll')],
                   check=True, capture_output=True)
    exe = build / 'halo-slices'
    # PTROFS_64BIT must be defined for SR's support files too, or their helpers treat guest
    # addresses as host pointers (string/rep helpers dereference raw 32-bit values).
    cmd = ['clang', '-target', target, '-O2', '-fno-fast-math', '-ffp-contract=off', '-w', '-DPTROFS_64BIT=1', '-std=c2x', '-I', str(SUPPORT),
           str(ROOT / 'tests/halo_slice_harness.c'), str(build / 'slice_table.c'), str(ROOT / 'port/runtime/halopad_slice_runtime.c'),
           *[str(p) for p in sorted(SUPPORT.glob('llasm_*.c'))], str(build / 'stubs.ll'), str(work / 'haloce.o'), '-o', str(exe)]
    link = subprocess.run(cmd, capture_output=True, text=True)
    (evid / 'link.log').write_text(' '.join(cmd) + '\n' + link.stdout + link.stderr)
    if link.returncode:
        print('FAIL link:', link.stderr[-1500:])
        return 1
    image = oracle.Image()
    summary, overall = {}, True
    for name in names:
        s = SLICES[name]
        fn_index = list(SLICES).index(name)
        rng = random.Random(a.seed ^ s['address'])
        cases = s['cases'](rng)
        blob = b''
        for args in cases:
            blob += encode(fn_index, HALO_FPCW, args)
        (build / f'{name}.bin').write_bytes(blob)
        run = subprocess.run([str(exe), str(build / f'{name}.bin')], capture_output=True, text=True, timeout=600)
        (evid / f'{name}.native.txt').write_text(run.stdout)
        if run.stderr:
            (evid / f'{name}.native.stderr').write_text(run.stderr)
        native = [l.split() for l in run.stdout.splitlines()]
        modes = [HALO_FPCW, 0x027F] if s['x87'] else [None]
        per_mode = {}
        mismatches = []
        for mode in modes:
            orc = oracle.Oracle(image, fpcw=mode)
            ok = 0
            for i, args in enumerate(cases):
                o = oracle_observe(orc, s, args)
                if i >= len(native):
                    continue
                n = native[i]
                n_eax, n_delta, n_ref, n_bufs = int(n[0], 16), int(n[1]), n[2], n[3:]
                same = (n_ref == o['ref'] and n_delta + 4 == o['stack_delta'] and n_bufs == o['buffers']
                        and (n_ref.startswith('B') or n_eax == o['eax']))
                ok += same
                if not same and mode in (None, HALO_FPCW) and len(mismatches) < 5:
                    mismatches.append({'case': i, 'native': n, 'oracle': o})
            per_mode[hex(mode) if mode is not None else 'default'] = f'{ok}/{len(cases)}'
        primary = per_mode[hex(HALO_FPCW) if s['x87'] else 'default']
        passed = primary == f'{len(cases)}/{len(cases)}' and run.returncode == 0 and len(native) == len(cases)
        overall &= passed
        summary[name] = {'address': hex(s['address']), 'result': 'PASS' if passed else 'FAIL', 'agreement': per_mode,
                         'native_exit': run.returncode, 'native_lines': len(native), 'first_mismatches': mismatches}
        print(f"{name:22} {summary[name]['result']:4} " + ' '.join(f'{k}:{v}' for k, v in per_mode.items())
              + (f" exit {run.returncode}" if run.returncode else ''))
        if run.stderr:
            print('   stderr:', run.stderr.strip()[:200])
    report = {'work': str(work.relative_to(ROOT)), 'halo_fpcw': hex(HALO_FPCW),
              'identities': {'image_sha256': image.sha256, 'haloce.o': sha(work / 'haloce.o'), 'executable': sha(exe)},
              'slices': summary}
    (evid / 'result.json').write_text(json.dumps(report, indent=1) + '\n')
    print('evidence', evid.relative_to(ROOT))
    return 0 if overall else 1


if __name__ == '__main__':
    sys.exit(main())
