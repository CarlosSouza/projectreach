#!/usr/bin/env python3
"""Run the real import transaction on disposable inert fixtures, on macOS or a booted Simulator."""
import argparse
import datetime
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device', help='Booted project Simulator UDID; otherwise native macOS')
    args = parser.parse_args()
    now = datetime.datetime.now(datetime.timezone.utc)
    evidence = ROOT / 'docs/artifacts' / now.strftime('%Y-%m-%d') / 'G9' / ('import-' + now.strftime('%Y%m%dT%H%M%SZ'))
    evidence.mkdir(parents=True)
    sdkname = 'iphonesimulator' if args.device else 'macosx'
    sdk = subprocess.check_output(['xcrun', '--sdk', sdkname, '--show-sdk-path'], text=True).strip()
    target = 'arm64-apple-ios17.0-simulator' if args.device else 'arm64-apple-macosx14.0'
    exe = evidence / 'import-test'
    cmd = ['xcrun', 'clang', '-target', target, '-isysroot', sdk, '-fobjc-arc', '-O1',
           '-Wno-deprecated-declarations', str(ROOT / 'tests/halo_import_test.m'),
           '-framework', 'Foundation', '-o', str(exe)]
    build = subprocess.run(cmd, capture_output=True, text=True)
    (evidence / 'build.txt').write_text(build.stdout + build.stderr)
    build.check_returncode()
    launch = ['xcrun', 'simctl', 'spawn', args.device, str(exe)] if args.device else [str(exe)]
    result = subprocess.run(launch, capture_output=True, text=True, timeout=30)
    (evidence / 'run.txt').write_text(result.stdout + result.stderr)
    sources = ['tests/halo_import_test.m', 'port/ios/HaloPadImport.m', 'port/ios/HaloPadImport.h',
               'port/ios/HaloPadApp.m', 'scripts/test-ios-import.py']
    (evidence / 'result.json').write_text(json.dumps(dict(device=args.device, exit=result.returncode,
        build_command=cmd, source_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources}), indent=2) + '\n')
    print(result.stdout + result.stderr, end='')
    print('evidence', evidence.relative_to(ROOT))
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
