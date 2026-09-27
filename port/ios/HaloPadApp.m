/* HaloPad for iPadOS (G3/G8): the app shell around the native core.
 *
 * The core (halopad_core_run: Halo from its PE entry point) runs on its own thread. Windows
 * Halo creates appear in the app's view: the shell attaches each window's Metal layer
 * (halopad_host_attach_view), letterboxed. When Halo's first-run license check runs, the shell
 * shows the game's own Eula.rtf with Accept and Decline and returns the player's choice
 * (halopad_host_license_prompt); nothing is chosen for the player. A hardware keyboard, touch
 * (as the left mouse button), an iPad pointer (buttons, hover, scrolling) and scene activation
 * are queued for Halo's thread (halopad_host_post_input).
 *
 * Development builds on the Simulator get their data paths from the environment
 * (scripts/build-ios-app.py passes HALOPAD_* through simctl launch). */
#import <UIKit/UIKit.h>
#include <stdio.h>
#include <math.h>

#include "../runtime/halopad_input.h"

int halopad_core_run(void);
void halopad_host_set_window_handler(void (*handler)(void *window));
void halopad_host_attach_view(void *window, UIView *view);
void halopad_host_window_size(void *window, uint32_t *w, uint32_t *h);

@interface HPGameViewController : UIViewController
@end

static HPGameViewController *game_vc;
static UIView *game_view;

static void *input_window;                          /* the window touches and the pointer act on */

static void on_window(void *w)
{
    if (!game_view) return;
    halopad_host_attach_view(w, game_view);
    input_window = w;
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
}
- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self becomeFirstResponder];
    static int started;
    if (started) return;
    started = 1;
    halopad_host_set_window_handler(on_window);
    NSThread *t = [[NSThread alloc] initWithBlock:^{
        int code = halopad_core_run();
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

/* A view point as the window's client pixels (the layer is letterboxed: aspect fit). */
- (BOOL)client:(CGPoint)p x:(int32_t *)x y:(int32_t *)y scale:(double *)scale
{
    if (!input_window) return NO;
    uint32_t w, h;
    halopad_host_window_size(input_window, &w, &h);
    CGRect b = self.view.bounds;
    double s = fmin(b.size.width / w, b.size.height / h);
    double ox = (b.size.width - w * s) / 2, oy = (b.size.height - h * s) / 2;
    *x = (int32_t)floor((p.x - ox) / s);
    *y = (int32_t)floor((p.y - oy) / s);
    *scale = 1 / s;
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

/* ---- application and scene ---- */

@interface HPSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@end
@implementation HPSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options
{
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    game_vc = [HPGameViewController new];
    self.window.rootViewController = game_vc;
    [self.window makeKeyAndVisible];
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
