/* HaloPad Windows runtime: console, version resources, security checks, clipboard (G3).
 *
 * How Halo uses them (haloce.exe):
 *  - console input (0x499be0) from the main loop: haloce.exe is a GUI program with no
 *    console, so every console call fails with ERROR_INVALID_HANDLE, as on Windows;
 *  - version resources (0x4ca2e0, a report header): GetFileVersionInfoSizeA/
 *    GetFileVersionInfoA/VerQueryValueA on its own module, read here from the PE file;
 *  - "is the player a local administrator" (0x545c50): tokens, the Administrators SID, a
 *    security descriptor with a DACL, AccessCheck. HaloPad's player account is a member of
 *    Administrators, Users and Everyone, as Windows XP accounts were by default;
 *  - chat paste (0x544ed0): CF_TEXT from the host pasteboard, in Windows-1252 with CRLF. */
#include "halopad_win32.h"
#include <ctype.h>

int halopad_host_path(const char *guest, char *out, size_t size);
uint32_t GlobalAlloc_c(uint32_t flags, uint32_t size);
uint32_t LocalAlloc_c(uint32_t flags, uint32_t size);
uint32_t LocalFree_c(uint32_t a);
uint32_t halopad_object_token_new(int impersonation);
int halopad_object_token_impersonation(uint32_t h);
int halopad_host_clipboard_text(char *out, size_t size);

static uint32_t fail(uint32_t e) { halopad_last_error = e; return 0; }

/* ---- the console: none ---- */

uint32_t GetNumberOfConsoleInputEvents_c(uint32_t h, uint32_t n) { (void)h; (void)n; return fail(6); }
uint32_t ReadConsoleInputA_c(uint32_t h, uint32_t buf, uint32_t len, uint32_t n) { (void)h; (void)buf; (void)len; (void)n; return fail(6); }
uint32_t WriteConsoleA_c(uint32_t h, uint32_t buf, uint32_t len, uint32_t n, uint32_t r) { (void)h; (void)buf; (void)len; (void)n; (void)r; return fail(6); }
uint32_t GetConsoleScreenBufferInfo_c(uint32_t h, uint32_t info) { (void)h; (void)info; return fail(6); }
uint32_t GetConsoleCursorInfo_c(uint32_t h, uint32_t info) { (void)h; (void)info; return fail(6); }
uint32_t SetConsoleCursorInfo_c(uint32_t h, uint32_t info) { (void)h; (void)info; return fail(6); }
uint32_t SetConsoleCursorPosition_c(uint32_t h, uint32_t pos) { (void)h; (void)pos; return fail(6); }
uint32_t FillConsoleOutputAttribute_c(uint32_t h, uint32_t a, uint32_t n, uint32_t at, uint32_t w) { (void)h; (void)a; (void)n; (void)at; (void)w; return fail(6); }
uint32_t FillConsoleOutputCharacterA_c(uint32_t h, uint32_t c, uint32_t n, uint32_t at, uint32_t w) { (void)h; (void)c; (void)n; (void)at; (void)w; return fail(6); }
uint32_t WriteConsoleOutputCharacterA_c(uint32_t h, uint32_t s, uint32_t n, uint32_t at, uint32_t w) { (void)h; (void)s; (void)n; (void)at; (void)w; return fail(6); }

/* ---- version resources ---- */

/* The RT_VERSION resource of a PE file on the host: malloc'd bytes, or NULL. */
static uint8_t *version_resource(uint32_t guest_path, uint32_t *size)
{
    char host[1024];
    if (!guest_path || !halopad_host_path(G(guest_path), host, sizeof host)) { halopad_last_error = 2; return NULL; }
    FILE *f = fopen(host, "rb");
    if (!f) { halopad_last_error = 2; return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *d = malloc((size_t)n);
    if (fread(d, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(d); return NULL; }
    fclose(f);
    uint8_t *res = NULL;
#define U32(o) ((o) + 4 <= (uint32_t)n ? (uint32_t)d[o] | (uint32_t)d[(o) + 1] << 8 | (uint32_t)d[(o) + 2] << 16 | (uint32_t)d[(o) + 3] << 24 : 0u)
#define U16(o) ((o) + 2 <= (uint32_t)n ? (uint32_t)(d[o] | d[(o) + 1] << 8) : 0u)
    uint32_t pe = U32(0x3C), nsec = U16(pe + 6), opt = pe + 24, optsz = U16(pe + 20);
    uint32_t rsrc_rva = U32(opt + 96 + 2 * 8), secs = opt + optsz;
    uint32_t base_off = 0, base_rva = 0, found = 0;
    for (uint32_t i = 0; i < nsec; i++) {
        uint32_t s = secs + 40 * i, va = U32(s + 12), vs = U32(s + 8), raw = U32(s + 20);
        if (rsrc_rva >= va && rsrc_rva < va + (vs ? vs : U32(s + 16))) { base_off = raw; base_rva = va; found = 1; }
    }
    if (!rsrc_rva || !found) { halopad_last_error = 1813; free(d); return NULL; }   /* ERROR_RESOURCE_TYPE_NOT_FOUND */
    uint32_t root = base_off + (rsrc_rva - base_rva), dir = root, entry = 0;
    uint32_t want[3] = {16, 0, 0};                                  /* RT_VERSION, then the first name and language */
    for (int level = 0; level < 3; level++) {
        uint32_t named = U16(dir + 12), ids = U16(dir + 14), next = 0;
        for (uint32_t k = 0; k < named + ids; k++) {
            uint32_t e = dir + 16 + 8 * k, id = U32(e), off = U32(e + 4);
            if (level == 0 && (id & 0x80000000u || id != want[0])) continue;
            next = off; break;
        }
        if (!next) { halopad_last_error = 1813; free(d); return NULL; }
        if (next & 0x80000000u) dir = root + (next & 0x7FFFFFFFu); else { entry = root + next; break; }
    }
    uint32_t drva = U32(entry), dsize = U32(entry + 4);
    uint32_t doff = base_off + (drva - base_rva);
    if (doff + dsize <= (uint32_t)n) { res = malloc(dsize); memcpy(res, d + doff, dsize); *size = dsize; }
#undef U32
#undef U16
    free(d);
    return res;
}

uint32_t GetFileVersionInfoSizeA_c(uint32_t path, uint32_t handle)
{
    if (handle) wr32(handle, 0);
    uint32_t n;
    uint8_t *r = version_resource(path, &n);
    if (!r) return 0;
    free(r);
    return n * 2 + 4;                                               /* room for the ANSI copies VerQueryValueA makes */
}

uint32_t GetFileVersionInfoA_c(uint32_t path, uint32_t handle, uint32_t len, uint32_t data)
{
    (void)handle;
    uint32_t n;
    uint8_t *r = version_resource(path, &n);
    if (!r) return 0;
    if (len < n + 4) { free(r); return fail(122); }
    memset(G(data), 0, len);
    memcpy(G(data), r, n);
    wr32(data + n, len);                                            /* where the ANSI area ends (ours) */
    free(r);
    return 1;
}

/* VS_VERSIONINFO nodes: wLength, wValueLength, wType, szKey (UTF-16), pad, value, pad, children */
static uint32_t node_key_end(const uint8_t *b, uint32_t node)
{
    uint32_t k = node + 6;
    while (b[k] || b[k + 1]) k += 2;
    return (k + 2 + 3) & ~3u;
}
static int key_is(const uint8_t *b, uint32_t node, const char *want)
{
    uint32_t k = node + 6;
    for (size_t i = 0;; i++, k += 2) {
        uint16_t c = (uint16_t)(b[k] | b[k + 1] << 8);
        char w = want[i];
        if (!c || !w) return !c && !w;
        if (tolower(c) != tolower((unsigned char)w)) return 0;
    }
}

uint32_t VerQueryValueA_c(uint32_t data, uint32_t sub, uint32_t outp, uint32_t lenp)
{
    const uint8_t *b = G(data);
    char path[256];
    snprintf(path, sizeof path, "%s", sub ? (const char *)G(sub) : "\\");
    uint32_t node = 0;
    char *save = NULL;
    for (char *part = strtok_r(path, "\\", &save); part; part = strtok_r(NULL, "\\", &save)) {
        uint32_t len = (uint32_t)(b[node] | b[node + 1] << 8), vlen = (uint32_t)(b[node + 2] | b[node + 3] << 8), type = (uint32_t)(b[node + 4] | b[node + 5] << 8);
        uint32_t child = (node_key_end(b, node) + (type ? 2 * vlen : vlen) + 3) & ~3u, end = node + len, found = 0;
        while (child < end) {
            uint32_t clen = (uint32_t)(b[child] | b[child + 1] << 8);
            if (!clen) break;
            if (key_is(b, child, part)) { node = child; found = 1; break; }
            child = (child + clen + 3) & ~3u;
        }
        if (!found) return fail(1813);
    }
    uint32_t vlen = (uint32_t)(b[node + 2] | b[node + 3] << 8), type = (uint32_t)(b[node + 4] | b[node + 5] << 8);
    uint32_t value = node_key_end(b, node);
    if (!vlen) return fail(1813);
    if (type == 1) {                                                /* text: an ANSI copy in the buffer's spare area */
        uint32_t rlen = (uint32_t)(b[0] | b[1] << 8), area = (rlen + 3) & ~3u, dst = area + value / 2;
        for (uint32_t i = 0; i < vlen; i++) {
            uint16_t c = (uint16_t)(b[value + 2 * i] | b[value + 2 * i + 1] << 8);
            ((uint8_t *)G(data))[dst + i] = c < 0x100 ? (uint8_t)c : '?';
        }
        wr32(outp, data + dst);
        if (lenp) wr32(lenp, vlen);                                 /* characters, including the terminator */
    } else {
        wr32(outp, data + value);
        if (lenp) wr32(lenp, vlen);
    }
    return 1;
}

/* ---- security: tokens, SIDs, descriptors, AccessCheck ---- */

uint32_t OpenThreadToken_c(uint32_t thread, uint32_t access, uint32_t as_self, uint32_t out)
{
    (void)thread; (void)access; (void)as_self; (void)out;
    return fail(1008);                                              /* ERROR_NO_TOKEN: not impersonating */
}
uint32_t OpenProcessToken_c(uint32_t process, uint32_t access, uint32_t out)
{
    (void)access;
    if (process != 0xFFFFFFFFu) hp_unsupported("OpenProcessToken", "another process (0x%x)", process);
    wr32(out, halopad_object_token_new(0));
    return 1;
}
uint32_t DuplicateToken_c(uint32_t token, uint32_t level, uint32_t out)
{
    if (halopad_object_token_impersonation(token) < 0) return fail(6);
    if (level > 3) return fail(87);
    wr32(out, halopad_object_token_new(1));                          /* an impersonation token */
    return 1;
}

uint32_t AllocateAndInitializeSid_c(uint32_t auth, uint32_t count, uint32_t s0, uint32_t s1, uint32_t s2, uint32_t s3, uint32_t s4,
                                    uint32_t s5, uint32_t s6, uint32_t s7, uint32_t out)
{
    if (count > 8) return fail(87);
    uint32_t subs[8] = {s0, s1, s2, s3, s4, s5, s6, s7};
    uint32_t sid = LocalAlloc_c(0x40, 8 + 4 * count);
    uint8_t *p = G(sid);
    p[0] = 1; p[1] = (uint8_t)count;
    memcpy(p + 2, G(auth), 6);
    memcpy(p + 8, subs, 4 * count);
    wr32(out, sid);
    return 1;
}
uint32_t FreeSid_c(uint32_t sid) { LocalFree_c(sid); return 0; }
uint32_t GetLengthSid_c(uint32_t sid) { return 8 + 4u * ((uint8_t *)G(sid))[1]; }

uint32_t InitializeSecurityDescriptor_c(uint32_t sd, uint32_t rev)
{
    if (rev != 1) return fail(1338);                                /* ERROR_UNKNOWN_REVISION */
    memset(G(sd), 0, 20);
    ((uint8_t *)G(sd))[0] = 1;
    return 1;
}
uint32_t IsValidSecurityDescriptor_c(uint32_t sd) { return sd && ((uint8_t *)G(sd))[0] == 1; }
uint32_t SetSecurityDescriptorDacl_c(uint32_t sd, uint32_t present, uint32_t dacl, uint32_t defaulted)
{
    uint16_t c; memcpy(&c, (uint8_t *)G(sd) + 2, 2);
    c = (uint16_t)((c & ~0x000Cu) | (present ? 0x4u : 0) | (defaulted ? 0x8u : 0));   /* SE_DACL_PRESENT, SE_DACL_DEFAULTED */
    memcpy((uint8_t *)G(sd) + 2, &c, 2);
    wr32(sd + 16, present ? dacl : 0);
    return 1;
}
uint32_t SetSecurityDescriptorGroup_c(uint32_t sd, uint32_t sid, uint32_t def) { (void)def; wr32(sd + 8, sid); return 1; }
uint32_t SetSecurityDescriptorOwner_c(uint32_t sd, uint32_t sid, uint32_t def) { (void)def; wr32(sd + 4, sid); return 1; }

uint32_t InitializeAcl_c(uint32_t acl, uint32_t len, uint32_t rev)
{
    if (rev != 2 && rev != 4) return fail(1338);
    if (len < 8) return fail(122);
    memset(G(acl), 0, 8);
    uint8_t *a = G(acl);
    a[0] = (uint8_t)rev;
    uint16_t l = (uint16_t)len;
    memcpy(a + 2, &l, 2);
    return 1;
}
uint32_t AddAccessAllowedAce_c(uint32_t acl, uint32_t rev, uint32_t mask, uint32_t sid)
{
    (void)rev;
    uint8_t *a = G(acl);
    uint16_t size, count;
    memcpy(&size, a + 2, 2); memcpy(&count, a + 4, 2);
    uint32_t at = 8;
    for (uint16_t i = 0; i < count; i++) { uint16_t s; memcpy(&s, a + at + 2, 2); at += s; }
    uint32_t sl = GetLengthSid_c(sid), ace = 8 + sl;
    if (at + ace > size) return fail(1344);                         /* ERROR_ALLOTTED_SPACE_EXCEEDED */
    a[at] = 0; a[at + 1] = 0;                                       /* ACCESS_ALLOWED_ACE_TYPE, no flags */
    uint16_t as = (uint16_t)ace; memcpy(a + at + 2, &as, 2);
    memcpy(a + at + 4, &mask, 4);
    memcpy(a + at + 8, G(sid), sl);
    count++; memcpy(a + 4, &count, 2);
    return 1;
}

/* the player's groups: Everyone, Administrators, Users, INTERACTIVE, Authenticated Users */
static int member(const uint8_t *sid)
{
    static const uint8_t groups[][16] = {
        {1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},                                    /* S-1-1-0 */
        {1, 2, 0, 0, 0, 0, 0, 5, 32, 0, 0, 0, 0x20, 0x02, 0, 0},                  /* S-1-5-32-544 */
        {1, 2, 0, 0, 0, 0, 0, 5, 32, 0, 0, 0, 0x21, 0x02, 0, 0},                  /* S-1-5-32-545 */
        {1, 1, 0, 0, 0, 0, 0, 5, 4, 0, 0, 0},                                    /* S-1-5-4 */
        {1, 1, 0, 0, 0, 0, 0, 5, 11, 0, 0, 0},                                   /* S-1-5-11 */
    };
    for (size_t i = 0; i < sizeof groups / sizeof groups[0]; i++) {
        size_t len = 8 + 4u * groups[i][1];
        if (sid[1] == groups[i][1] && !memcmp(sid, groups[i], len)) return 1;
    }
    return 0;
}

uint32_t AccessCheck_c(uint32_t sd, uint32_t token, uint32_t desired, uint32_t mapping, uint32_t privs, uint32_t privlen, uint32_t granted,
                       uint32_t status)
{
    int imp = halopad_object_token_impersonation(token);
    if (imp < 0) return fail(6);
    if (!imp) return fail(1309);                                    /* ERROR_NO_IMPERSONATION_TOKEN */
    if (!IsValidSecurityDescriptor_c(sd)) return fail(1338);
    if (rd32(privlen) < 20) { wr32(privlen, 20); return fail(122); }
    wr32(privs, 0); wr32(privs + 4, 0);                             /* PrivilegeCount, Control */
    uint32_t want = desired;                                        /* map generic rights */
    if (want & 0x80000000u) want = (want & ~0x80000000u) | rd32(mapping);
    if (want & 0x40000000u) want = (want & ~0x40000000u) | rd32(mapping + 4);
    if (want & 0x20000000u) want = (want & ~0x20000000u) | rd32(mapping + 8);
    if (want & 0x10000000u) want = (want & ~0x10000000u) | rd32(mapping + 12);
    uint16_t control; memcpy(&control, (uint8_t *)G(sd) + 2, 2);
    uint32_t dacl = rd32(sd + 16), have = 0;
    if (!(control & 0x4) || !dacl) have = want;                     /* no DACL: everyone has access */
    else {
        const uint8_t *a = G(dacl);
        uint16_t count; memcpy(&count, a + 4, 2);
        uint32_t at = 8;
        for (uint16_t i = 0; i < count; i++) {
            uint16_t s; uint32_t mask;
            memcpy(&s, a + at + 2, 2); memcpy(&mask, a + at + 4, 4);
            if (a[at] == 0 && member(a + at + 8)) have |= mask;
            else if (a[at] == 1 && member(a + at + 8)) { if (mask & want & ~have) break; }   /* denied before granted */
            at += s;
        }
    }
    int ok = (want & ~have) == 0;
    wr32(granted, ok ? want : 0);
    wr32(status, (uint32_t)ok);
    if (!ok) halopad_last_error = 5;
    return 1;
}

/* ---- the clipboard: text from the host pasteboard ---- */

static int clip_open;
uint32_t OpenClipboard_c(uint32_t hwnd) { (void)hwnd; if (clip_open) return fail(5); clip_open = 1; return 1; }
uint32_t CloseClipboard_c(void) { if (!clip_open) return fail(1418); clip_open = 0; return 1; }   /* ERROR_CLIPBOARD_NOT_OPEN */
uint32_t IsClipboardFormatAvailable_c(uint32_t fmt)
{
    char t[8];
    if (fmt != 1 && fmt != 7 && fmt != 13) return 0;               /* CF_TEXT, CF_OEMTEXT, CF_UNICODETEXT */
    return halopad_host_clipboard_text(t, sizeof t) > 0;
}
uint32_t GetClipboardData_c(uint32_t fmt)
{
    if (!clip_open) return fail(1418);
    if (fmt != 1) hp_unsupported("GetClipboardData", "format %u", fmt);
    char text[8192];
    int n = halopad_host_clipboard_text(text, sizeof text);
    if (n <= 0) return 0;
    uint32_t m = GlobalAlloc_c(0, 2u * (uint32_t)n + 1), o = 0;
    for (int i = 0; i < n; i++) {                                   /* Windows text: CRLF */
        if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r')) ((char *)G(m))[o++] = '\r';
        ((char *)G(m))[o++] = text[i];
    }
    ((char *)G(m))[o] = 0;
    return m;
}
