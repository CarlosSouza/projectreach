"""Bounded native callback capture and fail-closed signal inspection."""
import importlib.util
import json
import pathlib
import struct
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audio_capture', ROOT / 'scripts/xbox/audio_capture.py')
audio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audio)


class XboxAudioCaptureTests(unittest.TestCase):
    def test_actual_native_skip_bound_and_handoff(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'audio-test'
            subprocess.run(['clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                            '-fsanitize=address,undefined', '-I', str(ROOT / 'port/xbox'),
                            '-o', str(binary)], input=r'''
#include <assert.h>
#include "xg_audio_capture.h"
int main(void) {
    struct xg_audio_capture capture = {0}, disabled = {0};
    float samples[64];
    for (int i = 0; i < 64; ++i) samples[i] = i / 64.f;
    xg_audio_capture_append(&disabled, samples, 16, 16);
    assert(!xg_audio_capture_init(&capture, 0, 2));
    assert(!xg_audio_capture_init(&capture, 192001, 2));
    assert(!xg_audio_capture_init(&capture, 48000, 3));
    assert(xg_audio_capture_init(&capture, 2, 2)); /* skip 20, retain 8 frames */
    xg_audio_capture_append(&capture, samples, 16, 16);
    assert(capture.skip == 4 && capture.frames == 0);
    xg_audio_capture_append(&capture, samples, 6, 5);
    assert(capture.frames == 2 && capture.underrun_frames == 1);
    assert(capture.samples[0] == samples[8]);
    assert(!atomic_load_explicit(&capture.ready, memory_order_acquire));
    xg_audio_capture_append(&capture, samples, 16, 4);
    assert(atomic_load_explicit(&capture.ready, memory_order_acquire));
    assert(capture.frames == 8 && capture.underrun_frames == 3);
    assert(capture.samples[4] == samples[0] && capture.samples[15] == samples[11]);
    xg_audio_capture_append(&capture, samples, 16, 0);
    assert(capture.frames == 8 && capture.underrun_frames == 3);
    free(capture.samples);
    return 0;
}
''', text=True, check=True, capture_output=True)
            subprocess.run([str(binary)], check=True, capture_output=True)

    def make_capture(self, path, samples, **changes):
        metadata = dict(rate=2, channels=2, frames=8, underrun_frames=0)
        metadata.update(changes)
        (path / 'audio-output.json').write_text(json.dumps(metadata))
        (path / 'audio-output.f32le').write_bytes(struct.pack('<' + 'f' * len(samples), *samples))

    def test_valid_signal_reports_underruns_and_range_without_quality_claim(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp)
            self.make_capture(path, [.5, -.5] * 8, underrun_frames=2)
            result = audio.inspect_audio(path)
            self.assertTrue(result['signal_present'])
            self.assertEqual(result['rms'], .5)
            self.assertEqual(result['peak'], .5)
            self.assertEqual(result['seconds'], 4)
            self.assertEqual(result['underrun_frames'], 2)
            self.assertIn('not speaker', result['scope'])

    def test_silence_and_nonfinite_do_not_pass_signal_gate(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp)
            for samples in ([0.] * 16, [float('nan')] + [.5] * 15, [float('inf')] * 16):
                self.make_capture(path, samples)
                self.assertFalse(audio.inspect_audio(path)['signal_present'])

    def test_outside_range_is_reported_not_hidden(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp)
            self.make_capture(path, [1.5, -2.] + [0.] * 14)
            self.assertEqual(audio.inspect_audio(path)['outside_unit_range'], 2)

    def test_invalid_metadata_or_truncated_samples_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp)
            for changes in ({'rate': 0}, {'channels': 3}, {'frames': 7},
                            {'underrun_frames': 9}, {'rate': True}):
                self.make_capture(path, [.5] * 16, **changes)
                with self.assertRaises(ValueError):
                    audio.inspect_audio(path)
            for count in (15, 17):
                self.make_capture(path, [.5] * count)
                with self.assertRaises(ValueError):
                    audio.inspect_audio(path)

    def test_missing_capture_is_not_a_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(OSError):
                audio.inspect_audio(tmp)


if __name__ == '__main__':
    unittest.main()
