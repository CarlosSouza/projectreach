/* Shader differential-test runner (G4): executes translated Direct3D shaders on Metal.
 * For every <name>.job in a directory: translate in test-kernel mode, run one thread per
 * case, write <name>.out. Samplers are bilinear, clamp-to-edge, level 0.
 * Job: 'HPSJ', kind (0 vs, 1 ps), token count, tokens, case count, input float4s per case
 * (16 vs, 11 ps), the inputs, constant-block size and bytes, then for 16 stages:
 * dim (0 none, 2 2D, 3 cube, 4 volume), width, height, depth, RGBA8 texels (cube: 6 faces). */
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdint.h>

typedef struct { uint8_t sampler_dim[16]; uint8_t projected[16]; uint8_t test_kernel; } hp_shader_key;
char *halopad_shader_to_msl(const uint32_t *t, uint32_t n, const hp_shader_key *key, char *err, size_t errlen);

static const uint8_t *cur;
static uint32_t u32(void) { uint32_t v; memcpy(&v, cur, 4); cur += 4; return v; }

int main(int argc, char **argv)
{
    @autoreleasepool {
        if (argc < 2) return 2;
        id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
        id<MTLCommandQueue> q = [gpu newCommandQueue];
        MTLSamplerDescriptor *sd = [MTLSamplerDescriptor new];
        sd.minFilter = MTLSamplerMinMagFilterLinear; sd.magFilter = MTLSamplerMinMagFilterLinear;
        sd.sAddressMode = sd.tAddressMode = sd.rAddressMode = MTLSamplerAddressModeClampToEdge;
        id<MTLSamplerState> smp = [gpu newSamplerStateWithDescriptor:sd];
        NSString *dir = [NSString stringWithUTF8String:argv[1]];
        int failures = 0;
        for (NSString *f in [[NSFileManager defaultManager] contentsOfDirectoryAtPath:dir error:nil]) {
            if (![f hasSuffix:@".job"]) continue;
            NSData *job = [NSData dataWithContentsOfFile:[dir stringByAppendingPathComponent:f]];
            cur = job.bytes;
            if (u32() != 0x4A535048u) { printf("%s: bad job\n", f.UTF8String); failures++; continue; }
            uint32_t kind = u32(), ntok = u32();
            const uint32_t *tok = (const uint32_t *)cur; cur += 4 * ntok;
            uint32_t ncases = u32(), stride = u32();
            const float *ins = (const float *)cur; cur += 16 * ncases * stride;
            uint32_t cbytes = u32();
            const uint8_t *consts = cur; cur += cbytes;
            hp_shader_key key = {{0}, {0}, 1};
            id<MTLTexture> tex[16] = {nil};
            for (int s = 0; s < 16; s++) {
                uint32_t dim = u32(), w = u32(), h = u32(), d = u32();
                if (!dim) continue;
                key.sampler_dim[s] = (uint8_t)dim;
                MTLTextureDescriptor *td = [MTLTextureDescriptor new];
                td.pixelFormat = MTLPixelFormatRGBA8Unorm; td.width = w; td.height = h;
                td.textureType = dim == 3 ? MTLTextureTypeCube : dim == 4 ? MTLTextureType3D : MTLTextureType2D;
                if (dim == 4) td.depth = d;
                td.storageMode = MTLStorageModeShared;
                tex[s] = [gpu newTextureWithDescriptor:td];
                if (dim == 3) {
                    for (int face = 0; face < 6; face++) {
                        [tex[s] replaceRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:0 slice:face withBytes:cur bytesPerRow:4 * w bytesPerImage:4 * w * h];
                        cur += 4 * w * h;
                    }
                } else {
                    [tex[s] replaceRegion:MTLRegionMake3D(0, 0, 0, w, h, dim == 4 ? d : 1) mipmapLevel:0 slice:0 withBytes:cur bytesPerRow:4 * w bytesPerImage:4 * w * h];
                    cur += 4 * w * h * (dim == 4 ? d : 1);
                }
            }
            char err[512] = "";
            char *msl = halopad_shader_to_msl(tok, ntok, &key, err, sizeof err);
            NSString *outp = [[dir stringByAppendingPathComponent:[f stringByDeletingPathExtension]] stringByAppendingPathExtension:@"out"];
            if (!msl) { printf("%s: translate: %s\n", f.UTF8String, err); failures++; continue; }
            NSError *e = nil;
            id<MTLLibrary> lib = [gpu newLibraryWithSource:[NSString stringWithUTF8String:msl] options:nil error:&e];
            id<MTLFunction> fn = [lib newFunctionWithName:kind ? @"hp_ps_test" : @"hp_vs_test"];
            if (!fn) { printf("%s: compile: %s\n", f.UTF8String, e.localizedDescription.UTF8String); failures++; free(msl); continue; }
            id<MTLComputePipelineState> ps = [gpu newComputePipelineStateWithFunction:fn error:&e];
            uint32_t ostride = kind ? 2 : 12;
            id<MTLBuffer> bin = [gpu newBufferWithBytes:ins length:16 * ncases * stride options:MTLResourceStorageModeShared];
            id<MTLBuffer> bout = [gpu newBufferWithLength:16 * ncases * ostride options:MTLResourceStorageModeShared];
            /* The VS constant block is 4432 bytes: larger than setBytes' 4 KiB limit. */
            id<MTLBuffer> bconst = [gpu newBufferWithBytes:consts length:cbytes options:MTLResourceStorageModeShared];
            id<MTLCommandBuffer> cb = [q commandBuffer];
            id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
            [enc setComputePipelineState:ps];
            if (kind) {
                [enc setBuffer:bconst offset:0 atIndex:0];
                [enc setBuffer:bin offset:0 atIndex:1];
                [enc setBuffer:bout offset:0 atIndex:2];
                for (int s = 0; s < 16; s++) if (tex[s]) { [enc setTexture:tex[s] atIndex:s]; [enc setSamplerState:smp atIndex:s]; }
            } else {
                [enc setBuffer:bin offset:0 atIndex:0];
                [enc setBuffer:bout offset:0 atIndex:1];
                [enc setBuffer:bconst offset:0 atIndex:16];
            }
            [enc dispatchThreads:MTLSizeMake(ncases, 1, 1) threadsPerThreadgroup:MTLSizeMake(64, 1, 1)];
            [enc endEncoding];
            [cb commit];
            [cb waitUntilCompleted];
            [[NSData dataWithBytes:bout.contents length:16 * ncases * ostride] writeToFile:outp atomically:NO];
            free(msl);
        }
        return failures ? 1 : 0;
    }
}
