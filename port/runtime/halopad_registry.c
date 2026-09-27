/* HaloPad registry (G3): RegOpenKeyA/ExA, RegCreateKeyExA, RegQueryValueExA,
 * RegSetValueExA, RegCloseKey over a persistent virtual registry.
 *
 * Contents start from config/runtime/registry-machine.txt (what the reference machine has
 * before Halo is installed; documented in docs/G3-RUNTIME.md) and are then read from and
 * written to the state file (HALOPAD_REGISTRY, default generated/runtime-state/registry.txt).
 * Nothing is invented: a key or value that is not present fails with ERROR_FILE_NOT_FOUND,
 * as on Windows. In particular no product-key-derived value (PID, DigitalProductID) is
 * ever created by HaloPad.
 *
 * File format, one entry per line:  key <PATH>   |   value <PATH> <type> <name-hex> <data-hex>
 * PATH is ROOT\sub\key with ROOT one of HKCR, HKCU, HKLM, HKU; comparison is case-insensitive. */
#include "halopad_win32.h"
#include <ctype.h>
#include <strings.h>

#define ERROR_SUCCESS 0
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_MORE_DATA 234

typedef struct { char *path; } rkey;
typedef struct { char *path; char *name; uint32_t type, size; uint8_t *data; } rvalue;

static rkey *keys; static uint32_t nkeys;
static rvalue *vals; static uint32_t nvals;
static char state_path[1024];
static int loaded;

static const char *root_name(uint32_t h)
{
    switch (h) {
    case 0x80000000u: return "HKCR";
    case 0x80000001u: return "HKCU";
    case 0x80000002u: return "HKLM";
    case 0x80000003u: return "HKU";
    }
    return NULL;
}

/* open handles: small table of key paths; handle values 0x100, 0x104, ... */
static char *handles[256];

static const char *handle_path(uint32_t h)
{
    const char *r = root_name(h);
    if (r) return r;
    if (h >= 0x100 && (h - 0x100) % 4 == 0 && (h - 0x100) / 4 < 256) return handles[(h - 0x100) / 4];
    return NULL;
}

static int find_key(const char *path)
{
    for (uint32_t i = 0; i < nkeys; i++) if (!strcasecmp(keys[i].path, path)) return (int)i;
    return -1;
}

static void add_key(const char *path)
{
    /* parents first, so every prefix exists as a key */
    char buf[1024];
    snprintf(buf, sizeof buf, "%s", path);
    for (char *p = buf; *p; p++) {
        if (*p != '\\') continue;
        *p = 0;
        if (find_key(buf) < 0) { keys = realloc(keys, (nkeys + 1) * sizeof *keys); keys[nkeys++].path = strdup(buf); }
        *p = '\\';
    }
    if (find_key(buf) < 0) { keys = realloc(keys, (nkeys + 1) * sizeof *keys); keys[nkeys++].path = strdup(buf); }
}

static int find_value(const char *path, const char *name)
{
    for (uint32_t i = 0; i < nvals; i++) if (!strcasecmp(vals[i].path, path) && !strcasecmp(vals[i].name, name)) return (int)i;
    return -1;
}

static int unhex(const char *s, uint8_t *out, size_t max)
{
    size_t n = strlen(s) / 2;
    if (n > max) return -1;
    for (size_t i = 0; i < n; i++) { unsigned v; if (sscanf(s + 2 * i, "%2x", &v) != 1) return -1; out[i] = (uint8_t)v; }
    return (int)n;
}

static void load_file(const char *file, int required)
{
    FILE *f = fopen(file, "r");
    if (!f) { if (required) hp_unsupported("registry", "cannot read %s", file); return; }
    char line[8192];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || line[0] == '#') continue;
        char path[1024], namehex[1024], datahex[4096];
        uint32_t type;
        if (!strncmp(line, "key ", 4)) { add_key(line + 4); continue; }
        if (sscanf(line, "value %1023s %u %1023s %4095s", path, &type, namehex, datahex) == 4) {
            uint8_t name[512], data[2048];
            int nn = strcmp(namehex, "-") ? unhex(namehex, name, sizeof name - 1) : 0;
            int dn = strcmp(datahex, "-") ? unhex(datahex, data, sizeof data) : 0;
            if (nn < 0 || dn < 0) hp_unsupported("registry", "malformed line in %s: %s", file, line);
            name[nn] = 0;
            add_key(path);
            vals = realloc(vals, (nvals + 1) * sizeof *vals);
            vals[nvals] = (rvalue){strdup(path), strdup((char *)name), type, (uint32_t)dn, malloc((size_t)dn + 1)};
            memcpy(vals[nvals].data, data, (size_t)dn);
            nvals++;
            continue;
        }
        hp_unsupported("registry", "malformed line in %s: %s", file, line);
    }
    fclose(f);
}

static void save(void)
{
    char tmp[1100];
    snprintf(tmp, sizeof tmp, "%s.tmp", state_path);
    FILE *f = fopen(tmp, "w");
    if (!f) hp_unsupported("registry", "cannot write %s", tmp);
    fprintf(f, "# HaloPad registry state (written by the runtime)\n");
    for (uint32_t i = 0; i < nkeys; i++) fprintf(f, "key %s\n", keys[i].path);
    for (uint32_t i = 0; i < nvals; i++) {
        fprintf(f, "value %s %u ", vals[i].path, vals[i].type);
        if (!vals[i].name[0]) fputc('-', f);
        for (const char *p = vals[i].name; *p; p++) fprintf(f, "%02x", (uint8_t)*p);
        fputc(' ', f);
        if (!vals[i].size) fputc('-', f);
        for (uint32_t k = 0; k < vals[i].size; k++) fprintf(f, "%02x", vals[i].data[k]);
        fputc('\n', f);
    }
    fclose(f);
    rename(tmp, state_path);
}

static void ensure_loaded(void)
{
    if (loaded) return;
    loaded = 1;
    const char *s = getenv("HALOPAD_REGISTRY");
    const char *root = getenv("HALOPAD_REPO_ROOT");
    if (s) snprintf(state_path, sizeof state_path, "%s", s);
    else snprintf(state_path, sizeof state_path, "%s/generated/runtime-state/registry.txt", root ? root : ".");
    FILE *f = fopen(state_path, "r");
    if (f) { fclose(f); load_file(state_path, 1); return; }
    char seed[1100];
    snprintf(seed, sizeof seed, "%s/config/runtime/registry-machine.txt", root ? root : ".");
    load_file(seed, 1);
    save();
}

static int join(char *out, size_t size, uint32_t h, uint32_t sub)
{
    const char *base = handle_path(h);
    if (!base) return 0;
    const char *s = sub ? G(sub) : "";
    while (*s == '\\') s++;
    snprintf(out, size, "%s%s%s", base, *s ? "\\" : "", s);
    size_t n = strlen(out);
    while (n && out[n - 1] == '\\') out[--n] = 0;
    return 1;
}

static uint32_t new_handle(const char *path, uint32_t out)
{
    for (uint32_t i = 0; i < 256; i++)
        if (!handles[i]) { handles[i] = strdup(path); wr32(out, 0x100 + 4 * i); return ERROR_SUCCESS; }
    hp_unsupported("RegOpenKeyExA", "more than 256 open keys");
}

uint32_t RegOpenKeyExA_c(uint32_t h, uint32_t sub, uint32_t options, uint32_t sam, uint32_t out)
{
    (void)sam;
    if (options) hp_unsupported("RegOpenKeyExA", "options 0x%x", options);
    ensure_loaded();
    char path[1024];
    if (!join(path, sizeof path, h, sub)) return HP_ERROR_INVALID_HANDLE;
    if (strchr(path, '\\') && find_key(path) < 0) return ERROR_FILE_NOT_FOUND;   /* roots always exist */
    return new_handle(path, out);
}

uint32_t RegOpenKeyA_c(uint32_t h, uint32_t sub, uint32_t out) { return RegOpenKeyExA_c(h, sub, 0, 0x2001F, out); }

uint32_t RegCreateKeyExA_c(uint32_t h, uint32_t sub, uint32_t reserved, uint32_t cls, uint32_t options, uint32_t sam,
                           uint32_t security, uint32_t out, uint32_t disposition)
{
    (void)reserved; (void)cls; (void)sam;
    if (options || security) hp_unsupported("RegCreateKeyExA", "options 0x%x / security attributes 0x%x", options, security);
    ensure_loaded();
    char path[1024];
    if (!join(path, sizeof path, h, sub)) return HP_ERROR_INVALID_HANDLE;
    int existed = find_key(path) >= 0 || !strchr(path, '\\');
    if (!existed) { add_key(path); save(); }
    if (disposition) wr32(disposition, existed ? 2 /* REG_OPENED_EXISTING_KEY */ : 1 /* REG_CREATED_NEW_KEY */);
    return new_handle(path, out);
}

uint32_t RegCloseKey_c(uint32_t h)
{
    if (root_name(h)) return ERROR_SUCCESS;
    if (h >= 0x100 && (h - 0x100) % 4 == 0 && (h - 0x100) / 4 < 256 && handles[(h - 0x100) / 4]) {
        free(handles[(h - 0x100) / 4]);
        handles[(h - 0x100) / 4] = NULL;
        return ERROR_SUCCESS;
    }
    return HP_ERROR_INVALID_HANDLE;
}

uint32_t RegQueryValueExA_c(uint32_t h, uint32_t name, uint32_t reserved, uint32_t type, uint32_t data, uint32_t size)
{
    (void)reserved;
    ensure_loaded();
    const char *path = handle_path(h);
    if (!path) return HP_ERROR_INVALID_HANDLE;
    int i = find_value(path, name ? (const char *)G(name) : "");
    if (i < 0) return ERROR_FILE_NOT_FOUND;
    if (type) wr32(type, vals[i].type);
    if (!size) return data ? HP_ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
    uint32_t have = rd32(size);
    wr32(size, vals[i].size);
    if (!data) return ERROR_SUCCESS;
    if (have < vals[i].size) return ERROR_MORE_DATA;
    memcpy(G(data), vals[i].data, vals[i].size);
    return ERROR_SUCCESS;
}

uint32_t RegSetValueExA_c(uint32_t h, uint32_t name, uint32_t reserved, uint32_t type, uint32_t data, uint32_t size)
{
    (void)reserved;
    ensure_loaded();
    const char *path = handle_path(h);
    if (!path || root_name(h)) return HP_ERROR_INVALID_HANDLE;
    const char *n = name ? (const char *)G(name) : "";
    int i = find_value(path, n);
    if (i < 0) {
        vals = realloc(vals, (nvals + 1) * sizeof *vals);
        i = (int)nvals++;
        vals[i] = (rvalue){strdup(path), strdup(n), 0, 0, NULL};
    }
    free(vals[i].data);
    vals[i].type = type;
    vals[i].size = size;
    vals[i].data = malloc(size + 1);
    memcpy(vals[i].data, G(data), size);   /* stored exactly as given, as Windows does */
    save();
    return ERROR_SUCCESS;
}

/* Internal helpers for runtime components that act like Windows DLLs (e.g. Eula.dll). */
void halopad_registry_set_dword(const char *path, const char *name, uint32_t value)
{
    ensure_loaded();
    add_key(path);
    int i = find_value(path, name);
    if (i < 0) {
        vals = realloc(vals, (nvals + 1) * sizeof *vals);
        i = (int)nvals++;
        vals[i] = (rvalue){strdup(path), strdup(name), 0, 0, NULL};
    }
    free(vals[i].data);
    vals[i].type = 4;                      /* REG_DWORD */
    vals[i].size = 4;
    vals[i].data = malloc(4);
    memcpy(vals[i].data, &value, 4);
    save();
}

uint32_t halopad_registry_get_dword(const char *path, const char *name, uint32_t *value)
{
    ensure_loaded();
    int i = find_value(path, name);
    if (i < 0 || vals[i].type != 4 || vals[i].size != 4) return 0;
    memcpy(value, vals[i].data, 4);
    return 1;
}
