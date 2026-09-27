#!/usr/bin/env python3
"""SRW llasm capability scan (G2c) — reproducible.

Builds tools/udis-scan.c against the udis86 bundled with a SRW tool build, decodes
every audited instruction start with it, compares mnemonics with the cases the
pinned SRW llasm backend implements, and writes
generated/analysis/<profile>/srw-unsupported.json.

Usage: .venv/bin/python scripts/srw-capability-scan.py --build generated/tool-builds/<key>
"""
import argparse
import bisect
import collections
import json
import pathlib
import re
import subprocess
import sys

import pefile
import capstone

import hpmodule

ROOT = pathlib.Path(__file__).resolve().parents[1]
PROFILE_ID = 'custom-en-1.0.10.0621'


def family(m, cs_name=''):
    # udis86 uses one name for string and SSE2 forms (cmpsd, movsd); Capstone's
    # name ("repe cmpsd" vs "cmpsd") disambiguates.
    if cs_name.split()[-1:] and cs_name.split()[-1] in ('cmpsb', 'cmpsw', 'cmpsd', 'movsd', 'movsw', 'movsb',
                                                        'stosd', 'lodsd', 'scasb', 'scasd') and \
            (cs_name.startswith(('rep', 'repe', 'repne')) or m in ('cmpsw', 'cmpsb')):
        return 'string'
    if m in ('cmpsw', 'cmpsb'):
        return 'string'
    if m.startswith('pf') or m in ('femms', 'pi2fd', 'pswapd'):
        return '3DNow!'
    if m.startswith('f'):
        return 'x87'
    if m in ('movq', 'movd', 'emms') or m.startswith('p'):
        return 'MMX/SSE-integer'
    if m.endswith(('ps', 'ss', 'pd', 'sd')) or m.startswith(('cvt', 'ucomi', 'comi', 'rsqrt', 'rcp', 'unpck', 'shuf',
                                                           'movdq', 'movhl', 'movlh')) or m in ('stmxcsr', 'ldmxcsr'):
        return 'SSE/SSE2 float'
    return 'other'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=pathlib.Path, required=True)
    hpmodule.add_argument(ap)
    a = ap.parse_args()
    mod = hpmodule.Module(a.module)
    analysis = mod.analysis
    udis = a.build / 'SRW' / 'udis86-1.7.2'
    tool = ROOT / 'generated' / 'tools' / 'udis-scan'
    tool.parent.mkdir(parents=True, exist_ok=True)
    lib = udis / 'libudis86'
    subprocess.run(['clang', '-O2', '-w', '-include', 'string.h', f'-I{udis}', f'-I{lib}',
                    str(ROOT / 'tools' / 'udis-scan.c')] +
                   [str(lib / f) for f in ('decode.c', 'itab.c', 'syn.c', 'syn-intel.c', 'syn-att.c', 'udis86.c')] +
                   ['-o', str(tool)], check=True)
    pe = pefile.PE(str(mod.exe))
    image = analysis / 'image.bin'
    image.write_bytes(pe.get_memory_mapped_image())
    out = subprocess.run([str(tool), str(image), hex(pe.OPTIONAL_HEADER.ImageBase), str(analysis / 'instructions.u32')],
                         check=True, capture_output=True, text=True, env={'UDIS_SCAN_ALL': '1'}).stdout
    # The same decode with SRW's flag tables (needed/modified per mnemonic) for
    # scripts/srw-flags.py and scripts/srw-traps.py.
    flags_tool = tool.parent / 'udis-flags'
    subprocess.run(['clang', '-O2', '-w', '-include', 'string.h', '-DWITH_SRW_FLAGS', f'-I{udis}', f'-I{lib}',
                    f'-I{a.build / "SRW"}', str(ROOT / 'tools' / 'udis-scan.c'), str(a.build / 'SRW' / 'udis86_dep.c')] +
                   [str(lib / f) for f in ('decode.c', 'itab.c', 'syn.c', 'syn-intel.c', 'syn-att.c', 'udis86.c')] +
                   ['-o', str(flags_tool)], check=True)
    fl = subprocess.run([str(flags_tool), str(image), hex(pe.OPTIONAL_HEADER.ImageBase), str(analysis / 'instructions.u32')],
                        check=True, capture_output=True, text=True, env={'UDIS_SCAN_ALL': '1'}).stdout
    (analysis / 'udis-flags.txt').write_text(''.join(l + '\n' for l in fl.splitlines() if l.startswith('I ')))
    rows = [l.split() for l in out.splitlines() if l.startswith('I ')]
    total_line = next(l for l in out.splitlines() if l.startswith('TOTAL'))
    (analysis / 'udis-all.txt').write_text(''.join(' '.join(r) + '\n' for r in rows))
    # the build's patched backend, including HaloPad's added cases
    handled = set(re.findall(r'case UD_I([a-z0-9_]+):', (a.build / 'SRW' / 'SR_full_llasm_instr.c').read_text()))
    fns = sorted(int(x, 16) for x in json.loads((analysis / 'functions.json').read_text()))
    present = collections.Counter(r[3] for r in rows)
    cnt, fset, mn = collections.Counter(), collections.defaultdict(set), collections.defaultdict(collections.Counter)
    sites = collections.defaultdict(list)
    fs_funcs, fs_count = set(), 0
    img = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for _, addr, length, m, seg in rows:
        addr = int(addr, 16)
        f = fns[bisect.bisect_right(fns, addr) - 1]
        if seg != '-':
            fs_count += 1
            fs_funcs.add(f)
        if m in handled:
            continue
        ins = next(md.disasm(img[addr - base:addr - base + 16], addr, 1))
        c = family(m, ins.mnemonic)
        cnt[c] += 1
        fset[c].add(f)
        mn[c][m] += 1
        sites[c].append([hex(addr), int(length), m])
    report = {'decoder': total_line, 'mnemonics_present': len(present),
              'mnemonics_handled': sum(1 for m in present if m in handled),
              'unsupported_instances': sum(cnt.values()),
              'unsupported_functions': len(set().union(*fset.values())) if fset else 0,
              'functions_total': len(fns),
              'by_family': {c: {'instances': cnt[c], 'mnemonics': dict(mn[c]),
                                'functions': sorted(hex(f) for f in fset[c]), 'sites': sites[c]} for c in cnt},
              'fs_prefixed': {'instances': fs_count, 'functions': sorted(hex(f) for f in fs_funcs)}}
    (analysis / 'srw-unsupported.json').write_text(json.dumps(report, indent=1) + '\n')
    print(f"{total_line}; mnemonics {report['mnemonics_handled']}/{report['mnemonics_present']} handled; "
          f"unsupported {report['unsupported_instances']:,} in {report['unsupported_functions']} of {len(fns):,} functions; "
          + ', '.join(f'{c} {cnt[c]:,}' for c in sorted(cnt)) + f"; FS {fs_count} in {len(fs_funcs)} functions")
    return 0


if __name__ == '__main__':
    sys.exit(main())
