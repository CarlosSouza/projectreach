#!/usr/bin/env python3
"""Build and test the native package importer without a game/window, on Mac or a booted Simulator."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from halopad_package import read_identity, record, digest, inventory

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--device')
    ap.add_argument('--real-package', type=Path, help='Also import this prepared archive into disposable evidence storage')
    ap.add_argument('--real-folder', type=Path, help='Also import this installation folder into disposable evidence storage')
    ap.add_argument('--app-data', type=Path, help='Trusted built app data for --real-package')
    ap.add_argument('--sanitize', action='store_true', help='Address/undefined behavior sanitizers (macOS)')
    args = ap.parse_args()
    if args.real_package and args.real_folder:
        ap.error('choose either --real-package or --real-folder')
    if bool(args.real_package or args.real_folder) != bool(args.app_data):
        ap.error('a real input and --app-data must be used together')
    if args.sanitize and args.device:
        ap.error('--sanitize currently supports macOS only')
    stamp = datetime.datetime.now(datetime.timezone.utc)
    evidence = ROOT / 'docs/artifacts' / stamp.strftime('%Y-%m-%d') / 'G9' / ('native-package-' + stamp.strftime('%Y%m%dT%H%M%SZ'))
    evidence.mkdir(parents=True)
    sdk = subprocess.check_output(['xcrun', '--sdk', 'iphonesimulator' if args.device else 'macosx', '--show-sdk-path'], text=True).strip()
    target = 'arm64-apple-ios17.0-simulator' if args.device else 'arm64-apple-macosx14.0'
    binary = evidence / 'package-test'
    command = ['xcrun', 'clang', '-target', target, '-isysroot', sdk, '-fobjc-arc', '-O1', '-Wall', '-Wextra',
               '-Werror', '-Wno-deprecated-declarations', str(ROOT / 'tests/halo_package_test.m'), str(ROOT / 'port/ios/HaloPadDataIdentity.m'), '-framework', 'Foundation', '-lz', '-o', str(binary)]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    result = subprocess.run(command, capture_output=True, text=True)
    (evidence / 'build.txt').write_text(result.stdout + result.stderr)
    result.check_returncode()
    run = ['xcrun', 'simctl', 'spawn', args.device, str(binary)] if args.device else [str(binary)]
    env = dict(os.environ, HALOPAD_PACKAGE_TEST_COMMAND=json.dumps(run))
    result = subprocess.run([sys.executable, '-m', 'unittest', 'discover', '-s', 'tests', '-p', 'test_native_package.py', '-v'],
                            cwd=ROOT, env=env, capture_output=True, text=True, timeout=240)
    (evidence / 'run.txt').write_text(result.stdout + result.stderr)
    sources = ['port/ios/HaloPadDataIdentity.m', 'port/ios/HaloPadDataIdentity.h', 'port/ios/HaloPadPackage.m', 'port/ios/HaloPadImport.m', 'port/ios/HaloPadImport.h',
               'tests/halo_package_test.m', 'tests/test_native_package.py', 'scripts/test-native-package.py']
    (evidence / 'result.json').write_text(json.dumps(dict(device=args.device, exit=result.returncode, build=command,
        sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources}), indent=2)+'\n')
    if result.returncode == 0 and args.real_package:
        identity = read_identity(args.app_data)
        archive_hash = digest(args.real_package)
        destination = evidence / 'real-installed'
        destination.mkdir()
        (destination / 'previous-install-marker').write_text('retained test installation')
        started = time.monotonic()
        imported = subprocess.run(run + [str((args.app_data / 'core-identity.json').resolve()), str(args.real_package.resolve()), str(destination)],
                                  capture_output=True, text=True, timeout=120)
        (evidence / 'real-import.txt').write_text(imported.stdout + imported.stderr)
        imported.check_returncode()
        response = json.loads(imported.stdout)
        found = inventory(destination)
        if set(found) != set(identity['stock_files']):
            raise RuntimeError('Imported file inventory differs from trusted identity')
        for name, path in found.items():
            if record(path) != identity['stock_files'][name]:
                raise RuntimeError('Imported file content mismatch: ' + name)
        backup = Path(response['backup'])
        if (backup / 'previous-install-marker').read_text() != 'retained test installation' or digest(args.real_package) != archive_hash:
            raise RuntimeError('Archive or prior installation was modified')
        (evidence / 'real-result.json').write_text(json.dumps(dict(archive_sha256=archive_hash, core_id=identity['id'],
            game_files=len(found), verified_core_data_files=len(identity['core_data']), elapsed_seconds=time.monotonic()-started,
            device=args.device, backup_preserved=True, source_unchanged=True), indent=2)+'\n')
        print('PASS: real package imported; every stock byte matches, prior folder retained, archive unchanged')
    if result.returncode == 0 and args.real_folder:
        identity = read_identity(args.app_data)
        source = args.real_folder.resolve()
        before = {name: record(path) for name, path in inventory(source).items()}
        destination = evidence / 'real-folder-installed'
        destination.mkdir()
        (destination / 'previous-install-marker').write_text('retained test installation')
        started = time.monotonic()
        imported = subprocess.run(run + [str((args.app_data / 'core-identity.json').resolve()), str(source), str(destination), 'folder'],
                                  capture_output=True, text=True, timeout=120)
        (evidence / 'real-folder-import.txt').write_text(imported.stdout + imported.stderr)
        imported.check_returncode()
        response = json.loads(imported.stdout)
        actual = {name: record(path) for name, path in inventory(destination).items()}
        after = {name: record(path) for name, path in inventory(source).items()}
        if actual != identity['stock_files'] or before != after:
            raise RuntimeError('Folder output differs from approved inventory, or source was modified')
        if (Path(response['backup']) / 'previous-install-marker').read_text() != 'retained test installation':
            raise RuntimeError('Previous folder was not retained')
        validated = subprocess.run(run + [str((args.app_data / 'core-identity.json').resolve()), str(destination), str(destination), 'validate'],
                                   capture_output=True, text=True, timeout=120)
        (evidence / 'real-folder-validation.txt').write_text(validated.stdout + validated.stderr)
        validated.check_returncode()
        (evidence / 'real-folder-result.json').write_text(json.dumps(dict(source_files=len(before), installed_files=len(actual),
            excluded_files=sorted(set(before)-set(actual)), source_unchanged=True, backup_preserved=True, startup_validation=True,
            elapsed_seconds=time.monotonic()-started, device=args.device, core_id=identity['id']), indent=2)+'\n')
        print('PASS: real folder imported and startup-validated; all stock hashes match, source extras and prior install retained')
    print(result.stdout + result.stderr, end='')
    print('evidence', evidence.relative_to(ROOT))
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
