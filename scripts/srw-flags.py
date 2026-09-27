#!/usr/bin/env python3
"""Compute SRW instruction_flags.sci for condition flags that cross llasm region boundaries.

SRW's llasm backend splits code at every label into separate procedures and does not
carry x86 flags across them. Where a label's code reads flags before writing them,
SRW needs explicit hints: to_clear at the label (flags come from the materialized
state), and to_set at the last writer of those flags on every incoming path.

Flag sets come from SRW's own tables (tools/udis-scan.c built WITH_SRW_FLAGS). Writes
generated/analysis/<profile>/srw/instruction_flags.sci and srw-flags.json.
Unresolvable needs (after calls, at indirect entries) are reported, never guessed.
"""
import bisect
import collections
import glob
import json
import pathlib
import struct
import sys

import capstone
from capstone import x86_const as X

import hpmodule

ROOT = pathlib.Path(__file__).resolve().parents[1]
COND = 0x3F  # CF PF AF ZF SF OF


def main():
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    hpmodule.add_argument(ap)
    module = hpmodule.Module(ap.parse_args().module)
    A = module.analysis
    rows = [l.split() for l in (A / 'udis-flags.txt').read_text().splitlines()]
    info = {}
    for _, a, ln, m, seg, need, mod in rows:
        info[int(a, 16)] = (int(ln), m, int(need, 16), int(mod, 16))
    addrs = sorted(info)
    # Every replacement (generated trap or hand-written) is its own SRW region and
    # neither reads nor writes flags visible to the analysis.
    traps, trap_ends = set(), set()
    for sci in (A / 'srw' / 'instruction_replacements.sci',
                module.hand / 'instruction_replacements.sci'):
        if not sci.exists():
            continue
        for line in sci.read_text().splitlines():
            if not line.startswith('loc_'):
                continue
            a, n = line.split(',')[:2]
            a, n = int(a[4:], 16), int(n)
            traps.update(x for x in range(a, a + n) if x in info)
            trap_ends.add(a + n)
            traps.add(a)
    img = module.image()
    base = module.base
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    succ_jump = {}          # addr -> direct jump/jcc target
    calls, uncond, returns = set(), set(), set()
    for a in addrs:
        ins = next(md.disasm(img[a - base:a - base + 16], a, 1))
        g = set(ins.groups)
        if capstone.CS_GRP_CALL in g:
            calls.add(a)
        elif capstone.CS_GRP_JUMP in g:
            op = ins.operands[0]
            if op.type == X.X86_OP_IMM:
                succ_jump[a] = op.imm & 0xFFFFFFFF
            if ins.mnemonic == 'jmp':
                uncond.add(a)
        elif capstone.CS_GRP_RET in g or ins.mnemonic in ('int3', 'hlt', 'ud2'):
            returns.add(a)
    audit = module.audit()
    table_edges = collections.defaultdict(list)
    for jt in audit['jump_tables']:
        for i in range(jt['entries']):
            t = struct.unpack_from('<I', img, jt['table'] + 4 * i - base)[0]
            table_edges[t].append(jt['jump'])
            uncond.add(jt['jump'])
    functions = {int(x, 16) for x in json.loads((A / 'functions.json').read_text())}
    code_targets = {int(l[4:], 16) for l in (A / 'srw' / 'fixup_interpret_as_code.sci').read_text().split()}
    nxt = {a: a + info[a][0] for a in addrs}
    post_call = {nxt[a] for a in calls}
    call_target = {}
    call_sites = collections.defaultdict(list)     # function -> its direct call instructions
    for a in calls:
        op = next(md.disasm(img[a - base:a - base + 16], a, 1)).operands[0]
        if op.type == X.X86_OP_IMM:
            call_target[nxt[a]] = op.imm & 0xFFFFFFFF
            call_sites[op.imm & 0xFFFFFFFF].append(a)
    replacement_starts = {line_a for line_a in traps if line_a in info and (line_a - 1 not in traps)}
    labels = (set(succ_jump.values()) | set(table_edges) | post_call | functions | code_targets |
              replacement_starts | trap_ends)
    # SRW's llasm backend closes the procedure after a conditional jump and continues the
    # fall-through in a new one, unless the next instruction is another conditional jump.
    cond = {a for a in succ_jump if a not in uncond}
    labels |= {nxt[a] for a in cond if nxt[a] in info and nxt[a] not in cond}
    labels &= set(info)
    preds = collections.defaultdict(list)
    for a in addrs:
        n = nxt[a]
        if n in labels and a not in uncond and a not in returns and a not in calls:
            preds[n].append(('fall', a))
    for a, t in succ_jump.items():
        if t in labels:
            preds[t].append(('jump', a))
    for t, js in table_edges.items():
        for j in js:
            preds[t].append(('table', j))
    idx = {a: i for i, a in enumerate(addrs)}

    def flags(a):
        if a in traps:
            return 0, 0
        _, _, need, mod = info[a]
        need = 0 if need == 0xFFFFFFFF else need & COND
        mod = COND if mod == 0xFFFFFFFF else mod & COND
        return need, mod

    def region_start(a):
        i = idx[a]
        while addrs[i] not in labels and i > 0 and nxt.get(addrs[i - 1]) == addrs[i]:
            i -= 1
        return addrs[i]

    to_set, to_clear = collections.Counter(), collections.Counter()
    unresolved = []
    fn_sorted = sorted(functions)

    def callee_returns(target):
        import bisect as _b
        i = _b.bisect_right(fn_sorted, target)
        end = fn_sorted[i] if i < len(fn_sorted) else target + 0x10000
        return [a for a in addrs[idx.get(target, 0):] if a < end and a in returns and info[a][1].startswith('ret')] \
            if target in idx else []
    needs = {}
    for L in sorted(labels):
        need = written = 0
        a = L
        while True:
            r, w = flags(a)
            need |= r & ~written
            written |= w
            if a in uncond or a in returns or a in calls or a in succ_jump:
                break
            n = nxt[a]
            if n not in info or n in labels:
                break
            a = n
        if need:
            needs[L] = need
    visited = set()

    def produce(end, G, origin):
        """Require flags G to be live after 'end' (the last instruction of an incoming path).
        SRW itself finds and materializes the writer inside that region; if the region does
        not write all of G, its start gets to_clear for the rest and the requirement moves to
        that region's own incoming paths (or, after a call, to the callee's returns)."""
        to_set[end] |= G
        a = end
        while G:
            r, w = flags(a)
            if w & G:
                G &= ~w
                if not G:
                    return
            if a in labels:
                break
            i = idx[a]
            if i == 0 or nxt.get(addrs[i - 1]) != a:
                break
            a = addrs[i - 1]
        S = a if a in labels else region_start(a)
        key = (S, G)
        if key in visited:
            return
        visited.add(key)
        to_clear[S] |= G
        ends = incoming(S)
        if ends is None:
            unresolved.append({'label': hex(origin), 'via_region': hex(S), 'flags': hex(G),
                               'reason': 'after call' if S in post_call else 'no direct predecessor'})
            return
        for e in ends:
            produce(e, G, origin)

    def incoming(S):
        """The last instructions of every path into region S: the callee's returns after a
        call, the direct call sites of a function entry (a call does not change flags, so
        flags a hand-written routine reads at its entry come from its caller: the CRT's acos
        0x5ccd1d reads ZF from 0x5d7318, called just before it at 0x5ccd06), and direct
        predecessors. None if a path cannot be followed (a call whose callee has no
        returns found, or no path at all)."""
        ends = []
        if S in post_call:
            rets = callee_returns(call_target[S]) if S in call_target else []
            if not rets:
                return None
            ends += rets
        if S in functions:
            ends += call_sites.get(S, [])
        ends += [p for _, p in preds.get(S, [])]
        return ends or None

    for L, F in sorted(needs.items()):
        to_clear[L] |= F
        ends = incoming(L)
        if ends is None:
            reason = ('after call' if L in post_call else
                      'function entry with no direct caller' if L in functions else 'no direct predecessor')
            unresolved.append({'label': hex(L), 'via_region': hex(L), 'flags': hex(F), 'reason': reason})
            continue
        for e in ends:
            produce(e, F, L)
    lines = []
    for a in sorted(set(to_set) | set(to_clear)):
        lines.append(f'loc_{a:X},0x{to_set[a]:02x},0x{to_clear[a]:02x} ; {info[a][1]}')
    (A / 'srw' / 'instruction_flags.sci').write_text('\n'.join(lines) + '\n')
    (A / 'srw-flags.json').write_text(json.dumps({'labels': len(labels), 'labels_needing_flags': len(needs),
                                                  'hint_lines': len(lines), 'unresolved': unresolved}, indent=1) + '\n')
    print(f'labels {len(labels):,}; needing flags {len(needs):,}; hint lines {len(lines):,}; unresolved {len(unresolved)}')
    for u in unresolved[:10]:
        print('  unresolved', u)
    return 0


if __name__ == '__main__':
    sys.exit(main())
