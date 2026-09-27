#!/usr/bin/env python3
"""Census of Halo's shaders (G4 scoping): which versions, opcodes, register types and
modifiers the Direct3D-bytecode-to-Metal translator must handle.

Formats: vsh.bin is [u32 bytes][bytecode] records; an effect collection is u32 effect count, then per
effect a u32-length name and u32 shader count, then per shader a u32-length name, u32 token
count and the tokens.

Input: generated/analysis/shaders/*.bin, decrypted by Halo's own loader
(tools/halo_shader_extract.c). Output: docs/SHADER-CENSUS.md (aggregate statistics only;
the bytecode itself is game data and stays in generated/).

Usage: .venv/bin/python scripts/shader-census.py
"""
import collections
import pathlib
import struct

ROOT = pathlib.Path(__file__).resolve().parents[1]
S = ROOT / 'generated' / 'analysis' / 'shaders'
OPS = {0: 'nop', 1: 'mov', 2: 'add', 3: 'sub', 4: 'mad', 5: 'mul', 6: 'rcp', 7: 'rsq', 8: 'dp3', 9: 'dp4', 10: 'min', 11: 'max',
       12: 'slt', 13: 'sge', 14: 'exp', 15: 'log', 16: 'lit', 17: 'dst', 18: 'lrp', 19: 'frc', 20: 'm4x4', 21: 'm4x3',
       22: 'm3x4', 23: 'm3x3', 24: 'm3x2', 25: 'call', 26: 'callnz', 27: 'loop', 28: 'ret', 29: 'endloop', 30: 'label',
       31: 'dcl', 32: 'pow', 33: 'crs', 34: 'sgn', 35: 'abs', 36: 'nrm', 37: 'sincos', 38: 'rep', 39: 'endrep', 40: 'if',
       41: 'ifc', 42: 'else', 43: 'endif', 44: 'break', 45: 'breakc', 46: 'mova', 47: 'defb', 48: 'defi', 64: 'texcoord',
       65: 'texkill', 66: 'tex', 67: 'texbem', 68: 'texbeml', 69: 'texreg2ar', 70: 'texreg2gb', 71: 'texm3x2pad',
       72: 'texm3x2tex', 73: 'texm3x3pad', 74: 'texm3x3tex', 76: 'texm3x3spec', 77: 'texm3x3vspec', 78: 'expp', 79: 'logp',
       80: 'cnd', 81: 'def', 82: 'texreg2rgb', 83: 'texdp3tex', 84: 'texm3x2depth', 85: 'texdp3', 86: 'texm3x3',
       87: 'texdepth', 88: 'cmp', 89: 'bem', 90: 'dp2add', 91: 'dsx', 92: 'dsy', 93: 'texldd', 94: 'setp', 95: 'texldl',
       96: 'breakp', 0xFFFD: 'phase'}
REGS = {0: 'r (temp)', 1: 'v (input)', 2: 'c (const)', 3: 'a/t (addr/texture)', 4: 'oPos/rast', 5: 'oD/attr out', 6: 'oT/texcrd out',
        7: 'o (output)', 8: 'oC (color out)', 9: 'oDepth', 10: 's (sampler)', 11: 'c (const2)', 12: 'c (const3)',
        13: 'c (const4)', 14: 'b', 15: 'aL', 16: 'tempfloat16', 17: 'misc', 18: 'label', 19: 'p'}
SRCMOD = {0: None, 1: 'neg', 2: 'bias', 3: 'bias_neg', 4: 'sign(bx2)', 5: 'sign_neg', 6: 'comp(1-x)', 7: 'x2', 8: 'x2_neg',
          9: 'dz', 10: 'dw', 11: 'abs', 12: 'abs_neg', 13: 'not'}


def regtype(tok):
    return ((tok >> 28) & 7) | ((tok >> 8) & 0x18)


def walk(tokens, stats, label):
    ver = tokens[0]
    major, minor = (ver >> 8) & 0xFF, ver & 0xFF
    kind = 'vs' if ver >> 16 == 0xFFFE else 'ps'
    stats['versions'][f'{kind}_{major}_{minor}'] += 1
    i = 1
    while i < len(tokens):
        t = tokens[i]
        if t == 0x0000FFFF:
            return
        op = t & 0xFFFF
        if op == 0xFFFE:
            i += 1 + ((t >> 16) & 0x7FFF)
            continue
        name = OPS.get(op, f'op{op}')
        stats['ops'][(kind, name)] += 1
        if t & 0x40000000 and kind == 'ps' and major == 1:
            stats['misc']['co-issue (ps 1.x)'] += 1
        if major >= 2:
            n = (t >> 24) & 0xF
            params = tokens[i + 1:i + 1 + n]
            i += 1 + n
        elif op == 81:
            params = tokens[i + 1:i + 2]
            i += 6
        else:
            j = i + 1
            while j < len(tokens) and tokens[j] & 0x80000000:
                j += 1
            params = tokens[i + 1:j]
            i = j
        if op in (31,):  # dcl: usage token then register
            if params:
                stats['dcl'][(kind, params[0] & 0x1F, (params[0] >> 16) & 0xF, regtype(params[-1]))] += 1
            continue
        for k, p in enumerate(params):
            if not p & 0x80000000:
                continue
            rt = regtype(p)
            stats['regs'][(kind, REGS.get(rt, str(rt)))] += 1
            if k == 0 and op not in (65,):  # destination
                mods = (p >> 20) & 0xF
                if mods & 1: stats['misc']['dest _sat'] += 1
                if mods & 2: stats['misc']['dest _pp (partial precision)'] += 1
                if mods & 4: stats['misc']['dest _centroid'] += 1
                shift = (p >> 24) & 0xF
                if shift: stats['misc'][f'dest shift {shift}'] += 1
                if (p >> 16) & 0xF != 0xF: stats['misc']['dest write mask'] += 1
            else:
                sm = SRCMOD.get((p >> 24) & 0xF)
                if sm: stats['srcmod'][(kind, sm)] += 1
                if (p >> 16) & 0xFF != 0xE4: stats['misc']['source swizzle'] += 1
                if p & 0x2000: stats['misc']['relative addressing'] += 1
    raise ValueError(f'{label}: no END token')


def main():
    stats = {k: collections.Counter() for k in ('versions', 'ops', 'regs', 'srcmod', 'misc', 'dcl')}
    counts = collections.Counter()
    data = (S / 'vsh.bin').read_bytes()
    off = 0
    while off + 4 <= len(data):
        n, = struct.unpack_from('<I', data, off)
        if n == 0 or off + 4 + n > len(data):
            break
        walk(list(struct.unpack_from(f'<{n // 4}I', data, off + 4)), stats, f'vsh@{off}')
        counts['vertex shaders (vsh.enc)'] += 1
        off += 4 + n
    effects = {}
    bycol = collections.defaultdict(collections.Counter)
    for f in sorted(S.glob('EffectCollection_*.bin')):
        d = f.read_bytes()
        o = 0
        ne, = struct.unpack_from('<I', d, o); o += 4
        names = []
        for _ in range(ne):
            ln, = struct.unpack_from('<I', d, o); o += 4
            ename = d[o:o + ln].decode(); o += ln
            ns, = struct.unpack_from('<I', d, o); o += 4
            for _ in range(ns):
                ln, = struct.unpack_from('<I', d, o); o += 4
                o += ln
                sz, = struct.unpack_from('<I', d, o); o += 4          # size in 32-bit tokens
                toks = list(struct.unpack_from(f'<{sz}I', d, o))
                walk(toks, stats, f'{f.name}:{ename}')
                bycol[f.stem][f'{"vs" if toks[0] >> 16 == 0xFFFE else "ps"}_{(toks[0] >> 8) & 0xFF}_{toks[0] & 0xFF}'] += 1
                o += 4 * sz
                counts[f'pixel shaders ({f.stem})'] += 1
            names.append(ename)
        trailer = d[o:]
        # the file ends with a 32-hex-digit checksum string and a NUL
        effects[f.stem] = (ne, len(trailer) == 33 and trailer[-1] == 0 and all(c in b'0123456789abcdef' for c in trailer[:32]))
    out = ['# Halo shader census', '',
           'Generated by ~scripts/shader-census.py~ from the collections decrypted by Halo\'s own loader (~tools/halo_shader_extract.c~).',
           'It sets the scope of the Direct3D shader bytecode → Metal Shading Language translator. Only aggregate counts are recorded here;',
           'the bytecode is game data and stays in ~generated/~.', '', '## Programs', '', '| Set | Programs |', '|---|---|']
    out += [f'| {k} | {v} |' for k, v in sorted(counts.items())]
    out += ['', 'Each collection parses completely and ends with a 32-hex-digit checksum string: ' +
            ', '.join(f'~{k}~ {n} effects{"" if ok else " (UNEXPECTED TRAILER)"}' for k, (n, ok) in effects.items()) + '.', '',
            '## Versions per collection', '',
            'Halo loads the collection matching the device\'s pixel shader version; each mixes older versions.', '',
            '| Collection | ' + ' | '.join(sorted({v for c in bycol.values() for v in c})) + ' |',
            '|---|' + '---|' * len({v for c in bycol.values() for v in c})]
    vs_ = sorted({v for c in bycol.values() for v in c})
    out += [f'| ~{k}~ | ' + ' | '.join(str(bycol[k][v]) for v in vs_) + ' |' for k in sorted(bycol)]
    out += ['',
            '## Versions', '', '| Version | Programs |', '|---|---|'] + [f'| {k} | {v} |' for k, v in sorted(stats['versions'].items())]
    for kind in ('vs', 'ps'):
        out += ['', f'## {kind} instructions', '', '| Opcode | Uses |', '|---|---|']
        out += [f'| ~{n}~ | {c} |' for (k, n), c in stats['ops'].most_common() if k == kind]
        out += ['', f'## {kind} registers', '', '| Register file | Uses |', '|---|---|']
        out += [f'| {n} | {c} |' for (k, n), c in stats['regs'].most_common() if k == kind]
        out += ['', f'## {kind} source modifiers', '', '| Modifier | Uses |', '|---|---|']
        out += [f'| {n} | {c} |' for (k, n), c in stats['srcmod'].most_common() if k == kind] or ['| none | |']
    out += ['', '## Other features', '', '| Feature | Uses |', '|---|---|'] + [f'| {n} | {c} |' for n, c in stats['misc'].most_common()]
    out += ['', '## Declarations (dcl)', '', '| Kind | Usage | Index | Register type | Uses |', '|---|---|---|---|---|']
    out += [f'| {k} | {u} | {ix} | {REGS.get(rt, rt)} | {c} |' for (k, u, ix, rt), c in sorted(stats['dcl'].items())]
    (ROOT / 'docs' / 'SHADER-CENSUS.md').write_text('\n'.join(out).replace('~', chr(96)) + '\n')
    print('\n'.join(out[:40]).replace('~', chr(96)))


if __name__ == '__main__':
    main()
