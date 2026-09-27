/* HaloPad kernel32 string helpers (G3): the lstr* family used by Keystone.dll, ksimeui.dll
 * and Controls.dll. As on Windows XP: a NULL string has length 0; lstrcpy/lstrcat return the
 * destination; lstrcmpiA is CompareStringA(user default locale, NORM_IGNORECASE) - 2, so it
 * sorts by the locale's word order rather than by byte value. */
#include "halopad_win32.h"

uint32_t CompareStringA_c(uint32_t lcid, uint32_t flags, uint32_t s1, uint32_t n1, uint32_t s2, uint32_t n2);

uint32_t lstrlenA_c(uint32_t s) { return s ? (uint32_t)strlen((const char *)G(s)) : 0; }
uint32_t lstrlenW_c(uint32_t s)
{
    if (!s) return 0;
    uint32_t n = 0;
    for (uint16_t c; memcpy(&c, G(s + 2 * n), 2), c; ) n++;
    return n;
}
uint32_t lstrcpyA_c(uint32_t d, uint32_t s) { memmove(G(d), G(s), lstrlenA_c(s) + 1); return d; }
uint32_t lstrcpyW_c(uint32_t d, uint32_t s) { memmove(G(d), G(s), 2 * (lstrlenW_c(s) + 1)); return d; }
uint32_t lstrcatA_c(uint32_t d, uint32_t s) { lstrcpyA_c(d + lstrlenA_c(d), s); return d; }
uint32_t lstrcatW_c(uint32_t d, uint32_t s) { lstrcpyW_c(d + 2 * lstrlenW_c(d), s); return d; }
uint32_t lstrcmpiA_c(uint32_t a, uint32_t b)
{
    return CompareStringA_c(0x400 /* LOCALE_USER_DEFAULT */, 1 /* NORM_IGNORECASE */, a, 0xFFFFFFFFu, b, 0xFFFFFFFFu) - 2;
}
