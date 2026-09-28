/* HaloPad for iPadOS (G3/G8): the app shell around the native core.
 *
 * The core (halopad_core_run: Halo from its PE entry point) runs on its own thread. Windows
 * Halo creates appear in the app's view: the shell attaches each window's Metal layer
 * (halopad_host_attach_view), letterboxed. When Halo's first-run license check runs, the shell
 * shows the game's own Eula.rtf with Accept and Decline and returns the player's choice
 * (halopad_host_license_prompt); nothing is chosen for the player. A hardware keyboard, touch
 * (as the left mouse button), an iPad pointer (buttons, hover, scrolling) and scene activation
 * are queued for Halo's thread (halopad_host_post_input). Halo's warning and error dialogs
 * appear as a sheet laid out from their templates (halopad_host_dialog), and its "more
 * information" links open in Safari (halopad_host_open_url).
 *
 * Development builds on the Simulator get their data paths from the environment
 * (scripts/build-ios-app.py passes HALOPAD_* through simctl launch). */
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#include <stdio.h>
#include <math.h>
#include <stdatomic.h>
#include <sys/utsname.h>

#include "../runtime/halopad_input.h"
#import "HaloPadOverlay.h"

int halopad_core_run(void);
/* what the core thread runs: Halo from its entry point, unless a development scene replaces it
   (scripts/build-ios-app.py --scene) */
__attribute__((weak, noinline)) int halopad_app_entry(void) { return halopad_core_run(); }
void halopad_host_set_window_handler(void (*handler)(void *window));
void halopad_host_attach_view(void *window, UIView *view);
void halopad_host_window_size(void *window, uint32_t *w, uint32_t *h);
extern uint64_t halopad_guest_base;
void *halopad_guest_ptr(uint32_t guest);
extern void (*halopad_d3d9_present_hook)(uint32_t device);

/* frames Halo presents (the FPS counter) */
static atomic_int presented;
void *halopad_d3d9_device_target(uint32_t g);
void halopad_metal_read_image(void *p, uint32_t *out, uint32_t w, uint32_t h);
static void count_present(uint32_t device)
{
    int n = atomic_fetch_add(&presented, 1) + 1;
    /* development: the back buffer of frame 600 as a PPM beside the registry (HALOPAD_TRACE_WINDOWS) */
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (n == 600 && getenv("HALOPAD_TRACE_WINDOWS") && reg && strrchr(reg, '/')) {
        uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
        halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
        char path[1200];
        snprintf(path, sizeof path, "%.*s/frame600.ppm", (int)(strrchr(reg, '/') - reg), reg);
        FILE *f = fopen(path, "wb");
        if (f) {
            fprintf(f, "P6\n%u %u\n255\n", w, h);
            for (uint32_t i = 0; i < w * h; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
            fclose(f);
            fprintf(stderr, "HALOPAD APP: frame 600 saved to %s\n", path);
        }
        free(img);
    }
}

/* Halo's current map (0x643064 in haloce.exe 1.10): "ui" is its menus; empty before it starts */
static NSString *current_map(void)
{
    if (!halopad_guest_base) return @"";
    char m[32];
    memcpy(m, halopad_guest_ptr(0x643064), sizeof m - 1);
    m[sizeof m - 1] = 0;
    return [NSString stringWithCString:m encoding:NSASCIIStringEncoding] ?: @"";
}

/* The system keyboard for Halo's text entry: what is typed becomes key presses with characters. */
@interface HPKeyboardProxy : UIView <UIKeyInput>
@end
@implementation HPKeyboardProxy
- (BOOL)canBecomeFirstResponder { return YES; }
- (BOOL)hasText { return YES; }
- (void)insertText:(NSString *)text { [HPOverlay typeText:text]; }
- (void)deleteBackward { [HPOverlay typeText:@"\b"]; }
- (UITextAutocorrectionType)autocorrectionType { return UITextAutocorrectionTypeNo; }
- (UITextAutocapitalizationType)autocapitalizationType { return UITextAutocapitalizationTypeNone; }
- (UITextSpellCheckingType)spellCheckingType { return UITextSpellCheckingTypeNo; }
- (UIReturnKeyType)returnKeyType { return UIReturnKeySend; }
@end

@interface HPGameViewController : UIViewController <HPOverlayDelegate>
@end

static HPGameViewController *game_vc;
static UIView *game_view;
static HPOverlay *overlay;
static HPKeyboardProxy *keyboard;

static void *input_window;                          /* the window touches and the pointer act on */

@interface HPGameViewController ()
- (void)applyDisplay;
@end

static void on_window(void *w)
{
    if (!game_view) return;
    halopad_host_attach_view(w, game_view);
    input_window = w;
    if (getenv("HALOPAD_TRACE_WINDOWS")) {
        uint32_t cw, ch;
        halopad_host_window_size(w, &cw, &ch);
        fprintf(stderr, "HALOPAD APP: window %p attached, %ux%u\n", w, cw, ch);
    }
    dispatch_async(dispatch_get_main_queue(), ^{ [game_vc applyDisplay]; });
}

@implementation HPGameViewController
- (void)loadView
{
    UIView *v = [[UIView alloc] initWithFrame:UIScreen.mainScreen.bounds];
    v.backgroundColor = UIColor.blackColor;
    v.multipleTouchEnabled = NO;
    [v addGestureRecognizer:[[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(hover:)]];
    UIPanGestureRecognizer *scroll = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(scroll:)];
    scroll.allowedScrollTypesMask = UIScrollTypeMaskAll;
    scroll.allowedTouchTypes = @[];                     /* trackpad and mouse-wheel scrolling only */
    [v addGestureRecognizer:scroll];
    self.view = v;
    game_view = v;
    keyboard = [[HPKeyboardProxy alloc] initWithFrame:CGRectZero];
    [v addSubview:keyboard];
    overlay = [[HPOverlay alloc] initWithFrame:v.bounds];
    overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    overlay.delegate = self;
    overlay.layer.zPosition = 100;                    /* above the Metal layers Halo's windows attach later */
    overlay.opaque = NO;
    overlay.backgroundColor = UIColor.clearColor;
    keyboard.opaque = NO;
    overlay.hidden = getenv("HALOPAD_NO_OVERLAY") != NULL;   /* development: the game view alone */
    [v addSubview:overlay];
    /* in a game or in Halo's menus, and the frame rate: polled from Halo's state */
    [NSTimer scheduledTimerWithTimeInterval:0.25 repeats:YES block:^(NSTimer *t) {
        NSString *m = current_map();
        overlay.inGame = m.length && ![m isEqualToString:@"ui"];
        static int ticks, last;
        if (++ticks % 4 == 0) {
            int n = atomic_load(&presented);
            [overlay setFramesPerSecond:n - last];
            if (getenv("HALOPAD_TRACE_WINDOWS") && ticks % 20 == 0) fprintf(stderr, "HALOPAD APP: %d frames/s, map \"%s\"\n", n - last, m.UTF8String);
            last = n;
        }
    }];
}
- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self becomeFirstResponder];
    static int started;
    if (started) return;
    started = 1;
    [self applyDisplay];
    halopad_host_set_window_handler(on_window);
    if (!halopad_d3d9_present_hook) halopad_d3d9_present_hook = count_present;
    NSThread *t = [[NSThread alloc] initWithBlock:^{
        int code = halopad_app_entry();
        fprintf(stderr, "HALOPAD: Halo returned %d\n", code);
        exit(code);
    }];
    t.stackSize = 16u << 20;                          /* callbacks nest host frames; room as a desktop main thread has */
    t.name = @"Halo";
    [t start];
}
- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    for (CALayer *l in self.view.layer.sublayers) l.frame = self.view.layer.bounds;
    [self.view bringSubviewToFront:overlay];
}
/* ---- the overlay's requests ---- */
- (void)applyDisplay
{
    NSString *g = HPSettings.shared.aspect == HPAspectFill ? kCAGravityResize : kCAGravityResizeAspect;
    for (CALayer *l in self.view.layer.sublayers) if (l != overlay.layer && l != keyboard.layer) l.contentsGravity = g;
    if (getenv("HALOPAD_TRACE_WINDOWS"))
        for (CALayer *l in self.view.layer.sublayers)
            fprintf(stderr, "HALOPAD APP:   layer %s %s hidden %d opaque %d frame %.0fx%.0f z %.0f\n", l.class.description.UTF8String,
                    l == overlay.layer ? "(overlay)" : "", l.hidden, l.opaque, l.frame.size.width, l.frame.size.height, l.zPosition);
}
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
- (void)overlayDisplayChanged:(HPOverlay *)o { [self applyDisplay]; }
- (void)overlayRequestsKeyboard:(HPOverlay *)o { [keyboard becomeFirstResponder]; }
- (NSString *)overlayDiagnostics:(HPOverlay *)o
{
    struct utsname u;
    uname(&u);
    NSString *ver = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"?";
    return [NSString stringWithFormat:@"HaloPad %@\nDevice %s, %@ %@\nHalo's map: %@\nDisplay: %@, touch controls %@",
            ver, u.machine, UIDevice.currentDevice.systemName, UIDevice.currentDevice.systemVersion, current_map(),
            HPSettings.shared.aspect == HPAspectFill ? @"stretch to fill" : @"original 4:3", HPSettings.shared.hideTouchControls ? @"hidden" : @"shown"];
}
/* ---- input: hardware keyboard, touch and pointer, queued for Halo's thread ---- */
- (BOOL)canBecomeFirstResponder { return YES; }

- (void)keys:(NSSet<UIPress *> *)presses down:(int)down
{
    for (UIPress *p in presses) {
        UIKey *k = p.key;
        if (!k) continue;
        hp_input in = {.kind = HPI_KEY, .down = down};
        if (!halopad_hid_key((uint16_t)k.keyCode, &in.vk, &in.side_vk, &in.scan, &in.extended)) continue;
        if (down && !(k.modifierFlags & UIKeyModifierCommand)) {
            NSString *c = k.characters;
            for (NSUInteger i = 0; i < c.length && in.nchars < 4; i++) {
                unichar ch = [c characterAtIndex:i];
                if (ch >= 0xF700 && ch <= 0xF8FF) continue;          /* function keys: no character */
                if (ch == 0x7F) ch = 0x08;                          /* Delete is Backspace */
                if (ch == 0x0A) ch = 0x0D;
                in.chars[in.nchars++] = ch;
            }
        }
        halopad_host_post_input(&in);
    }
}
- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event { [self keys:presses down:1]; }
- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event { [self keys:presses down:0]; }
- (void)pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event { [self keys:presses down:0]; }

/* A view point as the window's client pixels (the layer is letterboxed, or stretched to fill). */
- (BOOL)client:(CGPoint)p x:(int32_t *)x y:(int32_t *)y scale:(double *)scale
{
    if (!input_window) return NO;
    uint32_t w, h;
    halopad_host_window_size(input_window, &w, &h);
    CGRect b = self.view.bounds;
    double sx = b.size.width / w, sy = b.size.height / h;
    if (HPSettings.shared.aspect != HPAspectFill) sx = sy = fmin(sx, sy);
    double ox = (b.size.width - w * sx) / 2, oy = (b.size.height - h * sy) / 2;
    *x = (int32_t)floor((p.x - ox) / sx);
    *y = (int32_t)floor((p.y - oy) / sy);
    *scale = 1 / fmin(sx, sy);
    return YES;
}
- (void)pointer:(UITouch *)t event:(UIEvent *)ev kind:(int)kind down:(int)down
{
    hp_input in = {.kind = kind, .down = down};
    double s;
    if (![self client:[t locationInView:self.view] x:&in.x y:&in.y scale:&s]) return;
    CGPoint a = [t locationInView:self.view], b = [t previousLocationInView:self.view];
    static double rx, ry;                               /* relative counts for DirectInput, fractions kept */
    rx += (a.x - b.x) * s; ry += (a.y - b.y) * s;
    in.dx = (int32_t)rx; in.dy = (int32_t)ry;
    rx -= in.dx; ry -= in.dy;
    in.button = (t.type == UITouchTypeIndirectPointer && (ev.buttonMask & UIEventButtonMaskSecondary)) ? 1 : 0;
    halopad_host_post_input(&in);
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    UITouch *t = touches.anyObject;
    [self pointer:t event:event kind:HPI_MOUSEMOVE down:0];
    [self pointer:t event:event kind:HPI_BUTTON down:1];
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self pointer:touches.anyObject event:event kind:HPI_MOUSEMOVE down:0]; }
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self pointer:touches.anyObject event:event kind:HPI_BUTTON down:0]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self pointer:touches.anyObject event:event kind:HPI_BUTTON down:0]; }
- (void)hover:(UIHoverGestureRecognizer *)g
{
    hp_input in = {.kind = HPI_MOUSEMOVE};
    double s;
    static CGPoint last;
    CGPoint p = [g locationInView:self.view];
    if (g.state == UIGestureRecognizerStateBegan) last = p;
    if (![self client:p x:&in.x y:&in.y scale:&s]) return;
    in.dx = (int32_t)lround((p.x - last.x) * s); in.dy = (int32_t)lround((p.y - last.y) * s);
    last = p;
    halopad_host_post_input(&in);
}
- (void)scroll:(UIPanGestureRecognizer *)g
{
    static double rest;                                 /* WHEEL_DELTA (120) per 10 points, fractions kept */
    CGPoint d = [g translationInView:self.view];
    [g setTranslation:CGPointZero inView:self.view];
    rest += d.y / 10.0 * 120.0;
    hp_input in = {.kind = HPI_WHEEL, .wheel = (int32_t)rest};
    rest -= in.wheel;
    double s;
    [self client:[g locationInView:self.view] x:&in.x y:&in.y scale:&s];
    if (in.wheel) halopad_host_post_input(&in);
}

- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
@end

/* ---- the first-run license, shown to the player ---- */

@interface HPLicenseViewController : UIViewController
@property(nonatomic, copy) NSString *path;
@property(nonatomic, copy) void (^choose)(int accepted);
@end

@implementation HPLicenseViewController
- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    UILabel *title = [UILabel new];
    title.text = @"Halo Custom Edition — License Agreement";
    title.font = [UIFont boldSystemFontOfSize:20];
    UITextView *text = [UITextView new];
    text.editable = NO;
    NSError *e = nil;
    NSAttributedString *rtf = [[NSAttributedString alloc] initWithURL:[NSURL fileURLWithPath:self.path]
                                                              options:@{NSDocumentTypeDocumentAttribute: NSRTFTextDocumentType}
                                                   documentAttributes:nil error:&e];
    text.attributedText = rtf ?: [[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"The license file could not be read: %@", e]];
    UIButton *decline = [UIButton buttonWithType:UIButtonTypeSystem];
    [decline setTitle:@"Decline" forState:UIControlStateNormal];
    UIButton *accept = [UIButton buttonWithType:UIButtonTypeSystem];
    [accept setTitle:@"I Accept" forState:UIControlStateNormal];
    accept.titleLabel.font = [UIFont boldSystemFontOfSize:18];
    decline.titleLabel.font = [UIFont systemFontOfSize:18];
    accept.enabled = rtf != nil;
    [decline addTarget:self action:@selector(declined) forControlEvents:UIControlEventPrimaryActionTriggered];
    [accept addTarget:self action:@selector(accepted) forControlEvents:UIControlEventPrimaryActionTriggered];
    UIStackView *buttons = [[UIStackView alloc] initWithArrangedSubviews:@[decline, accept]];
    buttons.axis = UILayoutConstraintAxisHorizontal;
    buttons.distribution = UIStackViewDistributionFillEqually;
    UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:@[title, text, buttons]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 12;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:stack];
    UILayoutGuide *g = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:24],
        [stack.trailingAnchor constraintEqualToAnchor:g.trailingAnchor constant:-24],
        [stack.topAnchor constraintEqualToAnchor:g.topAnchor constant:24],
        [stack.bottomAnchor constraintEqualToAnchor:g.bottomAnchor constant:-24],
        [buttons.heightAnchor constraintEqualToConstant:52]]];
}
- (void)accepted { [self dismissViewControllerAnimated:YES completion:nil]; self.choose(1); }
- (void)declined { [self dismissViewControllerAnimated:YES completion:nil]; self.choose(0); }
@end

/* Called on Halo's thread by EBUEula: shows the license and blocks until the player chooses. */
int halopad_host_license_prompt(const char *rtf_path)
{
    if ([NSThread isMainThread]) return -1;
    __block int choice = -1;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    NSString *path = [NSString stringWithUTF8String:rtf_path];
    dispatch_async(dispatch_get_main_queue(), ^{
        HPLicenseViewController *lic = [HPLicenseViewController new];
        lic.path = path;
        lic.modalPresentationStyle = UIModalPresentationFormSheet;
        lic.modalInPresentation = YES;                  /* only Accept or Decline closes it */
        lic.choose = ^(int accepted) { choice = accepted; dispatch_semaphore_signal(done); };
        [game_vc presentViewController:lic animated:YES completion:nil];
    });
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
    return choice;
}

/* ---- Halo's dialogs (its warnings and errors), shown to the player ---- */

#include "../runtime/halopad_dialog.h"

@interface HPDialogViewController : UIViewController
@property(nonatomic) halopad_dialog_view model;
@property(nonatomic, copy) void (^choose)(int index);
- (void)rebuild;
@end

static NSString *cp1252(const char *s) { return [[NSString alloc] initWithCString:s encoding:NSWindowsCP1252StringEncoding] ?: @""; }
static UIColor *colorref(uint32_t c) { return [UIColor colorWithRed:(c & 0xFF) / 255.0 green:(c >> 8 & 0xFF) / 255.0 blue:(c >> 16 & 0xFF) / 255.0 alpha:1]; }

@implementation HPDialogViewController {
    UIView *_canvas;
    CGFloat _scale;
}
- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.secondarySystemBackgroundColor;
    [self rebuild];
}
/* The controls at the template's positions (client pixels, scaled for the iPad), in the
   state Halo left them: text, enabled, checked, the link colour its WM_CTLCOLORSTATIC set. */
- (void)rebuild
{
    [_canvas removeFromSuperview];
    halopad_dialog_view m = self.model;
    _scale = 1.75;
    CGFloat title_h = 44;
    self.preferredContentSize = CGSizeMake(m.w * _scale + 32, m.h * _scale + title_h + 32);
    _canvas = [[UIView alloc] initWithFrame:CGRectMake(16, 16, m.w * _scale, m.h * _scale + title_h)];
    [self.view addSubview:_canvas];
    UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, m.w * _scale, title_h - 8)];
    title.text = cp1252(m.title);
    title.font = [UIFont boldSystemFontOfSize:19];
    [_canvas addSubview:title];
    UIFont *font = [UIFont systemFontOfSize:11 * _scale * 0.85];
    for (int i = 0; i < m.count; i++) {
        const halopad_dialog_item *it = &m.item[i];
        CGRect r = CGRectMake(it->x * _scale, title_h + it->y * _scale, it->w * _scale, it->h * _scale);
        NSString *text = cp1252(it->text);
        UIView *v = nil;
        switch (it->kind) {
        case HPD_TEXT: {
            UILabel *l = [[UILabel alloc] initWithFrame:r];
            l.text = text; l.font = font; l.numberOfLines = 0; l.textColor = colorref(it->color);
            l.textAlignment = it->align == 1 ? NSTextAlignmentCenter : it->align == 2 ? NSTextAlignmentRight : NSTextAlignmentLeft;
            [l sizeToFit];
            l.frame = CGRectMake(r.origin.x, r.origin.y, r.size.width, MAX(r.size.height, l.frame.size.height));
            if (it->align == 2) l.frame = r;
            v = l;
            break;
        }
        case HPD_ICON: {
            UIImageView *img = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"exclamationmark.triangle.fill"]];
            img.tintColor = UIColor.systemYellowColor;
            img.contentMode = UIViewContentModeScaleAspectFit;
            img.frame = r;
            v = img;
            break;
        }
        case HPD_LINK: case HPD_BUTTON: case HPD_CHECKBOX: {
            UIButton *b = [UIButton buttonWithType:UIButtonTypeSystem];
            b.frame = r;
            b.tag = i;
            b.enabled = it->enabled;
            b.titleLabel.font = it->is_default ? [UIFont boldSystemFontOfSize:font.pointSize] : font;
            if (it->kind == HPD_CHECKBOX) {
                [b setImage:[UIImage systemImageNamed:it->checked ? @"checkmark.square.fill" : @"square"] forState:UIControlStateNormal];
                b.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
                [b setTitle:[@" " stringByAppendingString:text] forState:UIControlStateNormal];
            } else if (it->kind == HPD_LINK) {
                NSDictionary *a = @{NSForegroundColorAttributeName: colorref(it->color), NSUnderlineStyleAttributeName: @(NSUnderlineStyleSingle), NSFontAttributeName: font};
                [b setAttributedTitle:[[NSAttributedString alloc] initWithString:text attributes:a] forState:UIControlStateNormal];
                b.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
            } else {
                [b setTitle:text forState:UIControlStateNormal];
                b.backgroundColor = UIColor.tertiarySystemFillColor;
                b.layer.cornerRadius = 6;
            }
            [b addTarget:self action:@selector(activated:) forControlEvents:UIControlEventPrimaryActionTriggered];
            v = b;
            break;
        }
        }
        if (v) [_canvas addSubview:v];
    }
}
- (void)activated:(UIButton *)b { if (self.choose) { void (^c)(int) = self.choose; self.choose = nil; c((int)b.tag); } }
@end

static HPDialogViewController *dialog_vc;

/* Called on Halo's thread by USER32's modal loop: shows the dialog's state and blocks until the
   player activates a control; the sheet stays up until Halo ends the dialog. */
int halopad_host_dialog(const halopad_dialog_view *v)
{
    if ([NSThread isMainThread] || !game_vc) return HPD_NO_SCREEN;
    __block int choice = HPD_CLOSE;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    halopad_dialog_view copy = *v;
    dispatch_async(dispatch_get_main_queue(), ^{
        void (^choose)(int) = ^(int i) { choice = i; dispatch_semaphore_signal(done); };
        if (dialog_vc) {
            dialog_vc.model = copy;
            dialog_vc.choose = choose;
            [dialog_vc rebuild];
            return;
        }
        dialog_vc = [HPDialogViewController new];
        dialog_vc.model = copy;
        dialog_vc.choose = choose;
        dialog_vc.modalPresentationStyle = UIModalPresentationFormSheet;
        dialog_vc.modalInPresentation = YES;            /* only the dialog's own buttons close it */
        [game_vc presentViewController:dialog_vc animated:YES completion:nil];
    });
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
    return choice;
}

void halopad_host_dialog_done(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [dialog_vc dismissViewControllerAnimated:YES completion:nil];
        dialog_vc = nil;
    });
}

/* ShellExecuteA "open" on a web address: Safari */
int halopad_host_open_url(const char *url)
{
    NSURL *u = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    if (!u || !([u.scheme isEqualToString:@"http"] || [u.scheme isEqualToString:@"https"])) return 0;
    dispatch_async(dispatch_get_main_queue(), ^{ [UIApplication.sharedApplication openURL:u options:@{} completionHandler:nil]; });
    return 1;
}

/* ---- application and scene ---- */

@interface HPSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@end
@implementation HPSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options
{
    /* Halo's sound: a playback session, so Remote I/O can start (DirectSound starts it) */
    NSError *err = nil;
    AVAudioSession *audio = AVAudioSession.sharedInstance;
    if (![audio setCategory:AVAudioSessionCategoryPlayback mode:AVAudioSessionModeDefault options:AVAudioSessionCategoryOptionMixWithOthers error:&err] ||
        ![audio setActive:YES error:&err])
        fprintf(stderr, "HALOPAD APP: audio session: %s\n", err.localizedDescription.UTF8String);
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    game_vc = [HPGameViewController new];
    self.window.rootViewController = game_vc;
    [self.window makeKeyAndVisible];
    /* landscape, as Halo's desktop is (iPadOS 26 no longer holds apps to Info.plist's list) */
    UIWindowSceneGeometryPreferencesIOS *land = [[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:UIInterfaceOrientationMaskLandscape];
    [(UIWindowScene *)scene requestGeometryUpdateWithPreferences:land errorHandler:^(NSError *e) { fprintf(stderr, "HALOPAD APP: landscape request: %s\n", e.localizedDescription.UTF8String); }];
}
- (void)sceneDidBecomeActive:(UIScene *)scene { hp_input e = {.kind = HPI_ACTIVATE, .down = 1}; halopad_host_post_input(&e); }
- (void)sceneWillResignActive:(UIScene *)scene { hp_input e = {.kind = HPI_ACTIVATE, .down = 0}; halopad_host_post_input(&e); }
@end

@interface HPAppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation HPAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)app configurationForConnectingSceneSession:(UISceneSession *)session
                              options:(UISceneConnectionOptions *)options
{
    UISceneConfiguration *c = [[UISceneConfiguration alloc] initWithName:@"HaloPad" sessionRole:session.role];
    c.delegateClass = HPSceneDelegate.class;
    return c;
}
@end

int main(int argc, char *argv[])
{
    @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(HPAppDelegate.class)); }
}
