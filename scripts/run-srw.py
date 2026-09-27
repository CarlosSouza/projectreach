#!/usr/bin/env python3
"""G2c: run the built SRW (OUT_LLASM) over the accepted Custom Edition 1.10 haloce.exe.

Stages the executable, the audit's relocations.csv and SRW hint files (.sci) in a
fresh ignored work directory, runs SRW, and records a capability report: exit
status, stage reached, error category and location, output size and timing.

Usage: .venv/bin/python scripts/run-srw.py --build generated/tool-builds/<key> [--extra-sci DIR]
"""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time

import hpmodule

ROOT = pathlib.Path(__file__).resolve().parents[1]
PROFILE_ID = 'custom-en-1.0.10.0621'


def sha(p):
    return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()


def write_extern_llinc(work, exe, stem='haloce'):
    """extern.llinc: every static import not already declared by SRW (imports referenced
    only from data, e.g. stored function pointers) plus HaloPad's trap helpers."""
    import pefile
    header = (work / f'{stem}.llasm').read_text()
    declared = set(re.findall(r'^proc (\S+) external', header, re.M))
    # Imports implemented by HaloPad's llasm runtime (port/llasm-runtime/*.llasm,
    # 'proc hpimp_<name> public') are redirected there; everything else stays external.
    # (The VA model redirects every import itself; see scripts/va-model.py.)
    implemented = set()
    for f in (ROOT / 'port' / 'llasm-runtime').glob('*.llasm'):
        implemented |= set(re.findall(r'^proc hpimp_(\S+) public', f.read_text(), re.M))
    pe = pefile.PE(str(exe))
    names = sorted({imp.name.decode() for d in getattr(pe, 'DIRECTORY_ENTRY_IMPORT', []) for imp in d.imports if imp.name})
    lines = [f'define {n} hpimp_{n}' for n in sorted(implemented)]
    lines += [f'proc hpimp_{n} external' for n in sorted(implemented) if n not in declared]
    lines += [f'proc {n} external' for n in names if n not in declared and n not in implemented]
    lines += ['funcv halopad_unreachable_simd address', 'funcv halopad_trap_unimplemented address']
    (work / 'extern.llinc').write_text('\n'.join(lines) + '\n')
    (work / 'macros.llinc').touch()
    return len(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=pathlib.Path, required=True)
    ap.add_argument('--extra-sci', type=pathlib.Path,
                    help='hand-maintained .sci/.cfg overrides (tracked; default: the module\'s config/srw directory); appended after audit hints')
    hpmodule.add_argument(ap)
    ap.add_argument('--timeout', type=int, default=3600)
    ap.add_argument('--diagnostic', action='store_true',
                    help='HALOPAD_SRW_DIAG=1: log every conversion failure and continue (census; output is not a build input)')
    a = ap.parse_args()

    build = a.build.resolve()
    manifest = json.loads((build / 'build-manifest.json').read_text())
    srw = build / 'SRW' / 'SRW.exe'
    if sha(srw) != manifest['artifacts']['SRW/SRW.exe']:
        sys.exit('FAIL: SRW binary does not match its build manifest')
    mod = hpmodule.Module(a.module)
    exe = mod.exe
    mod.verify_source()
    analysis = mod.analysis
    extra_sci = a.extra_sci or mod.hand
    stem = mod.stem

    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    run = f'run-{stamp}-{os.getpid()}'
    work = ROOT / 'generated' / 'srw' / PROFILE_ID / ('' if mod.primary else f'modules/{mod.name}') / run
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / 'G2c' / (run if mod.primary else f'{mod.name}-{run}')
    work.mkdir(parents=True)
    evid.mkdir(parents=True)
    in_name = 'haloce.exe' if mod.primary else mod.file
    shutil.copyfile(exe, work / in_name)
    inputs = {in_name: sha(work / in_name)}
    if mod.rebase is not None:
        inputs['source ' + mod.relpath] = mod.sha256
        inputs['rebased to'] = hex(mod.rebase)
    if mod.relocs_stripped:
        shutil.copyfile(analysis / 'relocations.csv', work / 'relocations.csv')
        inputs['relocations.csv'] = sha(work / 'relocations.csv')
    hint_files = {}
    for src_dir in (analysis / 'srw', extra_sci):
        if not src_dir.is_dir():
            continue
        for f in sorted(src_dir.iterdir()):
            if f.suffix not in ('.sci', '.cfg', '.csv') or not f.is_file():
                continue
            dst = work / f.name
            with open(dst, 'a') as out:
                out.write(f.read_text())
            hint_files.setdefault(f.name, []).append(str(f.relative_to(ROOT)))
    for name in hint_files:
        inputs[name] = sha(work / name)

    t0 = time.time()
    env = dict(os.environ)
    if a.diagnostic:
        env['HALOPAD_SRW_DIAG'] = '1'
    with open(evid / 'srw.stdout', 'w') as so, open(evid / 'srw.stderr', 'w') as se:
        try:
            proc = subprocess.run([str(srw), in_name, f'{stem}.llasm'], cwd=work, stdout=so, stderr=se,
                                  timeout=a.timeout, env=env)
            code = proc.returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
    elapsed = round(time.time() - t0, 2)
    err = (evid / 'srw.stderr').read_text(errors='replace')
    stages = re.findall(r'\d{4}-\d\d-\d\d [\d:.]+: (.+?)\.\.\.', err)
    errors = [l for l in err.splitlines() if l.startswith('Error')]
    category = 'none'
    location = None
    m = re.search(r'invalid instruction at (0x[0-9a-f]+)', err)
    if m:
        category, location = 'invalid-instruction', m.group(1)
    elif 'Import by ordinal' in err:
        category = 'import-by-ordinal'
    elif errors:
        category = 'other'
    outputs = {p.name: p.stat().st_size for p in work.iterdir() if p.suffix in ('.llasm', '.llinc')}
    if (work / f'{stem}.llasm').exists():
        # SRW writes 'define loc_X alias' (global_aliases.sci) after the data-segment
        # includes, so references from data would keep the old label. Hoist the defines.
        main_lines = (work / f'{stem}.llasm').read_text().splitlines()
        defines = [l for l in main_lines if l.startswith('define ')]
        rest = [l for l in main_lines if not l.startswith('define ')]
        first = next(i for i, l in enumerate(rest) if not l.startswith('include ')) if rest else 0
        (work / f'{stem}.llasm').write_text('\n'.join(rest[:first] + defines + rest[first:]) + '\n')
        outputs['extern.llinc(extra imports)'] = write_extern_llinc(work, work / in_name, stem)
    census = {}
    if a.diagnostic:
        lines = err.splitlines()
        diag = [l.split() for l in lines if l.startswith('HALOPAD-DIAG')]
        kinds = {}
        for i, l in enumerate(lines):
            if l.startswith('HALOPAD-DIAG'):
                parts = l.split()
                prev = lines[i - 1] if i else ''
                text_ = prev.rsplit(' - ', 1)[-1] if prev.startswith('Error') else ''
                kinds.setdefault(parts[1], []).append([parts[2], text_.lstrip(';')])
        census = {k: {'count': len(v), 'sites': v} for k, v in kinds.items()}
        (evid / 'census.json').write_text(json.dumps(census, indent=1) + '\n')
    report = {'run': run, 'module': mod.name, 'build': manifest['key'], 'exit': code, 'seconds': elapsed,
              'mode': 'diagnostic-census' if a.diagnostic else 'strict',
              'census_counts': {k: v['count'] for k, v in census.items()},
              'stages_reached': stages, 'errors': errors, 'category': category, 'location': location,
              'outputs': outputs, 'inputs': inputs, 'hint_sources': hint_files, 'work': str(work.relative_to(ROOT))}
    (evid / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    status = 'COMPLETED' if code == 0 else 'FAILED'
    print(f"{status} exit={code} {elapsed}s stage={stages[-1] if stages else '-'} category={category} "
          f"location={location} outputs={sum(outputs.values()):,} bytes")
    for e in errors[:5]:
        print(' ', e)
    print('evidence', evid.relative_to(ROOT))
    return 0 if code == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
