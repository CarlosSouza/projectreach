#!/usr/bin/env python3
"""Bounded Xbox regression pass in an explicitly selected iPad Simulator.

Use after building the Xbox library and HaloPad app. Saves/settings are
isolated in the evidence directory; the player's maps are linked read-only.
Screenshots still need visual review. A stand-in network machine is not a
second playable client or human gameplay acceptance.
"""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
WORK = ROOT / 'ref/xbox-build'
BUNDLE = 'dev.halopad.HaloPad'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device', required=True, help='Dedicated Simulator UDID')
    parser.add_argument('--out', type=pathlib.Path)
    parser.add_argument('--case', choices=('menu', 'campaign', 'match'), help='Rerun one failing case')
    parser.add_argument('--seconds', type=int, help='Override the bounded runtime for a targeted pass')
    args = parser.parse_args()
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    out = (args.out or WORK / 'simulator-results' / stamp).resolve()
    out.mkdir(parents=True, exist_ok=True)
    # Do not default to "booted": other projects may be testing concurrently.
    subprocess.run(['xcrun', 'simctl', 'boot', args.device], capture_output=True)
    subprocess.run(['xcrun', 'simctl', 'bootstatus', args.device, '-b'], check=True, capture_output=True)
    app = pathlib.Path(subprocess.check_output(
        ['xcrun', 'simctl', 'get_app_container', args.device, BUNDLE, 'app'], text=True).strip())
    manifest = json.loads((app / 'data/xbox/build.json').read_text())
    guest = app / 'data/xbox/halo_guest.elf'
    if hashlib.sha256(guest.read_bytes()).hexdigest() != manifest['guest_sha256']:
        raise SystemExit('Installed guest does not match its build manifest')
    results = {}
    address = None
    for interface in ('en0', 'en1'):
        result = subprocess.run(['ipconfig', 'getifaddr', interface], capture_output=True, text=True)
        if result.stdout.strip():
            address = result.stdout.strip()
            break
    for name, seconds in (('menu', 15), ('campaign', 60), ('match', 65)):
        if args.case and name != args.case:
            continue
        seconds = args.seconds or seconds
        folder = out / name
        folder.mkdir()
        (folder / 'maps').symlink_to(WORK / 'data/maps')
        if name == 'campaign':
            (folder / 'init.txt').write_text('map_name levels\\a10\\a10\n')
        frame = folder / 'frame.ppm'
        log = folder / 'stdout.log'
        error = folder / 'stderr.log'
        env = {k: v for k, v in os.environ.items() if not k.startswith('SIMCTL_CHILD_')}
        child = {'HALOPAD_CHOOSE': 'xbox', 'HALO_NET_ONLINE': 'false',
                 'XG_DATA': str(folder), 'XG_SAVE': str(folder / 'save'), 'XG_TOUCH_SHOW': '1',
                 'XG_FRAME_DUMP': str(frame), 'XG_FRAME_DUMP_SECONDS': '10'}
        if name == 'match':
            child.update(HALO_NETWORK_TEST='host:bloodgulch', HALO_NETWORK_TEST_START='8',
                         HALO_TEST_INPUT='bot:7', HALO_NETWORK_TEST_SHOOT='4')
        env.update({'SIMCTL_CHILD_' + k: v for k, v in child.items()})
        if os.environ.get('XG_GL_CHECK'):
            env['SIMCTL_CHILD_XG_GL_CHECK'] = os.environ['XG_GL_CHECK']
        subprocess.run(['xcrun', 'simctl', 'launch', '--terminate-running-process',
                        '--stdout=' + str(log), '--stderr=' + str(error), args.device, BUNDLE],
                       check=True, env=env, capture_output=True)
        helper = None
        helper_log = None
        deadline = time.monotonic() + seconds
        try:
            while time.monotonic() < deadline:
                text = log.read_text(errors='replace') if log.exists() else ''
                if error.exists():
                    text += error.read_text(errors='replace')
                if name == 'match' and address and helper is None and 'network test: hosting' in text:
                    helper_log = (folder / 'bot.log').open('w')
                    helper = subprocess.Popen([sys.executable, str(ROOT / 'scripts/xbox/network-bot.py'),
                                               '--host', '127.0.0.1', '--first-address', address,
                                               '--machines', '1', '--start', '--seconds', '70'],
                                              stdout=helper_log, stderr=subprocess.STDOUT)
                time.sleep(1)
            subprocess.run(['xcrun', 'simctl', 'io', args.device, 'screenshot', str(folder / 'screen.png')],
                           check=True, capture_output=True)
        finally:
            if helper and helper.poll() is None:
                helper.terminate()
                helper.wait(timeout=5)
            if helper_log:
                helper_log.close()
            subprocess.run(['xcrun', 'simctl', 'terminate', args.device, BUNDLE], capture_output=True)
        text = log.read_text(errors='replace') + error.read_text(errors='replace')
        debug = (folder / 'debug.txt').read_text(errors='replace') if (folder / 'debug.txt').exists() else ''
        pixels = frame.read_bytes().split(b'\n', 3)[-1] if frame.exists() else b''
        lit = sum(sum(pixels[i:i+3]) > 45 for i in range(0, len(pixels), 300)) / max(1, len(pixels) / 300)
        okay = 'OpenGL ES 3.0' in text and lit > 0.005 and '[xbox] signal' not in text
        row = {'pass': okay, 'lit': round(lit, 3)}
        if name == 'campaign':
            row['a10_load_requested'] = "starting precaching of map 'a10'" in debug
            row['pass'] &= row['a10_load_requested']
        if name == 'match':
            ticks = re.findall(r'network test: tick (\d+)', text)
            row.update(last_tick=int(ticks[-1]) if ticks else 0, shots=text.count('shoots player'))
            row['pass'] &= len(ticks) >= 10 and row['shots'] >= 2
        results[name] = row
        print(name, json.dumps(row), flush=True)
    result = {'engine_revision': manifest['revision'], 'device': args.device, 'results': results,
              'pass': all(row['pass'] for row in results.values())}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    return 0 if result['pass'] else 1


if __name__ == '__main__':
    sys.exit(main())
