#!/usr/bin/env python3
"""Generate SRW instruction replacements that turn every instruction SRW's llasm
backend cannot translate into an explicit, address-naming trap.

  halopad_unreachable_simd <addr>    site inside a SIMD-only function (never selected
                                     under the plain-CPU contract, docs/G2D-SIMD-SCOPE.md)
  halopad_trap_unimplemented <addr>  generic-code instruction still to be implemented

Writes generated/analysis/<profile>/srw/instruction_replacements.sci and a summary
JSON next to it. Nothing is silently dropped: each trap stops the program with the
original address when reached.
"""
import bisect
import collections
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
A = ROOT / 'generated' / 'analysis' / 'custom-en-1.0.10.0621'
HAND = ROOT / 'config' / 'srw' / 'custom-en-1.0.10.0621' / 'instruction_replacements.sci'


def hand_ranges():
    """Byte ranges covered by hand-maintained replacements (they take precedence)."""
    out = []
    if HAND.exists():
        for line in HAND.read_text().splitlines():
            if line.startswith('loc_'):
                a, n = line.split(',')[:2]
                out.append((int(a[4:], 16), int(a[4:], 16) + int(n)))
    return out


def main():
    u = json.loads((A / 'srw-unsupported.json').read_text())
    r = json.loads((A / 'simd-reachability.json').read_text())
    fns = sorted(int(x, 16) for x in json.loads((A / 'functions.json').read_text()))
    fn = lambda a: fns[bisect.bisect_right(fns, a) - 1]
    simd = {int(f['function'], 16) for f in r['functions']}
    lines, summary = [], collections.Counter()
    sites = [(int(a, 16), l, m) for fam in u['by_family'].values() for a, l, m in fam['sites']]
    covered = hand_ranges()
    for addr, length, mnemonic in sorted(sites):
        if any(lo <= addr < hi for lo, hi in covered):
            continue
        kind = 'halopad_unreachable_simd' if fn(addr) in simd else 'halopad_trap_unimplemented'
        summary[(kind, mnemonic)] += 1
        lines.append(f'loc_{addr:X},{length},call {kind} 0x{addr:x} ; {mnemonic}')
    out = A / 'srw' / 'instruction_replacements.sci'
    out.write_text('\n'.join(lines) + '\n')
    by_kind = collections.Counter()
    for (kind, _), n in summary.items():
        by_kind[kind] += n
    (A / 'srw-traps.json').write_text(json.dumps({
        'by_kind': dict(by_kind),
        'unimplemented': {m: n for (k, m), n in sorted(summary.items()) if k == 'halopad_trap_unimplemented'},
    }, indent=1) + '\n')
    print(f'{len(lines)} replacements: ' + ', '.join(f'{k} {v}' for k, v in by_kind.items()))
    return 0


if __name__ == '__main__':
    sys.exit(main())
