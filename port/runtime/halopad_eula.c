/* HaloPad replacement for the game's Eula.dll (G3): EBUEula(lpRegKeyLocation, lpEULAFileName,
 * lpWarrantyFileName, fCheckForFirstRun), called by Halo on first run.
 *
 * The real DLL shows the game's RTF license and, when the player accepts, writes REG_DWORD
 * FIRSTRUN = 1 under HKCU\<lpRegKeyLocation> and returns TRUE; declining returns FALSE and
 * Halo exits. HaloPad never accepts on the player's behalf: it returns TRUE only when the
 * player has recorded acceptance of this exact license file with scripts/accept-eula.sh
 * (HALOPAD_EULA_ACCEPTANCE names that record; it holds the file's SHA-256). */
#include "halopad_win32.h"
#include <CommonCrypto/CommonDigest.h>
#include <time.h>

int halopad_host_path(const char *guest, char *out, size_t size);
void halopad_registry_set_dword(const char *path, const char *name, uint32_t value);
uint32_t halopad_registry_get_dword(const char *path, const char *name, uint32_t *value);

static int file_sha256(const char *path, char hex[65])
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    CC_SHA256_CTX c;
    CC_SHA256_Init(&c);
    unsigned char buf[65536], md[32];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) CC_SHA256_Update(&c, buf, (CC_LONG)n);
    fclose(f);
    CC_SHA256_Final(md, &c);
    for (int i = 0; i < 32; i++) snprintf(hex + 2 * i, 3, "%02x", md[i]);
    return 1;
}

/* 1 if the record at path lists the license's SHA-256 */
static int recorded(const char *path, const char *sha)
{
    char line[256];
    FILE *f = path ? fopen(path, "r") : NULL;
    int ok = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f))
        if (!strncmp(line, "sha256 ", 7) && !strncmp(line + 7, sha, 64)) ok = 1;
    fclose(f);
    return ok;
}

/* The host's license screen: shows the RTF license to the player and waits for the choice.
   1 accepted, 0 declined, -1 no screen to show it on (the macOS runner, tests). The iPadOS app
   shell provides it (port/ios). */
__attribute__((weak)) int halopad_host_license_prompt(const char *rtf_path) { (void)rtf_path; return -1; }

uint32_t EBUEula_c(uint32_t regkey, uint32_t eulafile, uint32_t warranty, uint32_t check)
{
    if (warranty) hp_unsupported("EBUEula", "a warranty file");
    char key[512];
    snprintf(key, sizeof key, "HKCU\\%s", (const char *)G(regkey));
    uint32_t first = 0;
    if (check && halopad_registry_get_dword(key, "FIRSTRUN", &first) && first) return 1;

    char path[1024], sha[65];
    if (!halopad_host_path(G(eulafile), path, sizeof path) || !file_sha256(path, sha))
        hp_unsupported("EBUEula", "license file \"%s\" not found", (const char *)G(eulafile));

    const char *record = getenv("HALOPAD_EULA_ACCEPTANCE"), *state = getenv("HALOPAD_STATE_ROOT");
    char app_record[1100] = {0};
    if (state) snprintf(app_record, sizeof app_record, "%s/eula-acceptance.txt", state);
    int accepted = recorded(record, sha) || (state && recorded(app_record, sha));
    if (!accepted && state) {
        int choice = halopad_host_license_prompt(path);           /* the player reads it and chooses */
        if (choice == 0) { fprintf(stderr, "HALOPAD: the player declined the Halo license.\n"); return 0; }
        if (choice == 1) {
            FILE *out = fopen(app_record, "w");
            if (!out) hp_unsupported("EBUEula", "recording acceptance in %s", app_record);
            time_t now = time(NULL);
            char when[32];
            strftime(when, sizeof when, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
            fprintf(out, "sha256 %s\nfile Eula.rtf\naccepted %s by the player in the HaloPad app\n", sha, when);
            fclose(out);
            accepted = 1;
        }
    }
    if (!accepted) {
        fprintf(stderr, "HALOPAD: first run: the Halo license (%s, sha256 %s) has not been accepted by the player.\n"
                        "HALOPAD: read and accept it with scripts/accept-eula.sh; declining, as Eula.dll does.\n", path, sha);
        return 0;
    }
    halopad_registry_set_dword(key, "FIRSTRUN", 1);
    return 1;
}
