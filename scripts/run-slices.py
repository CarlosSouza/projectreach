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
import os
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


MAPS_ROOT = ROOT / 'generated' / 'slices-data' / 'map-header'


def prepare_map_root():
    """Game root for the map-header slice: the real Blood Gulch header plus derived
    headers that the validator must reject (only the first 0x800 bytes matter)."""
    real = (ROOT / 'ref/inputs/custom-original/maps/bloodgulch.map').read_bytes()[:0x800]
    maps = MAPS_ROOT / 'maps'
    maps.mkdir(parents=True, exist_ok=True)
    variants = {'bloodgulch': real}
    def patch(name, off, data):
        b = bytearray(real); b[off:off + len(data)] = data; variants[name] = bytes(b)
    patch('badversion', 4, struct.pack('<I', 0x260))
    patch('badfoot', 0x7FC, b'\0\0\0\0')  # 'foot' is stored little-endian as b'toof'
    patch('toolarge', 8, struct.pack('<I', 0x18000001))
    patch('longname', 0x20, b'x' * 32)
    for name, data in variants.items():
        (maps / f'{name}.map').write_bytes(data)
    return list(variants) + ['missing']


def cases_map_header(rng):
    names = prepare_map_root()
    out = []
    for name in names:
        # 0x4434a0: eax = map name, esi = 0x800-byte header buffer; returns al = valid
        out.append({'args': [name.encode() + b'\0', b'\0' * 0x800], 'regs': {'eax': ('ptr', 0, 0), 'esi': ('ptr', 1, 0)}})
    return out


def oracle_file_handlers(root):
    """Python equivalents of port/runtime/halopad_kernel32.c for the oracle run."""
    files, X = {}, oracle.X

    def arg(uc, i):
        esp = uc.reg_read(X.UC_X86_REG_ESP)
        return struct.unpack('<I', uc.mem_read(esp + 4 * (i + 1), 4))[0]

    def create(uc):
        p = arg(uc, 0)
        name = bytes(uc.mem_read(p, 260)).split(b'\0', 1)[0].decode()
        path = root / name.replace('\\', '/')
        if not path.is_file():
            return 0xFFFFFFFF, 28
        h = 0x100 + 4 * len(files)
        files[h] = open(path, 'rb')
        return h, 28

    def read(uc):
        h, buf, n, nread = arg(uc, 0), arg(uc, 1), arg(uc, 2), arg(uc, 3)
        data = files[h].read(n)
        uc.mem_write(buf, data)
        uc.mem_write(nread, struct.pack('<I', len(data)))
        return 1, 20

    def close(uc):
        f = files.pop(arg(uc, 0), None)
        if f:
            f.close()
        return (1 if f else 0), 4

    return {'KERNEL32.dll!CreateFileA': create, 'KERNEL32.dll!ReadFile': read, 'KERNEL32.dll!CloseHandle': close}


SLICES = {
    'crc32': {'address': 0x59F2A2, 'alias': 'halo_crc32', 'conv': 'stdcall', 'cases': cases_crc32, 'x87': False},
    'memmove': {'address': 0x5C83F0, 'alias': 'halo_memmove', 'conv': 'cdecl', 'cases': cases_memmove, 'x87': False},
    'strrchr': {'address': 0x5C88C0, 'alias': 'halo_strrchr', 'conv': 'cdecl', 'cases': cases_strrchr, 'x87': False},
    'vec3_transform_coord': {'address': 0x5834D7, 'alias': 'halo_vec3_transform_coord', 'conv': 'stdcall', 'cases': cases_vec3, 'x87': True},
    'vec4_transform': {'address': 0x583B65, 'alias': 'halo_vec4_transform', 'conv': 'stdcall', 'cases': cases_vec4, 'x87': True},
    'map_header': {'address': 0x4434A0, 'alias': 'halo_map_header_valid', 'conv': 'cdecl', 'cases': cases_map_header, 'x87': False,
                   'game_root': MAPS_ROOT},
}


def sha(p):
    return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()


REG_IDS = {'eax': 0, 'ecx': 1, 'edx': 2, 'ebx': 3, 'ebp': 5, 'esi': 6, 'edi': 7}


def split_case(case):
    return (case['args'], case.get('regs', {})) if isinstance(case, dict) else (case, {})


def encode(fn_index, fpcw, args, regs=None):
    out = struct.pack('<III', fn_index, fpcw, len(args))
    for a in args:
        if isinstance(a, int):
            out += struct.pack('<II', 0, a & 0xFFFFFFFF)
        elif isinstance(a, tuple):
            out += struct.pack('<III', 2, a[1], a[2])
        else:
            out += struct.pack('<II', 1, len(a)) + a
    regs = regs or {}
    out += struct.pack('<I', len(regs))
    for name, v in regs.items():
        if isinstance(v, tuple):
            out += struct.pack('<IIII', REG_IDS[name], 2, v[1], v[2])
        else:
            out += struct.pack('<III', REG_IDS[name], 0, v & 0xFFFFFFFF)
    return out


def oracle_observe(orc, s, args, regs=None):
    """Run the original function with arguments in call order."""
    call_args = list(args)
    reads = [{'arg': i} for i, a in enumerate(args) if isinstance(a, (bytes, bytearray))]
    obs = orc.call(s['address'], call_args, s['conv'], regs=regs, reads=reads)
    vals = obs['arg_values']
    ref = None
    for i, a in enumerate(args):
        if isinstance(a, (bytes, bytearray)) and vals[i] <= obs['eax'] <= vals[i] + len(a):
            ref = f'B{i}+{obs["eax"] - vals[i]}'
            break
    return {'eax': obs['eax'], 'stack_delta': obs['stack_delta'], 'ref': ref or f'V{obs["eax"]:08x}',
            'buffers': [obs['buffers'][i] or '-' for i in sorted(obs['buffers'])]}


def main():
    return _main()


def compare(s, cases, native, run, image, evid, name):
    modes = [HALO_FPCW, 0x027F] if s['x87'] else [None]
    per_mode, mismatches = {}, []
    for mode in modes:
        handlers = oracle_file_handlers(s['game_root']) if s.get('game_root') else None
        orc = oracle.Oracle(image, handlers, fpcw=mode)
        ok = 0
        for i, case in enumerate(cases):
            args, regs = split_case(case)
            o = oracle_observe(orc, s, args, regs)
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
    return {'address': hex(s['address']), 'result': 'PASS' if passed else 'FAIL', 'agreement': per_mode,
            'native_exit': run.returncode, 'native_lines': len(native), 'first_mismatches': mismatches}, passed


def run_va(a, names, work, target, evid, build):
    """Original-address model: link the VA-model translation with guest memory + dispatch."""
    va = work / 'va'
    if not (va / 'haloce.va.ll').exists():
        sys.exit('run scripts/va-model.py first')
    obj = build / 'haloce.va.o'
    if not obj.exists() or obj.stat().st_mtime < (va / 'haloce.va.ll').stat().st_mtime:
        subprocess.run(['clang', '-target', target, '-c', '-O1', '-fno-fast-math', '-ffp-contract=off', '-Wno-override-module',
                        str(va / 'haloce.va.ll'), '-o', str(obj)], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/gen-import-stubs.py'), str(va / 'haloce.va.ll'), str(build / 'stubs.ll')],
                   check=True, capture_output=True)
    exe = build / 'halo-va-slices'
    cmd = ['clang', '-target', target, '-O2', '-fno-fast-math', '-ffp-contract=off', '-w', '-DPTROFS_64BIT=1', '-std=c2x',
           '-Wno-override-module', '-I', str(SUPPORT),
           str(ROOT / 'tests/halo_va_harness.c'), str(ROOT / 'port/runtime/halopad_guest.c'),
           str(ROOT / 'port/runtime/halopad_slice_runtime.c'), str(ROOT / 'port/runtime/halopad_kernel32.c'),
           *[str(p) for p in sorted(SUPPORT.glob('llasm_*.c'))], str(va / 'dispatch.ll'),
           *[str(p) for p in sorted(va.glob('halopad-*.ll'))], str(build / 'stubs.ll'), str(obj), '-o', str(exe)]
    link = subprocess.run(cmd, capture_output=True, text=True)
    (evid / 'link.log').write_text(' '.join(cmd) + '\n' + link.stdout + link.stderr)
    if link.returncode:
        print('FAIL link:', link.stderr[-1500:])
        return 1
    image = oracle.Image()
    image_bin = ROOT / 'generated/analysis/custom-en-1.0.10.0621/image.bin'
    summary, overall = {}, True
    for name in names:
        s = SLICES[name]
        rng = random.Random(a.seed ^ s['address'])
        cases = s['cases'](rng)
        blob = b''
        for case in cases:
            args, regs = split_case(case)
            blob += encode(s['address'], HALO_FPCW, args, regs)
        (build / f'{name}.bin').write_bytes(blob)
        env = dict(os.environ, HALOPAD_IMAGE=str(image_bin))
        if s.get('game_root'):
            env['HALOPAD_GAME_ROOT'] = str(s['game_root'])
        prefix = list(a.run_prefix)
        if prefix:
            # simctl spawn passes the environment through SIMCTL_CHILD_ variables
            for k in ('HALOPAD_IMAGE', 'HALOPAD_GAME_ROOT'):
                if k in env:
                    env['SIMCTL_CHILD_' + k] = env[k]
        run = subprocess.run(prefix + [str(exe), str(build / f'{name}.bin')], capture_output=True, text=True, timeout=900, env=env)
        (evid / f'{name}.native.txt').write_text(run.stdout)
        if run.stderr:
            (evid / f'{name}.native.stderr').write_text(run.stderr)
        native = [l.split() for l in run.stdout.splitlines()]
        summary[name], passed = compare(s, cases, native, run, image, evid, name)
        overall &= passed
        print(f"{name:22} {summary[name]['result']:4} " + ' '.join(f'{k}:{v}' for k, v in summary[name]['agreement'].items())
              + (f" exit {run.returncode}" if run.returncode else ''))
        if run.stderr:
            print('   stderr:', run.stderr.strip()[:300])
    arch = subprocess.run(['lipo', '-archs', str(exe)], capture_output=True, text=True).stdout.strip()
    platform = subprocess.run(['vtool', '-show-build', str(exe)], capture_output=True, text=True).stdout
    report = {'model': 'va', 'target': target, 'architecture': arch, 'build_platform': platform.strip().splitlines()[-4:],
              'run_prefix': a.run_prefix, 'work': str(work.relative_to(ROOT)), 'dispatch_entries': sum(1 for _ in open(va / 'dispatch.ll') if _.startswith('declare hidden')),
              'identities': {'image_sha256': image.sha256, 'haloce.va.o': sha(obj), 'executable': sha(exe)}, 'slices': summary}
    (evid / 'result.json').write_text(json.dumps(report, indent=1) + '\n')
    print('evidence', evid.relative_to(ROOT))
    return 0 if overall else 1


def _main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--work', type=pathlib.Path)
    ap.add_argument('--seed', type=int, default=0x48414C4F)
    ap.add_argument('--model', choices=['offset', 'va'], default='va',
                    help="va: original-address guest memory + dispatch (G2e); offset: SR's pointer-offset model")
    ap.add_argument('--target', help='clang target triple (default: toolchains.lock.json target)')
    ap.add_argument('--run-prefix', nargs='*', default=[], help='command prefix to run the harness (e.g. xcrun simctl spawn booted)')
    a = ap.parse_args()
    names = a.only or list(SLICES)
    runs = sorted((ROOT / 'generated/srw/custom-en-1.0.10.0621').glob('run-*/haloce.o'), key=lambda p: p.stat().st_mtime)
    work = a.work or runs[-1].parent
    target = a.target or json.loads((ROOT / 'toolchains.lock.json').read_text())['target']
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    goal = 'G2e' if a.model == 'va' else 'G2d'
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / goal / f'slices-{a.model}-{target}-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    build = work / ('slices' if a.model == 'offset' else f'slices-va-{target}')
    build.mkdir(exist_ok=True)
    if a.model == 'va':
        return run_va(a, names, work, target, evid, build)
    table = ['#define PTROFS_64BIT 1', '#include "llasm_cpu.h"']
    table += [f'extern void c_{SLICES[n]["alias"]}(_cpu *);' for n in SLICES]
    table += ['void (*const halopad_slice_fns[])(_cpu *) = {' + ', '.join(f'c_{SLICES[n]["alias"]}' for n in SLICES) + '};',
              f'const unsigned halopad_slice_count = {len(SLICES)};']
    (build / 'slice_table.c').write_text('\n'.join(table) + '\n')
    subprocess.run([sys.executable, str(ROOT / 'scripts/gen-import-stubs.py'), str(work / 'haloce.target.ll'), str(build / 'stubs.ll')],
                   check=True, capture_output=True)
    # HaloPad llasm runtime (Windows services implemented so far)
    llasm_bin = sorted((ROOT / 'generated/tool-builds').glob('*/llasm/llasm'), key=lambda p: p.stat().st_mtime)[-1]
    runtime_objs = []
    for src in sorted((ROOT / 'port/llasm-runtime').glob('*.llasm')):
        ll = build / (src.stem + '.ll')
        subprocess.run([str(llasm_bin), '-m64', '-ptrofs', '-I', str(SUPPORT), '-I', str(ROOT / 'port/llasm-runtime'),
                        '-o', str(ll), str(src)], check=True, capture_output=True)
        tll = build / (src.stem + '.target.ll')
        tll.write_text(next(l for l in (work / 'haloce.target.ll').open() if l.startswith('target triple')) + ll.read_text())
        runtime_objs.append(str(tll))
    exe = build / 'halo-slices'
    # PTROFS_64BIT must be defined for SR's support files too, or their helpers treat guest
    # addresses as host pointers (string/rep helpers dereference raw 32-bit values).
    cmd = ['clang', '-target', target, '-O2', '-fno-fast-math', '-ffp-contract=off', '-w', '-DPTROFS_64BIT=1', '-std=c2x', '-I', str(SUPPORT),
           str(ROOT / 'tests/halo_slice_harness.c'), str(build / 'slice_table.c'), str(ROOT / 'port/runtime/halopad_slice_runtime.c'),
           str(ROOT / 'port/runtime/halopad_kernel32.c'), *runtime_objs, '-Wno-override-module',
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
            a_, r_ = split_case(args)
            blob += encode(fn_index, HALO_FPCW, a_, r_)
        (build / f'{name}.bin').write_bytes(blob)
        env = dict(os.environ)
        if s.get('game_root'):
            env['HALOPAD_GAME_ROOT'] = str(s['game_root'])
        run = subprocess.run([str(exe), str(build / f'{name}.bin')], capture_output=True, text=True, timeout=600, env=env)
        (evid / f'{name}.native.txt').write_text(run.stdout)
        if run.stderr:
            (evid / f'{name}.native.stderr').write_text(run.stderr)
        native = [l.split() for l in run.stdout.splitlines()]
        modes = [HALO_FPCW, 0x027F] if s['x87'] else [None]
        per_mode = {}
        mismatches = []
        for mode in modes:
            handlers = oracle_file_handlers(s['game_root']) if s.get('game_root') else None
            orc = oracle.Oracle(image, handlers, fpcw=mode)
            ok = 0
            for i, case in enumerate(cases):
                args, regs = split_case(case)
                o = oracle_observe(orc, s, args, regs)
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
