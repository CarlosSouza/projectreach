#!/usr/bin/env python3
"""Asset-free counted ANGLE test in one explicitly selected booted Simulator.

Uses a pbuffer, not the HaloPad app; no app install or game/save access.
Build the separate CMake candidate first; never point it at the shipping library.
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
    parser.add_argument('--angle-source', type=pathlib.Path, required=True)
    parser.add_argument('--angle-build', type=pathlib.Path, required=True)
    args = parser.parse_args()
    build = args.angle_build.resolve()
    identity = json.loads((build / 'counted-visibility-v1/identity.json').read_text())
    if identity['name'] != 'counted-visibility-v1':
        raise SystemExit('Not the counted candidate')
    evidence = ROOT / 'generated/xbox-counted-tests' / datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    evidence.mkdir(parents=True)
    library = build / 'libhalopad-angle.a'
    source = ROOT / 'tests/xbox_counted_visibility_test.mm'
    binary = evidence / 'counted-test'
    sdk = subprocess.check_output(['xcrun', '--sdk', 'iphonesimulator', '--show-sdk-path'], text=True).strip()
    command = ['xcrun', 'clang++', '-target', 'arm64-apple-ios17.0-simulator', '-isysroot', sdk,
               '-std=c++20', '-fobjc-arc', '-O1', '-Wall', '-Wextra', '-Werror',
               '-I', str(args.angle_source.resolve() / 'include'), str(source), str(library),
               '-framework', 'Foundation', '-framework', 'QuartzCore', '-framework', 'Metal',
               '-framework', 'IOSurface', '-framework', 'UIKit', '-framework', 'CoreGraphics',
               '-lz', '-o', str(binary)]
    compile_result = subprocess.run(command, text=True, capture_output=True)
    (evidence / 'build.log').write_text(compile_result.stdout + compile_result.stderr)
    if compile_result.returncode:
        print(compile_result.stderr)
        return compile_result.returncode
    runs = []
    for feature in ('on', 'off'):
        result = subprocess.run(['xcrun', 'simctl', 'spawn', args.device, str(binary), feature],
                                text=True, capture_output=True, timeout=60)
        (evidence / f'run-{feature}.log').write_text(result.stdout + result.stderr)
        print(result.stdout + result.stderr)
        runs.append({'allowBufferReadWrite': feature, 'exit': result.returncode})
    (evidence / 'result.json').write_text(json.dumps({
        'runs': runs, 'device': args.device, 'candidate': identity,
        'library_sha256': hashlib.sha256(library.read_bytes()).hexdigest(),
        'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
        'command': command,
    }, indent=2) + '\n')
    print('evidence', evidence)
    return int(any(run['exit'] for run in runs))


if __name__ == '__main__':
    raise SystemExit(main())
