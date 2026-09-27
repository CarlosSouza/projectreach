/* HaloPad resource services (G3): FindResourceA/ExA, LoadResource, LockResource,
 * SizeofResource and USER32 LoadStringA over PE resource directories in guest memory
 * (haloce.exe at its image base, and resource DLLs mapped by halopad_modules.c).
 *
 * Language selection for LANG_NEUTRAL follows the Windows fallback on the reference
 * machine (US English): neutral, en-US, English/sublang-neutral, then the first
 * language present. An HRSRC is the guest address of the resource's data entry, and
 * LoadResource/LockResource return the resource's guest address, as on Windows. */
#include "halopad_win32.h"

uint32_t halopad_module_mapped(uint32_t handle);   /* halopad_modules.c: 1 if a PE image is at handle */
uint32_t WideCharToMultiByte_c(uint32_t cp, uint32_t flags, uint32_t src, uint32_t srclen, uint32_t dst, uint32_t dstlen,
                               uint32_t defchar, uint32_t useddef);

static uint16_t rd16(uint32_t a) { uint16_t v; memcpy(&v, halopad_guest_ptr(a), 2); return v; }

static uint32_t resource_root(const char *service, uint32_t module)
{
    if (!module) module = HP_IMAGE_BASE;
    module &= ~1u;                                  /* LoadLibraryExA(AS_DATAFILE) handles carry the low bit */
    if (!halopad_module_mapped(module)) hp_unsupported(service, "module 0x%08x without a mapped image", module);
    uint32_t pe = module + rd32(module + 0x3C);
    uint32_t rva = rd32(pe + 0x78 + 2 * 8);            /* data directory 2: resources */
    return rva ? module + rva : 0;
}

/* Compare a directory entry's name (counted UTF-16, case-insensitive like Windows' upper-
   casing of ASCII names) with an ANSI name. */
static int name_matches(uint32_t str, const char *name)
{
    uint16_t len = rd16(str);
    if (strlen(name) != len) return 0;
    for (uint16_t i = 0; i < len; i++) {
        uint16_t c = rd16(str + 2 + 2 * i);
        char n = name[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        if (n >= 'a' && n <= 'z') n -= 32;
        if (c != (uint8_t)n) return 0;
    }
    return 1;
}

/* key: an integer id (< 0x10000) or a guest ANSI string ('#123' means id 123). */
static uint32_t find_entry(uint32_t root, uint32_t dir, uint32_t key)
{
    uint32_t id = 0xFFFFFFFFu;
    const char *name = NULL;
    if (key < 0x10000) id = key;
    else {
        name = G(key);
        if (name[0] == '#') id = (uint32_t)strtoul(name + 1, NULL, 10), name = NULL;
    }
    uint16_t named = rd16(dir + 12), ids = rd16(dir + 14);
    for (uint32_t i = 0; i < (uint32_t)named + ids; i++) {
        uint32_t e = dir + 16 + 8 * i, n = rd32(e), off = rd32(e + 4);
        int match = (n & 0x80000000u) ? (name && name_matches(root + (n & 0x7FFFFFFFu), name)) : (!name && n == id);
        if (match) return off;
    }
    return 0xFFFFFFFFu;
}

static uint32_t find_lang(uint32_t root, uint32_t dir, uint32_t lang)
{
    uint32_t tries[5] = {lang, 0x0409, 0x0009, 0x0000, 0xFFFFFFFFu};
    uint32_t n = lang ? 1 : 4;
    if (!lang) tries[0] = 0x0000;
    for (uint32_t t = 0; t < n; t++) {
        uint32_t off = find_entry(root, dir, tries[t]);
        if (off != 0xFFFFFFFFu) return off;
    }
    if (!lang && rd16(dir + 12) + rd16(dir + 14) > 0) return rd32(dir + 16 + 4);   /* first language present */
    return 0xFFFFFFFFu;
}

uint32_t FindResourceExA_c(uint32_t module, uint32_t type, uint32_t name, uint32_t lang)
{
    uint32_t root = resource_root("FindResourceExA", module);
    if (!root) { halopad_last_error = 1813; return 0; }             /* ERROR_RESOURCE_TYPE_NOT_FOUND */
    uint32_t t = find_entry(root, root, type);
    if (t == 0xFFFFFFFFu || !(t & 0x80000000u)) { halopad_last_error = 1813; return 0; }
    uint32_t n = find_entry(root, root + (t & 0x7FFFFFFFu), name);
    if (n == 0xFFFFFFFFu || !(n & 0x80000000u)) { halopad_last_error = 1814; return 0; }   /* ERROR_RESOURCE_NAME_NOT_FOUND */
    uint32_t l = find_lang(root, root + (n & 0x7FFFFFFFu), lang & 0xFFFF);
    if (l == 0xFFFFFFFFu || (l & 0x80000000u)) { halopad_last_error = 1815; return 0; }   /* ERROR_RESOURCE_LANG_NOT_FOUND */
    return root + l;
}

uint32_t FindResourceA_c(uint32_t module, uint32_t name, uint32_t type) { return FindResourceExA_c(module, type, name, 0); }

uint32_t halopad_heap_alloc(uint32_t size, int zero);
void halopad_heap_free(uint32_t p);
/* a wide name or type (or an integer id) as the ANSI key FindResourceExA takes */
static uint32_t ansi_key(uint32_t w)
{
    if (w < 0x10000) return w;
    uint32_t n = 0;
    while (rd16(w + 2 * n)) n++;
    uint32_t a = halopad_heap_alloc(n + 1, 1);
    for (uint32_t i = 0; i < n; i++) {
        uint16_t c = rd16(w + 2 * i);
        if (c >= 0x80) hp_unsupported("FindResourceW", "a resource name or type outside ASCII (U+%04X)", c);
        ((char *)G(a))[i] = (char)c;
    }
    return a;
}
uint32_t FindResourceW_c(uint32_t module, uint32_t name, uint32_t type)
{
    uint32_t n = ansi_key(name), t = ansi_key(type);
    uint32_t r = FindResourceExA_c(module, t, n, 0);
    uint32_t e = halopad_last_error;
    if (n != name) halopad_heap_free(n);
    if (t != type) halopad_heap_free(t);
    halopad_last_error = e;
    return r;
}

uint32_t LoadResource_c(uint32_t module, uint32_t hrsrc)
{
    if (!hrsrc) { halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    if (!module) module = HP_IMAGE_BASE;
    return (module & ~1u) + rd32(hrsrc);                          /* data-file handles carry the low bit */
}

uint32_t LockResource_c(uint32_t data) { return data; }

uint32_t SizeofResource_c(uint32_t module, uint32_t hrsrc)
{
    (void)module;
    if (!hrsrc) { halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    return rd32(hrsrc + 4);
}

/* String tables: RT_STRING (6), block (id >> 4) + 1, 16 counted UTF-16 strings per block. */
uint32_t LoadStringA_c(uint32_t module, uint32_t id, uint32_t buf, uint32_t size)
{
    if (!size) hp_unsupported("LoadStringA", "a zero-length buffer");
    uint32_t r = FindResourceExA_c(module, 6, (id >> 4) + 1, 0);
    ((char *)G(buf))[0] = 0;
    if (!r) return 0;
    uint32_t p = LoadResource_c(module, r);
    for (uint32_t i = 0; i < (id & 15); i++) p += 2 + 2 * rd16(p);
    uint32_t len = rd16(p);
    if (!len) return 0;
    if (len > size - 1) len = size - 1;
    uint32_t n = WideCharToMultiByte_c(0, 0, p + 2, len, buf, len, 0, 0);
    ((char *)G(buf))[n] = 0;
    return n;
}
