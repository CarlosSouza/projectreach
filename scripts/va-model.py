#!/usr/bin/env python3
"""G2e: rewrite a finished SRW/llasm run into HaloPad's original-address (VA) model.

In SR's pointer-offset model a guest pointer is a host address minus one offset, so
code, data and stack must share one 4 GiB host window, and code addresses stored by
the game are host-derived. In the VA model:

  * guest memory is a reserved region B; the PE image is loaded at B + original VA,
    so every guest-visible address (data pointers, function pointers, return
    addresses) is Halo's original address;
  * translated code refers to data and code *values* by original address;
  * every indirect transfer (register jump/call, return) goes through
    halopad_dispatch, which maps an original address to its compiled procedure
    through a finite table generated at build time and stops on anything unknown.

Usage: va-model.py --work generated/srw/<profile>/run-<id> --llasm <llasm binary>
Produces <work>/va/haloce.va.ll (translated code) and <work>/va/dispatch.ll.
"""
import argparse
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SUPPORT = ROOT / 'port' / 'llasm-support'
RUNTIME = ROOT / 'port' / 'llasm-runtime'
ALIASES = ROOT / 'config' / 'srw' / 'custom-en-1.0.10.0621' / 'global_aliases.sci'
REGS = {'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp', 'tmpadr', 'tmpcnd'}
HOST_RETURN_VA = 0xFFFFF000

LBL = r'(?<![\w.])((?:hp_)?loc_([0-9A-F]+))'
EXPR_PLUS = re.compile(r'\(\s*' + LBL + r'\s*\+\s*\((-?\d+)\)\s*\)')
EXPR_INDEX = re.compile(LBL + r'\[(-?\d+)\]')
PLAIN = re.compile(LBL)


def is_reg(tok):
    return tok in REGS or re.fullmatch(r'tmp\d+', tok) is not None


def value_labels(line):
    line = EXPR_PLUS.sub(lambda m: hex(int(m.group(2), 16) + int(m.group(3))), line)
    line = EXPR_INDEX.sub(lambda m: hex(int(m.group(2), 16) + int(m.group(3))), line)
    return PLAIN.sub(lambda m: hex(int(m.group(2), 16)), line)


def transform_code(lines):
    """Yield VA-model llasm lines. Counts rewritten forms."""
    stats = {'register-transfers': 0, 'value-labels': 0}
    out = []
    for raw in lines:
        line = raw.rstrip('\n')
        s = line.strip()
        if not s or s.startswith(';'):
            out.append(line)
            continue
        w = s.split()
        op = w[0]
        if op == 'tcall' and len(w) >= 2 and is_reg(w[1]):
            out.append(f'PUSH {w[1]}')
            out.append('tcall halopad_dispatch')
            stats['register-transfers'] += 1
            continue
        if op in ('tcall', 'ctcallz', 'ctcallnz', 'proc', 'endp'):
            if op.startswith('ctcall') and len(w) >= 3 and is_reg(w[-1]):
                raise SystemExit(f'conditional register transfer not supported: {s}')
            out.append(line)
            continue
        if op.startswith(('load', 'store')) and len(w) >= 3:
            # llasm load/store take a register or label as the address, not a constant:
            # materialize the original address in tmp19 (unused by SRW output).
            parts = [x.strip() for x in s[len(op):].split(',')]
            if len(parts) >= 2 and PLAIN.search(parts[1]):
                addr = value_labels(parts[1])
                parts[0] = value_labels(parts[0])
                out.append(f'mov tmp19, {addr}')
                out.append(f'{op} {parts[0]}, tmp19, ' + ', '.join(parts[2:]))
                stats['value-labels'] += 1
                continue
        new = value_labels(line)
        if new != line:
            stats['value-labels'] += 1
        out.append(new)
    return out, stats


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=pathlib.Path, required=True)
    ap.add_argument('--llasm', type=pathlib.Path, required=True)
    a = ap.parse_args()
    work = a.work.resolve()
    a.llasm = a.llasm.resolve()
    va = work / 'va'
    va.mkdir(exist_ok=True)

    code, stats = transform_code((work / 'seg01_code.llinc').open(errors='replace'))
    (va / 'seg01_code.va.llinc').write_text('\n'.join(code) + '\n')

    main_src = (work / 'haloce.llasm').read_text().splitlines()
    kept, skip = [], False
    for line in main_src:
        if line.startswith('datasegment '):
            skip = True
            continue
        if skip:
            if line.startswith('endd'):
                skip = False
            continue
        kept.append('include seg01_code.va.llinc' if line == 'include seg01_code.llinc' else line)
    (va / 'haloce.va.llasm').write_text('\n'.join(kept) + '\n')
    extern = (work / 'extern.llinc').read_text() + 'proc halopad_dispatch external\n'
    (va / 'extern.llinc').write_text(extern)
    (va / 'macros.llinc').write_text('')

    # Runtime llasm (imports) and SR's call macros: same transfer rewrite.
    calls = (RUNTIME / 'asm-calls.llinc').read_text().replace('    tcall tmp0\n', '    PUSH tmp0\n    tcall halopad_dispatch\n')
    (va / 'asm-calls.llinc').write_text(calls)
    runtime_ll = []
    for src in sorted(RUNTIME.glob('*.llasm')):
        text, _ = transform_code(src.read_text().splitlines(True))
        dst = va / src.name
        dst.write_text('proc halopad_dispatch external\n' + '\n'.join(text) + '\n')
        runtime_ll.append(dst)

    def llasm(src, out):
        p = subprocess.run([str(a.llasm), '-m64', '-ptrofs', '-I', str(va), '-I', str(SUPPORT), '-o', str(out), str(src)],
                           cwd=va, capture_output=True, text=True)
        if p.returncode:
            sys.exit(f'llasm failed on {src.name}:\n{p.stdout[-800:]}{p.stderr[-800:]}')

    llasm(va / 'haloce.va.llasm', va / 'haloce.va.raw.ll')
    for src in runtime_ll:
        llasm(src, va / (src.stem + '.ll'))

    # Procedures must be visible to the dispatch module.
    triple = next(l for l in (work / 'haloce.target.ll').open() if l.startswith('target triple'))
    procs = []
    with open(va / 'haloce.va.raw.ll') as fin, open(va / 'haloce.va.ll', 'w') as fout:
        fout.write(triple)
        for line in fin:
            if line.startswith('define private fastcc void @'):
                line = line.replace('define private fastcc', 'define hidden fastcc', 1)
            m = re.match(r'define (?:hidden|protected) fastcc void @([^(]+)\(', line)
            if m:
                procs.append(m.group(1))
            fout.write(line)
    for src in runtime_ll:
        ll = va / (src.stem + '.ll')
        ll.write_text(triple + ll.read_text())

    aliases = {}
    for line in ALIASES.read_text().splitlines():
        if line.startswith('loc_'):
            addr, name = line.split(',')
            aliases[name.strip()] = int(addr[4:], 16)
    table = {}
    for name in procs:
        m = re.fullmatch(r'(?:hp_)?loc_([0-9A-F]+)', name)
        vaddr = int(m.group(1), 16) if m else aliases.get(name)
        if vaddr is None:
            continue
        if vaddr in table:
            sys.exit(f'duplicate dispatch address {vaddr:#x}: {table[vaddr]} and {name}')
        table[vaddr] = name
    entries = sorted(table.items())
    ll = [triple.rstrip(), '', '; Generated by scripts/va-model.py: original address -> compiled procedure.']
    for _, name in entries:
        ll.append(f'declare hidden fastcc void @{name}(ptr)')
    ll.append(f'@halopad_dispatch_count = constant i32 {len(entries)}')
    ll.append(f'@halopad_dispatch_vas = constant [{len(entries)} x i32] [' + ', '.join(f'i32 {v}' for v, _ in entries) + ']')
    ll.append(f'@halopad_dispatch_fns = constant [{len(entries)} x ptr] [' + ', '.join(f'ptr @{n}' for _, n in entries) + ']')
    ll.append(f'''
declare ptr @halopad_lookup(i32)

; Pop the original target address from the guest stack and continue there.
define hidden fastcc void @halopad_dispatch(ptr %cpu) nounwind {{
  %espp = getelementptr i8, ptr %cpu, i64 16
  %esp = load i32, ptr %espp
  %ofsp = getelementptr i8, ptr %cpu, i64 112
  %ofs = load i64, ptr %ofsp
  %e64 = zext i32 %esp to i64
  %addr = add i64 %ofs, %e64
  %ap = inttoptr i64 %addr to ptr
  %target = load i32, ptr %ap, align 1
  %esp2 = add i32 %esp, 4
  store i32 %esp2, ptr %espp
  %fn = call ptr @halopad_lookup(i32 %target)
  musttail call fastcc void %fn(ptr %cpu)
  ret void
}}

; Reached when guest code returns to the sentinel pushed by halopad_enter.
define hidden fastcc void @halopad_return_to_host(ptr %cpu) nounwind {{
  ret void
}}

; Host -> guest: push the host-return sentinel and run the procedure at 'va'.
define void @halopad_enter(ptr %cpu, i32 %va) nounwind {{
  %espp = getelementptr i8, ptr %cpu, i64 16
  %esp = load i32, ptr %espp
  %esp2 = sub i32 %esp, 4
  store i32 %esp2, ptr %espp
  %ofsp = getelementptr i8, ptr %cpu, i64 112
  %ofs = load i64, ptr %ofsp
  %e64 = zext i32 %esp2 to i64
  %addr = add i64 %ofs, %e64
  %ap = inttoptr i64 %addr to ptr
  store i32 {HOST_RETURN_VA}, ptr %ap, align 1
  %fn = call ptr @halopad_lookup(i32 %va)
  call fastcc void %fn(ptr %cpu)
  ret void
}}

define ptr @halopad_return_to_host_address() {{
  ret ptr @halopad_return_to_host
}}
''')
    (va / 'dispatch.ll').write_text('\n'.join(ll) + '\n')
    print(f"VA model: {stats['register-transfers']:,} register transfers via dispatch, "
          f"{stats['value-labels']:,} lines with label values rewritten, {len(entries):,} dispatch entries")
    return 0


if __name__ == '__main__':
    sys.exit(main())
