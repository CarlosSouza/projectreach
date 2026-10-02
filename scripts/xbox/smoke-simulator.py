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
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from draw_capture import load_draw, compare_clip_positions, validate_raster_input, compare_raster_coverage, compare_live_depth, compare_native_pixels, compare_color_trace
from texture_decode import cache_symbol, compare_captures, validate_cache_revision
from audio_capture import inspect_audio
from mip_capture import inspect_mips

ROOT = pathlib.Path(__file__).resolve().parents[2]
WORK = ROOT / 'ref/xbox-build'
BUNDLE = 'dev.halopad.HaloPad'


def isolated_environment(folder, frame):
    # Exercise the installed candidate, without online joins or update prompts.
    return {'HALOPAD_CHOOSE': 'xbox', 'HALO_NET_ONLINE': 'false',
            'HALO_NET_JOIN_FROM_CLIPBOARD': 'false', 'HALO_NET_ALLOW_UPNP': 'false',
            'HALO_UPDATE_AUTO': 'false', 'XG_DATA': str(folder),
            'XG_SAVE': str(folder / 'save'), 'XG_TOUCH_SHOW': '1',
            'XG_FRAME_DUMP': str(frame), 'XG_FRAME_DUMP_SECONDS': '10'}


def query_environment(diagnostics=False, environ=None):
    environ = os.environ if environ is None else environ
    return {key: '1' if diagnostics and environ.get(key) == '1' else ''
            for key in ('XG_TRACE_QUERIES', 'XG_TRACE_QUERY_RECTS')}


def match_environment(stationary=False):
    return {'HALO_NETWORK_TEST': 'host:bloodgulch', 'HALO_NETWORK_TEST_START': '8',
            'HALO_TEST_INPUT': '' if stationary else 'bot:7',
            'HALO_NETWORK_TEST_SHOOT': '0' if stationary else '4'}


def campaign_init(map_name):
    return f'map_name levels\\{map_name}\\{map_name}\n'


def campaign_load_requested(debug, map_name):
    return f"starting precaching of map '{map_name}'" in debug


def campaign_environment(scripted=False):
    # Override inherited bot/network settings, including for ordinary campaign passes.
    return {'HALO_NETWORK_TEST': '', 'HALO_NETWORK_TEST_START': '',
            'HALO_NETWORK_TEST_SHOOT': '0', 'HALO_TEST_INPUT': 'bot:7' if scripted else ''}


def depth_probe_pass(text):
    probes = re.findall(r'depth probe: (same-program|separate-program) covered (\d+) failed (\d+) error 0x([0-9a-f]+)', text)
    return (len(probes) == 2 and {row[0] for row in probes} == {'same-program', 'separate-program'} and
            all(int(covered) > 0 and int(failed) == 0 and int(error, 16) == 0
                for _, covered, failed, error in probes))


def renderer_matches(text, renderer):
    if 'OpenGL ES 3.0' not in text:
        return False
    if renderer == 'angle-metal':
        return 'ANGLE Metal Renderer:' in text
    return renderer == 'apple-gles' and 'ANGLE ' not in text


def capture_campaign_frame(folder, since, samples):
    """Retain distinct complete drawable dumps in the late observation window.

    A cinematic fade can make the final ten-second sample black even while
    the displayed scene progresses. Keep the black samples too, not just hits.
    """
    path = folder / 'frame.ppm'
    try:
        before = path.stat()
        if before.st_mtime < since or any(s['mtime_ns'] == before.st_mtime_ns for s in samples):
            return
        data = path.read_bytes()
        after = path.stat()
        if (before.st_mtime_ns, before.st_size) != (after.st_mtime_ns, after.st_size):
            return
        magic, size, maximum, pixels = data.split(b'\n', 3)
        width, height = map(int, size.split())
        if magic != b'P6' or maximum != b'255' or width <= 0 or height <= 0 or len(pixels) != width * height * 3:
            return
    except (OSError, ValueError):
        return
    lit = sum(sum(pixels[i:i+3]) > 45 for i in range(0, len(pixels), 300)) / len(range(0, len(pixels), 300))
    name = f'campaign-frame-{len(samples):02}.ppm'
    (folder / name).write_bytes(data)
    samples.append({'frame': name, 'mtime_ns': before.st_mtime_ns, 'lit': lit})


def campaign_frames_pass(samples):
    return sum(sample['lit'] > 0.005 for sample in samples) >= 2


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
    parser.add_argument('--audio-diagnostics', action='store_true',
                        help='Capture four seconds of output-callback samples after ten seconds; not speaker acceptance')
    parser.add_argument('--stationary-match', action='store_true',
                        help='Rendering diagnostic only: no scripted movement, shooting or gathering')
    parser.add_argument('--campaign-map', choices=('a10', 'a30', 'a50', 'b30'), default='a10',
                        help='Campaign map (a10: Pillar of Autumn; a30: Halo; a50: Truth and Reconciliation; b30: Silent Cartographer)')
    parser.add_argument('--scripted-campaign', action='store_true',
                        help='Rendering diagnostic only: upstream bot movement/look/shoot, not human controls')
    args = parser.parse_args()
    mip_capture = args.render_diagnostics and os.environ.get('XG_CAPTURE_MIPS') == '1'
    if args.campaign_map != 'a10' and args.case != 'campaign':
        parser.error(f'--campaign-map {args.campaign_map} requires --case campaign')
    if args.scripted_campaign and (args.case != 'campaign' or not args.render_diagnostics):
        parser.error('--scripted-campaign requires --case campaign --render-diagnostics')
    if args.stationary_match and (args.case != 'match' or not args.render_diagnostics):
        parser.error('--stationary-match requires --case match --render-diagnostics')
    depth_pair = args.render_diagnostics and os.environ.get('XG_DEPTH_COMPARE') == 'paired'
    if args.render_diagnostics and os.environ.get('XG_DEPTH_COMPARE', '') not in ('', 'lequal', 'always', 'paired'):
        parser.error('XG_DEPTH_COMPARE must be lequal, always or paired')
    if depth_pair and not args.stationary_match:
        parser.error('paired depth requires --stationary-match')
    if args.render_diagnostics and os.environ.get('XG_CAPTURE_MIN_INDICES'):
        try:
            minimum = int(os.environ['XG_CAPTURE_MIN_INDICES'])
            if minimum < 3 or minimum > 100000:
                raise ValueError()
        except ValueError:
            parser.error('XG_CAPTURE_MIN_INDICES must be 3..100000')
        if not os.environ.get('XG_CAPTURE_SHADER_DIR'):
            parser.error('XG_CAPTURE_MIN_INDICES requires XG_CAPTURE_SHADER_DIR')
    raster = args.render_diagnostics and bool(os.environ.get('XG_DRAW_RASTER'))
    exact_count = None
    if args.render_diagnostics and os.environ.get('XG_CAPTURE_INDEX_COUNT'):
        try:
            exact_count = int(os.environ['XG_CAPTURE_INDEX_COUNT'])
            if exact_count < 3 or exact_count > 100000:
                raise ValueError()
        except ValueError:
            parser.error('XG_CAPTURE_INDEX_COUNT must be 3..100000')
        if not os.environ.get('XG_CAPTURE_SHADER_DIR'):
            parser.error('XG_CAPTURE_INDEX_COUNT requires XG_CAPTURE_SHADER_DIR')
    equal_shader = os.environ.get('XG_CAPTURE_EQUAL_SHADER') if args.render_diagnostics else None
    if equal_shader:
        if equal_shader not in ('vs007_0.glsl', 'vs041_0.glsl'):
            parser.error('XG_CAPTURE_EQUAL_SHADER must be vs007_0.glsl or vs041_0.glsl')
        if not os.environ.get('XG_CAPTURE_SHADER_DIR'):
            parser.error('XG_CAPTURE_EQUAL_SHADER requires XG_CAPTURE_SHADER_DIR')
    if args.render_diagnostics and os.environ.get('XG_CAPTURE_BASE_SKIP'):
        try:
            skip = int(os.environ['XG_CAPTURE_BASE_SKIP'])
            if not 0 <= skip <= 64:
                raise ValueError()
        except ValueError:
            parser.error('XG_CAPTURE_BASE_SKIP must be 0..64')
        if not os.environ.get('XG_CAPTURE_SHADER_DIR'):
            parser.error('XG_CAPTURE_BASE_SKIP requires XG_CAPTURE_SHADER_DIR')
    if args.render_diagnostics and os.environ.get('XG_CAPTURE_TEXTURES') and not os.environ.get('XG_CAPTURE_SHADER_DIR'):
        parser.error('XG_CAPTURE_TEXTURES requires XG_CAPTURE_SHADER_DIR')
    xbox_textures = args.render_diagnostics and bool(os.environ.get('XG_CAPTURE_XBOX_TEXTURES'))
    if xbox_textures and not (os.environ.get('XG_CAPTURE_TEXTURES') and os.environ.get('XG_CAPTURE_SHADER_DIR')):
        parser.error('XG_CAPTURE_XBOX_TEXTURES requires texture pixels and shader capture')
    live_depth = args.render_diagnostics and bool(os.environ.get('XG_CAPTURE_DEPTH'))
    if live_depth and not os.environ.get('XG_CAPTURE_SHADER_DIR'):
        parser.error('XG_CAPTURE_DEPTH requires XG_CAPTURE_SHADER_DIR')
    native_pixels = args.render_diagnostics and bool(os.environ.get('XG_CAPTURE_NATIVE_PIXELS'))
    if native_pixels and not live_depth:
        parser.error('XG_CAPTURE_NATIVE_PIXELS requires XG_CAPTURE_DEPTH')
    color_frame = None
    if args.render_diagnostics and os.environ.get('XG_TRACE_COLOR_FRAME'):
        try:
            color_frame = int(os.environ['XG_TRACE_COLOR_FRAME'])
            if not 0 <= color_frame <= 10000:
                raise ValueError()
        except ValueError:
            parser.error('XG_TRACE_COLOR_FRAME must be 0..10000')
        if not os.environ.get('XG_CAPTURE_SHADER_DIR'):
            parser.error('XG_TRACE_COLOR_FRAME requires XG_CAPTURE_SHADER_DIR')
    if raster and os.environ['XG_DRAW_RASTER'] not in ('1', 'renderbuffer', 'texture'):
        parser.error('XG_DRAW_RASTER must be renderbuffer or texture')
    trace_materials = args.render_diagnostics and bool(os.environ.get('XG_TRACE_MATERIALS'))
    if trace_materials and (color_frame is None or not os.environ.get('XG_CAPTURE_TEXTURES')):
        parser.error('XG_TRACE_MATERIALS requires XG_TRACE_COLOR_FRAME and XG_CAPTURE_TEXTURES')
    if raster and not os.environ.get('XG_DRAW_REPLAY'):
        parser.error('XG_DRAW_RASTER requires XG_DRAW_REPLAY')
    if args.render_diagnostics and os.environ.get('XG_DRAW_REPLAY'):
        try:
            for label in ('base', 'equal'):
                draw = load_draw(pathlib.Path(os.environ['XG_DRAW_REPLAY']) / label)
                if raster:
                    validate_raster_input(draw)
        except (OSError, ValueError, KeyError, TypeError) as error:
            parser.error('Invalid replay input: ' + str(error))
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
    buckets = None
    if xbox_textures:
        validate_cache_revision(manifest['revision'])
        llvm = pathlib.Path(os.environ.get('XBOX_LLVM_BIN', '/opt/homebrew/opt/llvm/bin'))
        symbols = subprocess.check_output([str(llvm / 'llvm-nm'), '--defined-only', str(guest)], text=True)
        buckets = cache_symbol(symbols)
    results = {}
    address = None
    for interface in ('en0', 'en1'):
        result = subprocess.run(['ipconfig', 'getifaddr', interface], capture_output=True, text=True)
        if result.stdout.strip():
            address = result.stdout.strip()
            break
    # The cold ANGLE shader build can leave the ten-second dump on loading.
    # Allow a later dump; retain every image/progression gate unchanged.
    menu_seconds = 30 if manifest.get('renderer') == 'angle-metal' else 15
    for name, seconds in (('menu', menu_seconds), ('campaign', 60), ('match', 65)):
        if args.case and name != args.case:
            continue
        seconds = args.seconds or seconds
        folder = out / name
        folder.mkdir()
        (folder / 'maps').symlink_to(WORK / 'data/maps')
        if name == 'campaign':
            (folder / 'init.txt').write_text(campaign_init(args.campaign_map))
        frame = folder / 'frame.ppm'
        log = folder / 'stdout.log'
        error = folder / 'stderr.log'
        pair_index, pair_started = 0, None
        pair_names = ('equal', 'always', 'equal-restored')
        control = folder / 'presentation.depth-mode'
        if depth_pair:
            control.write_text('equal\n')
        env = {k: v for k, v in os.environ.items() if not k.startswith('SIMCTL_CHILD_')}
        child = isolated_environment(folder, frame)
        child['XG_AUDIO_CAPTURE'] = '1' if args.audio_diagnostics else ''
        child.update(campaign_environment(name == 'campaign' and args.scripted_campaign))
        child.update(query_environment(args.render_diagnostics))
        child['XG_CAPTURE_MIPS'] = str(folder / 'mips') if mip_capture else ''
        if name == 'match':
            child.update(match_environment(args.stationary_match))
        env.update({'SIMCTL_CHILD_' + k: v for k, v in child.items()})
        if os.environ.get('XG_GL_CHECK'):
            env['SIMCTL_CHILD_XG_GL_CHECK'] = os.environ['XG_GL_CHECK']
        if args.render_diagnostics:
            child.update(XG_GL_TRACE=str(folder / 'presentation'), HALO_GPU_STATS='1', HALO_GL_DEBUG='1')
            if os.environ.get('XG_CAPTURE_SHADER_DIR'):
                child.update(XG_DRAW_CAPTURE=str(folder / 'draw-capture'),
                             XG_CAPTURE_SHADER_DIR=os.environ['XG_CAPTURE_SHADER_DIR'])
                if os.environ.get('XG_CAPTURE_MIN_INDICES'):
                    child['XG_CAPTURE_MIN_INDICES'] = os.environ['XG_CAPTURE_MIN_INDICES']
                if exact_count is not None:
                    child['XG_CAPTURE_INDEX_COUNT'] = str(exact_count)
                if os.environ.get('XG_CAPTURE_BASE_SKIP'):
                    child['XG_CAPTURE_BASE_SKIP'] = str(skip)
                if equal_shader:
                    child['XG_CAPTURE_EQUAL_SHADER'] = equal_shader
                if os.environ.get('XG_CAPTURE_TEXTURES'):
                    child['XG_CAPTURE_TEXTURES'] = '1'
                if live_depth:
                    child['XG_CAPTURE_DEPTH'] = '1'
                if native_pixels:
                    child['XG_CAPTURE_NATIVE_PIXELS'] = '1'
                if color_frame is not None:
                    child['XG_TRACE_COLOR_FRAME'] = str(color_frame)
                if trace_materials:
                    child['XG_TRACE_MATERIALS'] = '1'
                if buckets is not None:
                    child['XG_TEXTURE_BUCKETS'] = format(buckets, 'x')
            if os.environ.get('XG_DRAW_REPLAY'):
                child.update(XG_DRAW_REPLAY=os.environ['XG_DRAW_REPLAY'],
                             XG_DRAW_REPLAY_OUT=str(folder / 'draw-replay'))
                if raster:
                    child['XG_DRAW_RASTER'] = os.environ['XG_DRAW_RASTER']
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
        campaign_samples = []
        campaign_since = time.time() + max(0, seconds - 30)
        deadline = time.monotonic() + seconds
        try:
            while time.monotonic() < deadline:
                text = log.read_text(errors='replace') if log.exists() else ''
                if error.exists():
                    text += error.read_text(errors='replace')
                if name == 'campaign':
                    capture_campaign_frame(folder, campaign_since, campaign_samples)
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
            if name == 'campaign':
                capture_campaign_frame(folder, campaign_since, campaign_samples)
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
        renderer_ok = renderer_matches(text, manifest.get('renderer', 'apple-gles'))
        visible = campaign_frames_pass(campaign_samples) if name == 'campaign' else lit > 0.005
        okay = renderer_ok and visible and '[xbox] signal' not in text
        row = {'pass': okay, 'lit': round(lit, 3), 'renderer_matches': renderer_ok, 'seconds': seconds}
        if args.audio_diagnostics:
            try:
                row['audio'] = inspect_audio(folder)
                row['pass'] &= row['audio']['signal_present']
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['pass'] = False
                row['audio_error'] = str(error)
        if name == 'campaign':
            row.update(map=args.campaign_map,
                       frame_samples=campaign_samples,
                       input_mode='scripted-render-diagnostic' if args.scripted_campaign else 'no-scripted-input',
                       map_load_requested=campaign_load_requested(debug, args.campaign_map))
            if args.campaign_map == 'a10':
                row['a10_load_requested'] = row['map_load_requested']
            row['pass'] &= row['map_load_requested']
        if name == 'match':
            ticks = re.findall(r'network test: tick (\d+)', text)
            row.update(last_tick=int(ticks[-1]) if ticks else 0, shots=text.count('shoots player'))
            row['pass'] &= len(ticks) >= 10 and (args.stationary_match or row['shots'] >= 2)
        if args.render_diagnostics:
            row['presentation_captured'] = all((folder / ('presentation.' + part + '.ppm')).exists()
                                               for part in ('source', 'destination'))
            row['pass'] &= row['presentation_captured']
        if mip_capture:
            try:
                row['mips'] = inspect_mips(folder / 'mips')
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['pass'] = False
                row['mip_error'] = str(error)
        if depth_pair:
            row['depth_pair_complete'] = pair_index == len(pair_names)
            row['pass'] &= row['depth_pair_complete']
        if args.render_diagnostics and os.environ.get('XG_DEPTH_PROBE'):
            row['depth_probe_pass'] = depth_probe_pass(text)
            row['pass'] &= row['depth_probe_pass']
        if args.render_diagnostics and os.environ.get('XG_CAPTURE_SHADER_DIR'):
            try:
                for label in ('base', 'equal'):
                    load_draw(folder / 'draw-capture' / label)
                row['draw_capture_complete'] = True
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['draw_capture_complete'] = False
                row['draw_capture_error'] = str(error)
            row['pass'] &= row['draw_capture_complete']
        if live_depth:
            try:
                row['live_depth_comparison'] = compare_live_depth(folder / 'draw-capture')
                row['live_depth_complete'] = True
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['live_depth_complete'] = False
                row['live_depth_error'] = str(error)
            row['pass'] &= row['live_depth_complete']
        if xbox_textures:
            try:
                row['xbox_texture_comparison'] = compare_captures(folder / 'draw-capture')
                row['pass'] &= row['xbox_texture_comparison']['pass']
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['pass'] = False
                row['xbox_texture_error'] = str(error)
        if native_pixels:
            try:
                row['native_pixel_comparison'] = compare_native_pixels(folder / 'draw-capture')
                row['native_pixels_complete'] = True
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['native_pixels_complete'] = False
                row['native_pixels_error'] = str(error)
            row['pass'] &= row['native_pixels_complete']
        if color_frame is not None:
            try:
                row['color_trace'] = compare_color_trace(folder / 'draw-capture/color-trace', color_frame, trace_materials)
                row['color_trace_complete'] = True
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['color_trace_complete'] = False
                row['color_trace_error'] = str(error)
            row['pass'] &= row['color_trace_complete']
        if args.render_diagnostics and os.environ.get('XG_DRAW_REPLAY'):
            row['draw_replay_complete'] = 'draw replay: pair complete 1' in text
            try:
                draw = load_draw(pathlib.Path(os.environ['XG_DRAW_REPLAY']) / 'base')
                row['clip_comparison'] = compare_clip_positions(folder / 'draw-replay', draw['count'])
            except (OSError, ValueError, KeyError, TypeError) as error:
                row['draw_replay_complete'] = False
                row['draw_replay_error'] = str(error)
            row['pass'] &= row['draw_replay_complete']
            if raster:
                row['draw_raster_complete'] = 'draw raster: pair complete 1 error 0x0' in text
                try:
                    row['raster_comparison'] = compare_raster_coverage(folder / 'draw-replay')
                except (OSError, ValueError) as error:
                    row['draw_raster_complete'] = False
                    row['draw_raster_error'] = str(error)
                row['pass'] &= row['draw_raster_complete']
        results[name] = row
        print(name, json.dumps(row), flush=True)
    result = {'engine_revision': manifest['revision'], 'renderer': manifest.get('renderer', 'apple-gles'), 'device': args.device, 'results': results,
              'render_diagnostics': args.render_diagnostics,
              'audio_diagnostics': args.audio_diagnostics,
              'mip_capture': mip_capture,
              'blit_probe': bool(args.render_diagnostics and os.environ.get('XG_BLIT_PROBE')),
              'raw_present_blit': bool(args.render_diagnostics and os.environ.get('XG_PRESENT_RAW_BLIT')),
              'hidden_extension': os.environ.get('XG_NO_EXTENSION') if args.render_diagnostics else None,
              'depth_compare': os.environ.get('XG_DEPTH_COMPARE') if args.render_diagnostics else None,
              'depth_probe': bool(args.render_diagnostics and os.environ.get('XG_DEPTH_PROBE')),
              'trace_frame': os.environ.get('HALO_GPU_TRACE') if args.render_diagnostics else None,
              'dump_shaders': bool(args.render_diagnostics and os.environ.get('XG_DUMP_SHADERS')),
              'draw_capture': bool(args.render_diagnostics and os.environ.get('XG_CAPTURE_SHADER_DIR')),
              'native_pixels': native_pixels,
              'base_skip': os.environ.get('XG_CAPTURE_BASE_SKIP') if args.render_diagnostics else None,
              'equal_shader': equal_shader,
              'index_count_filter': exact_count,
              'color_trace_frame': color_frame,
              'trace_materials': trace_materials,
              'texture_pixels': bool(args.render_diagnostics and os.environ.get('XG_CAPTURE_TEXTURES')),
              'xbox_texture_source': xbox_textures,
              'live_depth': live_depth,
              'draw_replay': bool(args.render_diagnostics and os.environ.get('XG_DRAW_REPLAY')),
              'draw_raster': raster,
              'raster_attachments': ('texture' if os.environ.get('XG_DRAW_RASTER') == 'texture' else 'renderbuffer') if raster else None,
              'match_mode': 'stationary-render-diagnostic' if args.stationary_match else 'scripted-combat',
              'pass': all(row['pass'] for row in results.values())}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    return 0 if result['pass'] else 1


if __name__ == '__main__':
    sys.exit(main())
