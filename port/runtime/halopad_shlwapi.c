/* HaloPad Windows runtime: SHLWAPI string, path and URL services and the USER32/KERNEL32
 * wide services behind SHLWAPI's ordinal "wrap" exports, for msxml4.dll (G3). Behavior follows
 * Windows XP's shlwapi.dll as Wine's dlls/shlwapi and dlls/kernelbase record it. Characters go
 * through the code page 1252 tables (halopad_nls.c); URL forms not handled here stop. */
#include "halopad_win32.h"

uint16_t halopad_ctype1(const char *service, uint16_t w);
uint16_t halopad_upper(const char *service, uint16_t w);
uint16_t halopad_lower(const char *service, uint16_t w);
uint32_t halopad_guest_ansi(const char *service, uint32_t src);
uint16_t halopad_wide_of_ansi(uint8_t b);
void halopad_heap_free(uint32_t a);
uint32_t CompareStringW_c(uint32_t lcid, uint32_t flags, uint32_t s1, uint32_t n1, uint32_t s2, uint32_t n2);
uint32_t GetThreadLocale_c(void);
uint32_t CreateFileA_c(uint32_t name, uint32_t access, uint32_t share, uint32_t security, uint32_t disp, uint32_t flags, uint32_t tmpl);

#define C1_DIGIT 0x4u
#define C1_SPACE 0x8u
#define C1_ALPHA 0x100u

static uint16_t wat(uint32_t s, uint32_t i) { uint16_t c; memcpy(&c, G(s + 2 * i), 2); return c; }
static uint32_t wlen(uint32_t s) { uint32_t n = 0; while (wat(s, n)) n++; return n; }

/* ParseURLW's syntax check: a scheme of at least two letters, digits, '+', '-' or '.', then ':'. */
static int has_scheme(uint32_t s, uint32_t *scheme_len)
{
    uint32_t i = 0;
    for (uint16_t c; (c = wat(s, i)) != 0; i++) {
        if (c == '+' || c == '-' || c == '.') continue;
        if (c > 0xFF || !(halopad_ctype1("ParseURLW", c) & (C1_ALPHA | C1_DIGIT))) break;
    }
    if (wat(s, i) != ':' || i < 2) return 0;
    if (scheme_len) *scheme_len = i;
    return 1;
}

static void trace_w(const char *service, uint32_t s);
uint32_t PathIsURLW_c(uint32_t path) { trace_w("PathIsURLW", path); return path && wat(path, 0) && has_scheme(path, NULL); }

uint32_t PathIsRelativeW_c(uint32_t path)
{
    if (!path || !wat(path, 0)) return 1;
    return !(wat(path, 0) == '\\' || wat(path, 1) == ':');
}

static int prefix_ci(uint32_t s, const char *p)
{
    for (uint32_t i = 0; p[i]; i++) {
        uint16_t c = wat(s, i);
        if (c >= 'A' && c <= 'Z') c += 32;
        if (c != (uint8_t)p[i]) return 0;
    }
    return 1;
}

uint32_t UrlIsW_c(uint32_t url, uint32_t type)
{
    if (!url) return 0;
    switch (type) {
    case 0: return PathIsURLW_c(url);                       /* URLIS_URL */
    case 3: return (uint32_t)prefix_ci(url, "file:");        /* URLIS_FILEURL */
    default: hp_unsupported("UrlIsW", "URLIS type %u", type);
    }
}

uint32_t StrCpyW_c(uint32_t d, uint32_t s)
{
    if (d && s) memmove(G(d), G(s), 2 * (wlen(s) + 1));
    return d;
}

uint32_t StrCatW_c(uint32_t d, uint32_t s)
{
    if (d && s) StrCpyW_c(d + 2 * wlen(d), s);
    return d;
}

/* StrToIntW converts only when the string starts with '-' or a digit, as XP's does */
uint32_t StrToIntW_c(uint32_t s)
{
    if (!s) return 0;
    uint32_t i = 0;
    int neg = 0;
    if (wat(s, 0) == '-') { neg = 1; i = 1; }
    else if (!(wat(s, 0) >= '0' && wat(s, 0) <= '9')) return 0;
    uint32_t v = 0;
    for (uint16_t c; (c = wat(s, i)) >= '0' && c <= '9'; i++) v = v * 10 + (c - '0');
    return neg ? (uint32_t)-(int32_t)v : v;
}

/* locale comparisons: CompareStringW(thread locale) - CSTR_EQUAL; StrCmpN* stop at NUL or n */
static uint32_t cmp(uint32_t flags, uint32_t a, uint32_t b, int32_t n)
{
    uint32_t na = wlen(a), nb = wlen(b);
    if (n >= 0) { if (na > (uint32_t)n) na = (uint32_t)n; if (nb > (uint32_t)n) nb = (uint32_t)n; }
    return CompareStringW_c(GetThreadLocale_c(), flags, a, na, b, nb) - 2;
}
uint32_t StrCmpW_c(uint32_t a, uint32_t b) { return cmp(0, a, b, -1); }
uint32_t StrCmpNW_c(uint32_t a, uint32_t b, uint32_t n) { return cmp(0, a, b, (int32_t)n); }
uint32_t StrCmpNIW_c(uint32_t a, uint32_t b, uint32_t n) { return cmp(1 /* NORM_IGNORECASE */, a, b, (int32_t)n); }

/* ordinal comparisons (ordinals 156 StrCmpCW and 158 StrCmpICW): -1, 0 or 1 */
static uint32_t cmp_ordinal(uint32_t a, uint32_t b, int ic)
{
    for (uint32_t i = 0;; i++) {
        uint16_t x = wat(a, i), y = wat(b, i);
        if (ic) { x = halopad_upper("StrCmpICW", x); y = halopad_upper("StrCmpICW", y); }
        if (x != y) return x < y ? 0xFFFFFFFFu : 1u;
        if (!x) return 0;
    }
}
uint32_t StrCmpCW_c(uint32_t a, uint32_t b) { return cmp_ordinal(a, b, 0); }
uint32_t StrCmpICW_c(uint32_t a, uint32_t b) { return cmp_ordinal(a, b, 1); }

/* USER32 wide character services (SHLWAPI ordinals 28, 29, 38, 44) */
uint32_t IsCharAlphaNumericW_c(uint32_t c) { return (halopad_ctype1("IsCharAlphaNumericW", (uint16_t)c) & (C1_ALPHA | C1_DIGIT)) != 0; }
uint32_t IsCharSpaceW_c(uint32_t c) { return (halopad_ctype1("IsCharSpaceW", (uint16_t)c) & C1_SPACE) != 0; }

uint32_t CharUpperBuffW_c(uint32_t s, uint32_t n)
{
    if (!s) return 0;
    for (uint32_t i = 0; i < n; i++) wr16(s + 2 * i, halopad_upper("CharUpperBuffW", wat(s, i)));
    return n;
}

uint32_t CharLowerW_c(uint32_t s)
{
    if (!(s >> 16)) return halopad_lower("CharLowerW", (uint16_t)s);   /* a single character */
    for (uint32_t i = 0; wat(s, i); i++) wr16(s + 2 * i, halopad_lower("CharLowerW", wat(s, i)));
    return s;
}

/* OutputDebugString: without a debugger attached it has no visible effect. HALOPAD_DEBUG_OUTPUT=1
   copies the text to stderr for diagnostics. */
static int debug_output(void) { static int v = -1; if (v < 0) v = getenv("HALOPAD_DEBUG_OUTPUT") != NULL; return v; }
uint32_t OutputDebugStringA_c(uint32_t s)
{
    if (s && debug_output()) fprintf(stderr, "HALOPAD DEBUG OUTPUT: %s\n", (const char *)G(s));
    return 0;
}
uint32_t OutputDebugStringW_c(uint32_t s)
{
    if (!s || !debug_output()) return 0;
    fprintf(stderr, "HALOPAD DEBUG OUTPUT: ");
    for (uint32_t i = 0; wat(s, i); i++) { uint16_t c = wat(s, i); fputc(c < 0x80 ? (int)c : '?', stderr); }
    fputc('\n', stderr);
    return 0;
}

/* GetAcceptLanguagesW (ordinal 15): no Internet Explorer AcceptLanguage value is set on the
   reference machine, so XP answers from the user locale, English (United States). */
uint32_t GetAcceptLanguagesW_c(uint32_t buf, uint32_t pcch)
{
    static const char lang[] = "en-us";
    if (!buf || !pcch) return 0x80070057u;                  /* E_INVALIDARG */
    uint32_t n = (uint32_t)sizeof lang - 1;
    if (rd32(pcch) <= n) hp_unsupported("GetAcceptLanguagesW", "a %u-character buffer", rd32(pcch));
    for (uint32_t i = 0; i <= n; i++) wr16(buf + 2 * i, (uint8_t)lang[i]);
    wr32(pcch, n);
    return 0;
}

/* KERNEL32 CreateFileW (SHLWAPI ordinal 52): the ANSI service on the name in code page 1252 */
uint32_t CreateFileW_c(uint32_t name, uint32_t access, uint32_t share, uint32_t security, uint32_t disp, uint32_t flags, uint32_t tmpl)
{
    uint32_t a = halopad_guest_ansi("CreateFileW", name);
    uint32_t h = CreateFileA_c(a, access, share, security, disp, flags, tmpl);
    uint32_t e = halopad_last_error;
    if (a) halopad_heap_free(a);
    halopad_last_error = e;
    return h;
}

/* HALOPAD_TRACE_SHLWAPI=1 prints the URLs and paths msxml4 passes (diagnostics only) */
static int trace_urls(void) { static int v = -1; if (v < 0) v = getenv("HALOPAD_TRACE_SHLWAPI") != NULL; return v; }
static void trace_w(const char *service, uint32_t s)
{
    if (!trace_urls()) return;
    fprintf(stderr, "HALOPAD SHLWAPI: %s(\"", service);
    for (uint32_t i = 0; s && wat(s, i); i++) { uint16_t c = wat(s, i); fputc(c < 0x80 ? (int)c : '?', stderr); }
    fprintf(stderr, "\")\n");
}

static int hexval(uint16_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

#define URL_UNESCAPE_INPLACE 0x00100000u
#define URL_DONT_UNESCAPE_EXTRA_INFO 0x02000000u
#define E_POINTER 0x80004003u
#define E_INVALIDARG 0x80070057u

/* UrlUnescapeW: %XX becomes the character XX; with URL_DONT_UNESCAPE_EXTRA_INFO it stops at the
   first '#' or '?'. A short buffer gets what fits and E_POINTER with the size needed, as XP does. */
uint32_t UrlUnescapeW_c(uint32_t url, uint32_t out, uint32_t pcch, uint32_t flags)
{
    trace_w("UrlUnescapeW", url);
    if (flags & ~(URL_UNESCAPE_INPLACE | URL_DONT_UNESCAPE_EXTRA_INFO)) hp_unsupported("UrlUnescapeW", "flags 0x%x", flags);
    if (!url) return E_INVALIDARG;
    int inplace = (flags & URL_UNESCAPE_INPLACE) != 0;
    if (!inplace && (!out || !pcch)) return E_INVALIDARG;
    uint32_t dst = inplace ? url : out, cap = inplace ? 0 : rd32(pcch), needed = 0;
    int stop = 0;
    for (uint32_t i = 0; wat(url, i); i++, needed++) {
        uint16_t c = wat(url, i), next;
        if ((flags & URL_DONT_UNESCAPE_EXTRA_INFO) && (c == '#' || c == '?')) { stop = 1; next = c; }
        else if (c == '%' && !stop && hexval(wat(url, i + 1)) >= 0 && hexval(wat(url, i + 2)) >= 0) {
            next = (uint16_t)(hexval(wat(url, i + 1)) * 16 + hexval(wat(url, i + 2)));
            i += 2;
        } else next = c;
        if (inplace || needed < cap) wr16(dst + 2 * needed, next);
    }
    uint32_t ret = 0;
    if (inplace || needed < cap) wr16(dst + 2 * needed, 0);
    else { needed++; ret = E_POINTER; }
    if (!inplace) wr32(pcch, needed);
    return ret;
}

/* PathSearchAndQualifyW: SearchPathW, then GetFullPathNameW. For a fully qualified path both give
   the normalized path ('/' to '\', no '.' or '..' components). Relative names, which SearchPath
   looks up in the program, current, system and Windows directories and PATH, stop for now. */
int halopad_full_path(const char *in, char *out, size_t size);
uint32_t PathSearchAndQualifyW_c(uint32_t path, uint32_t buf, uint32_t cch)
{
    trace_w("PathSearchAndQualifyW", path);
    if (!path || !buf) hp_unsupported("PathSearchAndQualifyW", "a NULL path or buffer");
    int drive = wat(path, 0) < 0x80 && ((wat(path, 0) | 0x20) >= 'a' && (wat(path, 0) | 0x20) <= 'z') && wat(path, 1) == ':'
                && (wat(path, 2) == '\\' || wat(path, 2) == '/');
    if (!drive) hp_unsupported("PathSearchAndQualifyW", "a path that is not fully qualified");
    uint32_t a = halopad_guest_ansi("PathSearchAndQualifyW", path);
    char full[1100];
    int ok = halopad_full_path((const char *)G(a), full, sizeof full);
    halopad_heap_free(a);
    if (!ok) hp_unsupported("PathSearchAndQualifyW", "a path GetFullPathName rejects");
    uint32_t n = (uint32_t)strlen(full);
    if (n + 1 > cch) hp_unsupported("PathSearchAndQualifyW", "a %u-character buffer for a %u-character path", cch, n);
    for (uint32_t i = 0; i <= n; i++) wr16(buf + 2 * i, halopad_wide_of_ansi((uint8_t)full[i]));
    return 1;
}

/* UrlCreateFromPathW: a DOS path becomes "file:" ("file:///" before a drive letter) plus the path
   with '\' as '/' and UrlEscapeW(URL_ESCAPE_PERCENT)'s file-scheme escapes: '%', controls, space,
   "<>\"{}|^[]`" and, in file URLs, '#' and '?', as %XX. A string that already has a URL scheme is
   copied and gives S_FALSE. Non-ASCII characters and UNC paths are not handled yet and stop. */
uint32_t UrlCreateFromPathW_c(uint32_t path, uint32_t url, uint32_t pcch, uint32_t reserved)
{
    trace_w("UrlCreateFromPathW", path);
    (void)reserved;
    if (!path || !url || !pcch) return E_INVALIDARG;
    uint32_t n = wlen(path), cap = rd32(pcch);
    if (has_scheme(path, NULL)) {
        if (n >= cap) { wr32(pcch, n + 1); return E_POINTER; }
        StrCpyW_c(url, path);
        wr32(pcch, n);
        return 1;                                            /* S_FALSE */
    }
    if ((wat(path, 0) == '\\' || wat(path, 0) == '/') && (wat(path, 1) == '\\' || wat(path, 1) == '/'))
        hp_unsupported("UrlCreateFromPathW", "a UNC path");
    char out[4 * 1100];
    size_t o = (size_t)snprintf(out, sizeof out, "file:");
    uint16_t c0 = wat(path, 0);
    if (c0 < 0x80 && ((c0 | 0x20) >= 'a' && (c0 | 0x20) <= 'z') && wat(path, 1) == ':') o += (size_t)snprintf(out + o, sizeof out - o, "///");
    for (uint32_t i = 0; i < n; i++) {
        uint16_t c = wat(path, i);
        if (c >= 0x7F) hp_unsupported("UrlCreateFromPathW", "character U+%04X in a path", c);
        if (c == '&') hp_unsupported("UrlCreateFromPathW", "'&' in a path (escaping not verified against XP)");
        if (c == '\\') c = '/';
        if (o + 4 >= sizeof out) hp_unsupported("UrlCreateFromPathW", "a path of %u characters", n);
        if (c <= 31 || c == '%' || strchr(" <>\"{}|^[]`#?", c)) o += (size_t)snprintf(out + o, sizeof out - o, "%%%02X", c);
        else out[o++] = (char)c;
    }
    out[o] = 0;
    if (o >= cap) { wr32(pcch, (uint32_t)o + 1); return E_POINTER; }
    for (size_t i = 0; i <= o; i++) wr16(url + 2 * (uint32_t)i, (uint8_t)out[i]);
    wr32(pcch, (uint32_t)o);
    return 0;
}

/* PathCreateFromUrlW: a "file:" URL back to a DOS path, following XP's handling of the slash
   forms as Wine's tests record it: "file:///C:/x", "file:/C:/x" and "file://localhost/C:/x" are
   escaped paths; "file://C:/x" is unescaped. '/' becomes '\', "C|" becomes "C:", and %XX is
   decoded after the slash conversion. Host names and UNC forms stop. */
static int drive_at(const uint16_t *s) { return s[0] < 0x80 && ((s[0] | 0x20) >= 'a' && (s[0] | 0x20) <= 'z') && (s[1] == ':' || s[1] == '|'); }
uint32_t PathCreateFromUrlW_c(uint32_t url, uint32_t path, uint32_t pcch, uint32_t reserved)
{
    trace_w("PathCreateFromUrlW", url);
    (void)reserved;
    if (!url || !path || !pcch || !rd32(pcch)) return E_INVALIDARG;
    uint32_t n = wlen(url);
    if (n < 5 || !prefix_ci(url, "file:")) return E_INVALIDARG;
    uint16_t *u = calloc(n + 1, 2);
    for (uint32_t i = 0; i < n; i++) u[i] = wat(url, i);
    const uint16_t *src = u + 5;
    uint32_t slashes = 0;
    while (*src == '/' || *src == '\\') { slashes++; src++; }
    int unescape = 1;
    switch (slashes) {
    case 0: break;                                   /* file:C:/x */
    case 1: case 3:                                  /* file:/C:/x, file:///C:/x */
        if (!drive_at(src)) hp_unsupported("PathCreateFromUrlW", "a file URL without a drive letter");
        break;
    case 2: {
        static const char lh[] = "localhost";
        int is_lh = 1;
        for (int k = 0; k < 9; k++) { uint16_t c = src[k]; if (c >= 'A' && c <= 'Z') c += 32; if (c != (uint8_t)lh[k]) { is_lh = 0; break; } }
        if (is_lh && (src[9] == '/' || src[9] == '\\')) { src += 10; break; }   /* file://localhost/C:/x */
        if (drive_at(src)) { unescape = 0; break; }                             /* file://C:/x */
        hp_unsupported("PathCreateFromUrlW", "a file URL with a host name");
    }
    default: hp_unsupported("PathCreateFromUrlW", "a UNC file URL");
    }
    uint32_t len = 0;
    uint16_t *t = calloc(n + 3, 2);
    for (const uint16_t *p = src; *p; p++) t[len++] = *p == '/' ? '\\' : *p;
    if (drive_at(t) && t[1] == '|') t[1] = ':';
    if (unescape) {
        uint32_t o = 0;
        for (uint32_t i = 0; i < len; i++) {
            if (t[i] == '%' && hexval(t[i + 1]) >= 0 && hexval(t[i + 2]) >= 0) { t[o++] = (uint16_t)(hexval(t[i + 1]) * 16 + hexval(t[i + 2])); i += 2; }
            else t[o++] = t[i];
        }
        len = o;
    }
    uint32_t ret = 0;
    if (rd32(pcch) < len + 1) { ret = E_POINTER; wr32(pcch, len + 1); }
    else {
        for (uint32_t i = 0; i < len; i++) wr16(path + 2 * i, t[i]);
        wr16(path + 2 * len, 0);
        wr32(pcch, len);
    }
    free(t);
    free(u);
    return ret;
}

/* UrlCanonicalizeW, for strings without a URL scheme (msxml4 canonicalizes xs:anyURI values
   such as KSML's background="gallery\editbox480.png"). Follows Wine's rewrite_url for
   URL_SCHEME_INVALID, which matches Windows: only '/' separates segments, "./" is dropped, "../"
   rewinds one segment or stays when it would pass the start, a query and a hash are moved to the
   end (query first), then URL_UNESCAPE decodes %XX. Strings with a scheme, UNC or drive paths and
   the escaping flags are not handled yet and stop. */
#define URL_UNESCAPE 0x10000000u
#define URL_DONT_SIMPLIFY 0x08000000u
typedef struct { uint16_t *s; uint32_t len, cap; } wbuf;
static void wput(wbuf *b, const uint16_t *p, uint32_t n)
{
    if (b->len + n + 1 > b->cap) { b->cap = (b->len + n + 1) * 2; b->s = realloc(b->s, 2 * b->cap); }
    memcpy(b->s + b->len, p, 2 * n);
    b->len += n;
}
static void wputc(wbuf *b, uint16_t c) { wput(b, &c, 1); }
static int dot_sep(uint16_t c) { return c == 0 || c == '/' || c == '?' || c == '#'; }

uint32_t UrlCanonicalizeW_c(uint32_t url, uint32_t out, uint32_t pcch, uint32_t flags)
{
    trace_w("UrlCanonicalizeW", url);
    if (!url || !out || !pcch || !rd32(pcch)) return E_INVALIDARG;
    if (flags & ~(URL_UNESCAPE | URL_DONT_SIMPLIFY)) hp_unsupported("UrlCanonicalizeW", "flags 0x%x", flags);
    if (!wat(url, 0)) { wr16(out, 0); return 0; }
    /* strip leading and trailing C0 controls and spaces, drop tabs and line breaks */
    uint32_t n = wlen(url), b = 0;
    while (b < n && wat(url, b) <= 0x20) b++;
    while (n > b && wat(url, n - 1) <= 0x20) n--;
    uint16_t *u = calloc(n - b + 1, 2);
    uint32_t k = 0;
    for (uint32_t i = b; i < n; i++) { uint16_t c = wat(url, i); if (c != '\t' && c != '\n' && c != '\r') u[k++] = c; }
    u[k] = 0;
    char t[300];
    for (uint32_t i = 0; i < sizeof t - 1; i++) { t[i] = i < k ? (u[i] < 0x80 ? (char)u[i] : '?') : 0; if (i >= k) break; }
    t[sizeof t - 1] = 0;
    uint32_t i0 = 0;
    while (u[i0] == '+' || u[i0] == '-' || u[i0] == '.' || (u[i0] && u[i0] <= 0xFF && (halopad_ctype1("ParseURLW", u[i0]) & (C1_ALPHA | C1_DIGIT)))) i0++;
    if (u[i0] == ':' && i0 >= 2) hp_unsupported("UrlCanonicalizeW", "the URL \"%s\" (schemes are not handled yet)", t);
    if ((u[0] == '\\' && u[1] == '\\') || drive_at(u)) hp_unsupported("UrlCanonicalizeW", "the path \"%s\"", t);

    wbuf dst = {0};
    const uint16_t *src = u, *end = u + k;
    int initial_slash = 0;
    if (src[0] == '/') {                              /* the root is the single slash */
        initial_slash = 1;
        wputc(&dst, *src++);
        if (*src == '\\') src++;
    }
    uint32_t root = dst.len;
    const uint16_t *query = NULL, *hash = NULL;
    for (const uint16_t *p = src; p < end; p++) { if (!query && *p == '?') query = p; if (!hash && *p == '#') hash = p; }
    uint32_t qlen = query ? (uint32_t)(((hash && hash > query) ? hash : end) - query) : 0;
    uint32_t hlen = hash ? (uint32_t)(((query && query > hash) ? query : end) - hash) : 0;
    const uint16_t *src_end = end;
    if (query) src_end = query;
    if (hash && hash < src_end) src_end = hash;
    (void)initial_slash;
    while (src < src_end) {
        uint32_t len = 0;
        while (src + len < src_end && src[len] != '/') len++;
        int is_dots = 0;
        if (src[0] == '.' && dot_sep(src[1])) {
            if (flags & URL_DONT_SIMPLIFY) is_dots = 1;
            else { src++; if (*src == '/' || *src == '\\') src++; continue; }
        } else if (src[0] == '.' && src[1] == '.' && dot_sep(src[2])) {
            if (flags & URL_DONT_SIMPLIFY) is_dots = 1;
            else if (dst.len == root) {                /* always relative: keep ".." at the start */
                wputc(&dst, *src++); wputc(&dst, *src++);
                if (*src == '/' || *src == '\\') src++;
                wputc(&dst, '/');
                root = dst.len;
                continue;
            } else {
                if (dst.len > root) dst.len--;
                while (dst.len > root && dst.s[dst.len - 1] != '/') dst.len--;
                src += 2;
                if (*src == '/' || *src == '\\') src++;
                continue;
            }
        }
        if (len) { wput(&dst, src, len); src += len; }
        if (src >= src_end || *src == '?' || *src == '#' || !*src) {
            if (is_dots) wputc(&dst, '/');
        } else { wputc(&dst, '/'); src++; }
    }
    if (!dst.len && src_end != u) wputc(&dst, '/');
    if (query) wput(&dst, query, qlen);
    if (hash) wput(&dst, hash, hlen);
    wputc(&dst, 0);
    dst.len--;
    if (flags & URL_UNESCAPE) {                       /* in place, as UrlUnescapeW(URL_UNESCAPE_INPLACE) */
        uint32_t o = 0;
        for (uint32_t i = 0; i < dst.len; i++) {
            if (dst.s[i] == '%' && i + 2 < dst.len + 1 && hexval(dst.s[i + 1]) >= 0 && hexval(dst.s[i + 2]) >= 0) {
                dst.s[o++] = (uint16_t)(hexval(dst.s[i + 1]) * 16 + hexval(dst.s[i + 2]));
                i += 2;
            } else dst.s[o++] = dst.s[i];
        }
        dst.len = o;
    }
    uint32_t ret = 0;
    if (dst.len + 1 <= rd32(pcch)) {
        for (uint32_t i = 0; i < dst.len; i++) wr16(out + 2 * i, dst.s[i]);
        wr16(out + 2 * dst.len, 0);
        wr32(pcch, dst.len);
    } else { ret = E_POINTER; wr32(pcch, dst.len + 1); }
    free(dst.s);
    free(u);
    return ret;
}
