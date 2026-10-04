"""Read private GL snapshots; never treat incomplete captures as replay input."""
import json
import math
import pathlib
import struct


def read_texture_image(folder, image):
    if not image.get('complete') or image.get('gl_error') != 0 or image.get('framebuffer_status') != 0x8cd5:
        raise ValueError('Incomplete texture readback')
    width, height = image['width'], image['height']
    if not 0 < width <= 4096 or not 0 < height <= 4096:
        raise ValueError('Unsupported texture dimensions')
    file = image['file']
    if pathlib.Path(file).name != file:
        raise ValueError('Texture path must be local to capture')
    pixels = (folder / file).read_bytes()
    if len(pixels) != width * height * 4:
        raise ValueError('Truncated texture readback')
    if image.get('upload_compared'):
        file = image['upload_file']
        if pathlib.Path(file).name != file:
            raise ValueError('Upload path must be local to capture')
        upload = (folder / file).read_bytes()
        if len(upload) != len(pixels) or (upload == pixels) != bool(image['upload_equal']):
            raise ValueError('Invalid texture upload comparison')
    return pixels


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
    for unit in draw.get('texture_units', []):
        image = unit.get('texture-de1', {}).get('level0')
        if image is None:
            continue
        read_texture_image(folder, image)
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


def validate_raster_input(draw, width=640, height=480):
    expected = list(struct.unpack('<4I', struct.pack('<4f', 0, 0, width, height)))
    if draw['viewport_bits'] != expected or draw['depth_range_bits'] != [0, 0x3f800000]:
        raise ValueError(f'Raster probe requires the captured {width}x{height} / depth 0..1 layout')


def compare_raster_coverage(folder):
    """Depth coverage only; the magenta clear color is not game/texture fidelity."""
    masks = {}
    for label in ('base', 'base-control', 'equal', 'equal-coverage'):
        blob = (pathlib.Path(folder) / (label + '.rgba')).read_bytes()
        if len(blob) != 640 * 480 * 4:
            raise ValueError('Incomplete raster output')
        masks[label] = {index // 4 for index in range(0, len(blob), 4)
                        if blob[index:index+3] != b'\xff\x00\xff'}
    base = masks['base']
    if not base or not masks['equal-coverage']:
        raise ValueError('Raster probe has no visible coverage')
    return {'base_covered': len(base), 'same_program_missing': len(base - masks['base-control']),
            'equal_covered': len(masks['equal']), 'equal_missing': len(base - masks['equal']),
            'coverage_missing': len(base - masks['equal-coverage']),
            'coverage_extra': len(masks['equal-coverage'] - base)}


def read_depth(folder, phase):
    folder = pathlib.Path(folder)
    record = json.loads((folder / ('depth-' + phase + '.json')).read_text())
    width, height = record.get('width'), record.get('height')
    if (type(width) is not int or type(height) is not int or not 4 <= width <= 4096 or not 1 <= height <= 4096 or
            not record.get('complete') or not record.get('calibrated') or record.get('gl_error') != 0 or record.get('restore_error') != 0 or
            record.get('framebuffer_status') != 0x8cd5 or record.get('encoding') != 'normalized-float32-le' or
            not record.get('texture') or record.get('level') != 0):
        raise ValueError('Incomplete or unsupported depth observation')
    if type(record.get('capture_schema')) is int and record['capture_schema'] == 2:
        if (type(record.get('texture_width')) is not int or type(record.get('texture_height')) is not int or
                record['texture_width'] != width or record['texture_height'] != height):
            raise ValueError('Depth texture extent differs from viewport')
    elif record.get('capture_schema') is not None or (width, height) != (640, 480):
        raise ValueError('Incomplete or unsupported depth observation schema')
    file = record['file']
    if pathlib.Path(file).name != file:
        raise ValueError('Depth path must be local to capture')
    data = (folder / file).read_bytes()
    if len(data) != width * height * 4:
        raise ValueError('Truncated depth observation')
    values = struct.unpack(f'<{width * height}f', data)
    if any(not math.isfinite(value) or not 0 <= value <= 1 for value in values):
        raise ValueError('Invalid normalized depth samples')
    return record, data, values


def compare_depth_snapshots(folder):
    captures = [read_depth(pathlib.Path(folder) / label, phase)
                for label in ('base', 'equal') for phase in ('before', 'after')]
    targets = {(row[0]['framebuffer'], row[0]['texture'], row[0]['level'], row[0]['width'], row[0]['height']) for row in captures}
    if len(targets) != 1:
        raise ValueError('Depth observation target changed')
    frames = {row[0].get('presented_frames') for row in captures}
    if len(frames) != 1 or None in frames:
        raise ValueError('Depth observations cross a presentation boundary')
    blobs = [row[1] for row in captures]
    def changed(a, b):
        return sum(a[i:i+4] != b[i:i+4] for i in range(0, len(a), 4))
    base_changed = changed(blobs[0], blobs[1])
    if not base_changed:
        raise ValueError('Depth observation has no measurable base-draw response')
    before, after, later = [row[2] for row in captures[:3]]
    overwritten = [i for i, (a, b, c) in enumerate(zip(before, after, later)) if a != b and b != c]
    return {'pixels': len(captures[0][2]), 'width': captures[0][0]['width'], 'height': captures[0][0]['height'], 'base_changed': base_changed,
            'intervening_changed': changed(blobs[1], blobs[2]),
            'equal_changed': changed(blobs[2], blobs[3]),
            'after_base_equals_before_equal': blobs[1] == blobs[2],
            'presented_frames': next(iter(frames)), 'base_overwritten': len(overwritten),
            'base_overwritten_closer': sum(later[i] < after[i] for i in overwritten),
            'base_overwritten_farther': sum(later[i] > after[i] for i in overwritten),
            'after_base_range': [min(captures[1][2]), max(captures[1][2])]}


def compare_live_depth(folder):
    draws = [load_draw(pathlib.Path(folder) / label) for label in ('base', 'equal')]
    result = compare_depth_snapshots(folder)
    for draw in draws:
        validate_raster_input(draw, result['width'], result['height'])
    if draws[0].get('presented_frames') is None or draws[0]['presented_frames'] != draws[1].get('presented_frames'):
        raise ValueError('Depth observation draws cross a presentation boundary')
    names = {'c[0]', 'c[1]', 'c[2]', 'c[3]', 'c[58]', 'c[59]', 'viewport_scale', 'viewport_offset', 'screen_offset'}
    constants = [{row['name']: row['bits'] for row in draw['uniforms'] if row['name'] in names} for draw in draws]
    if len(constants[0]) != len(names) or constants[0] != constants[1] or position_bytes(draws[0]) != position_bytes(draws[1]):
        raise ValueError('Depth observation draw positions/projection differ')
    if result['presented_frames'] != draws[0]['presented_frames']:
        raise ValueError('Depth observations do not match the captured draw frame')
    return result


def compare_native_pixels(folder):
    """Full native color response, not a coverage mask or an occlusion verdict."""
    folder = pathlib.Path(folder)
    depth = compare_live_depth(folder)
    draw = load_draw(folder / 'equal')
    record = json.loads((folder / 'equal/native-pixels.json').read_text())
    live = json.loads((folder / 'equal/native-live.json').read_text())
    if (not record.get('complete') or not record.get('color_copy_equal') or
            record.get('gl_error') != 0 or record.get('restore_error') != 0 or
            record.get('framebuffer_status') != 0x8cd5 or record.get('width') != 640 or record.get('height') != 480 or
            record.get('depth_bits') != 24 or record.get('stencil_bits') != 8 or
            not live.get('complete') or live.get('gl_error') != 0):
        raise ValueError('Incomplete or unsupported native pixel observation')
    depth_record = read_depth(folder / 'equal', 'before')[0]
    copied_depth = read_depth(folder / 'equal', 'native-copy')
    if (copied_depth[0].get('presented_frames') != depth['presented_frames'] or
            copied_depth[1] != read_depth(folder / 'equal', 'before')[1]):
        raise ValueError('Native cloned depth differs from live depth')
    if (record.get('program') != draw['program'] or record.get('framebuffer') != draw['framebuffer']['draw'] or
            live.get('framebuffer') != record['framebuffer'] or record.get('depth_texture') != depth_record['texture']):
        raise ValueError('Native pixel program/target differs from captured draw')
    if any(row.get('presented_frames') != depth['presented_frames'] for row in (record, live, draw)):
        raise ValueError('Native pixels cross a presentation boundary')
    blobs = {}
    for label in ('before', 'equal', 'always', 'repeat', 'live'):
        data = (folder / 'equal' / ('native-' + label + '.rgba')).read_bytes()
        if len(data) != 640 * 480 * 4:
            raise ValueError('Truncated native pixel observation')
        blobs[label] = data
    if blobs['equal'] != blobs['repeat'] or blobs['equal'] != blobs['live']:
        raise ValueError('Native clone repeat/live color mismatch')
    def changed(a, b):
        return {i // 4 for i in range(0, len(a), 4) if a[i:i+4] != b[i:i+4]}
    response = changed(blobs['before'], blobs['always'])
    if not response:
        raise ValueError('Native ALWAYS draw has no measurable color response')
    difference = changed(blobs['equal'], blobs['always'])
    equal_response = changed(blobs['before'], blobs['equal'])
    always_only = response - equal_response
    before, after, later = [read_depth(folder / label, phase)[2] for label, phase in
                            (('base', 'before'), ('base', 'after'), ('equal', 'before'))]
    base_changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    closer = {i for i in base_changed if later[i] < after[i]}
    unchanged = {i for i in base_changed if later[i] == after[i]}
    return {'pixels': 640 * 480, 'presented_frames': depth['presented_frames'],
            'equal_color_changed': len(equal_response),
            'always_color_changed': len(response), 'equal_always_color_different': len(difference),
            'different_at_base_changed': len(difference & base_changed),
            'different_at_base_later_closer': len(difference & closer),
            'different_at_base_depth_unchanged': len(difference & unchanged),
            'always_only_color_response': len(always_only),
            'always_only_at_base_later_closer': len(always_only & closer),
            'always_only_at_base_depth_unchanged': len(always_only & unchanged),
            'repeat_equal': True, 'live_equal': True}


def compare_material_pair(folder, key, program, frame):
    folders = [folder / (key + '-' + phase + '-material') for phase in ('before', 'after')]
    records = [json.loads((path / 'material.json').read_text()) for path in folders]
    for record in records:
        if (not record.get('complete') or record.get('gl_error') != 0 or
                record.get('program') != program or record.get('presented_frames') != frame):
            raise ValueError('Incomplete or crossed-frame material capture')
        uniforms = record.get('uniforms', {})
        for name in ('alpha_reference', 'texture_scale[0]', 'texture_scale[1]', 'texture_lod_bias'):
            if name not in uniforms:
                raise ValueError('Missing material uniform')
        for name, bits in uniforms.items():
            if (len(bits) != (1 if name == 'alpha_reference' else 4) or
                    any(not isinstance(bit, int) or bit < 0 or bit > 0xffffffff for bit in bits) or
                    any(not math.isfinite(value) for value in struct.unpack('<' + 'f' * len(bits), struct.pack('<' + 'I' * len(bits), *bits)))):
                raise ValueError('Invalid material uniform bits')
        textures = record.get('textures', [])
        if len(textures) != 2 or [texture.get('stage') for texture in textures] != [0, 1]:
            raise ValueError('Missing material texture stages')
        for texture in textures:
            if (texture.get('unit') not in range(4) or not isinstance(texture.get('object'), int) or
                    texture['object'] <= 0):
                raise ValueError('Invalid material texture binding')
    if records[0] != records[1]:
        raise ValueError('Material state changed during draw')
    for stage in range(2):
        images = [read_texture_image(path, record['textures'][stage]['level0']) for path, record in zip(folders, records)]
        if images[0] != images[1]:
            raise ValueError('Material texture changed during draw')
    return {'textures_unchanged': True, 'uniforms_unchanged': True}


def compare_color_trace(folder, frame, require_materials=False):
    """Validate a one-frame draw timeline; changes are not artifact labels."""
    folder = pathlib.Path(folder)
    files = sorted(folder.glob('*-before.json'))
    if not files or len(files) > 1024 or (folder / 'overflow.txt').exists():
        raise ValueError('Empty or excessive color trace')
    if len(list(folder.glob('*-after.json'))) != len(files):
        raise ValueError('Unpaired color trace')
    result = []
    for serial, file in enumerate(files):
        key = f'{serial:04d}'
        if file.name != key + '-before.json':
            raise ValueError('Noncontiguous color trace')
        rows = [json.loads((folder / (key + '-' + phase + '.json')).read_text()) for phase in ('before', 'after')]
        for row in rows:
            if any(not isinstance(row.get(name), int) or row[name] <= 0 for name in ('framebuffer', 'program', 'count')):
                raise ValueError('Invalid color trace draw identifiers')
            if (not row.get('complete') or row.get('gl_error') != 0 or row.get('width') != 640 or row.get('height') != 480 or
                    not row.get('framebuffer') or not row.get('program') or row.get('presented_frames') != frame):
                raise ValueError('Incomplete or crossed-frame color trace')
            for stage in ('vertex', 'fragment'):
                if not (folder / f"program-{row['program']}-{stage}.glsl").read_bytes():
                    raise ValueError('Missing color trace program source')
        if rows[0] != rows[1]:
            raise ValueError('Color trace draw state changed')
        data = [(folder / (key + '-' + phase + '.rgba')).read_bytes() for phase in ('before', 'after')]
        if any(len(blob) != 640 * 480 * 4 for blob in data):
            raise ValueError('Truncated color trace')
        changed = [i // 4 for i in range(0, len(data[0]), 4) if data[0][i:i+4] != data[1][i:i+4]]
        row = {'key': key, 'program': rows[0]['program'], 'count': rows[0]['count'],
               'mode': rows[0]['mode'], 'depth_function': rows[0]['depth_function'], 'changed': len(changed)}
        if rows[0].get('material_captured'):
            row['material'] = compare_material_pair(folder, key, rows[0]['program'], frame)
        result.append(row)
    if require_materials and not any(row.get('material') for row in result):
        raise ValueError('No matching material captured')
    return {'presented_frames': frame, 'draws': result}
