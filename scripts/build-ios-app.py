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
import os
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
        'MinimumOSVersion': '17.0', 'UIDeviceFamily': [1, 2], 'UIRequiresFullScreen': True, 'UILaunchScreen': {},
        'UIStatusBarHidden': True,
        'UISupportedInterfaceOrientations': ['UIInterfaceOrientationLandscapeLeft', 'UIInterfaceOrientationLandscapeRight'],
        'UISupportedInterfaceOrientations~ipad': ['UIInterfaceOrientationLandscapeLeft', 'UIInterfaceOrientationLandscapeRight'],
        'UIApplicationSceneManifest': {'UIApplicationSupportsMultipleScenes': False},
        # the player's Halo folder is copied into Documents (Files app, Finder) or picked from a folder
        'UIFileSharingEnabled': True, 'LSSupportsOpeningDocumentsInPlace': True,
    }
    with open(app / 'Info.plist', 'wb') as f:
        plistlib.dump(info, f)
    # the app's own data: the translated image and modules, the reference machine's files, the
    # registry seed and the input profile (the game files are the player's, imported on the device)
    data = app / 'data'
    (data / 'modules').mkdir(parents=True)
    shutil.copy2(run_core.IMAGE, data / 'image.bin')
    for m in sorted((run_core.IMAGE.parent / 'modules').iterdir()):
        if (m / 'image.bin').is_file():
            (data / 'modules' / m.name).mkdir()
            shutil.copy2(m / 'image.bin', data / 'modules' / m.name / 'image.bin')
    shutil.copytree(ROOT / 'ref' / 'inputs' / 'reference-machine', data / 'reference')
    (data / 'config' / 'runtime').mkdir(parents=True)
    shutil.copy2(ROOT / 'config' / 'runtime' / 'registry-machine.txt', data / 'config' / 'runtime' / 'registry-machine.txt')
    shutil.copy2(ROOT / 'config' / 'profiles' / 'custom-en-1.0.10.0621.json', data / 'profile.json')
    subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(app)], check=True, capture_output=True)
    return app


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=pathlib.Path)
    ap.add_argument('--device', default='E129A00F-D338-4FDC-8AE8-BB243E9BA61B', help='Simulator UDID ("HaloPad iPad Pro 13")')
    ap.add_argument('--launch', action='store_true')
    ap.add_argument('--wait', type=int, default=20, help='seconds to let the app run before the screenshot')
    ap.add_argument('--device-data', action='store_true',
                    help='launch without the Mac data paths: the app uses its bundle and Documents, as on a device (the import screen when no game folder is there)')
    ap.add_argument('--scene', type=pathlib.Path,
                    help='development only: a C file whose halopad_app_entry replaces the core start (evidence scenes in tests/)')
    a = ap.parse_args()
    work = (a.work or max(run_core.PROFILE.glob('run-*/va/haloce.va.ll'), key=lambda p: p.stat().st_mtime).parent.parent).resolve()
    extra = [ROOT / 'port' / 'ios' / name for name in ('HaloPadOverlay.m', 'HaloPadImport.m')] + ([a.scene.resolve()] if a.scene else [])
    exe, _ = run_core.build(work, TARGET, ROOT / 'port' / 'ios' / 'HaloPadApp.m', extra=extra)
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
    # development settings from the Mac's environment: Halo's command line, the network policy,
    # an overlay part to open for the screenshot
    env.update({k: os.environ[k] for k in ('HALOPAD_ARGS', 'HALOPAD_NET', 'HALOPAD_OVERLAY_DEMO', 'HALOPAD_TRACE_NET', 'HALOPAD_TRACE_WINDOWS', 'HALOPAD_NO_OVERLAY', 'HALOPAD_TOUCH_SELFTEST', 'HALOPAD_TRACE_INPUT', 'HALOPAD_TRACE_WEAPON', 'HALOPAD_TRACE_FRAMES', 'HALOPAD_TRACE_LIFECYCLE') if k in os.environ})
    if a.device_data:
        env = {k: v for k, v in env.items() if k not in ('HALOPAD_IMAGE', 'HALOPAD_MODULE_IMAGES', 'HALOPAD_REFERENCE_ROOT', 'HALOPAD_GAME_ROOT',
                                                          'HALOPAD_STATE_ROOT', 'HALOPAD_REPO_ROOT', 'HALOPAD_REGISTRY')}
    child = {'SIMCTL_CHILD_' + k: str(v) for k, v in env.items()}
    dev = a.device
    subprocess.run(['xcrun', 'simctl', 'boot', dev], capture_output=True)          # already booted is fine
    subprocess.run(['xcrun', 'simctl', 'bootstatus', dev, '-b'], check=True, capture_output=True)
    subprocess.run(['xcrun', 'simctl', 'install', dev, str(app)], check=True)
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
