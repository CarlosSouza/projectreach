"""Inspect only the bounded Simulator output-callback capture, not speaker audio."""
import array
import json
import math
import pathlib
import sys


def inspect_audio(folder):
    folder = pathlib.Path(folder)
    metadata = json.loads((folder / 'audio-output.json').read_text())
    rate, channels, frames = (metadata[key] for key in ('rate', 'channels', 'frames'))
    underruns = metadata['underrun_frames']
    if (any(type(value) is not int for value in (rate, channels, frames, underruns)) or
            not 0 < rate <= 192000 or channels not in (1, 2) or frames != rate * 4 or
            not 0 <= underruns <= frames):
        raise ValueError('invalid bounded audio capture metadata')
    expected = frames * channels * 4
    with (folder / 'audio-output.f32le').open('rb') as file:
        data = file.read(expected + 1)
    if len(data) != expected:
        raise ValueError('audio sample length does not match metadata')
    samples = array.array('f', data)
    if sys.byteorder != 'little':
        samples.byteswap()
    finite = all(math.isfinite(value) for value in samples)
    rms = math.sqrt(sum(value * value for value in samples) / len(samples)) if finite else None
    peak = max(abs(value) for value in samples) if finite else None
    return {**metadata, 'seconds': frames / rate, 'finite': finite,
            'rms': rms, 'peak': peak,
            'outside_unit_range': sum(abs(value) > 1 for value in samples),
            'signal_present': finite and rms > 1e-6,
            'scope': 'output-callback samples; not speaker, sync or quality acceptance'}
