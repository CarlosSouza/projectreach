#!/usr/bin/env python3
"""Extract maps/ from a Halo: Combat Evolved Xbox disc image and identify it.

Reads the image's XDVDFS file system (2048-byte sectors, a volume descriptor
"MICROSOFT*XBOX*MEDIA" at 0x10000 inside the game partition, directories as
binary trees of entries). Writes <out>/maps/ and prints each map's build
string and the SHA-256 of default.xbe, the facts HaloPad records for a disc.

Usage: extract-maps.py <disc image> <out folder> [--identify-only]
"""
import hashlib
import json
import os
import struct
import sys

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"
PARTITIONS = (0, 0x18300000, 0x2080000, 0xFD90000, 0x89D80000)


def find_partition(image):
    for offset in PARTITIONS:
        image.seek(offset + 0x10000)
        if image.read(20) == MAGIC:
            return offset
    raise SystemExit("not an Xbox disc image (no XDVDFS volume found)")


def read_directory(image, base, sector, size):
    """All entries of a directory: name -> (sector, size, is_directory)."""
    image.seek(base + sector * SECTOR)
    data = image.read(size)
    entries = {}
    stack = [0]
    while stack:
        offset = stack.pop()
        if offset + 14 > len(data):
            continue
        left, right, start, length, attributes, name_length = struct.unpack_from("<HHIIBB", data, offset)
        if left == 0xFFFF:
            continue
        name = data[offset + 14:offset + 14 + name_length].decode("latin-1")
        entries[name] = (start, length, bool(attributes & 0x10))
        if left:
            stack.append(left * 4)
        if right:
            stack.append(right * 4)
    return entries


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    path, out = sys.argv[1], sys.argv[2]
    identify_only = "--identify-only" in sys.argv
    with open(path, "rb") as image:
        base = find_partition(image)
        image.seek(base + 0x10000 + 20)
        root_sector, root_size = struct.unpack("<II", image.read(8))
        root = read_directory(image, base, root_sector, root_size)
        lookup = {name.lower(): value for name, value in root.items()}
        if "maps" not in lookup or "default.xbe" not in lookup:
            raise SystemExit("this disc has no maps folder or default.xbe: it is not Halo")
        xbe_start, xbe_size, _ = lookup["default.xbe"]
        image.seek(base + xbe_start * SECTOR)
        xbe_hash = hashlib.sha256(image.read(xbe_size)).hexdigest()
        maps_sector, maps_size, _ = lookup["maps"]
        maps = read_directory(image, base, maps_sector, maps_size)
        report = {"image": os.path.basename(path), "default_xbe_sha256": xbe_hash, "maps": {}}
        if not any(name.lower() == "ui.map" for name in maps):
            raise SystemExit("the maps folder has no ui.map: it is not a Halo disc")
        target = os.path.join(out, "maps")
        partial = target + ".partial"
        if not identify_only:
            os.makedirs(partial, exist_ok=True)
        for name, (start, length, is_directory) in sorted(maps.items()):
            if is_directory:
                continue
            image.seek(base + start * SECTOR)
            header = image.read(min(length, 0x800))
            build = None
            if name.lower().endswith(".map") and header[:4] == b"daeh":
                build = header[0x40:0x60].split(b"\0")[0].decode("latin-1")
            report["maps"][name] = {"size": length, "build": build}
            if identify_only:
                continue
            image.seek(base + start * SECTOR)
            with open(os.path.join(partial, name), "wb") as output:
                remaining = length
                while remaining:
                    chunk = image.read(min(remaining, 8 << 20))
                    output.write(chunk)
                    remaining -= len(chunk)
        if not identify_only:
            if os.path.isdir(target):
                raise SystemExit("%s already exists; move it aside first" % target)
            os.rename(partial, target)
    builds = sorted({m["build"] for m in report["maps"].values() if m["build"]})
    report["builds"] = builds
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
