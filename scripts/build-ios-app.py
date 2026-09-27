#!/usr/bin/env python3
"""G3/G8: build HaloPad for iPadOS (the app shell in port/ios around the native core) and, with
--launch, install and run it on a Simulator.

The app is the same link as scripts/run-core.py (translated Halo and DLLs, the runtime, the Apple
hosts) with port/ios/HaloPadApp.m as its program, packaged as HaloPad.app (ad-hoc signed for the
Simulator). Development builds read their data from the Mac through HALOPAD_* variables passed
by simctl; the app's state (virtual disk, registry, the player's license choice) lives in
generated/halopad-disk-ios/. With --launch the script waits, takes a screenshot of the device
and records stdout/stderr as evidence; it never taps anything in the app.

Usage: .venv/bin/python scripts/build-ios-app.py [--work RUN_DIR] [--device UDID] [--launch] [--wait S]
"""
import argparse
import datetime
import importlib.util
import json
import pathlib
import plistlib
import shutil
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('run_core', ROOT / 'scripts' / 'run-core.py')
run_core = importlib.util.module_from_spec(spec)
spec.loader.exec_module(run_core)

TARGET = 'arm64-apple-ios17.0-simulator'
BUNDLE_ID = 'dev.halopad.HaloPad'
STATE = ROOT / 'generated' / 'halopad-disk-ios'


def package(exe, out):
    app = out / 'HaloPad.app'
    if app.exists():
        shutil.rmtree(app)
    app.mkdir(parents=True)
    shutil.copy2(exe, app / 'HaloPad')
    info = {
        'CFBundleIdentifier': BUNDLE_ID, 'CFBundleExecutable': 'HaloPad', 'CFBundleName': 'HaloPad',
        'CFBundleDisplayName': 'HaloPad', 'CFBundlePackageType': 'APPL', 'CFBundleVersion': '1',
        'CFBundleShortVersionString': '0.1', 'CFBundleSupportedPlatforms': ['iPhoneSimulator'],
        'MinimumOSVersion': '17.0', 'UIDeviceFamily': [2], 'UIRequiresFullScreen': True, 'UILaunchScreen': {},
        'UIStatusBarHidden': True,
        'UISupportedInterfaceOrientations~ipad': ['UIInterfaceOrientationLandscapeLeft', 'UIInterfaceOrientationLandscapeRight'],
        'UIApplicationSceneManifest': {'UIApplicationSupportsMultipleScenes': False},
    }
    with open(app / 'Info.plist', 'wb') as f:
        plistlib.dump(info, f)
    subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(app)], check=True, capture_output=True)
    return app


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=pathlib.Path)
    ap.add_argument('--device', default='E129A00F-D338-4FDC-8AE8-BB243E9BA61B', help='Simulator UDID ("HaloPad iPad Pro 13")')
    ap.add_argument('--launch', action='store_true')
    ap.add_argument('--wait', type=int, default=20, help='seconds to let the app run before the screenshot')
    ap.add_argument('--scene', type=pathlib.Path,
                    help='development only: a C file whose halopad_app_entry replaces the core start (evidence scenes in tests/)')
    a = ap.parse_args()
    work = (a.work or max(run_core.PROFILE.glob('run-*/va/haloce.va.ll'), key=lambda p: p.stat().st_mtime).parent.parent).resolve()
    exe, _ = run_core.build(work, TARGET, ROOT / 'port' / 'ios' / 'HaloPadApp.m', extra=[a.scene.resolve()] if a.scene else [])
    app = package(exe, work / f'ios-app-{TARGET}')
    print('built', app.relative_to(ROOT))
    if not a.launch:
        return 0
    stamp = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    evid = ROOT / 'docs' / 'artifacts' / datetime.date.today().isoformat() / 'G3' / f'ios-app-{stamp}'
    evid.mkdir(parents=True, exist_ok=True)
    STATE.mkdir(parents=True, exist_ok=True)
    env = {'HALOPAD_IMAGE': run_core.IMAGE, 'HALOPAD_MODULE_IMAGES': run_core.IMAGE.parent / 'modules',
           'HALOPAD_REFERENCE_ROOT': ROOT / 'ref' / 'inputs' / 'reference-machine', 'HALOPAD_GAME_ROOT': run_core.GAME_ROOT,
           'HALOPAD_STATE_ROOT': STATE, 'HALOPAD_REPO_ROOT': ROOT, 'HALOPAD_REGISTRY': STATE / 'registry.txt'}
    child = {'SIMCTL_CHILD_' + k: str(v) for k, v in env.items()}
    dev = a.device
    subprocess.run(['xcrun', 'simctl', 'boot', dev], capture_output=True)          # already booted is fine
    subprocess.run(['xcrun', 'simctl', 'bootstatus', dev, '-b'], check=True, capture_output=True)
    subprocess.run(['xcrun', 'simctl', 'install', dev, str(app)], check=True)
    import os
    launch = subprocess.run(['xcrun', 'simctl', 'launch', '--terminate-running-process', f'--stdout={evid / "stdout.txt"}',
                             f'--stderr={evid / "stderr.txt"}', dev, BUNDLE_ID], env=dict(os.environ, **child),
                            capture_output=True, text=True)
    print(launch.stdout.strip(), launch.stderr.strip())
    time.sleep(a.wait)
    shot = evid / 'screen.png'
    subprocess.run(['xcrun', 'simctl', 'io', dev, 'screenshot', str(shot)], check=True, capture_output=True)
    report = {'target': TARGET, 'work': str(work.relative_to(ROOT)), 'device': dev, 'bundle': BUNDLE_ID,
              'launch': launch.stdout.strip(), 'waited_s': a.wait, 'screenshot': shot.name}
    (evid / 'result.json').write_text(json.dumps(report, indent=1) + '\n')
    print('evidence', evid.relative_to(ROOT))
    err = (evid / 'stderr.txt').read_text(errors='replace') if (evid / 'stderr.txt').exists() else ''
    print(err[-1500:])
    return 0


if __name__ == '__main__':
    sys.exit(main())
