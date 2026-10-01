#!/usr/bin/env python3
"""Mac smoke test of the Xbox engine build (docs/XBOX-ENGINE.md).

Runs ref/xbox-build/out/halopad-xbox three times, each in its own scratch
data folder that links the extracted maps (the player's saves and settings
are not touched), and checks:

  menu      the game reaches its main menu and draws a picture
  campaign  init.txt's "map_name levels\a10\a10" loads The Pillar of Autumn
  match     a Blood Gulch system link game with one stand-in machine
            (upstream's tools/system_link_bots.py) plays, with hits

Writes result.json and the frames to --out. Exit status 0 only if all pass.

Usage: smoke-mac.py [--out DIR]
"""
import argparse
import datetime
import json
import os
import pathlib
import re
import signal
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
WORK = ROOT / "ref" / "xbox-build"
EXE = WORK / "out" / "halopad-xbox"
IMAGE = WORK / "out" / "halo_guest.elf"
ANGLE = WORK / "angle"
ENGINE = WORK / "vol" / "engine"


def lit_fraction(ppm):
    """The share of sampled drawable pixels that are not black."""
    data = ppm.read_bytes()
    parts = data.split(b"\n", 3)
    pixels = parts[3]
    count = len(pixels) // 3
    step = max(1, count // 100000)
    lit = total = 0
    for index in range(0, count, step):
        r, g, b = pixels[index * 3:index * 3 + 3]
        total += 1
        lit += (r + g + b) > 45
    return lit / max(1, total)


def scratch(out, name, init=None):
    folder = out / (name + "-data")
    folder.mkdir()
    maps = folder / "maps"
    if not maps.exists():
        maps.symlink_to(WORK / "data" / "maps")
    init_file = folder / "init.txt"
    if init:
        init_file.write_text(init + "\n")
    elif init_file.exists():
        init_file.write_text("")
    return folder


def run(name, seconds, env_extra, init=None, during=None, out=None):
    folder = scratch(out, name, init)
    frame = out / f"{name}.ppm"
    log = out / f"{name}.log"
    env = dict(os.environ, XG_FRAME_DUMP=str(frame), XG_FRAME_DUMP_SECONDS="3", HALO_NET_ONLINE="false", **env_extra)
    with open(log, "w") as handle:
        game = subprocess.Popen([str(EXE), "--image", str(IMAGE), "--data", str(folder), "--angle", str(ANGLE)],
                                stdout=handle, stderr=subprocess.STDOUT, env=env)
        helper = during() if during else None
        deadline = time.time() + seconds
        while time.time() < deadline and game.poll() is None:
            time.sleep(1)
        alive = game.poll() is None
        for process in (helper, game):
            if process and process.poll() is None:
                process.send_signal(signal.SIGTERM)
                try:
                    process.wait(5)
                except subprocess.TimeoutExpired:
                    process.kill()
    text = log.read_text(errors="replace")
    lit = lit_fraction(frame) if frame.exists() else 0.0
    debug = folder / "debug.txt"
    return {"alive": alive, "log": text, "lit": lit, "frame": frame.name,
            "debug": debug.read_text(errors="replace") if debug.exists() else ""}


def lan_address():
    for interface in ("en0", "en1"):
        result = subprocess.run(["ipconfig", "getifaddr", interface], capture_output=True, text=True)
        if result.stdout.strip():
            return result.stdout.strip()
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=pathlib.Path)
    parser.add_argument("--case", choices=("menu", "campaign", "match"))
    parser.add_argument("--seconds", type=int, help="Bounded runtime for a targeted pass")
    parser.add_argument("--render-diagnostics", action="store_true")
    options = parser.parse_args()
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    out = options.out or WORK / "smoke-results" / stamp
    out.mkdir(parents=True, exist_ok=True)
    results = {}

    for name, seconds in (("menu", 25), ("campaign", 35), ("match", 75)):
        if options.case and name != options.case:
            continue
        extra = {}
        if options.render_diagnostics:
            extra.update(XG_GL_TRACE=str(out / name), HALO_GPU_STATS="1", HALO_GL_DEBUG="1")
        address = lan_address() if name == "match" else None
        bots = lambda: subprocess.Popen([sys.executable, str(ROOT / "scripts/xbox/network-bot.py"), "--host", "127.0.0.1",
                                         "--machines", "1", "--first-address", address, "--start", "--seconds", "80"],
                                        stdout=open(out / "bots.log", "w"), stderr=subprocess.STDOUT,
                                        preexec_fn=lambda: time.sleep(8))
        if name == "match":
            extra.update(HALO_NETWORK_TEST="host:bloodgulch", HALO_NETWORK_TEST_START="8",
                         HALO_TEST_INPUT="bot:7", HALO_NETWORK_TEST_SHOOT="4")
        case = run(name, options.seconds or seconds, extra,
                   init="map_name levels\\a10\\a10" if name == "campaign" else None,
                   during=bots if name == "match" and address else None, out=out)
        row = {"pass": case["alive"] and case["lit"] > (0.03 if name == "menu" else 0.005)
                       and "OpenGL" in case["log"] and "signal" not in case["log"],
               "lit": round(case["lit"], 3), "frame": case["frame"]}
        if name == "campaign":
            row["a10_load_requested"] = "starting precaching of map 'a10'" in case["debug"]
            row["pass"] &= row["a10_load_requested"]
        if name == "match":
            ticks = re.findall(r"network test: tick (\d+)", case["log"])
            row.update(ticks_logged=len(ticks), last_tick=int(ticks[-1]) if ticks else 0,
                       shots=case["log"].count("shoots player"))
            row["pass"] &= len(ticks) >= 10 and row["shots"] >= 2 and address is not None
        results[name] = row

    revision = subprocess.run(["git", "-C", str(ENGINE), "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip()
    summary = {"engine_revision": revision, "time": stamp, "results": results,
               "pass": all(item["pass"] for item in results.values())}
    (out / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    return 0 if summary["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
