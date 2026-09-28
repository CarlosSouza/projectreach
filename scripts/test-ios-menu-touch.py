#!/usr/bin/env python3
"""Menu touch state-machine tests and cursor convergence against original x86 Halo.

Reference-only Unicorn is used here, never by the app. Build artifacts/evidence stay ignored.
"""
import ctypes
import datetime
import hashlib
import importlib.util
import json
import pathlib
import struct
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]

def main():
    stamp = datetime.datetime.now(datetime.timezone.utc)
    evidence = ROOT / 'docs/artifacts' / stamp.strftime('%Y-%m-%d') / 'G9' / ('menu-touch-' + stamp.strftime('%Y%m%dT%H%M%SZ'))
    evidence.mkdir(parents=True)
    header = ROOT / 'port/ios/halopad_menu_touch.h'
    fixture = ROOT / 'tests/halo_menu_touch_test.c'
    exe = evidence / 'state-test'
    subprocess.run(['xcrun', 'clang', '-Wall', '-Wextra', '-Werror', '-O1', str(fixture), '-o', str(exe)], check=True)
    run = subprocess.run([str(exe)], capture_output=True, text=True, check=True)
    (evidence / 'state.txt').write_text(run.stdout + run.stderr)
    print(run.stdout, end='')
    wrapper = evidence / 'delta.c'
    wrapper.write_text('#include "' + str(header) + '"\nint delta(int d, float s) { return hp_menu_delta(d, s); }\n')
    lib = evidence / 'delta.dylib'
    subprocess.run(['xcrun', 'clang', '-dynamiclib', '-O1', str(wrapper), '-o', str(lib)], check=True)
    delta = ctypes.CDLL(str(lib)).delta
    delta.argtypes = [ctypes.c_int, ctypes.c_float]
    delta.restype = ctypes.c_int
    spec = importlib.util.spec_from_file_location('oracle', ROOT / 'scripts/x86-oracle.py')
    oracle = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(oracle)

    class CursorOracle(oracle.Oracle):
        def _machine(self):
            mu = super()._machine()
            x, y, dx, dy, sensitivity = self.sample
            mu.mem_write(0x6b4009, b'\0')
            mu.mem_write(0x64c734, struct.pack('<I', 1))
            mu.mem_write(0x64c529, b'\0')
            # Halo's input conversion stores raw DirectInput Y with the opposite sign.
            mu.mem_write(0x64c73c, struct.pack('<ii', dx, -dy))
            mu.mem_write(0x6b400c, struct.pack('<ii', x, y))
            mu.mem_write(0x629c64, struct.pack('<ff', sensitivity, sensitivity))
            return mu

    reference = CursorOracle(fpcw=0x007f)
    results = []
    targets = [(0, 0), (640, 480), (320, 370), (63, 417), (639, 1), (1, 479)]
    for sensitivity in (0, .5, 1, 3):
        for start in ((0, 0), (640, 480), (320, 240)):
            for target in targets:
                x, y = start
                path = []
                for _ in range(12):
                    if abs(x - target[0]) <= 1 and abs(y - target[1]) <= 1:
                        break
                    dx, dy = delta(target[0] - x, sensitivity), delta(target[1] - y, sensitivity)
                    reference.sample = x, y, dx, dy, sensitivity
                    output = reference.call(0x49a220, convention='cdecl')
                    for address, size, value in output['writes']:
                        if address == 0x6b400c and size == 4: x = value
                        if address == 0x6b4010 and size == 4: y = value
                    path.append([dx, dy, x, y])
                ok = abs(x - target[0]) <= 1 and abs(y - target[1]) <= 1
                results.append(dict(sensitivity=sensitivity, start=start, target=target, path=path, ok=ok))
    report = dict(image_sha256=reference.image.sha256, function='0x49a220', cases=results,
                  failures=sum(not row['ok'] for row in results),
                  source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in (header, fixture, pathlib.Path(__file__).resolve(), ROOT / 'port/ios/HaloPadApp.m')})
    (evidence / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"ORACLE: {len(results)} cursor routes, {report['failures']} failures")
    print('evidence', evidence.relative_to(ROOT))
    return int(report['failures'] != 0)

if __name__ == '__main__':
    raise SystemExit(main())
