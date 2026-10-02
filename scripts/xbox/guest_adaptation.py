"""Build a private guest with a small, separately identified HaloPad adaptation.

No upstream sources are stored here. A normal build is unmodified. The optional
experiment checks the complete input file, restores it after Ninja (even on a
failed build), and refuses to overwrite concurrent edits. An uncatchable kill
may leave the checkout dirty; prepare.sh deliberately refuses that state.
"""
import argparse
from contextlib import contextmanager
import fcntl
import hashlib
import json
import os
import pathlib
import shutil
import signal
import subprocess

RENDERER = pathlib.Path('port/linux/src/d3d8_gl.c')
SOURCE_SHA256 = '5c8c132048b1efaa57d322b9c8a0ef65df07c1755df653c0f1a178ce96831cc6'
ANCHOR = b'\tscale[0] = scale[1] = 1.0f;\n#else\n'
INSERT = b'''\t/* HaloPad private experiment: retain logical layout, scale only targets. */
\t{
\t\tconst char *requested = getenv("HALO_TEST_RENDER_SCALE");
\t\tif (requested && !strcmp(requested, "2"))
\t\t\tscale[0] = scale[1] = 2.0f;
\t}
'''
# World-filtering policy follows the reviewed idea in Tyberious's upstream
# PR 35, not its unmerged config/source patch. Keep HUD/point/non-mip paths intact.
FILTER_ANCHOR = b'\tif (xgpu_capabilities.border_clamp)\n'
FILTER_INSERT = b'''\t/* HaloPad opt-in world filtering, independent of target resolution. */
\tif (xgpu_capabilities.anisotropy && !hires && mipmapped &&
\t\tmin_filter != D3DTEXF_POINT && mip_filter != D3DTEXF_NONE)
\t{
\t\tstatic float requested;
\t\tif (!requested)
\t\t{
\t\t\tconst char *value = getenv("HALO_TEST_ANISOTROPY");
\t\t\tGLint maximum = 1;
\t\t\trequested = value && !strcmp(value, "4") ? 4.0f :
\t\t\t\tvalue && !strcmp(value, "16") ? 16.0f : 1.0f;
\t\t\tglGetIntegerv(0x84ff /* GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT */, &maximum);
\t\t\tif (maximum < 1) maximum = 1;
\t\t\tif (requested > maximum) requested = (float)maximum;
\t\t\tplatform_log("HaloPad world filtering: request %s, effective %.0fx (GPU %.0fx)",
\t\t\t\tvalue ? value : "unset", requested, (float)maximum);
\t\t}
\t\tif (requested > 1.0f && (min_filter != D3DTEXF_ANISOTROPIC ||
\t\t\tstate[D3DTSS_MAXANISOTROPY] < requested))
\t\t\tglSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, requested);
\t}
'''


def identity(name=None):
    name = os.environ.get('HALOPAD_XBOX_GUEST_ADAPTATION', 'none') if name is None else name
    if name == 'none':
        return {'name': 'none'}
    if name not in ('render-scale-v1', 'render-quality-v1'):
        raise ValueError('Unknown HALOPAD_XBOX_GUEST_ADAPTATION')
    recipe = ANCHOR + INSERT
    if name == 'render-quality-v1':
        recipe += FILTER_ANCHOR + FILTER_INSERT
    return {'name': name, 'upstream_renderer_sha256': SOURCE_SHA256,
            'recipe_sha256': hashlib.sha256(recipe).hexdigest()}


def adapted_source(original, name='render-scale-v1'):
    identity(name)
    if hashlib.sha256(original).hexdigest() != SOURCE_SHA256 or original.count(ANCHOR) != 1:
        raise ValueError('Renderer adaptation input changed; review the new upstream source first')
    if name == 'render-quality-v1' and original.count(FILTER_ANCHOR) != 1:
        raise ValueError('Renderer filtering input changed; review the new upstream source first')
    modified = original.replace(ANCHOR, ANCHOR[:-len(b'#else\n')] + INSERT + b'#else\n')
    if name == 'render-quality-v1':
        modified = modified.replace(FILTER_ANCHOR, FILTER_INSERT + FILTER_ANCHOR)
    return modified


@contextmanager
def renderer_adaptation(engine, adaptation):
    if adaptation['name'] == 'none':
        yield
        return
    path = engine / RENDERER
    original = path.read_bytes()
    modified = adapted_source(original, adaptation['name'])
    try:
        path.write_bytes(modified)
        yield
    finally:
        if path.read_bytes() != modified:
            raise RuntimeError('Renderer changed during build; preserving it for manual review')
        # Fresh mtime also makes the next unadapted Ninja build recompile it.
        path.write_bytes(original)


def build(engine, ndk, compiler, out):
    adaptation = identity()  # Reject unknown options before any mutation.
    (engine / 'build').mkdir(exist_ok=True)
    with (engine / 'build/halopad-guest.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        dirty = subprocess.check_output(
            ['git', 'status', '--porcelain', '--untracked-files=no'], cwd=engine, text=True)
        if dirty:
            raise ValueError('Xbox upstream checkout has local edits; preserve them before rebuilding')
        subprocess.run(['python3', 'configure.py', '--release', '--android-ndk', str(ndk),
                        '--android-guest-cc', str(compiler)], cwd=engine, check=True,
                       stdout=subprocess.DEVNULL)
        with renderer_adaptation(engine, adaptation):
            subprocess.run(['ninja', 'build/android/halo_guest.elf'], cwd=engine, check=True)
        out.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(engine / 'build/android/halo_guest.elf', out / 'halo_guest.elf')
        (out / 'guest-adaptation.json').write_text(json.dumps(adaptation, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('engine', 'ndk', 'compiler', 'out'):
        parser.add_argument('--' + name, type=pathlib.Path, required=True)
    args = parser.parse_args()

    def interrupted(signum, frame):
        raise KeyboardInterrupt(f'guest build interrupted by signal {signum}')

    for sig in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    build(args.engine, args.ndk, args.compiler, args.out)


if __name__ == '__main__':
    main()
