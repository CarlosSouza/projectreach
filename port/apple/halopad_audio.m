/* HaloPad Apple host, audio (G4/G9): the default output unit (macOS) or Remote I/O (iOS),
 * pulling 44,100 Hz stereo float from the DirectSound mixer; Core Audio converts to the
 * device's rate. */
#include <AudioToolbox/AudioToolbox.h>
#include <TargetConditionals.h>
#include <stdio.h>
#include <stdint.h>

static AudioComponentInstance unit;
static void (*mix)(float *out, uint32_t frames);

static OSStatus render(void *ref, AudioUnitRenderActionFlags *flags, const AudioTimeStamp *ts, UInt32 bus, UInt32 frames,
                       AudioBufferList *io)
{
    (void)ref; (void)flags; (void)ts; (void)bus;
    mix((float *)io->mBuffers[0].mData, frames);
    return noErr;
}

/* 1 if the output started */
int halopad_audio_start(void (*render_fn)(float *out, uint32_t frames), uint32_t rate)
{
    if (unit) return 1;
    mix = render_fn;
    AudioComponentDescription d = {kAudioUnitType_Output,
#if TARGET_OS_OSX
                                   kAudioUnitSubType_DefaultOutput,
#else
                                   kAudioUnitSubType_RemoteIO,
#endif
                                   kAudioUnitManufacturer_Apple, 0, 0};
    AudioComponent c = AudioComponentFindNext(NULL, &d);
    if (!c || AudioComponentInstanceNew(c, &unit)) { fprintf(stderr, "HALOPAD: no audio output unit\n"); unit = NULL; return 0; }
    AudioStreamBasicDescription f = {(Float64)rate, kAudioFormatLinearPCM, kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked,
                                     8, 1, 8, 2, 32, 0};
    AURenderCallbackStruct cb = {render, NULL};
    OSStatus e = 0;
    const char *step = "stream format";
    if (!(e = AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &f, sizeof f))
        && (step = "render callback", !(e = AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb, sizeof cb)))
        && (step = "initialize", !(e = AudioUnitInitialize(unit))))
        step = "start", e = AudioOutputUnitStart(unit);
    if (e) {
        fprintf(stderr, "HALOPAD: audio output unit would not start (%s: OSStatus %d)\n", step, (int)e);
        AudioComponentInstanceDispose(unit);
        unit = NULL;
        return 0;
    }
    return 1;
}

void halopad_audio_stop(void)
{
    if (!unit) return;
    AudioOutputUnitStop(unit);
    AudioUnitUninitialize(unit);
    AudioComponentInstanceDispose(unit);
    unit = NULL;
}
