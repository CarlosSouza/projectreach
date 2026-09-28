#!/usr/bin/env python3
"""Prepare or verify a private .halopad.zip for an exact locally built candidate.

Build the app first; --app-data names HaloPad.app/data. Outputs contain proprietary game data,
remain local, and default to generated/prepared/. Nothing is downloaded, installed or executed.
"""
import argparse
from pathlib import Path
import sys
import zipfile
from halopad_package import PackageError, digest, prepare, read_identity, verify

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app-data', required=True, type=Path)
    parser.add_argument('--game', type=Path, help='Your matching Custom Edition installation')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--verify', type=Path, help='Verify an existing package instead of creating one')
    args = parser.parse_args()
    try:
        identity = read_identity(args.app_data)
        if args.verify:
            if args.game or args.output:
                parser.error('--verify cannot be combined with --game or --output')
            manifest = verify(args.verify, identity)
            package = args.verify
        else:
            if not args.game:
                parser.error('--game is required for preparation')
            package = args.output or ROOT / 'generated/prepared' / identity['profile'] / (identity['id'][:16] + '.halopad.zip')
            manifest = prepare(args.game, args.app_data, package)
        print(f'PASS: {len(manifest["files"])} files verified for core {identity["id"]}')
        print(f'Private package: {package}')
        print(f'SHA-256: {digest(package)}')
        return 0
    except (PackageError, OSError, zipfile.BadZipFile, zipfile.LargeZipFile, RuntimeError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
