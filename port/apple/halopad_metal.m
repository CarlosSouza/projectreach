/* HaloPad Apple host, part 1 (G3/G4): host windows with a Metal layer, event pumping, and
 * the Metal back buffer behind IDirect3DDevice9 (clear, present, readback for tests).
 *
 * Each frame records into one command buffer; draws share a render encoder, which clears
 * end, and Present commits and waits.
 * Direct3D's back buffer is an offscreen BGRA8 texture of the size Halo asks for; the
 * depth/stencil buffer is Depth32Float_Stencil8. Present copies the back buffer into the
 * layer's drawable. Gamma ramps are not applied yet (the device refuses non-identity
 * ramps until they are). */
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdint.h>
#include "halopad_metal.h"

typedef struct {
    NSWindow *window;
    CAMetalLayer *layer;
    id<MTLTexture> back, depth;
    uint32_t width, height;
    id<MTLTexture> color, zs;                /* current attachments (Direct3D's render target and depth surface) */
    uint32_t level;                          /* mip level of color */
    id<MTLCommandBuffer> cb;                 /* the frame being recorded */
    id<MTLRenderCommandEncoder> enc;
} hp_target;

static id<MTLDevice> gpu;
static id<MTLCommandQueue> queue;
static void end_encoder(hp_target *t);
static id<MTLCommandBuffer> frame(hp_target *t);

static void ensure_app(void)
{
    static int done;
    if (done) return;
    done = 1;
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp finishLaunching];
    gpu = MTLCreateSystemDefaultDevice();
    queue = [gpu newCommandQueue];
    if (!gpu || !queue) { fprintf(stderr, "HALOPAD TRAP: no Metal device\n"); abort(); }
}

/* Process pending host events without blocking (called from the message loop). */
void halopad_host_pump(void)
{
    ensure_app();
    @autoreleasepool {
        NSEvent *e;
        while ((e = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES]))
            [NSApp sendEvent:e];
    }
}

/* A host window whose content is a Metal layer of width x height pixels. */
void *halopad_metal_target_create(uint32_t width, uint32_t height, int depth_stencil, const char *title)
{
    ensure_app();
    hp_target *t = calloc(1, sizeof *t);
    @autoreleasepool {
        CGFloat scale = NSScreen.mainScreen.backingScaleFactor;
        NSRect frame = NSMakeRect(80, 80, width / scale, height / scale);
        t->window = [[NSWindow alloc] initWithContentRect:frame
                                                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
                                                  backing:NSBackingStoreBuffered defer:NO];
        t->window.releasedWhenClosed = NO;
        t->window.title = [NSString stringWithUTF8String:title ? title : "HaloPad"];
        NSView *view = t->window.contentView;
        view.wantsLayer = YES;
        t->layer = [CAMetalLayer layer];
        t->layer.device = gpu;
        t->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        t->layer.framebufferOnly = NO;
        t->layer.contentsScale = scale;
        t->layer.drawableSize = CGSizeMake(width, height);
        view.layer = t->layer;
        [t->window makeKeyAndOrderFront:nil];
        MTLTextureDescriptor *d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                     width:width height:height mipmapped:NO];
        d.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        d.storageMode = MTLStorageModeShared;
        t->back = [gpu newTextureWithDescriptor:d];
        if (depth_stencil) {
            MTLTextureDescriptor *z = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                                                                         width:width height:height mipmapped:NO];
            z.usage = MTLTextureUsageRenderTarget;
            z.storageMode = MTLStorageModePrivate;
            t->depth = [gpu newTextureWithDescriptor:z];
        }
    }
    t->width = width;
    t->height = height;
    t->color = t->back;
    t->zs = t->depth;
    return t;
}

void halopad_metal_target_destroy(void *p)
{
    hp_target *t = p;
    @autoreleasepool { [t->window close]; }
    t->window = nil; t->layer = nil; t->back = nil; t->depth = nil; t->color = nil; t->zs = nil;
    free(t);
}

/* Clear the whole colour and/or depth/stencil attachment (Direct3D's D3DCLEAR_TARGET/ZBUFFER/STENCIL). */
void halopad_metal_clear(void *p, int color, int depth, int stencil, const float rgba[4], float z, uint32_t s)
{
    hp_target *t = p;
    @autoreleasepool {
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = t->color;
        rp.colorAttachments[0].level = t->level;
        rp.colorAttachments[0].loadAction = color ? MTLLoadActionClear : MTLLoadActionLoad;
        rp.colorAttachments[0].clearColor = MTLClearColorMake(rgba[0], rgba[1], rgba[2], rgba[3]);
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        if (t->zs) {
            rp.depthAttachment.texture = t->zs;
            rp.depthAttachment.loadAction = depth ? MTLLoadActionClear : MTLLoadActionLoad;
            rp.depthAttachment.clearDepth = z;
            rp.depthAttachment.storeAction = MTLStoreActionStore;
            rp.stencilAttachment.texture = t->zs;
            rp.stencilAttachment.loadAction = stencil ? MTLLoadActionClear : MTLLoadActionLoad;
            rp.stencilAttachment.clearStencil = s & 0xFF;
            rp.stencilAttachment.storeAction = MTLStoreActionStore;
        }
        end_encoder(t);
        [[frame(t) renderCommandEncoderWithDescriptor:rp] endEncoding];
    }
}

/* Copy the back buffer to the window and show it. */
int halopad_metal_present(void *p)
{
    hp_target *t = p;
    @autoreleasepool {
        end_encoder(t);
        id<CAMetalDrawable> drawable = [t->layer nextDrawable];
        if (!drawable) return 0;
        id<MTLCommandBuffer> cb = frame(t);
        t->cb = nil;
        id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
        [blit copyFromTexture:t->back sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
                   sourceSize:MTLSizeMake(t->width, t->height, 1) toTexture:drawable.texture destinationSlice:0
             destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
        [blit endEncoding];
        [cb presentDrawable:drawable];
        [cb commit];
        [cb waitUntilCompleted];
    }
    halopad_host_pump();
    return 1;
}

/* Test support: one back-buffer pixel as 0xAARRGGBB, after all submitted work. */
uint32_t halopad_metal_read_pixel(void *p, uint32_t x, uint32_t y)
{
    hp_target *t = p;
    @autoreleasepool {
        end_encoder(t);
        id<MTLCommandBuffer> cb = t->cb ? t->cb : [queue commandBuffer];
        t->cb = nil;
        [cb commit];
        [cb waitUntilCompleted];
        uint8_t bgra[4];
        [t->back getBytes:bgra bytesPerRow:4 fromRegion:MTLRegionMake2D(x, y, 1, 1) mipmapLevel:0];
        return (uint32_t)bgra[3] << 24 | (uint32_t)bgra[2] << 16 | (uint32_t)bgra[1] << 8 | bgra[0];
    }
}

/* ---- frame, pipelines, resources and draws ---- */

static NSMutableDictionary *pipelines, *depth_states, *samplers, *libraries;
static id<MTLBuffer> constant_attr;                 /* float4(0, 0, 0, 1) for missing vertex inputs */

static NSData *key_of(const void *p, size_t n, const char *a, const char *b)
{
    NSMutableData *k = [NSMutableData dataWithBytes:p length:n];
    if (a) [k appendBytes:a length:strlen(a)];
    if (b) [k appendBytes:b length:strlen(b)];
    return k;
}

static id<MTLFunction> function(const char *src, NSString *name, char *err, uint32_t errlen)
{
    if (!libraries) libraries = [NSMutableDictionary new];
    NSString *s = [NSString stringWithUTF8String:src];
    id<MTLLibrary> lib = libraries[s];
    if (!lib) {
        NSError *e = nil;
        lib = [gpu newLibraryWithSource:s options:nil error:&e];
        if (!lib) { snprintf(err, errlen, "%s", e.localizedDescription.UTF8String); return nil; }
        libraries[s] = lib;
    }
    return [lib newFunctionWithName:name];
}

void *halopad_metal_pipeline(const hp_pipeline_desc *d, char *err, uint32_t errlen)
{
    ensure_app();
    if (!pipelines) pipelines = [NSMutableDictionary new];
    hp_pipeline_desc copy = *d;
    copy.vs_msl = copy.ps_msl = NULL;
    NSData *key = key_of(&copy, sizeof copy, d->vs_msl, d->ps_msl);
    id<MTLRenderPipelineState> ps = pipelines[key];
    if (ps) return (__bridge void *)ps;
    @autoreleasepool {
        MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
        pd.vertexFunction = function(d->vs_msl, @"hp_vs", err, errlen);
        pd.fragmentFunction = function(d->ps_msl, @"hp_ps", err, errlen);
        if (!pd.vertexFunction || !pd.fragmentFunction) return NULL;
        MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
        for (uint32_t i = 0; i < d->nattr; i++) {
            vd.attributes[d->attr[i].reg].format = d->attr[i].format;
            vd.attributes[d->attr[i].reg].offset = d->attr[i].offset;
            vd.attributes[d->attr[i].reg].bufferIndex = d->attr[i].stream;
            vd.layouts[d->attr[i].stream].stride = d->stride[d->attr[i].stream];
        }
        for (int r = 0; r < 16; r++) {
            if (!(d->constant_regs >> r & 1)) continue;
            vd.attributes[r].format = MTLVertexFormatFloat4;
            vd.attributes[r].offset = 0;
            vd.attributes[r].bufferIndex = 18;
            vd.layouts[18].stride = 16;
            vd.layouts[18].stepFunction = MTLVertexStepFunctionConstant;
            vd.layouts[18].stepRate = 0;
        }
        pd.vertexDescriptor = vd;
        MTLRenderPipelineColorAttachmentDescriptor *ca = pd.colorAttachments[0];
        ca.pixelFormat = d->color_format ? d->color_format : MTLPixelFormatBGRA8Unorm;
        ca.blendingEnabled = d->blend;
        ca.sourceRGBBlendFactor = d->src_rgb; ca.destinationRGBBlendFactor = d->dst_rgb; ca.rgbBlendOperation = d->op_rgb;
        ca.sourceAlphaBlendFactor = d->src_a; ca.destinationAlphaBlendFactor = d->dst_a; ca.alphaBlendOperation = d->op_a;
        ca.writeMask = d->write_mask;
        if (d->depth) { pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8; pd.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8; }
        NSError *e = nil;
        ps = [gpu newRenderPipelineStateWithDescriptor:pd error:&e];
        if (!ps) { snprintf(err, errlen, "%s", e.localizedDescription.UTF8String); return NULL; }
    }
    pipelines[key] = ps;
    return (__bridge void *)ps;
}

void *halopad_metal_depth_state(const hp_depth_desc *d)
{
    ensure_app();
    if (!depth_states) depth_states = [NSMutableDictionary new];
    NSData *key = key_of(d, sizeof *d, NULL, NULL);
    id<MTLDepthStencilState> s = depth_states[key];
    if (s) return (__bridge void *)s;
    MTLDepthStencilDescriptor *dd = [MTLDepthStencilDescriptor new];
    dd.depthCompareFunction = d->depth_func;
    dd.depthWriteEnabled = d->depth_write;
    if (d->stencil) {
        MTLStencilDescriptor *sd[2] = {[MTLStencilDescriptor new], [MTLStencilDescriptor new]};
        for (int i = 0; i < 2; i++) {
            sd[i].stencilFailureOperation = d->fail[i]; sd[i].depthFailureOperation = d->zfail[i];
            sd[i].depthStencilPassOperation = d->pass[i]; sd[i].stencilCompareFunction = d->func[i];
            sd[i].readMask = d->read_mask & 0xFF; sd[i].writeMask = d->write_mask & 0xFF;
        }
        dd.frontFaceStencil = sd[0];
        dd.backFaceStencil = sd[1];
    }
    s = [gpu newDepthStencilStateWithDescriptor:dd];
    depth_states[key] = s;
    return (__bridge void *)s;
}

void *halopad_metal_sampler(const hp_sampler_desc *d)
{
    ensure_app();
    if (!samplers) samplers = [NSMutableDictionary new];
    NSData *key = key_of(d, sizeof *d, NULL, NULL);
    id<MTLSamplerState> s = samplers[key];
    if (s) return (__bridge void *)s;
    MTLSamplerDescriptor *sd = [MTLSamplerDescriptor new];
    sd.minFilter = d->min; sd.magFilter = d->mag; sd.mipFilter = d->mip;
    sd.sAddressMode = d->u; sd.tAddressMode = d->v; sd.rAddressMode = d->w;
    sd.borderColor = d->border;
    sd.maxAnisotropy = d->anisotropy ? d->anisotropy : 1;
    sd.lodMinClamp = d->lod_min;
    s = [gpu newSamplerStateWithDescriptor:sd];
    samplers[key] = s;
    return (__bridge void *)s;
}

void *halopad_metal_buffer(const void *data, uint32_t length)
{
    ensure_app();
    id<MTLBuffer> b = [gpu newBufferWithBytes:data length:length ? length : 4 options:MTLResourceStorageModeShared];
    return (__bridge_retained void *)b;
}

void halopad_metal_release(void *o) { if (o) CFRelease(o); }

void *halopad_metal_texture(int type, uint32_t format, uint32_t w, uint32_t h, uint32_t d, uint32_t levels, const uint8_t swizzle[4])
{
    ensure_app();
    MTLTextureDescriptor *td = [MTLTextureDescriptor new];
    td.textureType = type == 3 ? MTLTextureTypeCube : type == 4 ? MTLTextureType3D : MTLTextureType2D;
    td.pixelFormat = format;
    td.width = w; td.height = h; td.depth = type == 4 ? d : 1;
    td.mipmapLevelCount = levels;
    td.storageMode = MTLStorageModeShared;
    td.usage = MTLTextureUsageShaderRead;
    td.swizzle = MTLTextureSwizzleChannelsMake(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);
    id<MTLTexture> t = [gpu newTextureWithDescriptor:td];
    return (__bridge_retained void *)t;
}

void halopad_metal_texture_upload(void *tex, uint32_t level, uint32_t slice, const void *data, uint32_t bytes_per_row,
                                  uint32_t bytes_per_image, uint32_t w, uint32_t h, uint32_t d)
{
    id<MTLTexture> t = (__bridge id<MTLTexture>)tex;
    [t replaceRegion:MTLRegionMake3D(0, 0, 0, w, h, d) mipmapLevel:level slice:slice withBytes:data
         bytesPerRow:bytes_per_row bytesPerImage:bytes_per_image];
}

static void end_encoder(hp_target *t)
{
    if (t->enc) { [t->enc endEncoding]; t->enc = nil; }
}

static id<MTLCommandBuffer> frame(hp_target *t)
{
    if (!t->cb) t->cb = [queue commandBuffer];
    return t->cb;
}

void halopad_metal_draw(void *target, const hp_draw_desc *d)
{
    hp_target *t = target;
    if (!t->enc) {
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = t->color;
        rp.colorAttachments[0].level = t->level;
        rp.colorAttachments[0].loadAction = MTLLoadActionLoad;
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        if (t->zs) {
            rp.depthAttachment.texture = t->zs; rp.depthAttachment.loadAction = MTLLoadActionLoad;
            rp.depthAttachment.storeAction = MTLStoreActionStore;
            rp.stencilAttachment.texture = t->zs; rp.stencilAttachment.loadAction = MTLLoadActionLoad;
            rp.stencilAttachment.storeAction = MTLStoreActionStore;
        }
        t->enc = [frame(t) renderCommandEncoderWithDescriptor:rp];
    }
    id<MTLRenderCommandEncoder> e = t->enc;
    [e setRenderPipelineState:(__bridge id<MTLRenderPipelineState>)d->pipeline];
    if (d->depth_state) [e setDepthStencilState:(__bridge id<MTLDepthStencilState>)d->depth_state];
    [e setCullMode:d->cull];
    [e setFrontFacingWinding:d->front_cw ? MTLWindingClockwise : MTLWindingCounterClockwise];
    [e setTriangleFillMode:d->lines ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
    [e setDepthBias:0 slopeScale:d->slope_bias clamp:0];
    [e setStencilReferenceValue:d->stencil_ref];
    [e setBlendColorRed:d->blend_color[0] green:d->blend_color[1] blue:d->blend_color[2] alpha:d->blend_color[3]];
    [e setViewport:(MTLViewport){d->viewport[0], d->viewport[1], d->viewport[2], d->viewport[3], d->viewport[4], d->viewport[5]}];
    MTLScissorRect sc = d->scissor ? (MTLScissorRect){d->scissor_rect[0], d->scissor_rect[1], d->scissor_rect[2], d->scissor_rect[3]}
                                   : (MTLScissorRect){0, 0, MAX(t->color.width >> t->level, 1u), MAX(t->color.height >> t->level, 1u)};
    [e setScissorRect:sc];
    for (int s = 0; s < 16; s++) if (d->vbuf[s]) [e setVertexBuffer:(__bridge id<MTLBuffer>)d->vbuf[s] offset:d->voff[s] atIndex:s];
    if (!constant_attr) { float v[4] = {0, 0, 0, 1}; constant_attr = [gpu newBufferWithBytes:v length:16 options:MTLResourceStorageModeShared]; }
    [e setVertexBuffer:constant_attr offset:0 atIndex:18];
    /* constant blocks exceed setVertexBytes' 4 KB guideline: one buffer per draw */
    [e setVertexBuffer:[gpu newBufferWithBytes:d->vs_consts length:d->vs_len options:MTLResourceStorageModeShared] offset:0 atIndex:16];
    [e setFragmentBuffer:[gpu newBufferWithBytes:d->ps_consts length:d->ps_len options:MTLResourceStorageModeShared] offset:0 atIndex:0];
    for (int s = 0; s < 16; s++) {
        if (d->tex[s]) [e setFragmentTexture:(__bridge id<MTLTexture>)d->tex[s] atIndex:s];
        if (d->smp[s]) [e setFragmentSamplerState:(__bridge id<MTLSamplerState>)d->smp[s] atIndex:s];
    }
    if (d->ibuf)
        [e drawIndexedPrimitives:d->prim indexCount:d->count indexType:d->index32 ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                     indexBuffer:(__bridge id<MTLBuffer>)d->ibuf indexBufferOffset:d->index_offset instanceCount:1
                      baseVertex:d->base_vertex baseInstance:0];
    else
        [e drawPrimitives:d->prim vertexStart:d->start vertexCount:d->count];
}

/* ---- render targets ---- */

void *halopad_metal_target_back(void *p) { return (__bridge void *)((hp_target *)p)->back; }
void *halopad_metal_target_depth(void *p) { return (__bridge void *)((hp_target *)p)->depth; }

void halopad_metal_set_attachments(void *p, void *color, uint32_t level, void *depth)
{
    hp_target *t = p;
    id<MTLTexture> c = (__bridge id<MTLTexture>)color, z = (__bridge id<MTLTexture>)depth;
    if (c == t->color && level == t->level && z == t->zs) return;
    end_encoder(t);
    t->color = c; t->level = level; t->zs = z;
}

void *halopad_metal_render_texture(uint32_t format, uint32_t w, uint32_t h, uint32_t levels)
{
    ensure_app();
    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:w height:h mipmapped:NO];
    td.mipmapLevelCount = levels;
    td.storageMode = MTLStorageModeShared;
    td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    return (__bridge_retained void *)[gpu newTextureWithDescriptor:td];
}

void *halopad_metal_texture_view(void *tex, const uint8_t sw[4])
{
    id<MTLTexture> t = (__bridge id<MTLTexture>)tex;
    id<MTLTexture> v = [t newTextureViewWithPixelFormat:t.pixelFormat textureType:t.textureType levels:NSMakeRange(0, t.mipmapLevelCount)
                                                 slices:NSMakeRange(0, 1) swizzle:MTLTextureSwizzleChannelsMake(sw[0], sw[1], sw[2], sw[3])];
    return (__bridge_retained void *)v;
}

static const char stretch_msl[] =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct V { float4 p [[position]]; float2 uv; };\n"
    "struct P { float4 r; float lod; };\n"
    "vertex V hp_vs(uint i [[vertex_id]], constant P &c [[buffer(0)]]) {\n"
    "    float2 k = float2(i & 1, i >> 1);\n"
    "    V o; o.p = float4(k.x * 2 - 1, 1 - k.y * 2, 0, 1); o.uv = mix(c.r.xy, c.r.zw, k); return o;\n"
    "}\n"
    "fragment float4 hp_ps(V i [[stage_in]], constant P &c [[buffer(0)]], texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]]) {\n"
    "    return t.sample(s, i.uv, level(c.lod));\n"
    "}\n"
    "fragment float4 hp_ps_opaque(V i [[stage_in]], constant P &c [[buffer(0)]], texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]]) {\n"
    "    return float4(t.sample(s, i.uv, level(c.lod)).rgb, 1);\n"
    "}\n";

void halopad_metal_stretch(void *p, void *src, uint32_t slevel, const uint32_t sr[4], void *dst, uint32_t dlevel,
                           const uint32_t dr[4], int linear, int opaque)
{
    hp_target *t = p;
    id<MTLTexture> s = (__bridge id<MTLTexture>)src, d = (__bridge id<MTLTexture>)dst;
    static NSMutableDictionary *stretch_pipes;
    static id<MTLSamplerState> smp[2];
    @autoreleasepool {
        end_encoder(t);
        id<MTLCommandBuffer> cb = frame(t);
        if (!opaque && sr[2] == dr[2] && sr[3] == dr[3] && s.pixelFormat == d.pixelFormat) {
            id<MTLBlitCommandEncoder> b = [cb blitCommandEncoder];
            [b copyFromTexture:s sourceSlice:0 sourceLevel:slevel sourceOrigin:MTLOriginMake(sr[0], sr[1], 0)
                    sourceSize:MTLSizeMake(sr[2], sr[3], 1) toTexture:d destinationSlice:0 destinationLevel:dlevel
             destinationOrigin:MTLOriginMake(dr[0], dr[1], 0)];
            [b endEncoding];
            return;
        }
        if (!stretch_pipes) {
            stretch_pipes = [NSMutableDictionary new];
            for (int i = 0; i < 2; i++) {
                MTLSamplerDescriptor *sd = [MTLSamplerDescriptor new];
                sd.minFilter = sd.magFilter = i ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
                sd.sAddressMode = sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
                smp[i] = [gpu newSamplerStateWithDescriptor:sd];
            }
        }
        NSNumber *key = @(d.pixelFormat | (opaque ? 0x10000u : 0u));
        id<MTLRenderPipelineState> ps = stretch_pipes[key];
        if (!ps) {
            char err[512];
            MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
            pd.vertexFunction = function(stretch_msl, @"hp_vs", err, sizeof err);
            pd.fragmentFunction = function(stretch_msl, opaque ? @"hp_ps_opaque" : @"hp_ps", err, sizeof err);
            pd.colorAttachments[0].pixelFormat = d.pixelFormat;
            NSError *e = nil;
            ps = [gpu newRenderPipelineStateWithDescriptor:pd error:&e];
            if (!ps) { fprintf(stderr, "HALOPAD TRAP: stretch pipeline: %s\n", e.localizedDescription.UTF8String); abort(); }
            stretch_pipes[key] = ps;
        }
        float sw = (float)MAX(s.width >> slevel, 1u), sh = (float)MAX(s.height >> slevel, 1u);
        struct { float r[4]; float lod; float pad[3]; } c = {{sr[0] / sw, sr[1] / sh, (sr[0] + sr[2]) / sw, (sr[1] + sr[3]) / sh}, (float)slevel, {0}};
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = d;
        rp.colorAttachments[0].level = dlevel;
        rp.colorAttachments[0].loadAction = MTLLoadActionLoad;
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        id<MTLRenderCommandEncoder> e = [cb renderCommandEncoderWithDescriptor:rp];
        [e setRenderPipelineState:ps];
        [e setViewport:(MTLViewport){dr[0], dr[1], dr[2], dr[3], 0, 1}];
        [e setVertexBytes:&c length:sizeof c atIndex:0];
        [e setFragmentBytes:&c length:sizeof c atIndex:0];
        [e setFragmentTexture:s atIndex:0];
        [e setFragmentSamplerState:smp[linear ? 1 : 0] atIndex:0];
        [e drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [e endEncoding];
    }
}

void halopad_metal_read_texture(void *p, void *tex, uint32_t level, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                void *out, uint32_t bytes_per_row)
{
    hp_target *t = p;
    @autoreleasepool {
        end_encoder(t);
        id<MTLCommandBuffer> cb = t->cb ? t->cb : [queue commandBuffer];
        t->cb = nil;
        [cb commit];
        [cb waitUntilCompleted];
        [(__bridge id<MTLTexture>)tex getBytes:out bytesPerRow:bytes_per_row fromRegion:MTLRegionMake2D(x, y, w, h) mipmapLevel:level];
    }
}
