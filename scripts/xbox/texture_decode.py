"""Independent RGB565 oracle for private captured Xbox texture bytes.

Morton addresses are constructed directly from coordinate bits, rather than
upstream's mask/spread implementation. Reference layout:
https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/pgraph/swizzle.c
Output is BGRA storage, before GL swizzles.
"""
import argparse
import json
import pathlib
import struct


def cache_symbol(symbols):
    matches = [line.split() for line in symbols.splitlines()
               if len(line.split()) == 3 and line.split()[2] == 'texture_buckets']
    if len(matches) != 1 or matches[0][1] not in ('b', 'B'):
        raise ValueError('Missing or ambiguous texture cache symbol')
    address = int(matches[0][0], 16)
    if not 0x88000000 <= address <= 0xffffbfff or address % 4:
        raise ValueError('Invalid texture cache address')
    return address


def morton_index(x, y, width, height):
    address = 0
    output = 0
    for bit in range(max(width, height).bit_length() - 1):
        for value, extent in ((x, width), (y, height)):
            if (1 << bit) < extent:
                address |= ((value >> bit) & 1) << output
                output += 1
    return address


def decode_rgb565(source, width, height, linear=False, pitch=None):
    if not 0 < width <= 4096 or not 0 < height <= 4096:
        raise ValueError('Invalid texture dimensions')
    if not linear and (width & (width - 1) or height & (height - 1)):
        raise ValueError('Swizzled dimensions must be powers of two')
    pitch = width * 2 if pitch is None else pitch
    if pitch < width * 2 or len(source) != pitch * height or (not linear and pitch != width * 2):
        raise ValueError('Invalid RGB565 source layout')
    output = bytearray()
    for y in range(height):
        for x in range(width):
            offset = y * pitch + x * 2 if linear else morton_index(x, y, width, height) * 2
            value = struct.unpack_from('<H', source, offset)[0]
            r, g, b = value >> 11, (value >> 5) & 63, value & 31
            output.extend(((b << 3) | (b >> 2), (g << 2) | (g >> 4), (r << 3) | (r >> 2), 255))
    return bytes(output)


def compare_texture(folder, image):
    record = image['xbox_source']
    if not record.get('supported') or not record.get('complete') or record['format'] not in (5, 17):
        raise ValueError('Unsupported or incomplete Xbox source')
    if record['width'] != image['width'] or record['height'] != image['height']:
        raise ValueError('Xbox source dimensions differ from readback')
    for key in ('file',):
        if pathlib.Path(record[key]).name != record[key]:
            raise ValueError('Xbox source path must be local to capture')
    source = (pathlib.Path(folder) / record['file']).read_bytes()
    if len(source) != record['length']:
        raise ValueError('Truncated Xbox source')
    expected = decode_rgb565(source, record['width'], record['height'], bool(record['linear']), record['pitch'])
    if pathlib.Path(image['file']).name != image['file']:
        raise ValueError('Texture path must be local to capture')
    actual = (pathlib.Path(folder) / image['file']).read_bytes()
    if len(actual) != len(expected):
        raise ValueError('Truncated texture readback')
    changed = sum(actual[i:i+4] != expected[i:i+4] for i in range(0, len(actual), 4))
    return {'file': record['file'], 'width': record['width'], 'height': record['height'],
            'format': record['format'], 'pixels': len(expected) // 4, 'changed_pixels': changed,
            'identical': actual == expected}


def compare_captures(capture):
    # Full draw validation also checks the GPU-vs-upload reference bytes.
    from draw_capture import load_draw
    results = []
    for label in ('base', 'equal'):
        folder = pathlib.Path(capture) / label
        draw = load_draw(folder)
        seen = set()
        for unit in draw['texture_units']:
            image = unit.get('texture-de1', {}).get('level0', {})
            if image.get('xbox_source', {}).get('supported') and image['file'] not in seen:
                seen.add(image['file'])
                results.append(dict(draw=label, **compare_texture(folder, image)))
    return {'textures': results, 'pass': bool(results) and all(row['identical'] for row in results)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=pathlib.Path)
    args = parser.parse_args()
    result = compare_captures(args.capture)
    print(json.dumps(result, indent=2))
    return 0 if result['pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
