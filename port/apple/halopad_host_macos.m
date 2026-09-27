/* HaloPad Apple host, macOS: AppKit windows with a Metal layer, and events from AppKit to the
 * Windows side (halopad_input.h). */
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#import "halopad_host.h"
#include "../runtime/halopad_input.h"

void halopad_host_app(void)
{
    static int done;
    if (done) return;
    done = 1;
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp finishLaunching];
}

/* The Mac's main display, in pixels: the Windows desktop HaloPad presents. */
void halopad_host_screen_size(int32_t *w, int32_t *h)
{
    CGDirectDisplayID d = CGMainDisplayID();
    *w = (int32_t)CGDisplayPixelsWide(d);
    *h = (int32_t)CGDisplayPixelsHigh(d);
}

/* ---- host input: AppKit events to the Windows side (halopad_input.h) ---- */

int halopad_host_input_off;                        /* tests drive input themselves */
static NSWindow *input_windows[8];

@interface HPWindowDelegate : NSObject <NSWindowDelegate>
@end
@implementation HPWindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender
{
    (void)sender;
    hp_input e = {.kind = HPI_CLOSE};              /* Windows asks the window (WM_SYSCOMMAND SC_CLOSE) */
    halopad_input_event(&e);
    return NO;
}
@end
static HPWindowDelegate *window_delegate;

static int is_input_window(NSWindow *w)
{
    for (int i = 0; i < 8; i++) if (w && input_windows[i] == w) return 1;
    return 0;
}

static void key_event(NSEvent *e, int down)
{
    hp_input in = {.kind = HPI_KEY, .down = down};
    if (!halopad_mac_key(e.keyCode, &in.vk, &in.side_vk, &in.scan, &in.extended)) return;
    if (down && !(e.modifierFlags & NSEventModifierFlagCommand)) {
        NSString *s = e.characters;
        for (NSUInteger i = 0; i < s.length && in.nchars < 4; i++) {
            unichar c = [s characterAtIndex:i];
            if (c >= 0xF700 && c <= 0xF8FF) continue;       /* function keys: no character */
            if (c == 0x7F) c = 0x08;                        /* Delete is Backspace */
            else if (c == 0x03) c = 0x0D;                   /* keypad Enter */
            else if (c == 0x19) c = 0x09;                   /* Shift-Tab */
            in.chars[in.nchars++] = c;
        }
    }
    halopad_input_event(&in);
}

static void modifier_event(NSEvent *e)
{
    static const struct { uint16_t code; NSUInteger bit; } m[] = {
        {0x38, 0x2}, {0x3C, 0x4}, {0x3B, 0x1}, {0x3E, 0x2000}, {0x3A, 0x20}, {0x3D, 0x40}, {0x37, 0x8}, {0x36, 0x10}};
    if (e.keyCode == 0x39) { key_event(e, 1); key_event(e, 0); return; }   /* Caps Lock: one press per change */
    for (size_t i = 0; i < sizeof m / sizeof m[0]; i++)
        if (m[i].code == e.keyCode) { key_event(e, (e.modifierFlags & m[i].bit) != 0); return; }
}

static int mouse_event(NSEvent *e, hp_input *in)
{
    if (!is_input_window(e.window)) return 0;
    NSView *v = e.window.contentView;
    NSPoint p = [v convertPoint:e.locationInWindow fromView:nil];
    CGFloat s = e.window.backingScaleFactor;
    in->x = (int32_t)floor(p.x * s);
    in->y = (int32_t)floor((v.bounds.size.height - p.y) * s);
    return 1;
}

/* Process pending host events without blocking (called from the message loop). */
void halopad_host_pump(void)
{
    halopad_host_app();
    @autoreleasepool {
        static int was_active = -1;
        int act = NSApp.isActive;
        if (!halopad_host_input_off && was_active >= 0 && act != was_active) {
            hp_input e = {.kind = HPI_ACTIVATE, .down = act};
            halopad_input_event(&e);
        }
        was_active = act;
        NSEvent *e;
        while ((e = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES])) {
            if (!halopad_host_input_off) {
                hp_input in = {0};
                switch (e.type) {
                case NSEventTypeKeyDown: key_event(e, 1); continue;
                case NSEventTypeKeyUp: key_event(e, 0); continue;
                case NSEventTypeFlagsChanged: modifier_event(e); continue;
                case NSEventTypeMouseMoved: case NSEventTypeLeftMouseDragged: case NSEventTypeRightMouseDragged:
                case NSEventTypeOtherMouseDragged:
                    if (mouse_event(e, &in)) {
                        static double rx, ry;                /* relative counts for DirectInput, fractions kept */
                        rx += e.deltaX; ry += e.deltaY;
                        in.dx = (int32_t)rx; in.dy = (int32_t)ry;
                        rx -= in.dx; ry -= in.dy;
                        in.kind = HPI_MOUSEMOVE;
                        halopad_input_event(&in);
                    }
                    break;
                case NSEventTypeLeftMouseDown: case NSEventTypeLeftMouseUp: case NSEventTypeRightMouseDown:
                case NSEventTypeRightMouseUp: case NSEventTypeOtherMouseDown: case NSEventTypeOtherMouseUp:
                    if (mouse_event(e, &in) && e.buttonNumber <= 2) {
                        in.kind = HPI_BUTTON;
                        in.button = (int)e.buttonNumber;
                        in.down = e.type == NSEventTypeLeftMouseDown || e.type == NSEventTypeRightMouseDown
                               || e.type == NSEventTypeOtherMouseDown;
                        halopad_input_event(&in);
                    }
                    break;
                case NSEventTypeScrollWheel:
                    if (mouse_event(e, &in)) {
                        static double rest;                  /* WHEEL_DELTA (120) per line, fractions kept */
                        rest += (e.hasPreciseScrollingDeltas ? e.scrollingDeltaY / 10.0 : e.scrollingDeltaY) * 120.0;
                        in.wheel = (int32_t)rest;
                        rest -= in.wheel;
                        if (in.wheel) { in.kind = HPI_WHEEL; halopad_input_event(&in); }
                    }
                    break;
                default: break;
                }
            }
            [NSApp sendEvent:e];
        }
    }
}

/* DirectInput's exclusive mouse: the pointer is hidden and detached, so only relative
   movement reaches the game, as on Windows. */
void halopad_host_mouse_capture(int on)
{
    static int captured;
    if (on == captured) return;
    captured = on;
    if (halopad_host_input_off) return;
    CGAssociateMouseAndMouseCursorPosition(on ? false : true);
    if (on) [NSCursor hide]; else [NSCursor unhide];
}

/* Windows' cursor display (ShowCursor count and SetCursor) over the game. */
void halopad_host_cursor(int visible)
{
    static int shown = 1;
    if (visible == shown) return;
    shown = visible;
    if (visible) [NSCursor unhide]; else [NSCursor hide];
}

void *halopad_host_window_create(uint32_t width, uint32_t height, const char *title, int visible)
{
    id<MTLDevice> gpu = halopad_metal_gpu();
    hp_window *w = calloc(1, sizeof *w);
    w->refs = 1;
    for (int c = 0; c < 3; c++) for (int i = 0; i < 256; i++) w->lut[256 * c + i] = i / 255.0f;
    @autoreleasepool {
        CGFloat scale = NSScreen.mainScreen.backingScaleFactor;
        NSRect frame = NSMakeRect(80, 80, width / scale, height / scale);
        w->window = [[NSWindow alloc] initWithContentRect:frame
                                                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
                                                  backing:NSBackingStoreBuffered defer:NO];
        w->window.releasedWhenClosed = NO;
        w->window.title = [NSString stringWithUTF8String:title ? title : "HaloPad"];
        NSView *view = w->window.contentView;
        view.wantsLayer = YES;
        w->layer = [CAMetalLayer layer];
        w->layer.device = gpu;
        w->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        w->layer.framebufferOnly = YES;
        w->layer.contentsScale = scale;
        w->layer.drawableSize = CGSizeMake(width, height);
        view.layer = w->layer;
        if (!window_delegate) window_delegate = [HPWindowDelegate new];
        w->window.delegate = window_delegate;
        w->window.acceptsMouseMovedEvents = YES;
        for (int i = 0; i < 8; i++) if (!input_windows[i]) { input_windows[i] = w->window; break; }
        w->on_screen = 1;
        if (visible) [w->window makeKeyAndOrderFront:nil];
    }
    w->w = width; w->h = height;
    return w;
}

void halopad_host_window_destroy(void *p)
{
    hp_window *w = p;
    for (int i = 0; i < 8; i++) if (input_windows[i] == w->window) input_windows[i] = nil;
    @autoreleasepool { w->window.delegate = nil; [w->window close]; }
    halopad_host_window_unref(w);
}

void halopad_host_window_show(void *p, int visible)
{
    hp_window *w = p;
    @autoreleasepool { if (visible) [w->window makeKeyAndOrderFront:nil]; else [w->window orderOut:nil]; }
}

void halopad_host_window_title(void *p, const char *title)
{
    hp_window *w = p;
    @autoreleasepool { w->window.title = [NSString stringWithUTF8String:title ? title : ""]; }
}

/* The client area in pixels (GDI's surface follows it). */
void halopad_host_window_resize(void *p, uint32_t width, uint32_t height)
{
    hp_window *w = p;
    if (!width || !height || (width == w->w && height == w->h)) return;
    @autoreleasepool {
        CGFloat scale = w->window.backingScaleFactor;
        [w->window setContentSize:NSMakeSize(width / scale, height / scale)];
    }
    w->w = width; w->h = height;
    free(w->gdi);
    w->gdi = NULL;
    w->gdi_tex = nil;
}

/* The pasteboard's text in Windows-1252 (for CF_TEXT); its length, 0 if none. */
int halopad_host_clipboard_text(char *out, size_t size)
{
    @autoreleasepool {
        NSString *s = [NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeString];
        if (!s.length) return 0;
        NSData *d = [s dataUsingEncoding:NSWindowsCP1252StringEncoding allowLossyConversion:YES];
        size_t n = d.length < size - 1 ? d.length : size - 1;
        memcpy(out, d.bytes, n);
        out[n] = 0;
        return (int)n;
    }
}
#endif
