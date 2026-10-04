"""Validate bounded generated-mip captures, not original-renderer fidelity."""
import json
import pathlib


def inspect_mips(folder):
    captures = []
    for path in sorted(pathlib.Path(folder).glob('frame-*/mips.json')):
        row = json.loads(path.read_text())
        if not row['complete'] or not row['state_restored'] or row['prior_gl_error'] or row['gl_error']:
            raise ValueError('Incomplete mip capture or GL state/error failure')
        maximum = row['maximum_level']
        if not 0 <= row['base_level'] <= maximum <= 12 or maximum < 1:
            raise ValueError('Unsupported generated mip range')
        if [level['level'] for level in row['levels']] != list(range(maximum + 1)):
            raise ValueError('Missing or duplicate mip level')
        first = row['levels'][0]['pixels']
        width, height = first['width'], first['height']
        if not 2 <= width <= 512 or not 2 <= height <= 512:
            raise ValueError('Unsupported mip extent')
        pixels = []
        for level in row['levels']:
            image = level['pixels']
            index = level['level']
            w, h = max(1, width >> index), max(1, height >> index)
            if not image['complete'] or image['gl_error'] or image['framebuffer_status'] != 0x8cd5:
                raise ValueError('Unreadable mip level')
            if (image['width'], image['height']) != (w, h):
                raise ValueError('Unexpected mip dimensions')
            name = pathlib.Path(image['file'])
            if name.name != str(name):
                raise ValueError('Invalid mip filename')
            data = (path.parent / name).read_bytes()
            if len(data) != w * h * 4:
                raise ValueError('Truncated mip pixels')
            pixels.append(data)
        captures.append((row, pixels))
    captures.sort(key=lambda item: item[0]['presented_frames'])
    if len(captures) != 2:
        raise ValueError('Expected two mip snapshots')
    (a, before), (b, after) = captures
    if a['texture'] != b['texture'] or a['base_level'] != b['base_level'] or a['maximum_level'] != b['maximum_level']:
        raise ValueError('Mismatched mip objects or ranges')
    if b['presented_frames'] - a['presented_frames'] < 60:
        raise ValueError('Snapshots too close together')
    if [len(data) for data in before] != [len(data) for data in after]:
        raise ValueError('Mip extents changed between snapshots')
    return {'complete': True, 'texture': a['texture'],
            'frames': [a['presented_frames'], b['presented_frames']],
            'base_level': a['base_level'], 'maximum_level': a['maximum_level'],
            'levels': [{'level': i, 'width': a['levels'][i]['pixels']['width'],
                        'height': a['levels'][i]['pixels']['height'],
                        'changed_rgb_bytes': sum(x != y for n, (x, y) in enumerate(zip(left, right)) if n % 4 != 3)}
                       for i, (left, right) in enumerate(zip(before, after))]}
