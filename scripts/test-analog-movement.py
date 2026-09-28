#!/usr/bin/env python3
"""Compare Halo's native movement-consumer samples against original x86.

Run halo_dinput_test first, then pass its G3 evidence directory. Unicorn stays
reference-only. This validates configured input, not touch or physical gameplay.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
import math
import pathlib
import re
import struct

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-evidence', type=pathlib.Path, required=True)
    args = parser.parse_args()
    native = args.native_evidence.resolve()
    result = json.loads((native / 'result.json').read_text())
    log = (native / 'stdout.txt').read_text()
    if result['exit'] != 0 or 'PASS: 0 failure(s)' not in log:
        parser.error('native evidence must be a completed, passing DirectInput run')
    samples = re.findall(r'axes ([\d.-]+)/([\d.-]+) -> forward [\d.-]+, strafe [\d.-]+; bits ([0-9a-f]{8})/([0-9a-f]{8})', log)
    if len(samples) != 10:
        parser.error('expected all ten movement samples with exact float bits')

    spec = importlib.util.spec_from_file_location('oracle', ROOT / 'scripts/x86-oracle.py')
    oracle = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(oracle)

    class MovementOracle(oracle.Oracle):
        def _machine(self):
            mu = super()._machine()
            self.machine = mu
            # Equivalent neutral fixture to halo_dinput_test. The original input
            # consumer needs a mouse-present flag, but does not call that object.
            mu.mem_write(0x64c734, struct.pack('<I', 1))
            mu.mem_write(0x64c529, b'\0')
            mu.mem_write(0x64c73c, bytes(28))
            mu.mem_write(0x6ab328, bytes(0x868))
            mu.mem_write(0x6ab330, struct.pack('<H', 0x7fff) * ((0x6abb36 - 0x6ab330) // 2))
            mu.mem_write(0x64dc18, struct.pack('<4i', 0, -1, -1, -1))
            mu.mem_write(0x64c9cc, struct.pack('<3I', 5, 10, 1))
            mu.mem_write(0x64d998, bytes(0xa0))
            mu.mem_write(0x64d9f8, struct.pack('<i', -1))
            for index in range(4):
                mu.mem_write(0x6ab526 + 4 * index, struct.pack('<i', -1))
            mu.mem_write(0x6ab536, struct.pack('<4H', 22, 21, 20, 19))
            mu.mem_write(0x6abb58, struct.pack('<2f', 1, 1))
            mu.mem_write(0x64d9b8, struct.pack('<2h', *self.axes))
            return mu

    def directinput_axis(value):
        # Halo requests -4096..4096 and a 10% dead zone. Round to the
        # nearest integral axis count (half cases away from zero).
        magnitude = max(0, (abs(value) - 0.1) / 0.9)
        return int(math.copysign(math.floor(magnitude * 4096 + 0.5), value))

    reference = MovementOracle(fpcw=0x007f)
    cases = []
    for x, y, forward, strafe in samples:
        reference.axes = directinput_axis(float(x)), directinput_axis(-float(y))
        observed = reference.call(0x48f850, convention='cdecl')
        original = struct.unpack('<2I', reference.machine.mem_read(0x6ad4b8, 8))
        expected = int(forward, 16), int(strafe, 16)
        ok = original == expected and observed['stack_delta'] == 4
        cases.append(dict(stick=[float(x), float(y)], axes=reference.axes,
                          native_bits=expected, x86_bits=original, ok=ok))
    quantized = re.findall(r'quantize ([0-9a-f]{8}) -> ([0-9a-f]{8})', log)
    if len(quantized) != 12:
        parser.error('expected twelve player-command quantization samples')
    for input_bits, native_bits in quantized:
        observed = reference.call(0x473c30, args=[int(input_bits, 16)], convention='cdecl')
        # UC FP0 is physical register zero; x87 returns on the register named by TOP.
        top = (reference.machine.reg_read(oracle.X.UC_X86_REG_FPSW) >> 11) & 7
        significand, exponent = reference.machine.reg_read(getattr(oracle.X, f'UC_X86_REG_FP{top}'))
        value = 0.0 if not significand else math.ldexp(significand / 2**63, (exponent & 0x7fff) - 16383)
        if exponent & 0x8000:
            value = -value
        original_bits = struct.unpack('<I', struct.pack('<f', value))[0]
        cases.append(dict(function='0x473c30', input_bits=input_bits, native_bits=native_bits,
                          x86_bits=f'{original_bits:08x}',
                          ok=original_bits == int(native_bits, 16) and observed['stack_delta'] == 4))
    stamp = datetime.datetime.now(datetime.timezone.utc)
    evidence = ROOT / 'docs/artifacts' / stamp.strftime('%Y-%m-%d') / 'G9' / ('analog-oracle-' + stamp.strftime('%Y%m%dT%H%M%SZ'))
    evidence.mkdir(parents=True)
    report = dict(function='0x48f850', image_sha256=reference.image.sha256,
                  native_evidence=str(native.relative_to(ROOT)), native_result=result,
                  native_log_sha256=hashlib.sha256((native / 'stdout.txt').read_bytes()).hexdigest(),
                  cases=cases, failures=sum(not row['ok'] for row in cases),
                  source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in (pathlib.Path(__file__).resolve(), ROOT / 'tests/halo_dinput_test.c')})
    (evidence / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"ORACLE: {len(cases)} movement/quantization cases, {report['failures']} failures")
    print('evidence', evidence.relative_to(ROOT))
    return int(report['failures'] != 0)


if __name__ == '__main__':
    raise SystemExit(main())
