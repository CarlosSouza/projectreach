/* HaloPad Windows runtime: kernel32 file services (first slice: read-only opens).
 * Guest paths are resolved under HALOPAD_GAME_ROOT with '\\' mapped to '/'. Handles are
 * small table indices, never host pointers. Unsupported parameter combinations stop
 * loudly with their values. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define HP_INVALID_HANDLE 0xFFFFFFFFu
#define HP_FILE_BASE 0x100u
#define HP_MAX_FILES 64

static FILE *hp_files[HP_MAX_FILES];
uint32_t halopad_last_error;

static void hp_trap(const char *what, uint32_t a, uint32_t b)
{
    fprintf(stderr, "HALOPAD TRAP: %s (0x%x, 0x%x) not supported by the runtime yet\n", what, a, b);
    abort();
}

/* Guest paths: relative paths and paths under the install directory
   (C:\\Program Files\\Microsoft Games\\Halo Custom Edition) map into HALOPAD_GAME_ROOT, with
   '\\' as '/'. The current directory is the install directory. Returns 0 for any other
   path. */
int halopad_host_path(const char *guest, char *out, size_t size)
{
    static const char install[] = "C:\\Program Files\\Microsoft Games\\Halo Custom Edition";
    const char *root = getenv("HALOPAD_GAME_ROOT");
    if (!root) { fprintf(stderr, "HALOPAD TRAP: HALOPAD_GAME_ROOT is not set\n"); abort(); }
    const char *rel = guest;
    size_t il = strlen(install);
    if (!strncasecmp(guest, install, il) && (guest[il] == '\\' || guest[il] == 0)) rel = guest + il + (guest[il] ? 1 : 0);
    else if ((guest[0] && guest[1] == ':') || guest[0] == '\\' || guest[0] == '/') return 0;
    while (rel[0] == '.' && (rel[1] == '\\' || rel[1] == '/')) rel += 2;
    size_t n = (size_t)snprintf(out, size, "%s/%s", root, rel);
    if (n >= size) return 0;
    for (char *p = out + strlen(root); *p; p++) if (*p == '\\') *p = '/';
    return 1;
}

static FILE *hp_lookup(uint32_t handle)
{
    if (handle < HP_FILE_BASE || (handle - HP_FILE_BASE) % 4 != 0) return NULL;
    uint32_t i = (handle - HP_FILE_BASE) / 4;
    return i < HP_MAX_FILES ? hp_files[i] : NULL;
}

uint32_t halopad_file_handle_valid(uint32_t handle) { return hp_lookup(handle) != NULL; }

uint32_t CreateFileA_c(const char *name, uint32_t access, uint32_t share, uint32_t security,
                       uint32_t disposition, uint32_t flags, uint32_t template_file)
{
    (void)share; (void)flags;
    if (access != 0x80000000u || disposition != 3 /* OPEN_EXISTING */ || security != 0 || template_file != 0)
        hp_trap("CreateFileA access/disposition", access, disposition);
    char path[1024];
    if (!halopad_host_path(name, path, sizeof path)) hp_trap("CreateFileA path outside the game directory", 0, 0);
    FILE *f = fopen(path, "rb");
    if (!f) { halopad_last_error = 2; /* ERROR_FILE_NOT_FOUND */ return HP_INVALID_HANDLE; }
    for (uint32_t i = 0; i < HP_MAX_FILES; i++)
        if (!hp_files[i]) { hp_files[i] = f; return HP_FILE_BASE + 4 * i; }
    fclose(f);
    hp_trap("CreateFileA handle table full", HP_MAX_FILES, 0);
    return HP_INVALID_HANDLE;
}

uint32_t ReadFile_c(uint32_t handle, void *buffer, uint32_t count, uint32_t *read, uint32_t overlapped)
{
    if (overlapped != 0) hp_trap("ReadFile overlapped", handle, overlapped);
    FILE *f = hp_lookup(handle);
    if (!f) { halopad_last_error = 6; /* ERROR_INVALID_HANDLE */ return 0; }
    size_t got = fread(buffer, 1, count, f);
    *read = (uint32_t)got;
    return 1;
}

int halopad_is_object_handle(uint32_t handle);
uint32_t halopad_object_close(uint32_t handle);

uint32_t CloseHandle_c(uint32_t handle)
{
    if (halopad_is_object_handle(handle)) return halopad_object_close(handle);
    FILE *f = hp_lookup(handle);
    if (!f) { halopad_last_error = 6; return 0; }
    fclose(f);
    hp_files[(handle - HP_FILE_BASE) / 4] = NULL;
    return 1;
}
