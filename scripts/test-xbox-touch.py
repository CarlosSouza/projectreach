#!/usr/bin/env python3
"""Run asset-free Xbox UIKit handler tests on an explicitly named booted Simulator.

No install, window, game data or physical input; does not operate the running app.
"""
import argparse
import datetime
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device', required=True)
    args = parser.parse_args()
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    evidence = ROOT / 'generated/xbox-touch-tests' / stamp
    evidence.mkdir(parents=True)
    sdk = subprocess.check_output(['xcrun', '--sdk', 'iphonesimulator', '--show-sdk-path'], text=True).strip()
    sources = [ROOT / 'tests/xbox_touch_lifecycle_test.m', ROOT / 'port/xbox/xg_touch.m']
    binary = evidence / 'xbox-touch-test'
    command = ['xcrun', 'clang', '-target', 'arm64-apple-ios17.0-simulator', '-isysroot', sdk,
               '-fobjc-arc', '-O1', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
               '-I/opt/homebrew/include', *map(str, sources), '-framework', 'UIKit',
               '-framework', 'Foundation', '-framework', 'QuartzCore', '-framework', 'CoreGraphics',
               '-o', str(binary)]
    build = subprocess.run(command, capture_output=True, text=True)
    (evidence / 'build.txt').write_text(build.stdout + build.stderr)
    build.check_returncode()
    run = subprocess.run(['xcrun', 'simctl', 'spawn', args.device, str(binary)],
                         capture_output=True, text=True, timeout=30)
    (evidence / 'run.txt').write_text(run.stdout + run.stderr)
    inputs = sources + [ROOT / 'port/xbox/xg_ios.h', ROOT / 'port/xbox/xg_touch_input.h']
    (evidence / 'result.json').write_text(json.dumps({
        'device': args.device, 'exit': run.returncode, 'build_command': command,
        'sources': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in inputs},
        'executable_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    }, indent=2) + '\n')
    print(run.stdout + run.stderr, end='')
    print('evidence', evidence.relative_to(ROOT))
    return run.returncode


if __name__ == '__main__':
    raise SystemExit(main())
