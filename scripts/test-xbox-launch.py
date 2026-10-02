#!/usr/bin/env python3
"""Asset-free launch/save-helper checks on an explicitly named booted Simulator.

No install, game, window or real installation state; fixtures stay in generated/.
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
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    evidence = ROOT / 'generated/xbox-launch-tests' / stamp
    evidence.mkdir(parents=True)
    sdk = subprocess.check_output(['xcrun', '--sdk', 'iphonesimulator', '--show-sdk-path'], text=True).strip()
    source = ROOT / 'tests/xbox_launch_paths_test.m'
    binary = evidence / 'xbox-launch-test'
    command = ['xcrun', 'clang', '-target', 'arm64-apple-ios17.0-simulator', '-isysroot', sdk,
               '-fobjc-arc', '-O1', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
               '-Wno-deprecated-declarations', '-Wno-nonnull', '-I/opt/homebrew/include', '-I', str(ROOT / 'port/xbox'), str(source),
               '-framework', 'UIKit', '-framework', 'Foundation', '-framework', 'UniformTypeIdentifiers',
               '-framework', 'GameController',
               '-o', str(binary)]
    build = subprocess.run(command, capture_output=True, text=True)
    (evidence / 'build.txt').write_text(build.stdout + build.stderr)
    if build.returncode:
        print(build.stdout + build.stderr, end='')
        print('evidence', evidence.relative_to(ROOT))
        return build.returncode
    run = subprocess.run(['xcrun', 'simctl', 'spawn', args.device, str(binary), str(evidence / 'fixture')],
                         capture_output=True, text=True, timeout=30)
    (evidence / 'run.txt').write_text(run.stdout + run.stderr)
    inputs = [source, ROOT / 'port/ios/HaloPadXbox.m', ROOT / 'port/ios/HaloPadXboxSaveIdentity.h', ROOT / 'port/xbox/xg_ios.h',
              ROOT / 'port/ios/HaloPadOverlay.h', ROOT / 'port/xbox/xg_overlay_input.h',
              ROOT / 'port/xbox/xg_touch_input.h', ROOT / 'port/xbox/xg_xiso.h']
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
