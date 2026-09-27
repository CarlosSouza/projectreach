/* HaloPad Windows runtime: kernel32 file services (first slice: read-only opens).
 * Guest paths are resolved under HALOPAD_GAME_ROOT with '\\' mapped to '/'. Handles are
 * small table indices, never host pointers. Unsupported parameter combinations stop
 * loudly with their values. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static FILE *hp_lookup(uint32_t handle)
{
    if (handle < HP_FILE_BASE || (handle - HP_FILE_BASE) % 4 != 0) return NULL;
    uint32_t i = (handle - HP_FILE_BASE) / 4;
    return i < HP_MAX_FILES ? hp_files[i] : NULL;
}

uint32_t CreateFileA_c(const char *name, uint32_t access, uint32_t share, uint32_t security,
                       uint32_t disposition, uint32_t flags, uint32_t template_file)
{
    (void)share; (void)flags;
    if (access != 0x80000000u || disposition != 3 /* OPEN_EXISTING */ || security != 0 || template_file != 0)
        hp_trap("CreateFileA access/disposition", access, disposition);
    const char *root = getenv("HALOPAD_GAME_ROOT");
    if (!root) { fprintf(stderr, "HALOPAD TRAP: HALOPAD_GAME_ROOT is not set\n"); abort(); }
    char path[1024];
    size_t n = (size_t)snprintf(path, sizeof path, "%s/%s", root, name);
    if (n >= sizeof path) hp_trap("CreateFileA path length", (uint32_t)n, 0);
    for (char *p = path + strlen(root); *p; p++) if (*p == '\\') *p = '/';
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

uint32_t CloseHandle_c(uint32_t handle)
{
    FILE *f = hp_lookup(handle);
    if (!f) { halopad_last_error = 6; return 0; }
    fclose(f);
    hp_files[(handle - HP_FILE_BASE) / 4] = NULL;
    return 1;
}

