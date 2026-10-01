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


def match_environment(stationary=False):
    return {'HALO_NETWORK_TEST': 'host:bloodgulch', 'HALO_NETWORK_TEST_START': '8',
            'HALO_TEST_INPUT': '' if stationary else 'bot:7',
            'HALO_NETWORK_TEST_SHOOT': '0' if stationary else '4'}


def depth_probe_pass(text):
    probes = re.findall(r'depth probe: (same-program|separate-program) covered (\d+) failed (\d+) error 0x([0-9a-f]+)', text)
    return (len(probes) == 2 and {row[0] for row in probes} == {'same-program', 'separate-program'} and
            all(int(covered) > 0 and int(failed) == 0 and int(error, 16) == 0
                for _, covered, failed, error in probes))


def capture_depth_phase(folder, label, started):
    """Copy a complete, fresh PPM pair; never bless an in-progress trace write."""
    frames = []
    try:
        for part in ('source', 'destination'):
            path = folder / ('presentation.' + part + '.ppm')
            if path.stat().st_mtime <= started + 2:
                return False
            data = path.read_bytes()
            magic, size, maximum, pixels = data.split(b'\n', 3)
            width, height = map(int, size.split())
            if magic != b'P6' or maximum != b'255' or width <= 0 or height <= 0 or len(pixels) != width * height * 3:
                return False
            frames.append((path, data))
    except (OSError, ValueError):
        return False
    for path, data in frames:
        (folder / (label + '.' + path.name)).write_bytes(data)
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device', required=True, help='Dedicated Simulator UDID')
    parser.add_argument('--out', type=pathlib.Path)
    parser.add_argument('--case', choices=('menu', 'campaign', 'match'), help='Rerun one failing case')
    parser.add_argument('--seconds', type=int, help='Override the bounded runtime for a targeted pass')
    parser.add_argument('--render-diagnostics', action='store_true',
                        help='Capture before/after presentation and log renderer statistics (slower)')
    parser.add_argument('--stationary-match', action='store_true',
                        help='Rendering diagnostic only: no scripted movement, shooting or gathering')
    args = parser.parse_args()
    if args.stationary_match and (args.case != 'match' or not args.render_diagnostics):
        parser.error('--stationary-match requires --case match --render-diagnostics')
    depth_pair = args.render_diagnostics and os.environ.get('XG_DEPTH_COMPARE') == 'paired'
    if args.render_diagnostics and os.environ.get('XG_DEPTH_COMPARE', '') not in ('', 'lequal', 'always', 'paired'):
        parser.error('XG_DEPTH_COMPARE must be lequal, always or paired')
    if depth_pair and not args.stationary_match:
        parser.error('paired depth requires --stationary-match')
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
        pair_index, pair_started = 0, None
        pair_names = ('equal', 'always', 'equal-restored')
        control = folder / 'presentation.depth-mode'
        if depth_pair:
            control.write_text('equal\n')
        env = {k: v for k, v in os.environ.items() if not k.startswith('SIMCTL_CHILD_')}
        child = {'HALOPAD_CHOOSE': 'xbox', 'HALO_NET_ONLINE': 'false',
                 'XG_DATA': str(folder), 'XG_SAVE': str(folder / 'save'), 'XG_TOUCH_SHOW': '1',
                 'XG_FRAME_DUMP': str(frame), 'XG_FRAME_DUMP_SECONDS': '10'}
        if name == 'match':
            child.update(match_environment(args.stationary_match))
        env.update({'SIMCTL_CHILD_' + k: v for k, v in child.items()})
        if os.environ.get('XG_GL_CHECK'):
            env['SIMCTL_CHILD_XG_GL_CHECK'] = os.environ['XG_GL_CHECK']
        if args.render_diagnostics:
            child.update(XG_GL_TRACE=str(folder / 'presentation'), HALO_GPU_STATS='1', HALO_GL_DEBUG='1')
            for setting in ('HALO_GPU_TRACE', 'HALO_GPU_TRACE_CONSTANTS'):
                if os.environ.get(setting):
                    child[setting] = os.environ[setting]
            if os.environ.get('XG_DUMP_SHADERS'):
                shaders = folder / 'shaders'
                shaders.mkdir()
                child['HALO_GPU_DUMP_SHADERS'] = str(shaders)
            if os.environ.get('XG_BLIT_PROBE'):
                child['XG_BLIT_PROBE'] = os.environ['XG_BLIT_PROBE']
            if os.environ.get('XG_DEPTH_PROBE'):
                child['XG_DEPTH_PROBE'] = os.environ['XG_DEPTH_PROBE']
            if os.environ.get('XG_PRESENT_RAW_BLIT'):
                child['XG_PRESENT_RAW_BLIT'] = os.environ['XG_PRESENT_RAW_BLIT']
            if os.environ.get('XG_NO_EXTENSION'):
                child['XG_NO_EXTENSION'] = os.environ['XG_NO_EXTENSION']
            if os.environ.get('XG_DEPTH_COMPARE'):
                child['XG_DEPTH_COMPARE'] = os.environ['XG_DEPTH_COMPARE']
            env.update({'SIMCTL_CHILD_' + k: v for k, v in child.items()})
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
                if depth_pair and pair_index < len(pair_names):
                    if pair_started is None and 'network test: tick ' in text:
                        pair_started = time.time()
                    # Allow a full trace interval after the phase change; reject
                    # stale frames from the menu or the preceding depth mode.
                    confirmed = (pair_index == 0 or
                                 'paired depth mode: ' + ('always' if pair_index == 1 else 'equal') in text)
                    if (pair_started and time.time() - pair_started >= 18 and confirmed and
                            capture_depth_phase(folder, pair_names[pair_index], pair_started)):
                        label = pair_names[pair_index]
                        print('depth pair captured:', label, flush=True)
                        pair_index += 1
                        if pair_index < len(pair_names):
                            control.write_text(('always' if pair_index == 1 else 'equal') + '\n')
                            pair_started = time.time()
                if name == 'match' and address and helper is None and 'network test: hosting' in text:
                    helper_log = (folder / 'bot.log').open('w')
                    helper = subprocess.Popen([sys.executable, str(ROOT / 'scripts/xbox/network-bot.py'),
                                               '--host', '127.0.0.1', '--first-address', address,
                                               '--machines', '1', '--start', '--seconds',
                                               str(max(70, seconds + 5) if args.stationary_match else 70)],
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
            row['pass'] &= len(ticks) >= 10 and (args.stationary_match or row['shots'] >= 2)
        if args.render_diagnostics:
            row['presentation_captured'] = all((folder / ('presentation.' + part + '.ppm')).exists()
                                               for part in ('source', 'destination'))
            row['pass'] &= row['presentation_captured']
        if depth_pair:
            row['depth_pair_complete'] = pair_index == len(pair_names)
            row['pass'] &= row['depth_pair_complete']
        if args.render_diagnostics and os.environ.get('XG_DEPTH_PROBE'):
            row['depth_probe_pass'] = depth_probe_pass(text)
            row['pass'] &= row['depth_probe_pass']
        results[name] = row
        print(name, json.dumps(row), flush=True)
    result = {'engine_revision': manifest['revision'], 'device': args.device, 'results': results,
              'render_diagnostics': args.render_diagnostics,
              'blit_probe': bool(args.render_diagnostics and os.environ.get('XG_BLIT_PROBE')),
              'raw_present_blit': bool(args.render_diagnostics and os.environ.get('XG_PRESENT_RAW_BLIT')),
              'hidden_extension': os.environ.get('XG_NO_EXTENSION') if args.render_diagnostics else None,
              'depth_compare': os.environ.get('XG_DEPTH_COMPARE') if args.render_diagnostics else None,
              'depth_probe': bool(args.render_diagnostics and os.environ.get('XG_DEPTH_PROBE')),
              'trace_frame': os.environ.get('HALO_GPU_TRACE') if args.render_diagnostics else None,
              'dump_shaders': bool(args.render_diagnostics and os.environ.get('XG_DUMP_SHADERS')),
              'match_mode': 'stationary-render-diagnostic' if args.stationary_match else 'scripted-combat',
              'pass': all(row['pass'] for row in results.values())}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    return 0 if result['pass'] else 1


if __name__ == '__main__':
    sys.exit(main())
