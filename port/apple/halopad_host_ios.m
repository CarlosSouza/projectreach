/* HaloPad Apple host, iOS and iPadOS: windows as Metal layers, for an app shell to show.
 *
 * A Windows top-level window gets a CAMetalLayer. Until an app shell attaches it to a view
 * (halopad_host_attach_view), the layer is off screen: Direct3D still renders into its back
 * buffer (readable by tests), and Present shows nothing, as for a window no one can see.
 * Input comes from the shell's UIKit callbacks through halopad_input_event; this file has no
 * event source of its own: the shell queues events (halopad_host_post_input) and
 * halopad_host_pump delivers them on the thread that pumps, which is Halo's. */
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import "halopad_host.h"
#include "../runtime/halopad_input.h"

int halopad_host_input_off;                        /* tests drive input themselves */
static int pointer_captured, cursor_visible = 1;

void halopad_host_app(void) {}                     /* UIApplication belongs to the app shell */

/* The device's main screen in pixels, landscape (Halo's desktop). */
void halopad_host_screen_size(int32_t *w, int32_t *h)
{
    CGRect b = UIScreen.mainScreen.nativeBounds;
    int32_t a = (int32_t)b.size.width, c = (int32_t)b.size.height;
    *w = a > c ? a : c;
    *h = a > c ? c : a;
}

/* Events from the shell (UIKit's main thread) wait here until Halo's thread pumps. */
#include <pthread.h>
#define QSIZE 1024u
static hp_input queue_ev[QSIZE];
static uint32_t q_head, q_tail;
static pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;

void halopad_host_post_input(const hp_input *e)
{
    pthread_mutex_lock(&q_lock);
    if (q_tail - q_head < QSIZE) queue_ev[q_tail++ % QSIZE] = *e;   /* a full queue drops, as a stalled Windows queue would */
    pthread_mutex_unlock(&q_lock);
}

void halopad_host_pump(void)
{
    /* the thread that pumps is Halo's: the app's game thread, or the main thread of a test binary */
    if ([NSThread isMainThread]) [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
    for (;;) {
        hp_input e;
        pthread_mutex_lock(&q_lock);
        int have = q_head != q_tail;
        if (have) e = queue_ev[q_head++ % QSIZE];
        pthread_mutex_unlock(&q_lock);
        if (!have) return;
        if (!halopad_host_input_off) halopad_input_event(&e);
    }
}

/* For the shell's pointer mapping: a window's client size in pixels. */
void halopad_host_window_size(void *p, uint32_t *w, uint32_t *h) { hp_window *win = p; *w = win->w; *h = win->h; }

/* DirectInput's exclusive mouse and Windows' cursor display: recorded for the shell, which
   applies them with UIKit's pointer lock and pointer hiding. */
void halopad_host_mouse_capture(int on) { pointer_captured = on; }
void halopad_host_cursor(int visible) { cursor_visible = visible; }
int halopad_host_pointer_captured(void) { return pointer_captured; }
int halopad_host_cursor_visible(void) { return cursor_visible; }

/* The app shell's handler for new windows (it attaches their layers to its view). */
static void (*window_handler)(void *window);
void halopad_host_set_window_handler(void (*handler)(void *window)) { window_handler = handler; }

void *halopad_host_window_create(uint32_t width, uint32_t height, const char *title, int visible)
{
    (void)title;
    id<MTLDevice> gpu = halopad_metal_gpu();
    hp_window *w = calloc(1, sizeof *w);
    w->refs = 1;
    for (int c = 0; c < 3; c++) for (int i = 0; i < 256; i++) w->lut[256 * c + i] = i / 255.0f;
    @autoreleasepool {
        w->layer = [CAMetalLayer layer];
        w->layer.device = gpu;
        w->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        w->layer.framebufferOnly = YES;
        w->layer.drawableSize = CGSizeMake(width, height);
        w->layer.contentsGravity = kCAGravityResizeAspect;   /* letterboxed in the shell's view */
        w->layer.hidden = !visible;
        [CATransaction flush];                           /* Halo's thread has no run loop to commit it */
    }
    w->w = width; w->h = height;
    if (window_handler) {
        w->refs++;                                       /* held until the shell has seen it */
        void (*h)(void *) = window_handler;
        dispatch_async(dispatch_get_main_queue(), ^{ h(w); halopad_host_window_unref(w); });
    }
    return w;
}

/* For the app shell: show a window's layer in a view (filling it), or detach it (nil). */
void halopad_host_attach_view(void *p, UIView *view)
{
    hp_window *w = p;
    @autoreleasepool {
        [w->layer removeFromSuperlayer];
        w->window = view;
        w->on_screen = view != nil;
        if (view) {
            w->layer.frame = view.layer.bounds;
            w->layer.contentsScale = view.contentScaleFactor;
            [view.layer addSublayer:w->layer];
        }
    }
}

void halopad_host_window_destroy(void *p)
{
    hp_window *w = p;
    @autoreleasepool { [w->layer removeFromSuperlayer]; }
    w->window = nil;
    w->on_screen = 0;
    halopad_host_window_unref(w);
}

void halopad_host_window_show(void *p, int visible)
{
    hp_window *w = p;
    @autoreleasepool { w->layer.hidden = !visible; [CATransaction flush]; }
}

void halopad_host_window_title(void *p, const char *title) { (void)p; (void)title; }   /* iOS windows have no title bar */

/* The client area in pixels (GDI's surface follows it). */
void halopad_host_window_resize(void *p, uint32_t width, uint32_t height)
{
    hp_window *w = p;
    if (!width || !height || (width == w->w && height == w->h)) return;
    w->w = width; w->h = height;
    free(w->gdi);
    w->gdi = NULL;
    w->gdi_tex = nil;
}

/* The pasteboard's text in Windows-1252 (for CF_TEXT); its length, 0 if none. */
int halopad_host_clipboard_text(char *out, size_t size)
{
    @autoreleasepool {
        NSString *s = UIPasteboard.generalPasteboard.string;
        if (!s.length) return 0;
        NSData *d = [s dataUsingEncoding:NSWindowsCP1252StringEncoding allowLossyConversion:YES];
        size_t n = d.length < size - 1 ? d.length : size - 1;
        memcpy(out, d.bytes, n);
        out[n] = 0;
        return (int)n;
    }
}
#endif
