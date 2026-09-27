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
  * every static import has a fixed guest address in a reserved, never-mapped page
    (IMPORT_VA_BASE + 16*i). Halo's import address table is filled with these at
    load time and translated code that loads an import as a value gets the same
    address, so 'mov esi, [__imp_X]; call esi' reaches X through dispatch.
  * no host code address ever becomes a guest value; the build fails if one does.

Usage: va-model.py --work generated/srw/<profile>/run-<id> --llasm <llasm binary>
Produces <work>/va/haloce.va.ll (translated code) and <work>/va/dispatch.ll.
"""
import argparse
import filecmp
import pathlib
import re
import shutil
import subprocess
import sys

import pefile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import hpmodule  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
SUPPORT = ROOT / 'port' / 'llasm-support'
RUNTIME = ROOT / 'port' / 'llasm-runtime'
ALIASES = ROOT / 'config' / 'srw' / 'custom-en-1.0.10.0621' / 'global_aliases.sci'
REGS = {'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp', 'tmpadr', 'tmpcnd'}
HOST_RETURN_VA = 0xFFFFF000
IMPORT_VA_BASE = 0xFFFE0000
IMPORT_VA_STRIDE = 16
IMPORT_PREFIX = 'hpimp_'
COM_VA_BASE = 0xFFFA0000
COM_INTERFACES = pathlib.Path(__file__).resolve().parents[1] / 'config' / 'runtime' / 'com-interfaces.txt'
DYNAMIC_EXPORTS = pathlib.Path(__file__).resolve().parents[1] / 'config' / 'runtime' / 'dynamic-exports.txt'
TRANSFER_OPS = ('tcall', 'ctcallz', 'ctcallnz', 'proc', 'endp')

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


IDENT = re.compile(r'(?<![\w$@?.])[A-Za-z_][\w$@?]*(?![\w$@?])')


def value_imports(line, import_vas):
    """Replace import names used as values (not as transfer targets) by their guest address."""
    return IDENT.sub(lambda m: hex(import_vas[m.group(0)]) if m.group(0) in import_vas else m.group(0), line)


def static_imports(exe, known):
    """Static IAT slots in slot order: (slot VA, import name as SRW names it, DLL)."""
    pe = pefile.PE(str(exe), fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
    out = []
    for e in pe.DIRECTORY_ENTRY_IMPORT:
        for imp in e.imports:
            # Ordinals pefile cannot name get SRW's loader name <DLL stem>_ord<n>
            # (port/patches: e.g. WINSPOOL.DRV ordinal 203 -> WINSPOOL_ord203).
            name = imp.name.decode() if imp.name else f'{e.dll.decode().split(".")[0]}_ord{imp.ordinal}'
            if name not in known:
                sys.exit(f'import {e.dll.decode()}!{name or imp.ordinal} has no SRW procedure name')
            out.append((imp.address, name, e.dll.decode().lower()))
    return sorted(out)


def symbol_for(name):
    """Link symbol for an export name (delay-loaded names such as _BinkOpen@8 are not identifiers)."""
    return IMPORT_PREFIX + re.sub(r'[^A-Za-z0-9_]', lambda m: '_%02x' % ord(m.group()), name)


LLVM_IDENT = re.compile(r'[A-Za-z$._][\w$.]*')


def import_symbol(name):
    """Link symbol for an import: hpimp_<name>, sanitized when the name is not an LLVM identifier."""
    return IMPORT_PREFIX + name if LLVM_IDENT.fullmatch(name) else symbol_for(name)


def pe_exports(exe):
    pe = pefile.PE(str(exe), fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
    base = pe.OPTIONAL_HEADER.ImageBase
    return {e.name.decode(): base + e.address for e in getattr(pe, 'DIRECTORY_ENTRY_EXPORT', None).symbols if e.name} \
        if hasattr(pe, 'DIRECTORY_ENTRY_EXPORT') else {}


def latest_module_run(mod):
    runs = sorted((ROOT / 'generated' / 'srw' / hpmodule.PROFILE_ID / 'modules' / mod.name).glob('run-*/'),
                  key=lambda p: p.stat().st_mtime)
    runs = [r for r in runs if (r / f'{mod.stem}.ll').exists()]      # finished pipeline runs only
    if not runs:
        sys.exit(f'no finished SRW run for module {mod.name}; run scripts/srw-pipeline.sh <build> --module {mod.name}')
    return runs[-1]


def dynamic_exports(exe):
    """Exports reached only through LoadLibraryA/GetProcAddress: the delay-load imports and
    the names listed in config/runtime/dynamic-exports.txt. Returns [(dll, name)] where an
    ordinal import is named '#<ordinal>'."""
    pe = pefile.PE(str(exe), fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT']])
    out = []
    for e in getattr(pe, 'DIRECTORY_ENTRY_DELAY_IMPORT', []):
        for imp in e.imports:
            out.append((e.dll.decode().lower(), imp.name.decode() if imp.name else f'#{imp.ordinal}'))
    for line in DYNAMIC_EXPORTS.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith('#'):
            dll, name = line.split('!', 1)
            out.append((dll.lower(), name))
    return out


def com_interfaces():
    """[(interface, [method, ...])] in vtable order from config/runtime/com-interfaces.txt."""
    out = []
    for line in COM_INTERFACES.read_text().splitlines():
        s = line.strip()
        if not s or s.startswith('#'):
            continue
        if s.startswith('interface '):
            out.append((s.split()[1], []))
        else:
            out[-1][1].append(s)
    return out


def transform_code(lines, import_vas=None):
    """Yield VA-model llasm lines. Counts rewritten forms."""
    stats = {'register-transfers': 0, 'value-labels': 0, 'import-values': 0}
    import_vas = import_vas or {}
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
        if op in TRANSFER_OPS:
            if op.startswith('ctcall') and len(w) >= 3 and is_reg(w[-1]):
                raise SystemExit(f'conditional register transfer not supported: {s}')
            out.append(line)
            continue
        if import_vas:
            new = value_imports(line, import_vas)
            if new != line:
                stats['import-values'] += 1
                line, s = new, new.strip()
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


def postprocess_ll(raw, out, triple):
    """Drop llasm's C entry wrappers, make procedures visible to the dispatch module, and
    refuse host code addresses as guest values. Returns procedure names and dropped count."""
    procs, dropped, in_wrapper = [], 0, False
    new = out.with_suffix('.ll.new')
    with open(raw) as fin, open(new, 'w') as fout:
        fout.write(triple)
        for line in fin:
            if in_wrapper:
                in_wrapper = line.rstrip('\n') != '}'
                continue
            if line.startswith('define protected ccc void @c_'):
                in_wrapper, dropped = True, dropped + 1
                continue
            if line.startswith('define private fastcc void @'):
                line = line.replace('define private fastcc', 'define hidden fastcc', 1)
            if line.startswith('define hidden fastcc void @__return_procedure('):
                # llasm's helper for the removed C wrappers; every module defines one
                line = line.replace('define hidden', 'define internal', 1)
            m = re.match(r'define (?:hidden|protected) fastcc void @([^(]+)\(', line)
            if m:
                procs.append(m.group(1))
            if 'ptrtoint' in line and re.search(r'ptrtoint void\s*\(%_cpu\*\)\*', line):
                sys.exit(f'host code address would become a guest value: {line.strip()[:160]}')
            fout.write(line)
    if out.exists() and filecmp.cmp(new, out, shallow=False):
        new.unlink()
    else:
        new.replace(out)
    return procs, dropped


def build_modules(a, va, llasm, triple, registry, seen, table):
    """Translate each non-primary module into va/<name>/<name>.va.ll. Adds its procedures to
    the dispatch table and its non-module imports to the export registry; returns module
    records for module_tables()."""
    overrides = dict(x.split('=', 1) for x in a.module_work)
    mods = [m for m in hpmodule.all_modules() if not m.primary]
    exports = {m.file.lower(): pe_exports(m.exe) for m in mods}
    records = []
    for mod in mods:
        mw = pathlib.Path(overrides[mod.name]).resolve() if mod.name in overrides else latest_module_run(mod)
        out = va / mod.name
        out.mkdir(exist_ok=True)
        main_src = (mw / f'{mod.stem}.llasm').read_text().splitlines()
        extern_src = (mw / 'extern.llinc').read_text()
        # Export and alias names SRW gave procedures (define loc_X NAME) go back to loc_X:
        # C++ export names are not LLVM identifiers, and dispatch works by address.
        named = {}
        for line in main_src:
            m = re.fullmatch(r'define loc_([0-9A-F]+) (\S+)', line)
            if m:
                named[m.group(2)] = f'loc_{m.group(1)}'
        kept_extern = [l for l in extern_src.splitlines()
                       if not re.fullmatch(r'define (\S+) (?:\1_asm2c|hpimp_\1)', l)
                       and not re.fullmatch(r'proc (?:\S+_asm2c|hpimp_\S+) external', l)]
        known = {m.group(1) for m in re.finditer(r'^proc (\S+) external', '\n'.join(main_src) + '\n' + extern_src, re.M)}
        known |= set(re.findall(r'^define (\S+) (?:\S+_asm2c|hpimp_\S+)$', extern_src, re.M))
        imports = static_imports(mw / mod.file, known)
        values, redirect, forwarders = {}, {}, {}
        for slot, name, dll in imports:
            if dll in exports:                              # another translated module
                if name not in exports[dll]:
                    sys.exit(f'{mod.file} imports {dll}!{name}, which it does not export')
                values[name] = exports[dll][name]
                sym = symbol_for('fwd_' + name)
                forwarders[sym] = exports[dll][name]
                redirect[name] = sym
            else:
                if (dll, name) not in seen:
                    seen.add((dll, name))
                    registry.append((dll, name, import_symbol(name)))
                i = next(k for k, (d, n, _) in enumerate(registry) if (d, n) == (dll, name))
                values[name] = IMPORT_VA_BASE + IMPORT_VA_STRIDE * i
                redirect[name] = registry[i][2]
        body = '\n'.join(main_src + kept_extern)
        extern_out = '\n'.join([f'define {n} {redirect[n]}' for n in sorted(redirect)] + kept_extern
                               + [f'proc {sym} external' for sym in sorted(set(redirect.values()))
                                  if not any(re.search(rf'^proc {re.escape(n)} external', body, re.M)
                                             for n in redirect if redirect[n] == sym)]) + '\n'
        (out / 'extern.llinc').write_text(extern_out + 'proc halopad_dispatch external\n')
        (out / 'macros.llinc').write_text('')
        name_re = re.compile(r'(?<![\w$@?.])(' + '|'.join(re.escape(n) for n in sorted(named, key=len, reverse=True)) + r')(?![\w$@?])') \
            if named else None
        def unname(lines):
            for line in lines:
                yield name_re.sub(lambda m: named[m.group(1)], line) if name_re else line
        code, _ = transform_code(unname((mw / 'seg01_code.llinc').open(errors='replace')), values)
        (out / 'seg01_code.va.llinc').write_text('\n'.join(code) + '\n')
        kept, skip = [], False
        for line in main_src:
            if line.startswith('datasegment '):
                skip = True
                continue
            if skip:
                if line.startswith('endd'):
                    skip = False
                continue
            if re.fullmatch(r'define loc_[0-9A-F]+ \S+', line):
                continue
            kept.append('include seg01_code.va.llinc' if line == 'include seg01_code.llinc' else line)
        (out / f'{mod.stem}.va.llasm').write_text('\n'.join(kept) + '\n')
        p = subprocess.run([str(a.llasm), '-m64', '-ptrofs', '-I', str(out), '-I', str(SUPPORT), '-o',
                            str(out / f'{mod.stem}.va.raw.ll'), str(out / f'{mod.stem}.va.llasm')],
                           cwd=out, capture_output=True, text=True)
        if p.returncode:
            sys.exit(f'llasm failed on {mod.name}:\n{p.stdout[-800:]}{p.stderr[-800:]}')
        procs, dropped = postprocess_ll(out / f'{mod.stem}.va.raw.ll', out / f'{mod.stem}.va.ll', triple)
        (out / f'{mod.stem}.va.raw.ll').unlink()
        n = 0
        for pname in procs:
            m = re.fullmatch(r'(?:hp_)?loc_([0-9A-F]+)', pname)
            if not m:
                continue
            vaddr = int(m.group(1), 16)
            if not mod.base <= vaddr < mod.base + mod.size:
                sys.exit(f'{mod.name}: procedure {pname} outside the module image')
            if vaddr in table:
                sys.exit(f'duplicate dispatch address {vaddr:#x}: {table[vaddr]} and {pname}')
            table[vaddr] = pname
            n += 1
        records.append({'mod': mod, 'slots': [(slot, values[name]) for slot, name, _ in imports],
                         'forwarders': forwarders, 'procs': n, 'run': mw})
        print(f'module {mod.name}: {n:,} procedures, {len(imports)} imports ({len(forwarders)} from translated modules), '
              f'{dropped} C entry wrappers removed, run {mw.name}')
    return records


def module_tables(records, registry):
    """dispatch.ll data for the runtime loader (port/runtime/halopad_modules.c)."""
    ll = ['', '; Translated DLLs: name, base, size, entry, import slots (slot VA -> bound value).']
    n = len(records)
    ll.append(f'@halopad_tmodule_count = constant i32 {n}')
    slots, first = [], []
    for r in records:
        first.append(len(slots))
        slots.extend(r['slots'])
    for i, r in enumerate(records):
        nm = r['mod'].file                                  # the file's own spelling (GetModuleFileNameA)
        ll.append(f'@.hp_tmod_{i} = private unnamed_addr constant [{len(nm) + 1} x i8] c"{nm}\\00"')
    arr = lambda ty, xs: f'[{len(xs)} x {ty}] [' + ', '.join(f'{ty} {x}' for x in xs) + ']' if xs else f'[0 x {ty}] zeroinitializer'
    ll.append(f'@halopad_tmodule_names = constant ' + arr('ptr', [f'@.hp_tmod_{i}' for i in range(n)]))
    for i, r in enumerate(records):
        pth = r['mod'].relpath.replace('/', '\\5C')                 # LLVM's escape for a backslash
        ll.append(f'@.hp_tpath_{i} = private unnamed_addr constant [{len(r["mod"].relpath) + 1} x i8] c"{pth}\\00"')
    ll.append(f'@halopad_tmodule_paths = constant ' + arr('ptr', [f'@.hp_tpath_{i}' for i in range(n)]))
    ll.append(f'@halopad_tmodule_bases = constant ' + arr('i32', [r['mod'].base for r in records]))
    ll.append(f'@halopad_tmodule_sizes = constant ' + arr('i32', [r['mod'].size for r in records]))
    ll.append(f'@halopad_tmodule_entries = constant ' + arr('i32', [r['mod'].entry for r in records]))
    ll.append(f'@halopad_tmodule_slot_first = constant ' + arr('i32', first))
    ll.append(f'@halopad_tmodule_slot_count = constant ' + arr('i32', [len(r['slots']) for r in records]))
    ll.append(f'@halopad_tmodule_slot_vas = constant ' + arr('i32', [s for s, _ in slots]))
    ll.append(f'@halopad_tmodule_slot_values = constant ' + arr('i32', [v for _, v in slots]))
    # Calls from one translated module into another's exports go straight to the
    # exporting procedure (the import slot holds the export's own address).
    forwarders = {}                                         # one per export, whoever imports it
    for r in records:
        forwarders.update(r['forwarders'])
    for sym, target in sorted(forwarders.items()):
        ll.append(f'define hidden fastcc void @{sym}(ptr %cpu) nounwind {{')
        ll.append(f'  musttail call fastcc void @loc_{target:X}(ptr %cpu)')
        ll.append('  ret void')
        ll.append('}')
    return ll


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=pathlib.Path, required=True)
    ap.add_argument('--llasm', type=pathlib.Path, required=True)
    ap.add_argument('--module-work', action='append', default=[], metavar='NAME=RUN_DIR',
                    help='SRW run to use for a translated DLL (default: its latest finished run)')
    a = ap.parse_args()
    work = a.work.resolve()
    a.llasm = a.llasm.resolve()
    va = work / 'va'
    va.mkdir(exist_ok=True)

    main_src = (work / 'haloce.llasm').read_text().splitlines()
    extern_src = (work / 'extern.llinc').read_text()
    # Every import is reached through the stable symbol hpimp_<name>. The link resolves
    # it to HaloPad's service (port/llasm-runtime, 'proc hpimp_<name> public') or to a
    # stub that stops with the name, so adding a service only needs a relink, and import
    # names never collide with host symbols (send, recv, bind, select, ...).
    kept_extern = [l for l in extern_src.splitlines()
                   if not re.fullmatch(r'define (\S+) (?:\1_asm2c|hpimp_\1)', l)
                   and not re.fullmatch(r'proc (?:\S+_asm2c|hpimp_\S+) external', l)]
    known = {m.group(1) for m in re.finditer(r'^proc (\S+) external', '\n'.join(main_src) + '\n' + extern_src, re.M)}
    known |= set(re.findall(r'^define (\S+) (?:\S+_asm2c|hpimp_\S+)$', extern_src, re.M))
    imports = static_imports(work / 'haloce.exe', known)
    redirect = {name: IMPORT_PREFIX + name for _, name, _ in imports}
    extern_src = '\n'.join([f'define {n} {redirect[n]}' for n in sorted(redirect)] + kept_extern
                           + [f'proc {redirect[n]} external' for n in sorted(redirect)
                              if not re.search(rf'^proc {re.escape(n)} external', '\n'.join(main_src + kept_extern), re.M)]) + '\n'
    import_vas = {name: IMPORT_VA_BASE + IMPORT_VA_STRIDE * i for i, (_, name, _) in enumerate(imports)}
    # Export registry for GetProcAddress: every static import, then dynamic exports, each
    # with its own guest address in the import page. Entry i is at IMPORT_VA_BASE + 16*i.
    registry = [(dll, name, IMPORT_PREFIX + name) for _, name, dll in imports]
    seen = {(dll, name) for dll, name, _ in registry}
    for dll, name in dynamic_exports(work / 'haloce.exe'):
        if (dll, name) not in seen:
            seen.add((dll, name))
            registry.append((dll, name, symbol_for(name if not name.startswith('#') else f'{dll.split(".")[0]}_ord{name[1:]}')))
    if IMPORT_VA_BASE + IMPORT_VA_STRIDE * len(registry) > HOST_RETURN_VA:
        sys.exit('import address page overflow')
    a.registry_primary = len(registry)

    code, stats = transform_code((work / 'seg01_code.llinc').open(errors='replace'), import_vas)
    (va / 'seg01_code.va.llinc').write_text('\n'.join(code) + '\n')

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
    extern = extern_src + 'proc halopad_dispatch external\n'
    (va / 'extern.llinc').write_text(extern)
    (va / 'macros.llinc').write_text('')

    # Runtime llasm (imports) and SR's call macros: same transfer rewrite.
    calls = (RUNTIME / 'asm-calls.llinc').read_text().replace('    tcall tmp0\n', '    PUSH tmp0\n    tcall halopad_dispatch\n')
    (va / 'asm-calls.llinc').write_text(calls)
    runtime_ll = []
    subprocess.run([sys.executable, str(ROOT / 'scripts' / 'gen-com-wrappers.py')], check=True, capture_output=True)
    for src in sorted(RUNTIME.glob('*.llasm')) + [ROOT / 'generated' / 'runtime' / 'halopad-com.llasm']:
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

    # Procedures must be visible to the dispatch module. llasm's C entry wrappers
    # (c_<alias>) are SR's pointer-offset entry path: they store a host function
    # address minus the pointer offset as a guest return address. The VA model enters
    # through halopad_enter, so they are removed.
    target_ll = work / 'haloce.target.ll'
    if target_ll.exists():
        triple = next(l for l in target_ll.open() if l.startswith('target triple'))
    else:
        import json
        triple = 'target triple = "%s"\n' % json.loads((ROOT / 'toolchains.lock.json').read_text())['target']
    procs, dropped, in_wrapper = [], 0, False
    with open(va / 'haloce.va.raw.ll') as fin, open(va / 'haloce.va.ll.new', 'w') as fout:
        fout.write(triple)
        for line in fin:
            if in_wrapper:
                in_wrapper = line.rstrip('\n') != '}'
                continue
            if line.startswith('define protected ccc void @c_'):
                in_wrapper, dropped = True, dropped + 1
                continue
            if line.startswith('define private fastcc void @'):
                line = line.replace('define private fastcc', 'define hidden fastcc', 1)
            m = re.match(r'define (?:hidden|protected) fastcc void @([^(]+)\(', line)
            if m:
                procs.append(m.group(1))
            if 'ptrtoint' in line and re.search(r'ptrtoint void\s*\(%_cpu\*\)\*', line):
                sys.exit(f'host code address would become a guest value: {line.strip()[:160]}')
            fout.write(line)
    # Replace the translated module only if it changed, so runtime or registry changes do
    # not force the long recompile.
    new, cur = va / 'haloce.va.ll.new', va / 'haloce.va.ll'
    if cur.exists() and filecmp.cmp(new, cur, shallow=False):
        new.unlink()
    else:
        new.replace(cur)
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
    # Translated DLLs shipped with the game (Keystone.dll, ksimeui.dll): same transform,
    # own procedures at their own original addresses, own import slots (bound by the
    # runtime's LoadLibraryA like the Windows loader does).
    modules = build_modules(a, va, llasm, triple, registry, seen, table)
    reg_vas = [IMPORT_VA_BASE + IMPORT_VA_STRIDE * i for i in range(len(registry))]
    for iva in reg_vas:
        if iva in table:
            sys.exit(f'import address {iva:#x} collides with {table[iva]}')
    # COM methods: one guest address per (interface, method), dispatching to hpcom_<I>_<M>.
    ifaces = com_interfaces()
    com = [(iface, m) for iface, ms in ifaces for m in ms]
    com_vas = [COM_VA_BASE + IMPORT_VA_STRIDE * k for k in range(len(com))]
    if com_vas and com_vas[-1] >= IMPORT_VA_BASE:
        sys.exit('COM method page overflow')
    entries = sorted(list(table.items()) + [(iva, sym) for iva, (_, _, sym) in zip(reg_vas, registry)]
                     + [(cva, f'hpcom_{i}_{m}') for cva, (i, m) in zip(com_vas, com)])
    ll = [triple.rstrip(), '', '; Generated by scripts/va-model.py: original address -> compiled procedure.']
    for name in sorted({n for _, n in entries}):
        ll.append(f'declare hidden fastcc void @{name}(ptr)')
    ll.append(f'@halopad_dispatch_count = constant i32 {len(entries)}')
    ll.append(f'@halopad_dispatch_vas = constant [{len(entries)} x i32] [' + ', '.join(f'i32 {v}' for v, _ in entries) + ']')
    ll.append(f'@halopad_dispatch_fns = constant [{len(entries)} x ptr] [' + ', '.join(f'ptr @{n}' for _, n in entries) + ']')
    # Import binding: guest import table slot -> import guest address. Export registry:
    # (DLL, name) -> guest address for GetProcAddress; names also label traps.
    n = len(registry)
    ll.append(f'@halopad_import_count = constant i32 {n}')
    ll.append(f'@halopad_import_slot_count = constant i32 {len(imports)}')
    ll.append(f'@halopad_import_base = constant i32 {IMPORT_VA_BASE}')
    ll.append(f'@halopad_import_stride = constant i32 {IMPORT_VA_STRIDE}')
    ll.append(f'@halopad_import_slots = constant [{len(imports)} x i32] [' + ', '.join(f'i32 {s}' for s, _, _ in imports) + ']')
    for i, (dll, name, _) in enumerate(registry):
        ll.append(f'@.hp_import_{i} = private unnamed_addr constant [{len(name) + 1} x i8] c"{name}\\00"')
        ll.append(f'@.hp_dll_{i} = private unnamed_addr constant [{len(dll) + 1} x i8] c"{dll}\\00"')
    ll.append(f'@halopad_import_names = constant [{n} x ptr] [' + ', '.join(f'ptr @.hp_import_{i}' for i in range(n)) + ']')
    ll.append(f'@halopad_import_dlls = constant [{n} x ptr] [' + ', '.join(f'ptr @.hp_dll_{i}' for i in range(n)) + ']')
    # COM interface table: interface i's methods are com[first[i] .. first[i] + count[i]).
    ll.append(f'@halopad_com_base = constant i32 {COM_VA_BASE}')
    ll.append(f'@halopad_com_method_count = constant i32 {len(com)}')
    ll.append(f'@halopad_com_interface_count = constant i32 {len(ifaces)}')
    firsts, k = [], 0
    for _, ms in ifaces:
        firsts.append(k)
        k += len(ms)
    ll.append(f'@halopad_com_first = constant [{len(ifaces)} x i32] [' + ', '.join(f'i32 {x}' for x in firsts) + ']')
    ll.append(f'@halopad_com_count = constant [{len(ifaces)} x i32] [' + ', '.join(f'i32 {len(ms)}' for _, ms in ifaces) + ']')
    for i, (iface, _) in enumerate(ifaces):
        ll.append(f'@.hp_iface_{i} = private unnamed_addr constant [{len(iface) + 1} x i8] c"{iface}\\00"')
    ll.append(f'@halopad_com_interfaces = constant [{len(ifaces)} x ptr] [' + ', '.join(f'ptr @.hp_iface_{i}' for i in range(len(ifaces))) + ']')
    for k2, (iface, m) in enumerate(com):
        name = f'{iface}::{m}'
        ll.append(f'@.hp_method_{k2} = private unnamed_addr constant [{len(name) + 1} x i8] c"{name}\\00"')
    ll.append(f'@halopad_com_methods = constant [{len(com)} x ptr] [' + ', '.join(f'ptr @.hp_method_{k2}' for k2 in range(len(com))) + ']')
    ll.extend(module_tables(modules, registry))
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
          f"{stats['value-labels']:,} lines with label values rewritten, {stats['import-values']:,} import values, "
          f"{len(imports)} imports bound, {len(registry) - len(imports)} dynamic exports, {len(com)} COM methods, {dropped} C entry wrappers removed, {len(entries):,} dispatch entries")
    return 0


if __name__ == '__main__':
    sys.exit(main())
