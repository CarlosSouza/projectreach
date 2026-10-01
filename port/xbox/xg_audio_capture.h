/* Opt-in diagnostic: one audio-thread producer; game thread reads only after
 * ready's release/acquire handoff. No allocation or file I/O in the callback. */
#ifndef XG_AUDIO_CAPTURE_H
#define XG_AUDIO_CAPTURE_H
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct xg_audio_capture {
    float *samples;
    uint32_t skip, frames, capacity, channels, underrun_frames;
    atomic_bool ready;
};

static inline int xg_audio_capture_init(struct xg_audio_capture *capture,
                                       uint32_t rate, uint32_t channels)
{
    if (!rate || rate > 192000 || !channels || channels > 2) return 0;
    capture->skip = rate * 10; /* skip startup; capture the next four seconds */
    capture->capacity = rate * 4;
    capture->channels = channels;
    capture->frames = capture->underrun_frames = 0;
    atomic_init(&capture->ready, 0);
    capture->samples = malloc((size_t)capture->capacity * channels * sizeof(float));
    return capture->samples != NULL;
}

static inline void xg_audio_capture_append(struct xg_audio_capture *capture,
                                          const float *samples, uint32_t frames,
                                          uint32_t supplied_frames)
{
    if (!capture->samples || atomic_load_explicit(&capture->ready, memory_order_acquire)) return;
    uint32_t skip = frames < capture->skip ? frames : capture->skip;
    capture->skip -= skip;
    samples += (size_t)skip * capture->channels;
    frames -= skip;
    supplied_frames = supplied_frames > skip ? supplied_frames - skip : 0;
    uint32_t remaining = capture->capacity - capture->frames;
    uint32_t count = frames < remaining ? frames : remaining;
    memcpy(capture->samples + (size_t)capture->frames * capture->channels,
           samples, (size_t)count * capture->channels * sizeof(float));
    capture->underrun_frames += count > supplied_frames ? count - supplied_frames : 0;
    capture->frames += count;
    if (capture->frames == capture->capacity)
        atomic_store_explicit(&capture->ready, 1, memory_order_release);
}
#endif
