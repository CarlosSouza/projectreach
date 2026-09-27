/* Compile coverage for HaloPad's shader translator: translates every program in Halo's
 * shader collections (generated/analysis/shaders/, see tools/halo_shader_extract.c) and
 * compiles each with Metal. Prints totals and the first failures.
 * Build/run: scripts/shader-compile-test.sh */
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdint.h>

typedef struct { uint8_t sampler_dim[16]; uint8_t projected[16]; uint8_t test_kernel; } hp_shader_key;
char *halopad_shader_to_msl(const uint32_t *t, uint32_t n, const hp_shader_key *key, char *err, size_t errlen);

static int total, translated, compiled, shown;
static id<MTLDevice> gpu;

static void check(const char *label, const uint32_t *tok, uint32_t n)
{
    total++;
    char err[512] = "";
    char *msl = halopad_shader_to_msl(tok, n, NULL, err, sizeof err);
    if (!msl) {
        if (shown++ < 12) printf("TRANSLATE %s (%08x): %s\n", label, tok[0], err);
        return;
    }
    translated++;
    NSError *e = nil;
    id<MTLLibrary> lib = [gpu newLibraryWithSource:[NSString stringWithUTF8String:msl] options:nil error:&e];
    if (lib && [lib newFunctionWithName:(tok[0] >> 16) == 0xFFFE ? @"hp_vs" : @"hp_ps"]) compiled++;
    else if (shown++ < 12) printf("COMPILE %s (%08x): %s\n", label, tok[0], e.localizedDescription.UTF8String);
    if (getenv("HALOPAD_MSL_DUMP") && shown < 3) printf("%s\n", msl);
    free(msl);
}

int main(int argc, char **argv)
{
    @autoreleasepool {
        gpu = MTLCreateSystemDefaultDevice();
        NSString *dir = [NSString stringWithUTF8String:argc > 1 ? argv[1] : "generated/analysis/shaders"];
        NSData *vsh = [NSData dataWithContentsOfFile:[dir stringByAppendingPathComponent:@"vsh.bin"]];
        const uint8_t *d = vsh.bytes;
        size_t off = 0, len = vsh.length;
        int idx = 0;
        while (off + 4 <= len) {
            uint32_t sz;
            memcpy(&sz, d + off, 4);
            if (!sz || off + 4 + sz > len) break;
            char label[64];
            snprintf(label, sizeof label, "vsh#%d", idx++);
            check(label, (const uint32_t *)(d + off + 4), sz / 4);
            off += 4 + sz;
        }
        for (NSString *f in @[@"EffectCollection_ps_1_1.bin", @"EffectCollection_ps_1_4.bin", @"EffectCollection_ps_2_0.bin"]) {
            NSData *data = [NSData dataWithContentsOfFile:[dir stringByAppendingPathComponent:f]];
            const uint8_t *b = data.bytes;
            size_t o = 0;
            uint32_t ne, ln, ns, sz;
            memcpy(&ne, b + o, 4); o += 4;
            for (uint32_t e = 0; e < ne; e++) {
                memcpy(&ln, b + o, 4); o += 4;
                char ename[128];
                snprintf(ename, sizeof ename, "%.*s", (int)ln, (const char *)b + o);
                o += ln;
                memcpy(&ns, b + o, 4); o += 4;
                for (uint32_t s = 0; s < ns; s++) {
                    memcpy(&ln, b + o, 4); o += 4 + ln;
                    memcpy(&sz, b + o, 4); o += 4;
                    char label[256];
                    snprintf(label, sizeof label, "%s:%s#%u", f.UTF8String, ename, s);
                    check(label, (const uint32_t *)(b + o), sz);
                    o += 4 * sz;
                }
            }
        }
        printf("%d programs: %d translated, %d compiled with Metal\n", total, translated, compiled);
        return compiled == total ? 0 : 1;
    }
}
