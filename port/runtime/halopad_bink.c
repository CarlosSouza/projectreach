/* HaloPad Bink (G4/G7): binkw32.dll as Halo Custom Edition meets it.
 *
 * How Halo uses it (haloce.exe, 0x43ed20, the movie player): BinkSetSoundSystem(
 * BinkOpenDirectSound, 0) (it passes the function, it does not call it), BinkOpen(path, 0)
 * for bungie.bik, gearbox.bik, mgs.bik and ending.bik; when that returns NULL the movie is
 * skipped; otherwise each frame is BinkWait/BinkDoFrame/BinkCopyToBuffer into a 640x480
 * offscreen surface (StretchRect to the target)/BinkNextFrame, then BinkClose.
 *
 * Custom Edition ships none of those movies, so BinkOpen finds no file and returns NULL,
 * exactly as RAD's library does. A movie that does exist (the retail game, G7) stops the
 * program: decoding Bink video is not done yet. The frame calls can only receive a movie
 * handle, which BinkOpen never gives out, so they stop with the handle. */
#include "halopad_win32.h"

uint32_t GetFileAttributesA_c(uint32_t name);

static uint32_t sound_open, sound_param;

uint32_t BinkOpenDirectSound_c(uint32_t param)
{
    (void)param;
    hp_unsupported("BinkOpenDirectSound", "being called (Halo only hands it to BinkSetSoundSystem)");
}

uint32_t BinkSetSoundSystem_c(uint32_t open, uint32_t param)
{
    sound_open = open; sound_param = param;                         /* used when a movie opens */
    return 1;
}

uint32_t BinkOpen_c(uint32_t name, uint32_t flags)
{
    if (!name) return 0;
    uint32_t attrs = GetFileAttributesA_c(name);
    if (attrs == 0xFFFFFFFFu || (attrs & 0x10u)) return 0;         /* no such movie: Bink returns NULL */
    hp_unsupported("BinkOpen", "playing Bink movie \"%s\" (flags 0x%x): Bink video decoding is not done yet", (const char *)G(name), flags);
}

static uint32_t no_movie(const char *fn, uint32_t h) { hp_unsupported(fn, "movie handle 0x%x (BinkOpen never opened one)", h); }
uint32_t BinkClose_c(uint32_t h) { return no_movie("BinkClose", h); }
uint32_t BinkDoFrame_c(uint32_t h) { return no_movie("BinkDoFrame", h); }
uint32_t BinkNextFrame_c(uint32_t h) { return no_movie("BinkNextFrame", h); }
uint32_t BinkWait_c(uint32_t h) { return no_movie("BinkWait", h); }
uint32_t BinkPause_c(uint32_t h, uint32_t pause) { (void)pause; return no_movie("BinkPause", h); }
uint32_t BinkCopyToBuffer_c(uint32_t h, uint32_t dst, uint32_t pitch, uint32_t height, uint32_t x, uint32_t y, uint32_t flags)
{
    (void)dst; (void)pitch; (void)height; (void)x; (void)y; (void)flags;
    return no_movie("BinkCopyToBuffer", h);
}
