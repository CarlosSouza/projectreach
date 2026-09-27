#!/usr/bin/env python3
"""G2d scoping: how is each SIMD-containing function reached?

Classifies every function that contains MMX/SSE/3DNow! instructions (from
srw-unsupported.json) by its incoming references: direct call/jump from generic
code, pointer only (code immediates or data tables), only from other SIMD
functions, or no reference found. Writes generated/analysis/<profile>/simd-reachability.json.
"""
import bisect
import collections
import json
import pathlib
import struct
import sys

import capstone
from capstone import x86_const as X

import hpmodule

ROOT = pathlib.Path(__file__).resolve().parents[1]
SIMD_FAMILIES = ('3DNow!', 'MMX/SSE-integer', 'SSE/SSE2 float')


def main():
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    hpmodule.add_argument(ap)
    mod = hpmodule.Module(ap.parse_args().module)
    A = mod.analysis
    img = mod.image()
    base = mod.base
    raw = (A / 'instructions.u32').read_bytes()
    addrs = struct.unpack('<%dI' % (len(raw) // 4), raw)
    fns = sorted(int(x, 16) for x in json.loads((A / 'functions.json').read_text()))
    fn = lambda a: fns[bisect.bisect_right(fns, a) - 1]
    rep = json.loads((A / 'srw-unsupported.json').read_text())
    simd_sites = collections.defaultdict(list)
    for fam in SIMD_FAMILIES:
        for addr, _, m in rep['by_family'].get(fam, {}).get('sites', []):
            a = int(addr, 16)
            simd_sites[fn(a)].append((a, m, fam))
    S = set(simd_sites)
    text_lo, text_hi = mod.text_lo, mod.text_hi
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    inc = collections.defaultdict(list)
    for a in addrs:
        ins = next(md.disasm(img[a - base:a - base + 16], a, 1))
        src = fn(a)
        grp = set(ins.groups)
        for op in ins.operands:
            if op.type == X.X86_OP_IMM:
                t = op.imm & 0xFFFFFFFF
                if text_lo <= t < text_hi:
                    kind = 'call' if capstone.CS_GRP_CALL in grp else ('jump' if capstone.CS_GRP_JUMP in grp else 'code-imm')
                    tf = fn(t)
                    if tf in S and tf != src and (kind != 'jump' or t == tf):
                        inc[tf].append((kind, src, a))
    for f, t in mod.relocations():
        if not (text_lo <= f < text_hi) and text_lo <= t < text_hi and fn(t) in S and t == fn(t):
            inc[fn(t)].append(('data-ptr', None, f))
    out, cls = [], collections.Counter()
    for s in sorted(S):
        e = inc.get(s, [])
        generic = [x for x in e if x[0] in ('call', 'jump') and x[1] not in S]
        ptr = [x for x in e if x[0] in ('code-imm', 'data-ptr')]
        from_simd = [x for x in e if x[0] in ('call', 'jump') and x[1] in S]
        c = ('direct-from-generic' if generic else 'pointer-only' if ptr else
             'only-from-SIMD' if from_simd else 'no-reference-found')
        cls[c] += 1
        fams = collections.Counter(f for _, _, f in simd_sites[s])
        out.append({'function': hex(s), 'class': c, 'simd_instructions': len(simd_sites[s]),
                    'families': dict(fams), 'mnemonics': dict(collections.Counter(m for _, m, _ in simd_sites[s])),
                    'generic_callers': sorted({hex(x[1]) for x in generic}),
                    'pointer_refs': [[k, hex(a)] for k, _, a in ptr][:20]})
    (A / 'simd-reachability.json').write_text(json.dumps({'simd_functions': len(S), 'classes': dict(cls),
                                                          'functions': out}, indent=1) + '\n')
    print('SIMD functions', len(S), dict(cls))
    for o in out:
        if o['class'] == 'direct-from-generic':
            print(o['function'], o['simd_instructions'], o['mnemonics'] if o['simd_instructions'] < 4 else o['families'],
                  'callers', len(o['generic_callers']))
    return 0


if __name__ == '__main__':
    sys.exit(main())

