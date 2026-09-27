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
import re
import sys

import hpmodule

ROOT = pathlib.Path(__file__).resolve().parents[1]
A = HAND = MOD = None     # set from --module in main()


def hand_ranges():
    """Byte ranges covered by hand-maintained replacements (they take precedence)."""
    out = []
    if HAND.exists():
        for line in HAND.read_text().splitlines():
            if line.startswith('loc_'):
                a, n = line.split(',')[:2]
                out.append((int(a[4:], 16), int(a[4:], 16) + int(n)))
    return out


def srw_reachable():
    """Instructions SRW will translate: reachable by control flow from the entry point,
    TLS callbacks and relocation targets in .text (SRW does not see functions found only
    by gap probing). Traps outside this set would be emitted outside any procedure."""
    import glob
    import struct
    import capstone
    from capstone import x86_const as X
    info = {}
    for line in (A / 'udis-flags.txt').read_text().splitlines():
        _, a, ln, m = line.split()[:4]
        info[int(a, 16)] = (int(ln), m)
    img = MOD.image()
    base = MOD.base
    audit = MOD.audit()
    tables = {}
    for jt in audit['jump_tables']:
        tables[jt['jump']] = [struct.unpack_from('<I', img, jt['table'] + 4 * i - base)[0] for i in range(jt['entries'])]
    text_lo, text_hi = MOD.text_lo, MOD.text_hi
    roots = {MOD.entry} | {int(f, 16) for f, src in json.loads((A / 'functions.json').read_text()).items()
                           if src.startswith('export:')}
    noret = {int(l[4:], 16) for l in (A / 'srw' / 'noret_procedures.sci').read_text().split()} \
        if (A / 'srw' / 'noret_procedures.sci').exists() else set()
    for f, t in MOD.relocations():
        if text_lo <= t < text_hi and t in info:
            roots.add(t)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    seen, work = set(), list(roots)
    while work:
        a = work.pop()
        while a in info and a not in seen:
            seen.add(a)
            ins = next(md.disasm(img[a - base:a - base + 16], a, 1))
            g = set(ins.groups)
            op = ins.operands[0] if ins.operands else None
            if capstone.CS_GRP_RET in g or ins.mnemonic in ('int3', 'hlt', 'ud2'):
                break
            if (capstone.CS_GRP_JUMP in g or capstone.CS_GRP_CALL in g) and op is not None and op.type == X.X86_OP_IMM:
                work.append(op.imm & 0xFFFFFFFF)
                if capstone.CS_GRP_CALL in g and (op.imm & 0xFFFFFFFF) in noret:
                    break
            if a in tables:
                work.extend(tables[a])
            if ins.mnemonic == 'jmp':
                break
            a += info[a][0]
    return seen


def main():
    # SRW opens a new procedure at every label; a replacement followed by a label must
    # close the current one ("tcall <label>|endp"). Labels come from a previous SRW run.
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('--srw-output', type=pathlib.Path, help='work dir of a previous SRW run (seg01_code.llinc)')
    hpmodule.add_argument(ap)
    a = ap.parse_args()
    global A, HAND, MOD
    MOD = hpmodule.Module(a.module)
    A, HAND = MOD.analysis, MOD.hand / 'instruction_replacements.sci'
    srw_labels = set()
    orphans = set()
    if a.srw_output:
        code = (a.srw_output / 'seg01_code.llinc').read_text(errors='replace')
        srw_labels = {int(m, 16) for m in re.findall(r'^proc loc_([0-9A-F]+)$', code, re.M)}
        # SRW emits a replacement at its address even when its own disassembly never
        # reaches that code; such a line lands outside any procedure. Drop those sites:
        # SRW then emits nothing there and a transfer to it stops at dispatch.
        in_proc = False
        for line in code.splitlines():
            if line.startswith('proc '):
                in_proc = True
            elif line.startswith('endp'):
                in_proc = False
            elif not in_proc:
                m = re.match(r'call halopad_(?:unreachable_simd|trap_unimplemented) (0x[0-9a-f]+)', line)
                if m:
                    orphans.add(int(m.group(1), 16))
    u = json.loads((A / 'srw-unsupported.json').read_text())
    r = json.loads((A / 'simd-reachability.json').read_text())
    fns = sorted(int(x, 16) for x in json.loads((A / 'functions.json').read_text()))
    fn = lambda a: fns[bisect.bisect_right(fns, a) - 1]
    simd = {int(f['function'], 16) for f in r['functions']}
    lines, summary = [], collections.Counter()
    sites = [(int(a, 16), l, m) for fam in u['by_family'].values() for a, l, m in fam['sites']]
    covered = hand_ranges()
    reachable = srw_reachable()
    skipped = 0
    for addr, length, mnemonic in sorted(sites):
        if any(lo <= addr < hi for lo, hi in covered):
            continue
        if addr not in reachable or addr in orphans:
            skipped += 1
            continue
        kind = 'halopad_unreachable_simd' if fn(addr) in simd else 'halopad_trap_unimplemented'
        summary[(kind, mnemonic)] += 1
        tail = f'|tcall loc_{addr + length:X}|endp' if (addr + length) in srw_labels else ''
        lines.append(f'loc_{addr:X},{length},call {kind} 0x{addr:x}{tail} ; {mnemonic}')
    out = A / 'srw' / 'instruction_replacements.sci'
    # Negative-index table jumps (MSVC memcpy): index the real table from its start.
    audit = MOD.audit()
    for nj in audit.get('negative_index_jumps', []):
        reg = nj['index']
        # The replacement ends the procedure; cover trailing filler bytes (nop/int3) so SRW
        # does not emit them after 'endp'.
        img = MOD.image()
        length = nj['length']
        while img[nj['jump'] + length - MOD.base] in (0x90, 0xCC) and (nj['jump'] + length) not in srw_labels:
            length += 1
        lines.append(f"loc_{nj['jump']:X},{length},;jmp dword [{reg}*4+loc_{nj['base']:X}] (negative index into table loc_{nj['table']:X})"
                     f"|mov tmpadr, loc_{nj['table']:X}|add tmp2, {reg}, {nj['delta_entries']}|shl tmp0, tmp2, 2"
                     f"|add tmpadr, tmpadr, tmp0|load tmp1, tmpadr, 4|tcall tmp1|endp")
        summary[('negative-index-jump', 'jmp')] += 1
    out.write_text('\n'.join(lines) + '\n')
    by_kind = collections.Counter()
    for (kind, _), n in summary.items():
        by_kind[kind] += n
    (A / 'srw-traps.json').write_text(json.dumps({
        'by_kind': dict(by_kind),
        'unimplemented': {m: n for (k, m), n in sorted(summary.items()) if k == 'halopad_trap_unimplemented'},
    }, indent=1) + '\n')
    print(f'{len(lines)} replacements: ' + ', '.join(f'{k} {v}' for k, v in by_kind.items())
          + f'; {skipped} sites outside SRW-reachable code left untouched')
    return 0


if __name__ == '__main__':
    sys.exit(main())
