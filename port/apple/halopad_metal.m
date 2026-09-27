/* HaloPad Apple host, part 1 (G3/G4): host windows with a Metal layer, event pumping, and
 * the Metal back buffer behind IDirect3DDevice9 (clear, present, readback for tests).
 *
 * Direct3D's back buffer is an offscreen BGRA8 texture of the size Halo asks for; the
 * depth/stencil buffer is Depth32Float_Stencil8. Present copies the back buffer into the
 * layer's drawable. Gamma ramps are not applied yet (the device refuses non-identity
 * ramps until they are). */
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdint.h>

typedef struct {
    NSWindow *window;
    CAMetalLayer *layer;
    id<MTLTexture> back, depth;
    uint32_t width, height;
} hp_target;

static id<MTLDevice> gpu;
static id<MTLCommandQueue> queue;

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
    return t;
}

void halopad_metal_target_destroy(void *p)
{
    hp_target *t = p;
    @autoreleasepool { [t->window close]; }
    t->window = nil; t->layer = nil; t->back = nil; t->depth = nil;
    free(t);
}

/* Clear the whole back buffer and/or depth/stencil (Direct3D's D3DCLEAR_TARGET/ZBUFFER/STENCIL). */
void halopad_metal_clear(void *p, int color, int depth, int stencil, const float rgba[4], float z, uint32_t s)
{
    hp_target *t = p;
    @autoreleasepool {
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = t->back;
        rp.colorAttachments[0].loadAction = color ? MTLLoadActionClear : MTLLoadActionLoad;
        rp.colorAttachments[0].clearColor = MTLClearColorMake(rgba[0], rgba[1], rgba[2], rgba[3]);
        rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        if (t->depth) {
            rp.depthAttachment.texture = t->depth;
            rp.depthAttachment.loadAction = depth ? MTLLoadActionClear : MTLLoadActionLoad;
            rp.depthAttachment.clearDepth = z;
            rp.depthAttachment.storeAction = MTLStoreActionStore;
            rp.stencilAttachment.texture = t->depth;
            rp.stencilAttachment.loadAction = stencil ? MTLLoadActionClear : MTLLoadActionLoad;
            rp.stencilAttachment.clearStencil = s & 0xFF;
            rp.stencilAttachment.storeAction = MTLStoreActionStore;
        }
        id<MTLCommandBuffer> cb = [queue commandBuffer];
        [[cb renderCommandEncoderWithDescriptor:rp] endEncoding];
        [cb commit];
    }
}

/* Copy the back buffer to the window and show it. */
int halopad_metal_present(void *p)
{
    hp_target *t = p;
    @autoreleasepool {
        id<CAMetalDrawable> drawable = [t->layer nextDrawable];
        if (!drawable) return 0;
        id<MTLCommandBuffer> cb = [queue commandBuffer];
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
        id<MTLCommandBuffer> cb = [queue commandBuffer];
        [cb commit];
        [cb waitUntilCompleted];
        uint8_t bgra[4];
        [t->back getBytes:bgra bytesPerRow:4 fromRegion:MTLRegionMake2D(x, y, 1, 1) mipmapLevel:0];
        return (uint32_t)bgra[3] << 24 | (uint32_t)bgra[2] << 16 | (uint32_t)bgra[1] << 8 | bgra[0];
    }
}
