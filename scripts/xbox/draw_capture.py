"""Read private GL snapshots; never treat incomplete captures as replay input."""
import json
import math
import pathlib
import struct


def load_draw(folder):
    folder = pathlib.Path(folder)
    draw = json.loads((folder / 'draw.json').read_text())
    if not draw.get('complete') or draw.get('gl_error') != 0:
        raise ValueError('Incomplete GL capture')
    for name in ('vertex.glsl', 'fragment.glsl'):
        if not (folder / name).read_bytes():
            raise ValueError('Missing shader source')
    buffers = {}
    for key, record in draw['buffers'].items():
        file = record['file']
        if pathlib.Path(file).name != file:
            raise ValueError('Buffer path must be local to capture')
        data = (folder / file).read_bytes()
        if len(data) != record['size']:
            raise ValueError('Truncated captured buffer')
        buffers[int(key)] = data
    index_size = {0x1403: 2, 0x1405: 4}.get(draw['index_type'])
    if not index_size or draw['count'] <= 0 or draw['index_offset'] < 0:
        raise ValueError('Unsupported index layout')
    end = draw['index_offset'] + draw['count'] * index_size
    elements = buffers.get(draw['element_buffer'], b'')
    if end > len(elements):
        raise ValueError('Index range outside captured buffer')
    if len(draw['attributes']) != 16:
        raise ValueError('Missing attribute state')
    for attribute in draw['attributes']:
        if attribute['enabled'] and attribute['buffer'] not in buffers:
            raise ValueError('Missing attribute buffer')
    draw['_buffers'] = buffers
    draw['_indices'] = struct.unpack('<' + ('H' if index_size == 2 else 'I') * draw['count'],
                                      elements[draw['index_offset']:end])
    return draw


def position_bytes(draw):
    attribute = draw['attributes'][0]
    if not attribute['enabled'] or attribute['type'] != 0x1406:
        raise ValueError('Expected a float position stream')
    size = attribute['size'] * 4
    stride = attribute['stride'] or size
    data = draw['_buffers'][attribute['buffer']]
    positions = []
    for index in draw['_indices']:
        start = attribute['offset'] + index * stride
        if start < 0 or start + size > len(data):
            raise ValueError('Position range outside captured buffer')
        positions.append(data[start:start + size])
    return positions


def compare_clip_positions(folder, count):
    data = [(pathlib.Path(folder) / (label + '-position.bin')).read_bytes()
            for label in ('base', 'equal')]
    if count <= 0 or any(len(blob) != count * 16 for blob in data):
        raise ValueError('Incomplete transform-feedback output')
    values = [struct.unpack('<' + 'f' * (count * 4), blob) for blob in data]
    if any(not math.isfinite(value) for row in values for value in row):
        raise ValueError('Nonfinite transform-feedback output')
    a, b = values
    depth_delta = [abs(a[i + 2] / a[i + 3] - b[i + 2] / b[i + 3])
                   for i in range(0, count * 4, 4)
                   if a[i + 3] and b[i + 3]]
    return {'vertices': count, 'identical': data[0] == data[1],
            'changed_vertices': sum(data[0][i:i+16] != data[1][i:i+16] for i in range(0, count * 16, 16)),
            'max_component_delta': max(abs(a - b) for a, b in zip(*values)),
            'max_ndc_depth_delta': max(depth_delta, default=0)}
