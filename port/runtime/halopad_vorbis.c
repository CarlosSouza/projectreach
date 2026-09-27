/* HaloPad vorbisfile.dll (G4): Ogg Vorbis decoding for Halo's sounds, with Xiph's libvorbis
 * (pinned in dependencies.lock.json, built by scripts/xiph.py). The game ships libVorbis I
 * 20020717 (1.0); the bitstream is the same, so current libvorbis decodes the same streams
 * (float rounding may differ in the last bit of a sample).
 *
 * How Halo uses it (haloce.exe, 0x548300/0x548620): two OggVorbis_File structures inside its
 * sound object (+0x8 and +0x2d8, 720 bytes each), opened with ov_open_callbacks over its own
 * memory reader (read 0x5481a0, seek 0x5481f0, close 0x548210, tell 0x548230, passed by
 * value); ov_read into 16-bit signed little-endian PCM; ov_crosslap between the two for
 * seamless transitions; ov_clear. All are cdecl. Halo never looks inside the structures, so
 * the decoder state lives on the host, keyed by the structure's guest address, and the
 * callbacks are Halo's own code, called through the guest. */
#include "halopad_win32.h"
#include <pthread.h>
#include <vorbis/vorbisfile.h>

uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);

typedef struct { int used; uint32_t vf, datasource, read, seek, close, tell; OggVorbis_File file; } stream;
#define MAXS 64
static stream streams[MAXS];
static pthread_mutex_t slock = PTHREAD_MUTEX_INITIALIZER;

static stream *find(uint32_t vf, int create)
{
    pthread_mutex_lock(&slock);
    stream *free_slot = NULL;
    for (int i = 0; i < MAXS; i++) {
        if (streams[i].used && streams[i].vf == vf) { pthread_mutex_unlock(&slock); return &streams[i]; }
        if (!streams[i].used && !free_slot) free_slot = &streams[i];
    }
    if (create) {
        if (!free_slot) { pthread_mutex_unlock(&slock); hp_unsupported("vorbisfile", "more than %d open streams", MAXS); }
        memset(free_slot, 0, sizeof *free_slot);
        free_slot->used = 1;
        free_slot->vf = vf;
    }
    pthread_mutex_unlock(&slock);
    return create ? free_slot : NULL;
}

/* Halo's callbacks, called as cdecl guest functions */
static size_t cb_read(void *ptr, size_t size, size_t nmemb, void *ds)
{
    stream *s = ds;
    uint32_t n = (uint32_t)(size * nmemb);
    uint32_t buf = halopad_heap_alloc(n ? n : 1, 0);
    uint32_t args[4] = {buf, (uint32_t)size, (uint32_t)nmemb, s->datasource};
    uint32_t got = halopad_call_guest_ex(s->read, 4, args, 0, 0);
    if (got > nmemb) got = (uint32_t)nmemb;
    memcpy(ptr, G(buf), got * size);
    halopad_heap_free(buf);
    return got;
}
static int cb_seek(void *ds, ogg_int64_t off, int whence)
{
    stream *s = ds;
    uint32_t args[4] = {s->datasource, (uint32_t)off, (uint32_t)((uint64_t)off >> 32), (uint32_t)whence};
    return (int)halopad_call_guest_ex(s->seek, 4, args, 0, 0);
}
static int cb_close(void *ds)
{
    stream *s = ds;
    uint32_t args[1] = {s->datasource};
    return (int)halopad_call_guest_ex(s->close, 1, args, 0, 0);
}
static long cb_tell(void *ds)
{
    stream *s = ds;
    uint32_t args[1] = {s->datasource};
    return (long)(int32_t)halopad_call_guest_ex(s->tell, 1, args, 0, 0);
}

uint32_t ov_open_callbacks_c(uint32_t datasource, uint32_t vf, uint32_t initial, uint32_t ibytes, uint32_t read, uint32_t seek,
                             uint32_t close, uint32_t tell)
{
    stream *old = find(vf, 0);
    if (old) { ov_clear(&old->file); old->used = 0; }              /* a structure reused without ov_clear */
    stream *s = find(vf, 1);
    s->datasource = datasource; s->read = read; s->seek = seek; s->close = close; s->tell = tell;
    ov_callbacks cb = {read ? cb_read : NULL, seek ? cb_seek : NULL, close ? cb_close : NULL, tell ? cb_tell : NULL};
    int r = ov_open_callbacks(s, &s->file, initial ? G(initial) : NULL, (long)(int32_t)ibytes, cb);
    if (r < 0) s->used = 0;                                         /* on failure the caller must not ov_clear */
    return (uint32_t)r;
}

uint32_t ov_read_c(uint32_t vf, uint32_t buffer, uint32_t length, uint32_t bigendian, uint32_t word, uint32_t sgned, uint32_t bitstream)
{
    stream *s = find(vf, 0);
    if (!s) return (uint32_t)OV_EINVAL;
    int bs = 0;
    long r = ov_read(&s->file, length ? G(buffer) : NULL, (int)length, (int)bigendian, (int)word, (int)sgned, &bs);
    if (bitstream) wr32(bitstream, (uint32_t)bs);
    return (uint32_t)r;
}

uint32_t ov_crosslap_c(uint32_t vf1, uint32_t vf2)
{
    stream *a = find(vf1, 0), *b = find(vf2, 0);
    if (!a || !b) return (uint32_t)OV_EINVAL;
    return (uint32_t)ov_crosslap(&a->file, &b->file);
}

uint32_t ov_clear_c(uint32_t vf)
{
    stream *s = find(vf, 0);
    if (!s) return 0;                                               /* as ov_clear on a cleared structure */
    ov_clear(&s->file);
    s->used = 0;
    return 0;
}
