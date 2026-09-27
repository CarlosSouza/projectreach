/* HaloPad code page and character services for code page 1252 (G3): MultiByteToWideChar,
 * WideCharToMultiByte, GetStringTypeA/W (CT_CTYPE1), LCMapStringA/W (case mapping).
 * Tables are generated from Microsoft's bestfit1252.txt by scripts/gen-nls-tables.py.
 * Other code pages, mapping flags or characters outside code page 1252's range stop. */
#include "halopad_win32.h"
#include "halopad_nls1252.h"

static int ansi_page(uint32_t cp) { return cp == 0 || cp == 1252 || cp == 3 /* CP_THREAD_ACP */; }

static int wc_to_mb(uint16_t w, uint8_t *b)
{
    uint32_t lo = 0, hi = NLS1252_WC_COUNT;
    while (lo < hi) {
        uint32_t m = (lo + hi) / 2;
        if (nls1252_wc2mb[m][0] == w) { *b = (uint8_t)nls1252_wc2mb[m][1]; return 1; }
        if (nls1252_wc2mb[m][0] < w) lo = m + 1; else hi = m;
    }
    return 0;
}

static const uint16_t *char_info(const char *service, uint16_t w)
{
    uint32_t lo = 0, hi = NLS1252_CHAR_COUNT;
    while (lo < hi) {
        uint32_t m = (lo + hi) / 2;
        if (nls1252_chars[m][0] == w) return nls1252_chars[m];
        if (nls1252_chars[m][0] < w) lo = m + 1; else hi = m;
    }
    hp_unsupported(service, "character U+%04X outside code page 1252's character table", w);
}

uint32_t MultiByteToWideChar_c(uint32_t cp, uint32_t flags, uint32_t src, uint32_t srclen, uint32_t dst, uint32_t dstlen)
{
    if (!ansi_page(cp)) hp_unsupported("MultiByteToWideChar", "code page %u", cp);
    if (flags & ~(1u | 8u) /* MB_PRECOMPOSED, MB_ERR_INVALID_CHARS */) hp_unsupported("MultiByteToWideChar", "flags 0x%x", flags);
    if (!srclen) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    const uint8_t *s = G(src);
    uint32_t n = srclen == 0xFFFFFFFFu ? (uint32_t)strlen((const char *)s) + 1 : srclen;
    if (!dstlen) return n;
    if (dstlen < n) { halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER; return 0; }
    for (uint32_t i = 0; i < n; i++) wr16(dst + 2 * i, nls1252_mb2wc[s[i]]);
    return n;
}

uint32_t WideCharToMultiByte_c(uint32_t cp, uint32_t flags, uint32_t src, uint32_t srclen, uint32_t dst, uint32_t dstlen,
                               uint32_t defchar, uint32_t useddef)
{
    if (!ansi_page(cp)) hp_unsupported("WideCharToMultiByte", "code page %u", cp);
    if (flags) hp_unsupported("WideCharToMultiByte", "flags 0x%x", flags);
    if (!srclen) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    uint8_t def = defchar ? *(uint8_t *)G(defchar) : NLS1252_DEFAULT;
    const uint16_t *s = G(src);
    uint32_t n = srclen;
    if (n == 0xFFFFFFFFu) { n = 0; while (s[n]) n++; n++; }
    if (dstlen && dstlen < n) { halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER; return 0; }
    uint32_t used = 0;
    for (uint32_t i = 0; i < n; i++) {
        uint8_t b;
        if (!wc_to_mb(s[i], &b)) { b = def; used = 1; }
        if (dstlen) ((uint8_t *)G(dst))[i] = b;
    }
    if (useddef) wr32(useddef, used);
    return n;
}

#define CT_CTYPE1 1u

uint32_t GetStringTypeW_c(uint32_t type, uint32_t src, uint32_t count, uint32_t out)
{
    if (type != CT_CTYPE1) hp_unsupported("GetStringTypeW", "info type %u", type);
    const uint16_t *s = G(src);
    if (count == 0xFFFFFFFFu) { count = 0; while (s[count]) count++; count++; }
    for (uint32_t i = 0; i < count; i++) wr16(out + 2 * i, char_info("GetStringTypeW", s[i])[1]);
    return 1;
}

uint32_t GetStringTypeA_c(uint32_t lcid, uint32_t type, uint32_t src, uint32_t count, uint32_t out)
{
    (void)lcid;                          /* every locale here uses code page 1252 */
    if (type != CT_CTYPE1) hp_unsupported("GetStringTypeA", "info type %u", type);
    const uint8_t *s = G(src);
    if (count == 0xFFFFFFFFu) count = (uint32_t)strlen((const char *)s) + 1;
    for (uint32_t i = 0; i < count; i++) wr16(out + 2 * i, char_info("GetStringTypeA", nls1252_mb2wc[s[i]])[1]);
    return 1;
}

#define LCMAP_LOWERCASE 0x100u
#define LCMAP_UPPERCASE 0x200u

static int case_column(const char *service, uint32_t flags)
{
    if (flags == LCMAP_UPPERCASE) return 2;
    if (flags == LCMAP_LOWERCASE) return 3;
    hp_unsupported(service, "map flags 0x%x", flags);
}

uint32_t LCMapStringW_c(uint32_t lcid, uint32_t flags, uint32_t src, uint32_t count, uint32_t dst, uint32_t dstlen)
{
    (void)lcid;
    int col = case_column("LCMapStringW", flags);
    const uint16_t *s = G(src);
    if (count == 0xFFFFFFFFu) { count = 0; while (s[count]) count++; count++; }
    if (!dstlen) return count;
    if (dstlen < count) { halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER; return 0; }
    for (uint32_t i = 0; i < count; i++) wr16(dst + 2 * i, char_info("LCMapStringW", s[i])[col]);
    return count;
}

uint32_t LCMapStringA_c(uint32_t lcid, uint32_t flags, uint32_t src, uint32_t count, uint32_t dst, uint32_t dstlen)
{
    (void)lcid;
    int col = case_column("LCMapStringA", flags);
    const uint8_t *s = G(src);
    if (count == 0xFFFFFFFFu) count = (uint32_t)strlen((const char *)s) + 1;
    if (!dstlen) return count;
    if (dstlen < count) { halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER; return 0; }
    for (uint32_t i = 0; i < count; i++) {
        uint8_t b;
        if (!wc_to_mb(char_info("LCMapStringA", nls1252_mb2wc[s[i]])[col], &b)) b = NLS1252_DEFAULT;
        ((uint8_t *)G(dst))[i] = b;
    }
    return count;
}
