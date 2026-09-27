#!/usr/bin/env python3
"""Generate LLVM IR stubs for external procedures in llasm output that no linked
support file defines. Each stub calls halopad_missing_import(name) and never returns.

Usage: gen-import-stubs.py <haloce.target.ll> <out.ll>
"""
import re
import sys


def main():
    src, out = sys.argv[1], sys.argv[2]
    text = open(src, errors='replace').read()
    names = sorted(set(re.findall(r'^declare hidden fastcc void @([A-Za-z0-9_$@?.]+)\(%_cpu\*\)', text, re.M)))
    # <name>_asm2c procedures are implemented by HaloPad's llasm runtime object.
    # halopad_* procedures come from the dispatch module (VA model).
    names = [n for n in names if not n.endswith('_asm2c') and not n.startswith('halopad_')]
    lines = ['declare void @halopad_missing_import(ptr)', '']
    for i, n in enumerate(names):
        s = n.encode() + b'\0'
        lines.append(f'@.hp_name_{i} = private unnamed_addr constant [{len(s)} x i8] c"{n}\\00"')
        lines.append(f'define hidden fastcc void @{n}(ptr %cpu) {{')
        lines.append(f'  call void @halopad_missing_import(ptr @.hp_name_{i})')
        lines.append('  unreachable')
        lines.append('}')
    open(out, 'w').write('\n'.join(lines) + '\n')
    print(f'{len(names)} import stubs')


if __name__ == '__main__':
    main()
