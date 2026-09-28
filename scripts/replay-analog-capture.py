#!/usr/bin/env python3
"""Replay the development scene's private input snapshot in original x86.

The capture is data only. Original code and import traps come from the locked
image; no captured bytes are executable. Output goes beside the ignored capture.
"""
import argparse
import hashlib
import importlib.util
import json
import pathlib
import struct

ROOT = pathlib.Path(__file__).resolve().parents[1]
RANGES = ((0x612000, 0x1000), (0x64c000, 0x2000), (0x6ab000, 0x3000),
          (0x68c000, 0x2000), (0x815000, 0x2000))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('capture', type=pathlib.Path)
    ap.add_argument('--control-word', type=lambda value: int(value, 0), required=True)
    args = ap.parse_args()
    raw = args.capture.read_bytes()
    blocks, offset = [], 0
    for address, size in RANGES:
        if struct.unpack_from('<II', raw, offset) != (address, size):
            ap.error('unexpected capture range')
        offset += 8
        block = raw[offset:offset + size]
        if len(block) != size:
            ap.error('truncated capture')
        blocks.append((address, block))
        offset += size
    if offset != len(raw):
        ap.error('unexpected trailing data')
    spec = importlib.util.spec_from_file_location('oracle', ROOT / 'scripts/x86-oracle.py')
    oracle = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(oracle)

    class CapturedInput(oracle.Oracle):
        def _machine(self):
            mu = super()._machine()
            self.machine = mu
            for address, block in blocks:
                mu.mem_write(address, block)
            self.before = struct.unpack('<2f', mu.mem_read(0x6ad4b8, 8))
            return mu

    instance = CapturedInput(fpcw=args.control_word)
    result = instance.call(0x48f850, convention='cdecl')
    original = struct.unpack('<2f', instance.machine.mem_read(0x6ad4b8, 8))
    report = dict(capture_sha256=hashlib.sha256(raw).hexdigest(), image_sha256=instance.image.sha256,
                  control_word=args.control_word, captured=instance.before, original_x86=original,
                  execution=result)
    args.capture.with_suffix('.replay.json').write_text(json.dumps(report, indent=2) + '\n')
    print('captured forward/strafe:', instance.before, 'original x86:', original)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
