#!/usr/bin/env python3
"""G2d scoping experiment: are Halo's SIMD implementations only selected by CPU features?

Runs the code that installs function pointers to SIMD routines inside the x86
oracle with CPUID controlled, under three simulated CPUs:
  plain   GenuineIntel, x87 + TSC + CX8 + CMOV only (no MMX/SSE/3DNow!)
  sse2    GenuineIntel Pentium 4 class (MMX, FXSR, SSE, SSE2)
  athlon  AuthenticAMD Athlon class (MMX, MMX-ext, 3DNow!, 3DNow!-ext)
and records every code pointer written. Pass for the hypothesis: under 'plain'
no written pointer targets a SIMD-containing function, while 'sse2'/'athlon'
(positive controls) do install SIMD routines.
"""
import bisect
import collections
import datetime
import importlib.util
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
A = ROOT / 'generated' / 'analysis' / 'custom-en-1.0.10.0621'
spec = importlib.util.spec_from_file_location('x86_oracle', ROOT / 'scripts' / 'x86-oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)

FPU, TSC, CX8, CMOV, MMX, FXSR, SSE, SSE2 = 1 << 0, 1 << 4, 1 << 8, 1 << 15, 1 << 23, 1 << 24, 1 << 25, 1 << 26
MMXEXT, AMD3DNOWEXT, AMD3DNOW = 1 << 22, 1 << 30, 1 << 31


def vendor(s):
    b = s.encode()
    w = [int.from_bytes(b[i:i + 4], 'little') for i in (0, 4, 8)]
    return w[0], w[2], w[1]  # ebx, ecx, edx order for "GenuineIntel": ebx=Genu, edx=ineI, ecx=ntel


PROFILES = {
    'plain': ('GenuineIntel', 0x633, FPU | TSC | CX8 | CMOV, 0),
    'sse2': ('GenuineIntel', 0xF29, FPU | TSC | CX8 | CMOV | MMX | FXSR | SSE | SSE2, 0),
    'athlon': ('AuthenticAMD', 0x642, FPU | TSC | CX8 | CMOV | MMX, MMX | MMXEXT | AMD3DNOW | AMD3DNOWEXT),
}


def cpuid_for(name):
    v, sig, edx1, edx_ext = PROFILES[name]
    ebx, ecx, edx = vendor(v)

    def cpuid(leaf, sub):
        if leaf == 0:
            return 1, ebx, ecx, edx
        if leaf == 1:
            return sig, 0, 0, edx1
        if leaf == 0x80000000:
            return 0x80000001, 0, 0, 0
        if leaf == 0x80000001:
            return sig, 0, 0, edx_ext
        return 0, 0, 0, 0
    return cpuid


def main():
    reach = json.loads((A / 'simd-reachability.json').read_text())
    fns = sorted(int(x, 16) for x in json.loads((A / 'functions.json').read_text()))
    simd = {int(f['function'], 16) for f in reach['functions']}
    fn = lambda a: fns[bisect.bisect_right(fns, a) - 1]
    text_lo, text_hi = 0x401000, 0x401000 + 0x1dd156
    installers = sorted({int(r[1], 16) for f in reach['functions'] if f['class'] == 'pointer-only'
                         for r in [[k, hex(fn(int(a, 16)))] for k, a in f['pointer_refs'] if k == 'code-imm']})
    image = oracle.Image()
    results = {}
    for name in PROFILES:
        per = {}
        for inst in installers:
            o = oracle.Oracle(image, cpuid=cpuid_for(name), max_instructions=5_000_000)
            try:
                obs = o.call(inst, [('alloc', 0x800), ('alloc', 0x800), 0, 0], 'cdecl')
                writes = obs['writes']
                status = 'returned'
            except oracle.OracleError as exc:
                writes, status = [], 'stopped: ' + str(exc)[:160]
            code_ptrs = sorted({v for _, size, v in writes if size == 4 and text_lo <= v < text_hi})
            simd_ptrs = sorted(v for v in code_ptrs if fn(v) in simd)
            per[hex(inst)] = {'status': status, 'cpuid_leaves': sorted(set(o.cpuid_calls)),
                              'code_pointers_written': len(code_ptrs),
                              'simd_pointers_written': [hex(v) for v in simd_ptrs]}
        results[name] = per
    # Phase 2: the real selector (0x5953cd) end to end. IsProcessorFeaturePresent answers
    # from the simulated CPU; RegOpenKeyA reports ERROR_FILE_NOT_FOUND (no configuration).
    import struct
    SELECTOR, TABLE, ENTRIES = 0x5953CD, 0x615088, 0x47
    PF = {'plain': set(), 'sse2': {6, 10}, 'athlon': {7}}
    selector = {}
    for name in PROFILES:
        def ipfp(uc, name=name):
            esp = uc.reg_read(oracle.X.UC_X86_REG_ESP)
            feature = struct.unpack('<I', uc.mem_read(esp + 4, 4))[0]
            selector_calls.append(feature)
            return (1 if feature in PF[name] else 0), 4
        selector_calls = []
        handlers = {'KERNEL32.dll!IsProcessorFeaturePresent': ipfp,
                    'ADVAPI32.dll!RegOpenKeyA': lambda uc: (2, 12)}
        o = oracle.Oracle(image, handlers, cpuid=cpuid_for(name), max_instructions=5_000_000)
        try:
            obs = o.call(SELECTOR, [1], 'stdcall')  # 1 = enable CPU-specific routines; 0 restores generic
            status = 'returned'
            final = {}
            for addr, size, value in obs['writes']:
                if size == 4 and TABLE <= addr < TABLE + 4 * ENTRIES:
                    final[addr] = value
            initial = {TABLE + 4 * i: struct.unpack_from('<I', image.mapped, TABLE - image.base + 4 * i)[0]
                       for i in range(ENTRIES)}
            table = {a: final.get(a, initial[a]) for a in initial}
        except oracle.OracleError as exc:
            status, table = 'stopped: ' + str(exc)[:200], {}
        simd_entries = sorted(hex(v) for v in table.values() if text_lo <= v < text_hi and fn(v) in simd)
        selector[name] = {'status': status, 'is_processor_feature_present': selector_calls,
                          'cpuid_leaves': sorted(set(o.cpuid_calls)), 'table_entries': len(table),
                          'simd_entries': simd_entries}
        print(f"selector {name:7} {status[:60]:60} IsProcessorFeaturePresent{selector_calls} "
              f"table {len(table)} entries, SIMD {len(simd_entries)}")
    results['selector_0x5953cd'] = selector

    # Phase 3: Halo's own feature query (0x5436a0) after its CPUID detection (0x5441a0),
    # run in one machine through a small call snippet. 0x4d07f0 installs the SSE routine
    # 0x4cf650 only if query(0x1d) != 0 and the 3DNow!/MMX routine 0x4cf7a0 only if query(0x1a) != 0.
    def snippet(calls):
        code, base = b'', oracle.HEAP_BASE
        for target, args in calls:
            for a in reversed(args):
                code += b'\x68' + struct.pack('<I', a)
            nxt = base + len(code) + 5
            code += b'\xe8' + struct.pack('<i', target - nxt)
            if args:
                code += b'\x83\xc4' + bytes([4 * len(args)])
        return code + b'\xc3'
    halo = {}
    for name in PROFILES:
        answers = {}
        for feature in (0x1D, 0x1A):
            o = oracle.Oracle(image, cpuid=cpuid_for(name))
            answers[hex(feature)] = o.call(oracle.HEAP_BASE, [snippet([(0x5441A0, []), (0x5436A0, [feature])])], 'cdecl')['eax']
        halo[name] = answers
        print(f'halo query {name:7} {answers}')
    results['halo_feature_query'] = halo

    # Phase 4: MMX installers 0x589c3c / 0x589c74. Detection (0x595278) reads CPUID, the
    # registry (absent: ERROR_FILE_NOT_FOUND) and GetSystemInfo (single x86 processor).
    def get_system_info(uc):
        esp = uc.reg_read(oracle.X.UC_X86_REG_ESP)
        ptr = struct.unpack('<I', uc.mem_read(esp + 4, 4))[0]
        uc.mem_write(ptr, struct.pack('<HHIIIIIIIHH', 0, 0, 4096, 0x10000, 0x7FFEFFFF, 1, 1, 586, 0x10000, 6, 0x0803))
        return 0, 4
    mmx = {}
    for name in PROFILES:
        per = {}
        for inst in (0x589C3C, 0x589C74):
            o = oracle.Oracle(image, {'ADVAPI32.dll!RegOpenKeyA': lambda uc: (2, 12),
                                      'ADVAPI32.dll!RegOpenKeyExA': lambda uc: (2, 20),
                                      'KERNEL32.dll!GetSystemInfo': get_system_info}, cpuid=cpuid_for(name))
            obs = o.call(inst, [], 'cdecl')
            installed = sorted({v for a, s, v in obs['writes'] if s == 4 and text_lo <= v < text_hi})
            per[hex(inst)] = {'installed': [hex(v) for v in installed],
                              'simd': [hex(v) for v in installed if fn(v) in simd]}
        mmx[name] = per
        print(f"mmx installers {name:7} " + ', '.join(f"{k}: SIMD {len(v['simd'])} of {len(v['installed'])}" for k, v in per.items()))
    results['mmx_installers'] = mmx
    for name, per in results.items():
        if name in ('selector_0x5953cd', 'halo_feature_query', 'mmx_installers'):
            continue
        total = sum(len(v['simd_pointers_written']) for v in per.values())
        print(f'{name:7} SIMD pointers installed: {total}')
        for inst, v in per.items():
            print(f"   {inst} {v['status'][:70]:70} code ptrs {v['code_pointers_written']:3} simd {len(v['simd_pointers_written'])} cpuid {v['cpuid_leaves']}")
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / 'G2d' / f'simd-dispatch-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    (evid / 'results.json').write_text(json.dumps({'profiles': {k: [PROFILES[k][0], hex(PROFILES[k][2]), hex(PROFILES[k][3])] for k in PROFILES},
                                                   'installers': [hex(i) for i in installers], 'results': results}, indent=1) + '\n')
    print('evidence', evid.relative_to(ROOT))
    return 0


if __name__ == '__main__':
    sys.exit(main())
