/* HaloPad DirectSound 8 (G4/G9): DirectSound's software mixer, played through Core Audio.
 *
 * How Halo uses it (haloce.exe, 0x549270): DirectSoundCreate8(NULL), SetCooperativeLevel
 * (PRIORITY), GetCaps, a primary buffer with CTRL3D whose format it sets to 16-bit stereo
 * PCM at 22,050 or 44,100 Hz (by the caps' maximum rate), the 3D listener from the primary
 * buffer (distance factor 3.048, its rolloff factor, Doppler off, all immediate), then it
 * probes for hardware 3D voices with static LOCHARDWARE buffers and falls back to software
 * buffers when fewer than 16 can be made. HaloPad reports what Windows has reported since
 * Vista: no hardware voices, so Halo takes its software path.
 *
 * Mixing is DirectSound's software emulation at 44,100 Hz stereo float: volume in
 * hundredths of a decibel, DirectSound's pan law, frequency by linear interpolation, and
 * for 3D buffers the listener-space position (normal and head-relative modes), min/max
 * distance with the rolloff factor, cones, and Doppler, with deferred settings applied at
 * CommitDeferredSettings. Buffers without GLOBALFOCUS are silent (but keep playing) while
 * another application has the foreground. Buffer memory is guest memory, so Lock hands
 * Halo pointers the mixer reads directly, as with a real software buffer. EAX
 * (IKsPropertySet) is answered E_NOINTERFACE, as on a system without it. */
#include "halopad_win32.h"
#include <math.h>
#include <pthread.h>

uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *));
uint32_t halopad_com_addref(uint32_t g);
uint32_t halopad_com_release(uint32_t g);
void *halopad_com_state(const char *iface, uint32_t g);
uint32_t GetForegroundWindow_c(void);
int halopad_audio_start(void (*render)(float *out, uint32_t frames), uint32_t rate);
void halopad_audio_stop(void);
int halopad_audio_manual;                  /* tests: no device; they call halopad_dsound_mix */

#define DS_OK 0u
#define DSERR_INVALIDPARAM 0x80070057u
#define DSERR_INVALIDCALL 0x88780032u
#define DSERR_PRIOLEVELNEEDED 0x88780046u
#define DSERR_BADFORMAT 0x88780064u
#define DSERR_NOAGGREGATION 0x80040110u
#define DSERR_CONTROLUNAVAIL 0x8878001Eu
#define DSERR_NODRIVER 0x88780078u
#define DSERR_ALREADYINITIALIZED 0x88780082u
#define E_NOINTERFACE 0x80004002u
#define RATE 44100u

enum { PRIMARY = 0x1, STATIC = 0x2, LOCHARDWARE = 0x4, LOCSOFTWARE = 0x8, CTRL3D = 0x10, CTRLFREQUENCY = 0x20, CTRLPAN = 0x40,
       CTRLVOLUME = 0x80, CTRLPOSITIONNOTIFY = 0x100, CTRLFX = 0x200, STICKYFOCUS = 0x4000, GLOBALFOCUS = 0x8000,
       GETCURRENTPOSITION2 = 0x10000, MUTE3DATMAXDISTANCE = 0x20000, LOCDEFER = 0x40000 };

typedef struct { float pos[3], vel[3]; uint32_t in_angle, out_angle; float cone[3]; int32_t cone_out; float min, max; uint32_t mode; } params3d;
typedef struct { float pos[3], vel[3], front[3], top[3], dist, rolloff, doppler; } listener;

typedef struct sbuf {
    uint32_t guest, guest3d, guest_listener, flags, bytes, mem, *mem_refs;
    uint16_t channels, bits, block;
    uint32_t rate, freq;                   /* freq: the buffer's rate now (DSBFREQUENCY_ORIGINAL 0 = rate) */
    uint8_t wfx[18];                       /* primary: the format Halo set */
    int playing, looping;
    double pos;                            /* frames */
    int32_t volume, pan;
    params3d p3, d3;                       /* applied and deferred */
    int d3_dirty;
    struct sbuf *next;
} sbuf;

typedef struct { uint32_t guest, coop, speaker; } dsound;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static sbuf *buffers;                      /* all live buffers (mixer list) */
static listener L = {{0, 0, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, 1, 1, 1}, Ld;
static int L_dirty;
static int32_t master_volume, master_pan;
static int devices;

static float f32(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
static uint32_t u32(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static void rdvec(uint32_t g, float v[3]) { v[0] = f32(rd32(g)); v[1] = f32(rd32(g + 4)); v[2] = f32(rd32(g + 8)); }
static void wrvec(uint32_t g, const float v[3]) { wr32(g, u32(v[0])); wr32(g + 4, u32(v[1])); wr32(g + 8, u32(v[2])); }
static double db(int32_t hundredths) { return hundredths <= -10000 ? 0.0 : pow(10.0, hundredths / 2000.0); }

static int guid_is(uint32_t g, uint32_t d1, uint16_t d2, uint16_t d3, const uint8_t d4[8])
{
    return g && rd32(g) == d1 && (rd32(g + 4) & 0xFFFF) == d2 && rd32(g + 4) >> 16 == d3 && !memcmp(G(g + 8), d4, 8);
}
static int guid_null(uint32_t g) { static const uint8_t z[16]; return !g || !memcmp(G(g), z, 16); }
static const uint8_t ds_tail[8] = {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60};

/* ---- the mixer ---- */

static void gains3d(const sbuf *b, double *gl, double *gr, double *pitch)
{
    const params3d *p = &b->p3;
    *pitch = 1.0;
    if (p->mode == 2) return;                                       /* DS3DMODE_DISABLE */
    double rel[3];
    for (int i = 0; i < 3; i++) rel[i] = p->mode == 1 ? p->pos[i] : p->pos[i] - L.pos[i];   /* HEADRELATIVE */
    double d = sqrt(rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2]);
    /* listener space: right = top x front (DirectSound is left-handed) */
    const float *f = L.front, *t = L.top;
    double right[3] = {t[1] * f[2] - t[2] * f[1], t[2] * f[0] - t[0] * f[2], t[0] * f[1] - t[1] * f[0]};
    double rn = sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
    double x = p->mode == 1 ? rel[0] : (rn > 0 ? (rel[0] * right[0] + rel[1] * right[1] + rel[2] * right[2]) / rn : 0);
    double att = 1.0;
    if (d > p->min && p->min > 0) {
        if (d > p->max && (b->flags & MUTE3DATMAXDISTANCE)) { *gl = *gr = 0; return; }
        double dc = d < p->max ? d : p->max;
        att = p->min / (p->min + L.rolloff * (dc - p->min));
    }
    /* cone: the angle between the cone orientation and the direction to the listener */
    if (p->in_angle < 360 && d > 0) {
        double c[3] = {p->cone[0], p->cone[1], p->cone[2]};
        double cn = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
        if (cn > 0) {
            double cosang = -(rel[0] * c[0] + rel[1] * c[1] + rel[2] * c[2]) / (cn * d);
            double ang = acos(cosang < -1 ? -1 : cosang > 1 ? 1 : cosang) * 360.0 / M_PI;   /* full angle */
            double outside = db(p->cone_out);
            if (ang > p->out_angle) att *= outside;
            else if (ang > p->in_angle) att *= 1.0 + (outside - 1.0) * (ang - p->in_angle) / (double)(p->out_angle - p->in_angle);
        }
    }
    double s = d > 0 ? x / d : 0;                                   /* -1 left .. 1 right */
    *gl = att * (s > 0 ? 1 - s : 1);
    *gr = att * (s < 0 ? 1 + s : 1);
    if (L.doppler > 0 && d > 0) {
        double c = 343.3 / (L.dist > 0 ? L.dist : 1), u[3] = {rel[0] / d, rel[1] / d, rel[2] / d};
        double vl = L.vel[0] * u[0] + L.vel[1] * u[1] + L.vel[2] * u[2];          /* toward the source */
        double vs = p->vel[0] * u[0] + p->vel[1] * u[1] + p->vel[2] * u[2];       /* away from the listener */
        double num = c + L.doppler * vl, den = c + L.doppler * vs;
        if (den < c * 0.1) den = c * 0.1;
        if (num < 0) num = 0;
        *pitch = num / den;
    }
}

static double sample(const sbuf *b, uint32_t frame, int ch)
{
    const uint8_t *m = G(b->mem);
    uint32_t o = frame * b->block + (b->channels == 2 ? (uint32_t)ch * (b->bits / 8u) : 0);
    if (b->bits == 8) return (m[o] - 128) / 128.0;
    int16_t v;
    memcpy(&v, m + o, 2);
    return v / 32768.0;
}

/* Mix 'frames' stereo float frames into out (44,100 Hz), advancing every playing buffer. */
void halopad_dsound_mix(float *out, uint32_t frames)
{
    memset(out, 0, sizeof(float) * 2 * frames);
    pthread_mutex_lock(&lock);
    int background = GetForegroundWindow_c() == 0;
    double mv = db(master_volume), ml = master_pan > 0 ? db(-master_pan) : 1, mr = master_pan < 0 ? db(master_pan) : 1;
    for (sbuf *b = buffers; b; b = b->next) {
        if (!b->playing || (b->flags & PRIMARY)) continue;
        uint32_t nframes = b->bytes / b->block;
        double gl = 1, gr = 1, pitch = 1;
        if (b->flags & CTRL3D) gains3d(b, &gl, &gr, &pitch);
        double v = db(b->volume) * mv;
        if (b->pan > 0) gl *= db(-b->pan); else if (b->pan < 0) gr *= db(b->pan);
        gl *= v * ml; gr *= v * mr;
        if (background && !(b->flags & GLOBALFOCUS)) gl = gr = 0;   /* muted, still playing */
        double step = (double)(b->freq ? b->freq : b->rate) * pitch / RATE;
        for (uint32_t i = 0; i < frames && b->playing; i++) {
            uint32_t f0 = (uint32_t)b->pos;
            double t = b->pos - f0;
            uint32_t f1 = f0 + 1 < nframes ? f0 + 1 : (b->looping ? 0 : f0);
            if (gl != 0 || gr != 0) {
                double l = sample(b, f0, 0) * (1 - t) + sample(b, f1, 0) * t;
                double r = b->channels == 2 ? sample(b, f0, 1) * (1 - t) + sample(b, f1, 1) * t : l;
                out[2 * i] += (float)(l * gl);
                out[2 * i + 1] += (float)(r * gr);
            }
            b->pos += step;
            if (b->pos >= nframes) {
                if (b->looping) b->pos = fmod(b->pos, nframes);
                else { b->playing = 0; b->pos = 0; }                /* a one-shot buffer stops and rewinds */
            }
        }
    }
    pthread_mutex_unlock(&lock);
    for (uint32_t i = 0; i < 2 * frames; i++) out[i] = out[i] > 1 ? 1 : out[i] < -1 ? -1 : out[i];
}

/* ---- DirectSoundCreate8 and IDirectSound8 ---- */

static void ds_destroy(void *p)
{
    free(p);
    if (--devices == 0 && !halopad_audio_manual) halopad_audio_stop();
}

uint32_t DirectSoundCreate8_c(uint32_t guid, uint32_t out, uint32_t outer)
{
    static const uint8_t def_tail[8] = {0xAA, 0xF1, 0x4D, 0xDA, 0x8F, 0x2B, 0x5C, 0x03};
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, 0);
    if (outer) return DSERR_NOAGGREGATION;
    extern const uint8_t halopad_audio_guid[16];
    if (!guid_null(guid) && !guid_is(guid, 0xDEF00000, 0x9C6D, 0x47ED, def_tail) && !guid_is(guid, 0xDEF00002, 0x9C6D, 0x47ED, def_tail)
        && memcmp(G(guid), halopad_audio_guid, 16))
        return DSERR_NODRIVER;                                      /* the one playback device, by any of its names */
    if (devices == 0 && !halopad_audio_manual && !halopad_audio_start(halopad_dsound_mix, RATE)) return DSERR_NODRIVER;
    devices++;
    dsound *d = calloc(1, sizeof *d);
    d->speaker = 4u | 20u << 16;                                    /* DSSPEAKER_STEREO, wide geometry */
    d->guest = halopad_com_new("IDirectSound8", 4, d, ds_destroy);
    wr32(out, d->guest);
    return DS_OK;
}

static dsound *DS(uint32_t g) { return halopad_com_state("IDirectSound8", g); }
uint32_t hpcom_IDirectSound8_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    (void)out; DS(g);
    hp_unsupported("IDirectSound8::QueryInterface", "interface %08x-...", rd32(iid));
}
uint32_t hpcom_IDirectSound8_AddRef_c(uint32_t g) { DS(g); return halopad_com_addref(g); }
uint32_t hpcom_IDirectSound8_Release_c(uint32_t g) { DS(g); return halopad_com_release(g); }

uint32_t hpcom_IDirectSound8_SetCooperativeLevel_c(uint32_t g, uint32_t hwnd, uint32_t level)
{
    dsound *d = DS(g);
    (void)hwnd;
    if (level < 1 || level > 4) return DSERR_INVALIDPARAM;
    if (level == 4) hp_unsupported("IDirectSound8::SetCooperativeLevel", "DSSCL_WRITEPRIMARY");
    d->coop = level;
    return DS_OK;
}

uint32_t hpcom_IDirectSound8_GetCaps_c(uint32_t g, uint32_t caps)
{
    DS(g);
    if (!caps || rd32(caps) != 96) return DSERR_INVALIDPARAM;
    memset((uint8_t *)G(caps) + 4, 0, 92);
    wr32(caps + 4, 0x1u | 0x2u | 0x4u | 0x8u | 0x10u | 0x100u | 0x200u | 0x400u | 0x800u);   /* primary and secondary formats, continuous rate */
    wr32(caps + 8, 100); wr32(caps + 12, 200000);                   /* secondary sample rates */
    wr32(caps + 16, 1);                                             /* dwPrimaryBuffers */
    /* no hardware mixing or 3D voices, no hardware memory, as on Windows since Vista */
    return DS_OK;
}

uint32_t hpcom_IDirectSound8_Compact_c(uint32_t g) { DS(g); return DS_OK; }
uint32_t hpcom_IDirectSound8_GetSpeakerConfig_c(uint32_t g, uint32_t out) { if (!out) return DSERR_INVALIDPARAM; wr32(out, DS(g)->speaker); return DS_OK; }
uint32_t hpcom_IDirectSound8_SetSpeakerConfig_c(uint32_t g, uint32_t cfg) { DS(g)->speaker = cfg; return DS_OK; }
uint32_t hpcom_IDirectSound8_Initialize_c(uint32_t g, uint32_t guid) { (void)guid; DS(g); return DSERR_ALREADYINITIALIZED; }

/* ---- buffers ---- */

static sbuf *B(uint32_t g) { return halopad_com_state("IDirectSoundBuffer8", g); }

static void buf_destroy(void *p)
{
    sbuf *b = p;
    pthread_mutex_lock(&lock);
    for (sbuf **q = &buffers; *q; q = &(*q)->next) if (*q == b) { *q = b->next; break; }
    pthread_mutex_unlock(&lock);
    if (b->mem_refs && --*b->mem_refs == 0) { halopad_heap_free(b->mem); free(b->mem_refs); }
    if (b->guest3d) halopad_com_release(b->guest3d);
    if (b->guest_listener) halopad_com_release(b->guest_listener);
    free(b);
}

static void default3d(params3d *p)
{
    memset(p, 0, sizeof *p);
    p->in_angle = p->out_angle = 360; p->cone[2] = 1; p->min = 1; p->max = 1e9f;
}

static sbuf *new_buffer(uint32_t flags)
{
    sbuf *b = calloc(1, sizeof *b);
    b->flags = flags;
    default3d(&b->p3);
    b->d3 = b->p3;
    b->guest = halopad_com_new("IDirectSoundBuffer8", 4, b, buf_destroy);
    pthread_mutex_lock(&lock);
    b->next = buffers;
    buffers = b;
    pthread_mutex_unlock(&lock);
    return b;
}

uint32_t hpcom_IDirectSound8_CreateSoundBuffer_c(uint32_t g, uint32_t desc, uint32_t out, uint32_t outer)
{
    dsound *d = DS(g);
    if (!desc || !out) return DSERR_INVALIDPARAM;
    wr32(out, 0);
    if (outer) return DSERR_NOAGGREGATION;
    uint32_t size = rd32(desc), flags = rd32(desc + 4), bytes = rd32(desc + 8), wfx = rd32(desc + 16);
    if (size != 20 && size != 36) return DSERR_INVALIDPARAM;
    if (!d->coop) return DSERR_PRIOLEVELNEEDED;
    if (flags & ~(0x7FFFFu)) return DSERR_INVALIDPARAM;
    if (flags & CTRLFX) hp_unsupported("IDirectSound8::CreateSoundBuffer", "effects (DSBCAPS_CTRLFX)");
    if ((flags & LOCHARDWARE) && (flags & LOCSOFTWARE)) return DSERR_INVALIDPARAM;
    if (flags & LOCHARDWARE) return DSERR_INVALIDCALL;              /* no hardware voices */
    if (size == 36 && !guid_null(desc + 20)) {
        static const uint8_t nov_tail[8] = {0x94, 0xF5, 0x00, 0xC0, 0x4F, 0xC2, 0x8A, 0xCA};
        if (!guid_is(desc + 20, 0xC241333F, 0x1C1B, 0x11D2, nov_tail))
            hp_unsupported("IDirectSound8::CreateSoundBuffer", "a 3D algorithm other than the default ({%08x-...})", rd32(desc + 20));
    }
    if (flags & PRIMARY) {
        if (bytes || wfx || (flags & (CTRLFREQUENCY | CTRLPOSITIONNOTIFY | STATIC))) return DSERR_INVALIDPARAM;
        for (sbuf *b = buffers; b; b = b->next)
            if (b->flags & PRIMARY) { halopad_com_addref(b->guest); wr32(out, b->guest); return DS_OK; }   /* one primary */
        sbuf *b = new_buffer(flags);
        static const uint8_t def_wfx[18] = {1, 0, 2, 0, 0x22, 0x56, 0, 0, 0x88, 0x58, 1, 0, 4, 0, 16, 0, 0, 0};   /* 22,050 Hz 16-bit stereo */
        memcpy(b->wfx, def_wfx, 18);
        b->bytes = 32768; b->block = 4; b->channels = 2; b->bits = 16; b->rate = 22050;
        wr32(out, b->guest);
        return DS_OK;
    }
    if (!wfx) return DSERR_INVALIDPARAM;
    uint16_t tag = (uint16_t)rd32(wfx), ch = (uint16_t)(rd32(wfx) >> 16), block = (uint16_t)rd32(wfx + 12), bits = (uint16_t)(rd32(wfx + 12) >> 16);
    uint32_t rate = rd32(wfx + 4);
    if (tag == 0xFFFE) hp_unsupported("IDirectSound8::CreateSoundBuffer", "WAVE_FORMAT_EXTENSIBLE");
    if (tag != 1 || (ch != 1 && ch != 2) || (bits != 8 && bits != 16) || block != ch * bits / 8 || rate < 100 || rate > 200000)
        return DSERR_BADFORMAT;
    if ((flags & CTRL3D) && (ch != 1 || (flags & CTRLPAN))) return DSERR_INVALIDPARAM;   /* 3D buffers are mono, without pan */
    if (bytes < 4 || bytes > 0x0FFFFFFF) return DSERR_INVALIDPARAM;
    sbuf *b = new_buffer(flags);
    b->channels = ch; b->bits = bits; b->block = block; b->rate = rate;
    b->bytes = bytes - bytes % block;
    b->mem = halopad_heap_alloc(b->bytes, 1);
    b->mem_refs = calloc(1, sizeof *b->mem_refs);
    *b->mem_refs = 1;
    wr32(out, b->guest);
    return DS_OK;
}

uint32_t hpcom_IDirectSound8_DuplicateSoundBuffer_c(uint32_t g, uint32_t orig, uint32_t out)
{
    DS(g);
    if (!orig || !out) return DSERR_INVALIDPARAM;
    sbuf *o = B(orig);
    if (o->flags & PRIMARY) return DSERR_INVALIDCALL;
    sbuf *b = new_buffer(o->flags);
    pthread_mutex_lock(&lock);
    b->channels = o->channels; b->bits = o->bits; b->block = o->block; b->rate = o->rate; b->freq = o->freq;
    b->bytes = o->bytes; b->mem = o->mem; b->mem_refs = o->mem_refs; ++*b->mem_refs;
    b->volume = o->volume; b->pan = o->pan; b->p3 = o->p3; b->d3 = o->d3;
    pthread_mutex_unlock(&lock);
    wr32(out, b->guest);
    return DS_OK;
}

/* 3D buffer and listener objects share their buffer's reference count, as COM
   interfaces of one object do. */
static uint32_t sub_object(sbuf *b, uint32_t *slot, const char *iface)
{
    if (!*slot) {
        uint32_t *state = calloc(1, sizeof *state);
        *state = b->guest;
        *slot = halopad_com_new(iface, 4, state, free);
    }
    halopad_com_addref(b->guest);
    return *slot;
}

uint32_t hpcom_IDirectSoundBuffer8_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    sbuf *b = B(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, 0);
    static const uint8_t b8_tail[8] = {0x92, 0x0F, 0x50, 0xE3, 0x6A, 0xB3, 0xAB, 0x1E};
    static const uint8_t ks_tail[8] = {0xA9, 0xAA, 0x00, 0xAA, 0x00, 0x61, 0xBE, 0x93};
    if (guid_is(iid, 0x279AFA85, 0x4981, 0x11CE, ds_tail) || guid_is(iid, 0x6825A449, 0x7524, 0x4D82, b8_tail)) {
        halopad_com_addref(g); wr32(out, g); return DS_OK;          /* IDirectSoundBuffer(8) */
    }
    if (guid_is(iid, 0x279AFA86, 0x4981, 0x11CE, ds_tail)) {        /* IDirectSound3DBuffer */
        if (!(b->flags & CTRL3D) || (b->flags & PRIMARY)) return E_NOINTERFACE;
        wr32(out, sub_object(b, &b->guest3d, "IDirectSound3DBuffer"));
        return DS_OK;
    }
    if (guid_is(iid, 0x279AFA84, 0x4981, 0x11CE, ds_tail)) {        /* IDirectSound3DListener */
        if (!(b->flags & CTRL3D) || !(b->flags & PRIMARY)) return E_NOINTERFACE;
        wr32(out, sub_object(b, &b->guest_listener, "IDirectSound3DListener"));
        return DS_OK;
    }
    if (guid_is(iid, 0x31EFAC30, 0x515C, 0x11D0, ks_tail)) return E_NOINTERFACE;   /* IKsPropertySet (EAX): none */
    hp_unsupported("IDirectSoundBuffer8::QueryInterface", "interface %08x-...", rd32(iid));
}
uint32_t hpcom_IDirectSoundBuffer8_AddRef_c(uint32_t g) { B(g); return halopad_com_addref(g); }
uint32_t hpcom_IDirectSoundBuffer8_Release_c(uint32_t g) { B(g); return halopad_com_release(g); }

uint32_t hpcom_IDirectSoundBuffer8_GetCaps_c(uint32_t g, uint32_t caps)
{
    sbuf *b = B(g);
    if (!caps || rd32(caps) != 20) return DSERR_INVALIDPARAM;
    wr32(caps + 4, (b->flags & ~(LOCHARDWARE | LOCDEFER)) | LOCSOFTWARE); wr32(caps + 8, b->bytes); wr32(caps + 12, 0); wr32(caps + 16, 0);
    return DS_OK;
}

/* play and write cursors in bytes; the write cursor is ~10 ms ahead while playing */
static void cursors(sbuf *b, uint32_t *p, uint32_t *w)
{
    pthread_mutex_lock(&lock);
    *p = (uint32_t)b->pos * b->block;
    *w = *p;
    if (b->playing) {
        *w = *p + ((b->freq ? b->freq : b->rate) / 100) * b->block;
        *w = b->looping ? *w % b->bytes : (*w > b->bytes ? b->bytes : *w);
    }
    pthread_mutex_unlock(&lock);
}

uint32_t hpcom_IDirectSoundBuffer8_GetCurrentPosition_c(uint32_t g, uint32_t play, uint32_t write)
{
    sbuf *b = B(g);
    if (b->flags & PRIMARY) hp_unsupported("IDirectSoundBuffer8::GetCurrentPosition", "on the primary buffer");
    uint32_t p, w;
    cursors(b, &p, &w);
    if (play) wr32(play, p);
    if (write) wr32(write, w);
    return DS_OK;
}

uint32_t hpcom_IDirectSoundBuffer8_GetFormat_c(uint32_t g, uint32_t wfx, uint32_t size, uint32_t written)
{
    sbuf *b = B(g);
    uint8_t f[18];
    if (b->flags & PRIMARY) memcpy(f, b->wfx, 18);
    else {
        uint16_t w[9] = {1, b->channels, (uint16_t)b->rate, (uint16_t)(b->rate >> 16), (uint16_t)(b->rate * b->block), (uint16_t)((b->rate * b->block) >> 16),
                         b->block, b->bits, 0};
        memcpy(f, w, 18);
    }
    if (!wfx) { if (!written) return DSERR_INVALIDPARAM; wr32(written, 18); return DS_OK; }
    uint32_t n = size < 18 ? size : 18;
    memcpy(G(wfx), f, n);
    if (written) wr32(written, n);
    return DS_OK;
}

uint32_t hpcom_IDirectSoundBuffer8_GetVolume_c(uint32_t g, uint32_t out)
{
    sbuf *b = B(g);
    if (!(b->flags & CTRLVOLUME)) return DSERR_CONTROLUNAVAIL;
    wr32(out, (uint32_t)((b->flags & PRIMARY) ? master_volume : b->volume));
    return DS_OK;
}
uint32_t hpcom_IDirectSoundBuffer8_GetPan_c(uint32_t g, uint32_t out)
{
    sbuf *b = B(g);
    if (!(b->flags & CTRLPAN)) return DSERR_CONTROLUNAVAIL;
    wr32(out, (uint32_t)((b->flags & PRIMARY) ? master_pan : b->pan));
    return DS_OK;
}
uint32_t hpcom_IDirectSoundBuffer8_GetFrequency_c(uint32_t g, uint32_t out)
{
    sbuf *b = B(g);
    if (!(b->flags & CTRLFREQUENCY)) return DSERR_CONTROLUNAVAIL;
    wr32(out, b->freq ? b->freq : b->rate);
    return DS_OK;
}
uint32_t hpcom_IDirectSoundBuffer8_GetStatus_c(uint32_t g, uint32_t out)
{
    sbuf *b = B(g);
    if (!out) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    uint32_t s = (b->playing ? 1u : 0) | (b->playing && b->looping ? 4u : 0) | 0x10u;   /* PLAYING, LOOPING, LOCSOFTWARE */
    pthread_mutex_unlock(&lock);
    wr32(out, s);
    return DS_OK;
}
uint32_t hpcom_IDirectSoundBuffer8_Initialize_c(uint32_t g, uint32_t ds, uint32_t desc) { (void)ds; (void)desc; B(g); return DSERR_ALREADYINITIALIZED; }

uint32_t hpcom_IDirectSoundBuffer8_Lock_c(uint32_t g, uint32_t offset, uint32_t bytes, uint32_t p1, uint32_t n1, uint32_t p2, uint32_t n2,
                                          uint32_t flags)
{
    sbuf *b = B(g);
    if (b->flags & PRIMARY) return DSERR_PRIOLEVELNEEDED;           /* only WRITEPRIMARY may lock the primary */
    if (!p1 || !n1 || (flags & ~3u)) return DSERR_INVALIDPARAM;
    if (flags & 1) { uint32_t p; cursors(b, &p, &offset); if (offset == b->bytes) offset = 0; }   /* FROMWRITECURSOR */
    if (flags & 2) bytes = b->bytes;                                /* ENTIREBUFFER */
    if (offset >= b->bytes || bytes == 0 || bytes > b->bytes) return DSERR_INVALIDPARAM;
    uint32_t first = b->bytes - offset < bytes ? b->bytes - offset : bytes;
    wr32(p1, b->mem + offset); wr32(n1, first);
    if (p2) { wr32(p2, bytes > first ? b->mem : 0); if (n2) wr32(n2, bytes - first); }
    else if (bytes > first) return DSERR_INVALIDPARAM;
    return DS_OK;
}

uint32_t hpcom_IDirectSoundBuffer8_Unlock_c(uint32_t g, uint32_t p1, uint32_t n1, uint32_t p2, uint32_t n2)
{
    (void)p1; (void)n1; (void)p2; (void)n2;                         /* the mixer reads the guest memory directly */
    B(g);
    return DS_OK;
}

uint32_t hpcom_IDirectSoundBuffer8_Play_c(uint32_t g, uint32_t reserved, uint32_t priority, uint32_t flags)
{
    sbuf *b = B(g);
    if (reserved || priority || (flags & ~1u)) {
        if (flags & ~0x3Fu) return DSERR_INVALIDPARAM;
        if (flags & ~1u) hp_unsupported("IDirectSoundBuffer8::Play", "voice management flags 0x%x", flags);
        if (priority) return DSERR_INVALIDPARAM;                   /* only LOCDEFER buffers take a priority */
    }
    if ((b->flags & PRIMARY) && !(flags & 1)) return DSERR_INVALIDPARAM;   /* the primary plays looping */
    pthread_mutex_lock(&lock);
    b->playing = 1; b->looping = flags & 1;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}

uint32_t hpcom_IDirectSoundBuffer8_Stop_c(uint32_t g)
{
    sbuf *b = B(g);
    pthread_mutex_lock(&lock);
    b->playing = 0;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}

uint32_t hpcom_IDirectSoundBuffer8_SetCurrentPosition_c(uint32_t g, uint32_t pos)
{
    sbuf *b = B(g);
    if (b->flags & PRIMARY) return DSERR_INVALIDCALL;
    if (pos >= b->bytes) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    b->pos = pos / b->block;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}

uint32_t hpcom_IDirectSoundBuffer8_SetFormat_c(uint32_t g, uint32_t wfx)
{
    sbuf *b = B(g);
    if (!(b->flags & PRIMARY)) return DSERR_INVALIDCALL;
    if (!wfx) return DSERR_INVALIDPARAM;
    uint16_t tag = (uint16_t)rd32(wfx), ch = (uint16_t)(rd32(wfx) >> 16), bits = (uint16_t)(rd32(wfx + 12) >> 16);
    if (tag != 1 || (ch != 1 && ch != 2) || (bits != 8 && bits != 16)) return DSERR_BADFORMAT;
    memcpy(b->wfx, G(wfx), 16);
    b->wfx[16] = b->wfx[17] = 0;
    return DS_OK;                                                   /* the mix stays 44,100 Hz float; Core Audio converts */
}

uint32_t hpcom_IDirectSoundBuffer8_SetVolume_c(uint32_t g, uint32_t v)
{
    sbuf *b = B(g);
    int32_t vol = (int32_t)v;
    if (!(b->flags & CTRLVOLUME)) return DSERR_CONTROLUNAVAIL;
    if (vol > 0 || vol < -10000) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    if (b->flags & PRIMARY) master_volume = vol; else b->volume = vol;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSoundBuffer8_SetPan_c(uint32_t g, uint32_t v)
{
    sbuf *b = B(g);
    int32_t pan = (int32_t)v;
    if (!(b->flags & CTRLPAN)) return DSERR_CONTROLUNAVAIL;
    if (pan > 10000 || pan < -10000) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    if (b->flags & PRIMARY) master_pan = pan; else b->pan = pan;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSoundBuffer8_SetFrequency_c(uint32_t g, uint32_t f)
{
    sbuf *b = B(g);
    if (!(b->flags & CTRLFREQUENCY) || (b->flags & PRIMARY)) return DSERR_CONTROLUNAVAIL;
    if (f && (f < 100 || f > 200000)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    b->freq = f;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSoundBuffer8_Restore_c(uint32_t g) { B(g); return DS_OK; }   /* software buffers are never lost */

/* ---- IDirectSound3DBuffer ---- */

static sbuf *B3(uint32_t g) { return B(*(uint32_t *)halopad_com_state("IDirectSound3DBuffer", g)); }
/* DS3D_DEFERRED (1) changes a pending copy that CommitDeferredSettings applies; an
   immediate change (0) applies now and to any pending copy. */
static params3d *target3d(sbuf *b, uint32_t apply)
{
    if (apply != 1) return &b->p3;
    if (!b->d3_dirty) { b->d3 = b->p3; b->d3_dirty = 1; }
    return &b->d3;
}

uint32_t hpcom_IDirectSound3DBuffer_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    return hpcom_IDirectSoundBuffer8_QueryInterface_c(B3(g)->guest, iid, out);
}
uint32_t hpcom_IDirectSound3DBuffer_AddRef_c(uint32_t g) { return halopad_com_addref(B3(g)->guest); }
uint32_t hpcom_IDirectSound3DBuffer_Release_c(uint32_t g) { return halopad_com_release(B3(g)->guest); }

uint32_t hpcom_IDirectSound3DBuffer_GetAllParameters_c(uint32_t g, uint32_t p)
{
    sbuf *b = B3(g);
    if (!p || rd32(p) != 64) return DSERR_INVALIDPARAM;
    const params3d *s = &b->p3;
    wrvec(p + 4, s->pos); wrvec(p + 16, s->vel); wr32(p + 28, s->in_angle); wr32(p + 32, s->out_angle);
    wrvec(p + 36, s->cone); wr32(p + 48, (uint32_t)s->cone_out); wr32(p + 52, u32(s->min)); wr32(p + 56, u32(s->max)); wr32(p + 60, s->mode);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetAllParameters_c(uint32_t g, uint32_t p, uint32_t apply)
{
    sbuf *b = B3(g);
    if (!p || rd32(p) != 64 || apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    rdvec(p + 4, s->pos); rdvec(p + 16, s->vel); s->in_angle = rd32(p + 28); s->out_angle = rd32(p + 32);
    rdvec(p + 36, s->cone); s->cone_out = (int32_t)rd32(p + 48); s->min = f32(rd32(p + 52)); s->max = f32(rd32(p + 56)); s->mode = rd32(p + 60);
    if (apply == 0 && b->d3_dirty) b->d3 = b->p3;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetConeOutsideVolume_c(uint32_t g, uint32_t out)
{
    sbuf *b = B3(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, (uint32_t)b->p3.cone_out);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetMaxDistance_c(uint32_t g, uint32_t out)
{
    sbuf *b = B3(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, u32(b->p3.max));
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetMinDistance_c(uint32_t g, uint32_t out)
{
    sbuf *b = B3(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, u32(b->p3.min));
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetMode_c(uint32_t g, uint32_t out)
{
    sbuf *b = B3(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, b->p3.mode);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetPosition_c(uint32_t g, uint32_t out)
{
    sbuf *b = B3(g);
    if (!out) return DSERR_INVALIDPARAM;
    wrvec(out, b->p3.pos);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetVelocity_c(uint32_t g, uint32_t out)
{
    sbuf *b = B3(g);
    if (!out) return DSERR_INVALIDPARAM;
    wrvec(out, b->p3.vel);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetConeOrientation_c(uint32_t g, uint32_t out)
{
    sbuf *b = B3(g);
    if (!out) return DSERR_INVALIDPARAM;
    wrvec(out, b->p3.cone);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_GetConeAngles_c(uint32_t g, uint32_t in, uint32_t out)
{
    sbuf *b = B3(g);
    if (in) wr32(in, b->p3.in_angle);
    if (out) wr32(out, b->p3.out_angle);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetConeAngles_c(uint32_t g, uint32_t in, uint32_t out, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1 || !(in <= out && out <= 360)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->in_angle = in; s->out_angle = out;
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->in_angle = in; s->out_angle = out; }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetConeOrientation_c(uint32_t g, uint32_t x, uint32_t y, uint32_t z, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->cone[0] = f32(x); s->cone[1] = f32(y); s->cone[2] = f32(z);
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->cone[0] = f32(x); s->cone[1] = f32(y); s->cone[2] = f32(z); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetConeOutsideVolume_c(uint32_t g, uint32_t v, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1 || !((int32_t)v <= 0 && (int32_t)v >= -10000)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->cone_out = (int32_t)v;
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->cone_out = (int32_t)v; }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetMaxDistance_c(uint32_t g, uint32_t v, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1 || !(f32(v) > 0)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->max = f32(v);
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->max = f32(v); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetMinDistance_c(uint32_t g, uint32_t v, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1 || !(f32(v) > 0)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->min = f32(v);
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->min = f32(v); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetMode_c(uint32_t g, uint32_t v, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1 || !(v <= 2)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->mode = v;
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->mode = v; }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetPosition_c(uint32_t g, uint32_t x, uint32_t y, uint32_t z, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->pos[0] = f32(x); s->pos[1] = f32(y); s->pos[2] = f32(z);
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->pos[0] = f32(x); s->pos[1] = f32(y); s->pos[2] = f32(z); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DBuffer_SetVelocity_c(uint32_t g, uint32_t x, uint32_t y, uint32_t z, uint32_t apply)
{
    sbuf *b = B3(g);
    if (apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    params3d *s = target3d(b, apply);
    s->vel[0] = f32(x); s->vel[1] = f32(y); s->vel[2] = f32(z);
    if (apply == 0 && b->d3_dirty) { s = &b->d3; s->vel[0] = f32(x); s->vel[1] = f32(y); s->vel[2] = f32(z); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}

/* ---- IDirectSound3DListener ---- */

static sbuf *BL(uint32_t g) { return B(*(uint32_t *)halopad_com_state("IDirectSound3DListener", g)); }
static listener *LT(uint32_t apply) { if (apply == 1) { if (!L_dirty) Ld = L; L_dirty = 1; return &Ld; } return &L; }

uint32_t hpcom_IDirectSound3DListener_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    return hpcom_IDirectSoundBuffer8_QueryInterface_c(BL(g)->guest, iid, out);
}
uint32_t hpcom_IDirectSound3DListener_AddRef_c(uint32_t g) { return halopad_com_addref(BL(g)->guest); }
uint32_t hpcom_IDirectSound3DListener_Release_c(uint32_t g) { return halopad_com_release(BL(g)->guest); }

uint32_t hpcom_IDirectSound3DListener_GetAllParameters_c(uint32_t g, uint32_t p)
{
    BL(g);
    if (!p || rd32(p) != 64) return DSERR_INVALIDPARAM;
    wrvec(p + 4, L.pos); wrvec(p + 16, L.vel); wrvec(p + 28, L.front); wrvec(p + 40, L.top);
    wr32(p + 52, u32(L.dist)); wr32(p + 56, u32(L.rolloff)); wr32(p + 60, u32(L.doppler));
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_SetAllParameters_c(uint32_t g, uint32_t p, uint32_t apply)
{
    BL(g);
    if (!p || rd32(p) != 64 || apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    listener *l = LT(apply);
    rdvec(p + 4, l->pos); rdvec(p + 16, l->vel); rdvec(p + 28, l->front); rdvec(p + 40, l->top);
    l->dist = f32(rd32(p + 52)); l->rolloff = f32(rd32(p + 56)); l->doppler = f32(rd32(p + 60));
    if (apply == 0 && L_dirty) Ld = L;
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_GetDistanceFactor_c(uint32_t g, uint32_t out)
{
    BL(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, u32(L.dist));
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_GetDopplerFactor_c(uint32_t g, uint32_t out)
{
    BL(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, u32(L.doppler));
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_GetRolloffFactor_c(uint32_t g, uint32_t out)
{
    BL(g);
    if (!out) return DSERR_INVALIDPARAM;
    wr32(out, u32(L.rolloff));
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_GetPosition_c(uint32_t g, uint32_t out)
{
    BL(g);
    if (!out) return DSERR_INVALIDPARAM;
    wrvec(out, L.pos);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_GetVelocity_c(uint32_t g, uint32_t out)
{
    BL(g);
    if (!out) return DSERR_INVALIDPARAM;
    wrvec(out, L.vel);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_GetOrientation_c(uint32_t g, uint32_t front, uint32_t top)
{
    BL(g);
    if (front) wrvec(front, L.front);
    if (top) wrvec(top, L.top);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_SetDistanceFactor_c(uint32_t g, uint32_t v, uint32_t apply)
{
    BL(g);
    if (apply > 1 || !(f32(v) > 0)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    listener *l = LT(apply);
    l->dist = f32(v);
    if (apply == 0 && L_dirty) { l = &Ld; l->dist = f32(v); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_SetDopplerFactor_c(uint32_t g, uint32_t v, uint32_t apply)
{
    BL(g);
    if (apply > 1 || !(f32(v) >= 0 && f32(v) <= 10)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    listener *l = LT(apply);
    l->doppler = f32(v);
    if (apply == 0 && L_dirty) { l = &Ld; l->doppler = f32(v); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_SetRolloffFactor_c(uint32_t g, uint32_t v, uint32_t apply)
{
    BL(g);
    if (apply > 1 || !(f32(v) >= 0 && f32(v) <= 10)) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    listener *l = LT(apply);
    l->rolloff = f32(v);
    if (apply == 0 && L_dirty) { l = &Ld; l->rolloff = f32(v); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_SetPosition_c(uint32_t g, uint32_t x, uint32_t y, uint32_t z, uint32_t apply)
{
    BL(g);
    if (apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    listener *l = LT(apply);
    l->pos[0] = f32(x); l->pos[1] = f32(y); l->pos[2] = f32(z);
    if (apply == 0 && L_dirty) { l = &Ld; l->pos[0] = f32(x); l->pos[1] = f32(y); l->pos[2] = f32(z); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_SetVelocity_c(uint32_t g, uint32_t x, uint32_t y, uint32_t z, uint32_t apply)
{
    BL(g);
    if (apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    listener *l = LT(apply);
    l->vel[0] = f32(x); l->vel[1] = f32(y); l->vel[2] = f32(z);
    if (apply == 0 && L_dirty) { l = &Ld; l->vel[0] = f32(x); l->vel[1] = f32(y); l->vel[2] = f32(z); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}
uint32_t hpcom_IDirectSound3DListener_SetOrientation_c(uint32_t g, uint32_t fx, uint32_t fy, uint32_t fz, uint32_t tx, uint32_t ty, uint32_t tz, uint32_t apply)
{
    BL(g);
    if (apply > 1) return DSERR_INVALIDPARAM;
    pthread_mutex_lock(&lock);
    listener *l = LT(apply);
    l->front[0] = f32(fx); l->front[1] = f32(fy); l->front[2] = f32(fz); l->top[0] = f32(tx); l->top[1] = f32(ty); l->top[2] = f32(tz);
    if (apply == 0 && L_dirty) { l = &Ld; l->front[0] = f32(fx); l->front[1] = f32(fy); l->front[2] = f32(fz); l->top[0] = f32(tx); l->top[1] = f32(ty); l->top[2] = f32(tz); }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}

uint32_t hpcom_IDirectSound3DListener_CommitDeferredSettings_c(uint32_t g)
{
    BL(g);
    pthread_mutex_lock(&lock);
    if (L_dirty) { L = Ld; L_dirty = 0; }
    for (sbuf *b = buffers; b; b = b->next) if (b->d3_dirty) { b->p3 = b->d3; b->d3_dirty = 0; }
    pthread_mutex_unlock(&lock);
    return DS_OK;
}

/* dsound ordinal 9, GetDeviceID: the default (voice) playback device is HaloPad's audio
   output; there is no capture device yet. */
uint32_t GetDeviceID_c(uint32_t src, uint32_t dst)
{
    extern const uint8_t halopad_audio_guid[16];
    static const uint8_t def_tail[8] = {0xAA, 0xF1, 0x4D, 0xDA, 0x8F, 0x2B, 0x5C, 0x03};
    if (!dst) return DSERR_INVALIDPARAM;
    if (guid_null(src) || guid_is(src, 0xDEF00000, 0x9C6D, 0x47ED, def_tail) || guid_is(src, 0xDEF00002, 0x9C6D, 0x47ED, def_tail)
        || !memcmp(G(src), halopad_audio_guid, 16)) {
        memcpy(G(dst), halopad_audio_guid, 16);
        return DS_OK;
    }
    if (guid_is(src, 0xDEF00001, 0x9C6D, 0x47ED, def_tail) || guid_is(src, 0xDEF00003, 0x9C6D, 0x47ED, def_tail))
        return DSERR_NODRIVER;                                      /* default (voice) capture: none */
    return DSERR_NODRIVER;
}
