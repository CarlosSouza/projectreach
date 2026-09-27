#!/usr/bin/env python3
"""Generate LLVM IR stubs for external procedures in llasm output that no linked
support file defines. Each stub calls halopad_missing_import(name) and never returns.
Extra inputs (e.g. the VA model's dispatch.ll) add declarations; anything defined in
any input gets no stub.

Usage: gen-import-stubs.py <haloce.target.ll> <out.ll> [more.ll ...]
"""
import re
import sys


def main():
    src, out, extra = sys.argv[1], sys.argv[2], sys.argv[3:]
    declared, defined = set(), set()
    for path in [src, *extra]:
        text = open(path, errors='replace').read()
        declared |= set(re.findall(r'^declare hidden fastcc void @([A-Za-z0-9_$@?.]+)\((?:%_cpu\*|ptr)\)', text, re.M))
        defined |= set(re.findall(r'^define [^@\n]*@([A-Za-z0-9_$@?.]+)\(', text, re.M))
    names = sorted(declared - defined)
    # <name>_asm2c procedures are implemented by HaloPad's llasm runtime object.
    # halopad_* procedures come from the dispatch module (VA model).
    names = [n for n in names if not n.endswith('_asm2c') and not n.startswith('halopad_')]
    lines = ['declare void @halopad_missing_import(ptr)', 'declare void @halopad_missing_method(ptr)', '']
    for i, n in enumerate(names):
        shown = n[len('hpimp_'):] if n.startswith('hpimp_') else n
        if n.startswith('hpcom_'):
            iface, _, method = n[len('hpcom_'):].partition('_')
            shown = f'{iface}::{method}'
        s = shown.encode() + b'\0'
        lines.append(f'@.hp_name_{i} = private unnamed_addr constant [{len(s)} x i8] c"{shown}\\00"')
        lines.append(f'define hidden fastcc void @{n}(ptr %cpu) {{')
        lines.append(f'  call void @halopad_missing_{"method" if n.startswith("hpcom_") else "import"}(ptr @.hp_name_{i})')
        lines.append('  unreachable')
        lines.append('}')
    open(out, 'w').write('\n'.join(lines) + '\n')
    print(f'{len(names)} import stubs')


if __name__ == '__main__':
    main()
