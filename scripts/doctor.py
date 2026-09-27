#!/usr/bin/env python3
"""Read-only host preflight. Never boots a VM/Simulator or stops processes."""
import datetime
import json
import pathlib
import platform
import shutil
import subprocess
import sys
import uuid

ROOT = pathlib.Path(__file__).resolve().parents[1]

def main():
    checks = []
    def run(name, argv, required=True):
        try:
            p = subprocess.run(argv, cwd=str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=45)
            checks.append({'name': name, 'command': argv, 'required': required, 'exit_code': p.returncode, 'output': p.stdout})
        except (OSError, subprocess.TimeoutExpired) as exc:
            checks.append({'name': name, 'required': required, 'exit_code': 127, 'output': str(exc)})
    run('root_revision', ['git', 'rev-parse', 'HEAD'])
    run('root_state', ['git', 'status', '--short'])
    run('macOS', ['sw_vers'])
    run('Xcode', ['xcodebuild', '-version'])
    for sdk in ['macosx', 'iphoneos', 'iphonesimulator']:
        run(sdk, ['xcrun', '--sdk', sdk, '--show-sdk-version'])
    for tool in ['clang', 'cmake', 'ninja', 'python3']:
        run(tool, [tool, '--version'])
    run('python_dependencies', [str(ROOT / '.venv/bin/python'), '-c', 'import pefile, SCons; print("pefile", pefile.__version__, "SCons", SCons.__version__)'])
    run('booted_simulators', ['xcrun', 'simctl', 'list', 'devices', 'booted', '--json'])
    # Command names only; avoid command arguments that could carry credentials.
    p = subprocess.run(['ps', '-axo', 'pid=,comm='], stdout=subprocess.PIPE, text=True, check=True)
    candidates = [line for line in p.stdout.splitlines() if any(x in line.lower() for x in ['halopad', 'haloce.exe', 'haloceded.exe', 'halo.exe'])]
    run('source_pins', ['sh', 'scripts/verify-sources.sh'])
    run('repository_safety', ['sh', 'scripts/check-repo-safety.sh'])
    reports = {'machine': platform.machine(), 'checks': checks, 'candidate_processes': candidates,
        'scons_on_path': shutil.which('scons'), 'scons_venv': (ROOT / '.venv/bin/scons').exists(), 'pefile_venv': (ROOT / '.venv/bin/python').exists(),
        'reference_environment': 'NOT_IDENTIFIED; no original client baseline run',
        'native_core': 'NOT_BUILT', 'signing_and_physical_device': 'NOT_RUN',
        'deployment_targets': {'macOS': '14.0 provisional, no Halo build yet', 'iOS_iPadOS': '17.0 provisional'}}
    out = ROOT / 'docs/artifacts' / datetime.date.today().isoformat() / 'G0' / ('doctor-' + uuid.uuid4().hex[:12])
    out.mkdir(parents=True)
    (out / 'environment.json').write_text(json.dumps(reports, indent=2) + '\n')
    failed = platform.machine() != 'arm64' or any(c['required'] and c['exit_code'] for c in checks)
    for check in checks:
        print(('PASS' if check['exit_code'] == 0 else 'FAIL'), check['name'])
    print('Private report:', out.relative_to(ROOT) / 'environment.json')
    print('G0 remains incomplete until the original-client reference environment is identified.')
    return 1 if failed else 0

if __name__ == '__main__':
    sys.exit(main())
