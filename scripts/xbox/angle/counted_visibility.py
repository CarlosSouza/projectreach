"""Generate an isolated counted-visibility backend from exact reviewed inputs.

Never edit the external checkout. Output is private build material, not source
to publish. Ordinary GLES query results remain boolean in QueryMtl.mm.
"""
import argparse
import hashlib
import json
import pathlib

BASE = pathlib.Path('src/libANGLE/renderer/metal')
INPUTS = {
    'ContextMtl.mm': 'b567a0b42275444e527e20b9e0fab8c12ee1c4368be1f92d4da7de27ad47f67e',
    'DisplayMtl.mm': 'e38ec527cdf2de1f4b64fff57cc4a1ba133dcbaefdb1b6dac8b38c43fb4c69d4',
    'shaders/mtl_internal_shaders_src_autogen.h': 'a6303b9710a3d8a812a11a7ed16442187fc3e7c5133065a915fc04e72f460560',
}
OLD_SUM = 'finalResult16x4 = finalResult16x4 | renderpassResult;'
NEW_SUM = '''uint4 sum = uint4(finalResult16x4) + uint4(renderpassResult);
        sum.y += sum.x >> 16;
        sum.z += sum.y >> 16;
        sum.w += sum.z >> 16;
        // Saturate on 64-bit overflow; keep ordinary boolean semantics nonzero.
        finalResult16x4 = sum.w > 65535u ? ushort4(65535) : ushort4(sum & uint4(65535));'''


def adapt(name, data):
    if hashlib.sha256(data).hexdigest() != INPUTS[name]:
        raise ValueError(f'Counted visibility input changed: {name}')
    text = data.decode()
    if name == 'ContextMtl.mm':
        needle = 'setVisibilityResultMode(MTLVisibilityResultModeBoolean, resultOffset)'
        if text.count(needle) != 2:
            raise ValueError('Expected begin and continue query mode sites')
        text = text.replace(needle, needle.replace('ModeBoolean', 'ModeCounting'))
    elif name == 'DisplayMtl.mm':
        needle = '"libANGLE/renderer/metal/shaders/mtl_internal_shaders_src_autogen.h"'
        if text.count(needle) != 1:
            raise ValueError('Expected one runtime shader include')
        text = text.replace(needle, '"counted_shaders.h"')
    else:
        if text.count(OLD_SUM) != 1:
            raise ValueError('Expected one visibility reduction')
        text = text.replace(OLD_SUM, NEW_SUM)
    return text


def generate(source, output):
    # Validate all inputs before writing any outputs.
    generated = {name: adapt(name, (source / BASE / name).read_bytes()) for name in INPUTS}
    output.mkdir(parents=True, exist_ok=True)
    for name, text in generated.items():
        target = 'counted_shaders.h' if name.endswith('.h') else name
        (output / target).write_text(text)
    (output / 'identity.json').write_text(json.dumps({
        'name': 'counted-visibility-v1', 'inputs': INPUTS,
        'bridge_sha256': hashlib.sha256(pathlib.Path(__file__).with_suffix('.mm').read_bytes()).hexdigest(),
        'recipe_sha256': hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
        'outputs': {name: hashlib.sha256(text.encode()).hexdigest() for name, text in generated.items()},
    }, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    generate(args.source, args.output)
