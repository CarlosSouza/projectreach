#!/usr/bin/env python3
"""G2d slice 1: Halo's CRC32 (0x59f2a2) — translated ARM64 vs the x86 oracle.

Builds tests/halo_crc32_harness.c against the whole-image SRW/llasm object from the
latest pipeline run, runs the same cases through the oracle (original x86 bytes in
Unicorn) and the native build, and compares return value and stack effect.
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
SUPPORT = ROOT / 'ref' / 'sr' / 'SR' / 'llasm-support'
spec = importlib.util.spec_from_file_location('x86_oracle', ROOT / 'scripts' / 'x86-oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


def sha(p):
    return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=pathlib.Path, help='SRW run dir containing haloce.o (default: latest)')
    ap.add_argument('--cases', type=int, default=200)
    ap.add_argument('--seed', type=int, default=0x43524333)
    a = ap.parse_args()
    runs = sorted((ROOT / 'generated/srw/custom-en-1.0.10.0621').glob('run-*/haloce.o'), key=lambda p: p.stat().st_mtime)
    work = a.work or runs[-1].parent
    target = json.loads((ROOT / 'toolchains.lock.json').read_text())['target']
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / 'G2d' / f'slice-crc32-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    build = work / 'slice-crc32'
    build.mkdir(exist_ok=True)

    subprocess.run([sys.executable, str(ROOT / 'scripts/gen-import-stubs.py'), str(work / 'haloce.target.ll'),
                    str(build / 'stubs.ll')], check=True)
    exe = build / 'halo-crc32-slice'
    cmd = ['clang', '-target', target, '-O2', '-fno-fast-math', '-ffp-contract=off', '-w', '-DPTROFS_64BIT=1', '-std=c2x', '-I', str(SUPPORT),
           str(ROOT / 'tests/halo_crc32_harness.c'), str(ROOT / 'port/runtime/halopad_slice_runtime.c'),
           *[str(p) for p in sorted(SUPPORT.glob('llasm_*.c'))], str(build / 'stubs.ll'), str(work / 'haloce.o'),
           '-o', str(exe)]
    link = subprocess.run(cmd, capture_output=True, text=True)
    (evid / 'link.log').write_text(' '.join(cmd) + '\n' + link.stdout + link.stderr)
    if link.returncode:
        print('FAIL link; see', (evid / 'link.log').relative_to(ROOT))
        print(link.stderr[-2000:])
        return 1
    arch = subprocess.run(['lipo', '-archs', str(exe)], capture_output=True, text=True).stdout.strip()

    rng = random.Random(a.seed)
    lengths = list(range(0, 18)) + [255, 256, 257, 4096, 65536]
    while len(lengths) < a.cases:
        lengths.append(rng.randrange(0, 8192))
    cases = []
    for i, n in enumerate(lengths[:a.cases]):
        cases.append((bytes(rng.getrandbits(8) for _ in range(n)), rng.getrandbits(32) if i % 3 else 0))
    cases.append((b'123456789', 0))
    with open(build / 'cases.bin', 'wb') as f:
        for data, crc in cases:
            f.write(struct.pack('<II', len(data), crc) + data)
    run = subprocess.run([str(exe), str(build / 'cases.bin')], capture_output=True, text=True, timeout=600)
    (evid / 'native.stdout').write_text(run.stdout)
    (evid / 'native.stderr').write_text(run.stderr)
    native = [l.split() for l in run.stdout.splitlines()]

    orc = oracle.Oracle()
    results, failures = [], 0
    for i, (data, crc) in enumerate(cases):
        o = orc.call(0x59F2A2, [crc, data, len(data)], 'stdcall')
        n_eax, n_delta = (int(native[i][0], 16), int(native[i][1])) if i < len(native) else (None, None)
        ok = n_eax == o['eax'] and n_delta is not None and n_delta + 4 == o['stack_delta'] and not o['writes']
        failures += not ok
        results.append({'len': len(data), 'crc_in': crc, 'oracle_eax': o['eax'], 'native_eax': n_eax,
                        'oracle_stack_delta': o['stack_delta'], 'native_stack_delta_plus_ret': None if n_delta is None else n_delta + 4,
                        'ok': ok})
    check_ok = results[-1]['native_eax'] == 0xCBF43926
    report = {'slice': 'crc32 0x59f2a2 (stdcall)', 'result': 'PASS' if failures == 0 and check_ok and run.returncode == 0 else 'FAIL',
              'cases': len(results), 'failures': failures, 'check_value_native': hex(results[-1]['native_eax'] or 0),
              'native_exit': run.returncode, 'architecture': arch,
              'identities': {'image_sha256': orc.image.sha256, 'haloce.o': sha(work / 'haloce.o'),
                             'harness': sha(ROOT / 'tests/halo_crc32_harness.c'), 'executable': sha(exe)},
              'work': str(work.relative_to(ROOT)), 'results': results}
    (evid / 'result.json').write_text(json.dumps(report, indent=1) + '\n')
    print(f"{report['result']}: {len(results) - failures}/{len(results)} cases match the oracle; "
          f"check value {report['check_value_native']}; {arch}; evidence {evid.relative_to(ROOT)}")
    if run.stderr:
        print(run.stderr[-600:])
    return 0 if report['result'] == 'PASS' else 1


if __name__ == '__main__':
    sys.exit(main())
