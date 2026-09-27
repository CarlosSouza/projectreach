/* HaloPad CryptoAPI hashing (G3): advapi32's CryptAcquireContextA/CryptCreateHash/
 * CryptHashData/CryptGetHashParam as Halo uses them (0x5829e0, 0x582890): a verify-only
 * PROV_RSA_FULL context and CALG_SHA1 hashes, here with CommonCrypto (MD5 too). Keys,
 * encryption and other algorithms stop with their values. */
#include "halopad_win32.h"
#include <CommonCrypto/CommonDigest.h>

#define NTE_BAD_ALGID 0x80090008u
#define NTE_BAD_HASH_STATE 0x8009000Cu
#define NTE_BAD_TYPE 0x8009000Au
#define NTE_BAD_FLAGS 0x80090009u
#define ERROR_MORE_DATA 234u
#define PROV_BASE 0x5000u
#define HASH_BASE 0x5100u

typedef struct { int used, final; uint32_t alg; CC_SHA1_CTX sha; CC_MD5_CTX md5; uint8_t value[20]; } hash;
static hash hashes[64];
static int providers[16];

static uint32_t fail(uint32_t e) { halopad_last_error = e; return 0; }

uint32_t CryptAcquireContextA_c(uint32_t out, uint32_t container, uint32_t prov, uint32_t type, uint32_t flags)
{
    if (!out) return fail(87);
    if (type != 1) hp_unsupported("CryptAcquireContextA", "provider type %u", type);   /* PROV_RSA_FULL */
    if (prov) hp_unsupported("CryptAcquireContextA", "a named provider \"%s\"", (const char *)G(prov));
    if (!(flags & 0xF0000000u) || (flags & ~0xF0000000u) || container)
        hp_unsupported("CryptAcquireContextA", "a key container (flags 0x%x)", flags);   /* only CRYPT_VERIFYCONTEXT */
    for (int i = 0; i < 16; i++) if (!providers[i]) { providers[i] = 1; wr32(out, PROV_BASE + 4u * (uint32_t)i); return 1; }
    hp_unsupported("CryptAcquireContextA", "more than 16 contexts");
}

uint32_t CryptReleaseContext_c(uint32_t h, uint32_t flags)
{
    if (flags) return fail(NTE_BAD_FLAGS);
    uint32_t i = (h - PROV_BASE) / 4;
    if (h < PROV_BASE || i >= 16 || !providers[i]) return fail(6);
    providers[i] = 0;
    return 1;
}

static hash *H(uint32_t h) { uint32_t i = (h - HASH_BASE) / 4; return h >= HASH_BASE && i < 64 && hashes[i].used ? &hashes[i] : NULL; }

uint32_t CryptCreateHash_c(uint32_t prov, uint32_t alg, uint32_t key, uint32_t flags, uint32_t out)
{
    uint32_t pi = (prov - PROV_BASE) / 4;
    if (prov < PROV_BASE || pi >= 16 || !providers[pi]) return fail(6);
    if (key || flags) hp_unsupported("CryptCreateHash", "a keyed hash (key 0x%x, flags 0x%x)", key, flags);
    if (alg != 0x8004 && alg != 0x8003) hp_unsupported("CryptCreateHash", "algorithm 0x%x", alg);   /* CALG_SHA1, CALG_MD5 */
    for (uint32_t i = 0; i < 64; i++)
        if (!hashes[i].used) {
            hashes[i] = (hash){1, 0, alg, {0}, {0}, {0}};
            if (alg == 0x8004) CC_SHA1_Init(&hashes[i].sha); else CC_MD5_Init(&hashes[i].md5);
            wr32(out, HASH_BASE + 4 * i);
            return 1;
        }
    hp_unsupported("CryptCreateHash", "more than 64 hashes");
}

uint32_t CryptHashData_c(uint32_t h, uint32_t data, uint32_t len, uint32_t flags)
{
    hash *x = H(h);
    if (!x) return fail(6);
    if (flags) return fail(NTE_BAD_FLAGS);
    if (x->final) return fail(NTE_BAD_HASH_STATE);
    if (len) { if (x->alg == 0x8004) CC_SHA1_Update(&x->sha, G(data), len); else CC_MD5_Update(&x->md5, G(data), len); }
    return 1;
}

uint32_t CryptGetHashParam_c(uint32_t h, uint32_t param, uint32_t data, uint32_t lenp, uint32_t flags)
{
    hash *x = H(h);
    if (!x) return fail(6);
    if (flags) return fail(NTE_BAD_FLAGS);
    uint32_t size = x->alg == 0x8004 ? 20 : 16;
    uint8_t v[20];
    uint32_t n;
    if (param == 1) { memcpy(v, &x->alg, 4); n = 4; }               /* HP_ALGID */
    else if (param == 4) { memcpy(v, &size, 4); n = 4; }            /* HP_HASHSIZE */
    else if (param == 2) {                                          /* HP_HASHVAL: finishes the hash */
        if (!x->final) {
            if (x->alg == 0x8004) CC_SHA1_Final(x->value, &x->sha); else CC_MD5_Final(x->value, &x->md5);
            x->final = 1;
        }
        memcpy(v, x->value, size);
        n = size;
    } else return fail(NTE_BAD_TYPE);
    if (!lenp) return fail(87);
    if (!data) { wr32(lenp, n); return 1; }
    if (rd32(lenp) < n) { wr32(lenp, n); return fail(ERROR_MORE_DATA); }
    memcpy(G(data), v, n);
    wr32(lenp, n);
    return 1;
}

uint32_t CryptDestroyHash_c(uint32_t h)
{
    hash *x = H(h);
    if (!x) return fail(6);
    x->used = 0;
    return 1;
}

/* ---- winmm: timer resolution (the host's timers are already fine-grained) ---- */

static uint32_t periods[1001];
uint32_t timeBeginPeriod_c(uint32_t ms) { if (ms < 1 || ms > 1000) return 97; periods[ms]++; return 0; }   /* TIMERR_NOCANDO */
uint32_t timeEndPeriod_c(uint32_t ms) { if (ms < 1 || ms > 1000 || !periods[ms]) return 97; periods[ms]--; return 0; }
