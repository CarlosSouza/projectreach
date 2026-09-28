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
#include "halopad_menu_touch.h"
#include <pthread.h>

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
static HPOverlay *overlay;
static hp_menu_touch menu_touch;
static pthread_mutex_t menu_touch_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t menu_touch_root;
static void cancel_menu_touch(void)
{
    pthread_mutex_lock(&menu_touch_lock);
    menu_touch.cancel = 1;
    pthread_mutex_unlock(&menu_touch_lock);
}
/* Read the locked CE 1.10 UI state on Halo's presenting thread. The root widget
   exists in pause/child menus even while an online match continues to simulate. */
static void update_overlay_game_state(void)
{
    if (!halopad_guest_base) return;
    const char *map = halopad_guest_ptr(0x643064);
    uint32_t root;
    memcpy(&root, halopad_guest_ptr(0x6b401c), sizeof root);
    /* CE UI cursor coordinates are 640x480 regardless of render resolution. Halo's
       original input update applies acceleration and moves its own software cursor. */
    int32_t cursor[2]; float sensitivity[2];
    memcpy(cursor, halopad_guest_ptr(0x6b400c), sizeof cursor);
    memcpy(sensitivity, halopad_guest_ptr(0x629c64), sizeof sensitivity);
    hp_input event;
    pthread_mutex_lock(&menu_touch_lock);
    menu_touch_root = root;
    int emit = hp_menu_step(&menu_touch, root, cursor[0], cursor[1], sensitivity[0], sensitivity[1], &event);
    pthread_mutex_unlock(&menu_touch_lock);
    if (emit) {
        if (getenv("HALOPAD_TRACE_INPUT"))
            fprintf(stderr, "HALOPAD MENU TOUCH: cursor %d,%d event %d delta %d,%d down %d\n",
                    cursor[0], cursor[1], event.kind, event.dx, event.dy, event.down);
        halopad_input_event(&event);
    }
    BOOL inGame = map[0] && strcmp(map, "ui");
    BOOL menuVisible = root != 0;
    static int previous = -1;
    int state = inGame | (menuVisible << 1);
    if (state == previous) return;
    previous = state;
    if (getenv("HALOPAD_TRACE_INPUT"))
        fprintf(stderr, "HALOPAD MENU: in game %d menu visible %d\n", inGame, menuVisible);
    dispatch_async(dispatch_get_main_queue(), ^{
        overlay.haloMenuVisible = menuVisible;
        overlay.inGame = inGame;
    });
}
void *halopad_d3d9_device_target(uint32_t g);
void halopad_metal_read_image(void *p, uint32_t *out, uint32_t w, uint32_t h);
static void count_present(uint32_t device)
{
    int n = atomic_fetch_add(&presented, 1) + 1;
    update_overlay_game_state();
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
@implementation HPKeyboardProxy {
    UIToolbar *_keyboardBar;
}
- (UIView *)inputAccessoryView
{
    if (!_keyboardBar) {
        _keyboardBar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, 320, 44)];
        _keyboardBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
        _keyboardBar.items = @[
            [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil],
            [[UIBarButtonItem alloc] initWithTitle:@"Hide Keyboard" style:UIBarButtonItemStyleDone target:self action:@selector(hideKeyboard)]];
    }
    return _keyboardBar;
}
- (void)hideKeyboard { [self resignFirstResponder]; }
- (BOOL)canBecomeFirstResponder { return YES; }
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event { return NO; }
- (BOOL)hasText { return YES; }
- (void)insertText:(NSString *)text { [HPOverlay typeText:text]; }
- (void)deleteBackward { [HPOverlay typeText:@"\b"]; }
- (UITextAutocorrectionType)autocorrectionType { return UITextAutocorrectionTypeNo; }
- (UITextAutocapitalizationType)autocapitalizationType { return UITextAutocapitalizationTypeNone; }
- (UITextSpellCheckingType)spellCheckingType { return UITextSpellCheckingTypeNo; }
- (UIReturnKeyType)returnKeyType { return UIReturnKeySend; }
@end

/* ---- the game's files on the device (G9: prepared-data import) ----
   Without the Mac's HALOPAD_* paths (a device, or --device-data on the Simulator) the app uses its
   bundle for its own data (the translated image and modules, the reference machine's files, the
   registry seed, the input profile), Application Support for its state, and the player's own
   Halo Custom Edition folder in Documents (the Files app, Finder, or the folder picker below),
   accepted only after every stock file matches the signed bundle inventory. Nothing is downloaded and no key is involved. */
#import "HaloPadImport.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

static NSString *documents_game_dir(void)
{
    NSURL *docs = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    return [docs.path stringByAppendingPathComponent:@"Halo Custom Edition"];
}

static NSDictionary *game_core_identity(void)
{
    NSString *path = [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"data/core-identity.json"];
    id identity = [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:path] ?: NSData.data options:0 error:nil];
    return [identity isKindOfClass:NSDictionary.class] ? identity : nil;
}

static NSString *game_dir_problem(NSString *directory)
{
    return HPGameDirectoryProblem(directory, game_core_identity());
}

/* the app's own paths, when the environment does not name the Mac's */
static void resolve_device_paths(void)
{
    if (getenv("HALOPAD_IMAGE")) return; /* Explicit development data paths. */
    unsetenv("HALOPAD_GAME_ROOT"); /* Device startup always waits for full validation. */
    NSString *data = [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"data"];
    NSString *support = [NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject.path;
    NSString *state = [support stringByAppendingPathComponent:@"HaloPad/state"];
    [NSFileManager.defaultManager createDirectoryAtPath:state withIntermediateDirectories:YES attributes:nil error:nil];
    setenv("HALOPAD_IMAGE", [data stringByAppendingPathComponent:@"image.bin"].UTF8String, 1);
    setenv("HALOPAD_MODULE_IMAGES", [data stringByAppendingPathComponent:@"modules"].UTF8String, 1);
    setenv("HALOPAD_REFERENCE_ROOT", [data stringByAppendingPathComponent:@"reference"].UTF8String, 1);
    setenv("HALOPAD_REPO_ROOT", data.UTF8String, 1);                  /* config/runtime/registry-machine.txt lives under it */
    setenv("HALOPAD_STATE_ROOT", state.UTF8String, 1);
    setenv("HALOPAD_REGISTRY", [state stringByAppendingPathComponent:@"registry.txt"].UTF8String, 1);
    fprintf(stderr, "HALOPAD APP: device paths; game data validation pending\n");
}

/* The import screen: what to copy and where, a folder picker, and a re-check. */
@interface HPImportViewController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, copy) void (^ready)(void);
@property(nonatomic, strong) UILabel *status;
@property(nonatomic, strong) UIStackView *importButtons;
@property(nonatomic) BOOL importing, pickingPackage, checkedOnAppear;
@end

@implementation HPImportViewController
- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    UILabel *title = [UILabel new];
    title.text = @"Your Halo Custom Edition files";
    title.font = [UIFont boldSystemFontOfSize:24];
    UILabel *text = [UILabel new];
    text.numberOfLines = 0;
    text.font = [UIFont systemFontOfSize:17];
    text.text = @"Prepare your own Halo Custom Edition 1.10 files on your Mac with HaloPad’s preparation tool, using this app build. Transfer the resulting .halopad.zip package to Files, then choose it below. HaloPad checks the complete package before installing it.\n\nA replacement keeps your previous game folder as a backup. The game and its product key are not included. Choose Folder imports only supported stock files. Other files stay in the source folder or retained backup.";
    self.status = [UILabel new];
    self.status.numberOfLines = 0;
    self.status.font = [UIFont systemFontOfSize:15];
    self.status.textColor = UIColor.secondaryLabelColor;
    self.status.text = @"Checking your installed game data…";
    UIButton *package = [UIButton buttonWithType:UIButtonTypeSystem];
    [package setTitle:@"Choose Prepared Package…" forState:UIControlStateNormal];
    package.titleLabel.font = [UIFont boldSystemFontOfSize:18];
    [package addTarget:self action:@selector(pickPackage) forControlEvents:UIControlEventPrimaryActionTriggered];
    UIButton *pick = [UIButton buttonWithType:UIButtonTypeSystem];
    [pick setTitle:@"Choose Folder…" forState:UIControlStateNormal];
    pick.titleLabel.font = [UIFont boldSystemFontOfSize:18];
    [pick addTarget:self action:@selector(pick) forControlEvents:UIControlEventPrimaryActionTriggered];
    UIButton *again = [UIButton buttonWithType:UIButtonTypeSystem];
    [again setTitle:@"Check Again" forState:UIControlStateNormal];
    again.titleLabel.font = [UIFont systemFontOfSize:18];
    [again addTarget:self action:@selector(check) forControlEvents:UIControlEventPrimaryActionTriggered];
    UIStackView *buttons = [[UIStackView alloc] initWithArrangedSubviews:@[package, pick, again]];
    self.importButtons = buttons;
    buttons.axis = UILayoutConstraintAxisVertical;
    buttons.distribution = UIStackViewDistributionFillEqually;
    UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:@[title, text, self.status, buttons]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 18;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    UIScrollView *scroll = [UIScrollView new];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    scroll.alwaysBounceVertical = YES;
    [self.view addSubview:scroll];
    [scroll addSubview:stack];
    UILayoutGuide *g = self.view.safeAreaLayoutGuide;
    NSLayoutConstraint *preferredWidth = [stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-48];
    preferredWidth.priority = UILayoutPriorityDefaultHigh;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.leadingAnchor constraintEqualToAnchor:g.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:g.trailingAnchor],
        [scroll.topAnchor constraintEqualToAnchor:g.topAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:g.bottomAnchor],
        [stack.centerXAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.centerXAnchor],
        [stack.leadingAnchor constraintGreaterThanOrEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:24],
        [stack.trailingAnchor constraintLessThanOrEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-24],
        [stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:24],
        [stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-24],
        [scroll.contentLayoutGuide.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor],
        preferredWidth, [stack.widthAnchor constraintLessThanOrEqualToConstant:640],
        [buttons.heightAnchor constraintEqualToConstant:144]]];
    self.view.accessibilityIdentifier = @"HaloPadImport";
}
- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    if (!self.checkedOnAppear) { self.checkedOnAppear = YES; [self check]; }
}
- (void)check
{
    if (self.importing) return;
    self.importing = YES;
    self.importButtons.userInteractionEnabled = NO;
    self.importButtons.alpha = .5;
    self.status.text = @"Verifying your installed game files…";
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSString *problem = game_dir_problem(documents_game_dir());
        dispatch_async(dispatch_get_main_queue(), ^{
            self.status.text = problem ?: @"Game data verified. Starting…";
            if (problem) {
                self.importing = NO;
                self.importButtons.userInteractionEnabled = YES;
                self.importButtons.alpha = 1;
            } else {
                /* Keep the success latch until dismissal: no second core on repeated taps. */
                setenv("HALOPAD_GAME_ROOT", documents_game_dir().UTF8String, 1);
                if (self.ready) self.ready();
            }
        });
    });
}
- (void)pickPackage
{
    if (self.importing) return;
    self.pickingPackage = YES;
    [self presentPicker:UTTypeZIP];
}
- (void)pick
{
    if (self.importing) return;
    self.pickingPackage = NO;
    [self presentPicker:UTTypeFolder];
}
- (void)presentPicker:(UTType *)type
{
    UIDocumentPickerViewController *p = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[type]];
    p.delegate = self;
    [self presentViewController:p animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
    NSURL *src = urls.firstObject;
    if (!src || self.importing) return;
    self.importing = YES;
    self.importButtons.userInteractionEnabled = NO;
    self.importButtons.alpha = .5;
    BOOL package = self.pickingPackage;
    self.status.text = package ? @"Checking and importing your prepared package…" : @"Checking and copying your game files…";
    NSDictionary *identity = game_core_identity();
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        BOOL scoped = [src startAccessingSecurityScopedResource];
        NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
        NSError *coordinationError = nil;
        __block NSError *importError = nil;
        __block NSURL *backup = nil;
        __block BOOL ok = NO;
        [coordinator coordinateReadingItemAtURL:src options:NSFileCoordinatorReadingWithoutChanges
                              writingItemAtURL:[NSURL fileURLWithPath:documents_game_dir()]
                                       options:NSFileCoordinatorWritingForReplacing
                                         error:&coordinationError byAccessor:^(NSURL *readURL, NSURL *writeURL) {
            ok = package ? HPImportGamePackage(readURL, writeURL, identity, &backup, &importError)
                         : HPImportGameDirectory(readURL, writeURL, identity, &backup, &importError);
        }];
        if (scoped) [src stopAccessingSecurityScopedResource];
        dispatch_async(dispatch_get_main_queue(), ^{
            self.importing = NO;
            self.importButtons.userInteractionEnabled = YES;
            self.importButtons.alpha = 1;
            if (!ok) self.status.text = (importError ?: coordinationError).localizedDescription ?: @"The game data could not be imported.";
            else if (backup) {
                self.status.text = [NSString stringWithFormat:@"Game files imported. Your previous folder is saved as %@. Tap Check Again to start.", backup.lastPathComponent];
            } else [self check];
        });
    });
}
@end

@interface HPGameViewController : UIViewController <HPOverlayDelegate>
@property(nonatomic) BOOL trackingMenuTouch;
@property(nonatomic) unsigned menuTouchToken;
@end

static HPGameViewController *game_vc;
static UIView *game_view;
static NSLayoutConstraint *keyboard_bottom, *full_bottom;
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

/* ---- development: the touch controls' self-test (HALOPAD_TOUCH_SELFTEST) ----
   In a game, drive the overlay's controls through the handlers their touches use and check the
   result in Halo's game state (players 0x815920, objects 0x7fb710; see tests/halo_play_test.c):
   the move stick moves the player, dragging turns the look vector, FIRE shoots (rounds or
   projectiles), JUMP lifts the player. Run against the private reference server. */
static uint32_t g32(uint32_t a) { uint32_t v; memcpy(&v, halopad_guest_ptr(a), 4); return v; }
static uint8_t g8(uint32_t a) { return *(uint8_t *)halopad_guest_ptr(a); }
static uint16_t g16(uint32_t a) { uint16_t v; memcpy(&v, halopad_guest_ptr(a), 2); return v; }
static float gf(uint32_t a) { float v; memcpy(&v, halopad_guest_ptr(a), 4); return v; }
static uint32_t g_object(uint32_t h)
{
    uint32_t ot = g32(0x7fb710);
    if (!ot || h == 0xffffffff || (h & 0xffff) >= g16(ot + 0x20)) return 0;
    uint32_t e = g32(ot + 0x34) + (h & 0xffff) * 12;
    return g16(e) == h >> 16 ? g32(e + 8) : 0;
}
static uint32_t g_unit(void)
{
    if (!halopad_guest_base) return 0;
    uint32_t pt = g32(0x815920);
    return pt ? g_object(g32(g32(pt + 0x34) + 0x34)) : 0;
}
static uint32_t g_live_objects(void)
{
    uint32_t ot = g32(0x7fb710), n = 0;
    if (!ot) return 0;
    for (uint32_t i = 0; i < g16(ot + 0x20); i++) n += g16(g32(ot + 0x34) + i * 12) != 0;
    return n;
}
static int selftest_failures;
static void selftest_check(const char *what, int ok, NSString *detail)
{
    fprintf(stderr, "HALOPAD SELFTEST: %-58s %s (%s)\n", what, ok ? "PASS" : "FAIL", detail.UTF8String);
    selftest_failures += !ok;
}
static void after(double s, dispatch_block_t b) { dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(s * NSEC_PER_SEC)), dispatch_get_main_queue(), b); }

/* Opt-in action acceptance, through overlay handlers only. Reads the same object
   fields as halo_host_test; never writes guest state or supplies weapons/ammo. */
static void action_selftest(void)
{
    if (!overlay.inGame || !g_unit()) { after(1, ^{ action_selftest(); }); return; }
    after(6, ^{
        __block int phase = 0, ticks = 0, idleMelee = 0, meleePeak = 0, failures0 = selftest_failures;
        __block uint32_t target = 0, priorWeapon = 0, ammoWeapon = 0;
        __block uint16_t roundsBefore = 0, roundsFired = 0;
        __block BOOL used = NO;
        __block float bestDistance = 1e9f;
        __block int progressTick = 0, attempts = 0;
        NSMutableSet<NSNumber *> *skipped = [NSMutableSet set];
        fprintf(stderr, "HALOPAD ACTIONS: begin on %s\n", current_map().UTF8String);
        [NSTimer scheduledTimerWithTimeInterval:0.05 repeats:YES block:^(NSTimer *timer) {
            uint32_t u = g_unit();
            if (!overlay.inGame || overlay.haloMenuVisible || !u) {
                [overlay clearTouchInput]; [timer invalidate];
                selftest_check("action run remains in live gameplay", 0, @"interrupted or player lost");
                return;
            }
            ticks++;
            if (phase == 0) {
                idleMelee = g8(u + 0x505);
                [overlay driveControl:@"melee" down:YES];
                [overlay driveControl:@"melee" down:NO];
                phase = 1; ticks = 0;
            } else if (phase == 1) {
                /* Player melee: original 0x55d226 starts the biped timer;
                   0x55d263 decrements it. +0x289 belongs to the AI melee path. */
                int v = g8(u + 0x505);
                if (v > meleePeak) meleePeak = v;
                if (ticks < 30) return;
                selftest_check("one MELEE tap starts and finishes a swing", idleMelee == 0 && meleePeak != 0 && g8(u + 0x505) == 0,
                               [NSString stringWithFormat:@"idle %d, player melee ticks %d, final %d", idleMelee, meleePeak, g8(u + 0x505)]);
                phase = 2; ticks = 0;
            } else if (phase == 2) {
                /* Find a loose magazine-fed weapon on the player's level. A bounded
                   blocked route tries another candidate instead of teleporting. */
                uint32_t p = g32(g32(0x815920) + 0x34);
                if (target) for (int slot = 0; slot < 4; slot++) if (g32(u + 0x2f8 + 4 * slot) == target) {
                    [overlay driveMoveX:0 y:0]; [overlay driveControl:@"action" down:NO];
                    selftest_check("touch USE picks up a second weapon", used && g_object(g32(u + 0x2fc)) != 0,
                                   [NSString stringWithFormat:@"target %08x, USE held %d, slots %08x/%08x", target, used, g32(u + 0x2f8), g32(u + 0x2fc)]);
                    ammoWeapon = target; phase = 3; ticks = 0; return;
                }
                if (ticks > 1200 || attempts >= 8) {
                    [overlay clearTouchInput]; [timer invalidate];
                    selftest_check("action fixture obtains a magazine-fed weapon", 0, @"route exhausted; switch/reload unverified");
                    fprintf(stderr, "HALOPAD ACTIONS: FAIL (fixture incomplete)\n"); return;
                }
                uint32_t o = g_object(target);
                if (target && (!o || ticks - progressTick > 100)) {
                    [skipped addObject:@(target)]; target = 0; attempts++;
                    [overlay driveMoveX:0 y:0]; [overlay driveControl:@"action" down:NO];
                }
                if (!target) {
                    uint32_t ot = g32(0x7fb710), held = g_object(g32(u + 0x118));
                    float best = 25;
                    for (uint32_t i = 0; i < g16(ot + 0x20); i++) {
                        uint32_t e = g32(ot + 0x34) + i * 12, h = (uint32_t)g16(e) << 16 | i;
                        if (!g16(e) || [skipped containsObject:@(h)]) continue;
                        uint32_t item = g_object(h);
                        if (!item || (int16_t)g16(item + 0xb4) != 2 || g32(item + 0xcc) != 0xffffffff ||
                            !g16(item + 0x2b8) || (held && g32(item) == g32(held))) continue;
                        float d = hypotf(gf(item + 0x5c) - gf(u + 0x5c), gf(item + 0x60) - gf(u + 0x60));
                        if (fabsf(gf(item + 0x64) - gf(u + 0x64)) < 1 && d < best) { best = d; target = h; }
                    }
                    progressTick = ticks; bestDistance = 1e9f; used = NO;
                    if (!target) return;
                    fprintf(stderr, "HALOPAD ACTIONS: approach %08x, %.2f units away\n", target, best);
                    o = g_object(target);
                }
                float distance = hypotf(gf(o + 0x5c) - gf(u + 0x5c), gf(o + 0x60) - gf(u + 0x60));
                if (distance < bestDistance - 0.3f) { bestDistance = distance; progressTick = ticks; }
                float yaw = atan2f(gf(u + 0x240), gf(u + 0x23c));
                float bearing = atan2f(gf(o + 0x60) - gf(u + 0x60), gf(o + 0x5c) - gf(u + 0x5c));
                float err = remainderf((bearing - yaw) * 57.29578f, 360);
                float dx = fmaxf(-60, fminf(60, -err * 6));
                [overlay driveLookX:dx / (2.2f * HPSettings.shared.lookSensitivity) y:0];
                BOOL offered = g32(p + 0x24) == target;
                [overlay driveMoveX:0 y:(!offered && fabsf(err) < 20 ? 1 : 0)];
                [overlay driveControl:@"action" down:offered]; used |= offered;
            } else if (phase == 3 && ticks >= 30) {
                priorWeapon = g32(u + 0x118);
                [overlay driveControl:@"switch" down:YES]; [overlay driveControl:@"switch" down:NO];
                phase = 4; ticks = 0;
            } else if (phase == 4 && ticks >= 30) {
                uint32_t current = g32(u + 0x118);
                selftest_check("one SWAP tap changes the equipped weapon", current != priorWeapon && g_object(current),
                               [NSString stringWithFormat:@"%08x -> %08x", priorWeapon, current]);
                if (current != ammoWeapon) { [overlay driveControl:@"switch" down:YES]; [overlay driveControl:@"switch" down:NO]; }
                phase = 5; ticks = 0;
            } else if (phase == 5 && ticks >= 30) {
                uint32_t w = g_object(g32(u + 0x118));
                if (!w || g32(u + 0x118) != ammoWeapon || !g16(w + 0x2b8)) {
                    [overlay clearTouchInput]; [timer invalidate];
                    selftest_check("reload fixture equips the picked-up weapon", 0, @"no loaded magazine"); return;
                }
                roundsBefore = g16(w + 0x2b8);
                [overlay driveControl:@"fire" down:YES]; phase = 6; ticks = 0;
            } else if (phase == 6) {
                if (ticks == 12) [overlay driveControl:@"fire" down:NO];
                if (ticks < 35) return;
                uint32_t w = g_object(ammoWeapon);
                roundsFired = w ? g16(w + 0x2b8) : 0;
                selftest_check("touch FIRE spends magazine rounds", w && g32(u + 0x118) == ammoWeapon && roundsFired < roundsBefore,
                               [NSString stringWithFormat:@"%u -> %u", roundsBefore, roundsFired]);
                [overlay driveControl:@"reload" down:YES]; [overlay driveControl:@"reload" down:NO];
                phase = 7; ticks = 0;
            } else if (phase == 7 && ticks >= 90) {
                uint32_t w = g_object(ammoWeapon);
                uint16_t rounds = w ? g16(w + 0x2b8) : 0;
                selftest_check("one RELOAD tap refills the magazine", w && g32(u + 0x118) == ammoWeapon && rounds > roundsFired,
                               [NSString stringWithFormat:@"%u -> %u", roundsFired, rounds]);
                [overlay clearTouchInput]; [timer invalidate];
                fprintf(stderr, "HALOPAD ACTIONS: %s (%d failures)\n", selftest_failures == failures0 ? "PASS" : "FAIL", selftest_failures - failures0);
            }
        }];
    });
}

static void touch_selftest(void)
{
    uint32_t u = g_unit();
    if (!overlay.inGame || !u) { after(1, ^{ touch_selftest(); }); return; }
    static int started;
    if (!started++) { fprintf(stderr, "HALOPAD SELFTEST: in a game on \"%s\"; the player's unit is there\n", current_map().UTF8String); after(6, ^{ touch_selftest(); }); return; }
    /* 1. the move stick: forward for 2 s */
    float x0 = gf(u + 0x5c), y0 = gf(u + 0x60);
    [overlay driveMoveX:0 y:1];
    after(2, ^{
        [overlay driveMoveX:0 y:0];
        uint32_t u1 = g_unit();
        float d = u1 ? hypotf(gf(u1 + 0x5c) - x0, gf(u1 + 0x60) - y0) : 0;
        selftest_check("the move stick walks the player", d > 1.0f, [NSString stringWithFormat:@"moved %.2f units", d]);
        /* 2. looking: a 120-point drag to the right */
        float yaw0 = atan2f(gf(u1 + 0x240), gf(u1 + 0x23c));
        for (int i = 0; i < 12; i++) [overlay driveLookX:10 y:0];
        after(0.6, ^{
            uint32_t u2 = g_unit();
            float dyaw = fabsf(atan2f(gf(u2 + 0x240), gf(u2 + 0x23c)) - yaw0) * 57.29578f;
            if (dyaw > 180) dyaw = 360 - dyaw;
            selftest_check("dragging turns the view", dyaw > 5.0f, [NSString stringWithFormat:@"turned %.1f degrees", dyaw]);
            /* 3. FIRE: the magazine goes down or projectiles appear */
            uint32_t w = g_object(g32(u2 + 0x118));
            uint16_t rounds0 = w ? g16(w + 0x2b8) : 0;
            float battery0 = w ? gf(w + 0x134) : -1;              /* weapon +0x134: a plasma weapon's battery */
            fprintf(stderr, "HALOPAD SELFTEST: weapon handle %08x object %08x type %d rounds %u; slots %08x %08x\n", g32(u2 + 0x118), w,
                    w ? (int16_t)g16(w + 0xb4) : -9, rounds0, g32(u2 + 0x2f8), g32(u2 + 0x2fc));
            uint32_t objs0 = g_live_objects();
            __block uint32_t objs_most = objs0;
            /* held 1 s: an automatic weapon fires, a plasma pistol charges fully and fires its overcharged
               shot on release (a release before full charge fired nothing); sampled for 3 s */
            static uint32_t before[0x100];
            if (w) memcpy(before, halopad_guest_ptr(w), sizeof before);
            if (getenv("HALOPAD_TRACE_WEAPON"))
                for (int k = 0; k < 4; k++) after(0.25 + 0.5 * k, ^{
                    uint32_t ww = g_object(g32(g_unit() + 0x118));
                    if (!ww) return;
                    fprintf(stderr, "HALOPAD SELFTEST: weapon words changed at %.2f s:", 0.25 + 0.5 * k);
                    for (int q = 0; q < 0xb0; q++) { uint32_t v = g32(ww + 4 * q); if (v != before[q]) fprintf(stderr, " +%03x %08x->%08x", 4 * q, before[q], v); }
                    fprintf(stderr, "\n");
                });
            [overlay driveControl:@"fire" down:YES];
            after(1.0, ^{ [overlay driveControl:@"fire" down:NO]; });
            for (int k = 1; k <= 30; k++) after(0.1 * k, ^{ uint32_t n = g_live_objects(); if (n > objs_most) objs_most = n; });
            after(3.05, ^{
                uint32_t w2 = g_object(g32(g_unit() + 0x118));
                uint16_t rounds1 = w2 ? g16(w2 + 0x2b8) : 0;
                float battery1 = w2 ? gf(w2 + 0x134) : -1;
                selftest_check("FIRE shoots", rounds1 < rounds0 || battery1 < battery0 - 0.02f || objs_most > objs0,
                               [NSString stringWithFormat:@"rounds %u -> %u, battery %.2f -> %.2f, objects %u -> up to %u", rounds0, rounds1, battery0, battery1, objs0, objs_most]);
                /* 4. One immediate down/up JUMP, with no retry or minimum hold. */
                __block float z0 = 0, zmax = -1e9f;
                after(0.5, ^{ uint32_t u4 = g_unit(); z0 = zmax = u4 ? gf(u4 + 0x64) : 0; });
                after(0.6, ^{
                    [overlay driveControl:@"jump" down:YES];
                    [overlay driveControl:@"jump" down:NO];
                });
                for (int k = 1; k <= 30; k++) after(0.6 + 0.06 * k, ^{ uint32_t uu = g_unit(); if (uu && gf(uu + 0x64) > zmax) zmax = gf(uu + 0x64); });
                after(2.8, ^{
                    selftest_check("one immediate JUMP tap lifts the player", zmax > z0 + 0.2f, [NSString stringWithFormat:@"height %.2f -> up to %.2f", z0, zmax]);
                    uint32_t u5 = g_unit();
                    float aim0 = atan2f(gf(u5 + 0x240), gf(u5 + 0x23c));
                    [overlay driveAimX:1 y:0];
                    after(0.5, ^{
                        [overlay driveAimX:0 y:0];
                        uint32_t u6 = g_unit();
                        float angle = fabsf(atan2f(gf(u6 + 0x240), gf(u6 + 0x23c)) - aim0) * 57.29578f;
                        if (angle > 180) angle = 360 - angle;
                        selftest_check("the LOOK stick turns while held", angle > 5,
                                       [NSString stringWithFormat:@"turned %.1f degrees", angle]);
                        fprintf(stderr, "HALOPAD SELFTEST: %s: %d failure(s)\n", selftest_failures ? "FAIL" : "PASS", selftest_failures);
                    });
                });
            });
        });
    });
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
    /* Keep the console/chat prompt above a docked software keyboard. The input view stays
       full size; only the guest's render host follows the keyboard. */
    game_view = [UIView new];
    game_view.userInteractionEnabled = NO;
    game_view.translatesAutoresizingMaskIntoConstraints = NO;
    [v addSubview:game_view];
    v.keyboardLayoutGuide.usesBottomSafeArea = NO;
    keyboard_bottom = [game_view.bottomAnchor constraintEqualToAnchor:v.keyboardLayoutGuide.topAnchor];
    full_bottom = [game_view.bottomAnchor constraintEqualToAnchor:v.bottomAnchor];
    [NSLayoutConstraint activateConstraints:@[
        [game_view.leadingAnchor constraintEqualToAnchor:v.leadingAnchor],
        [game_view.trailingAnchor constraintEqualToAnchor:v.trailingAnchor],
        [game_view.topAnchor constraintEqualToAnchor:v.topAnchor],
        full_bottom]];
    keyboard = [[HPKeyboardProxy alloc] initWithFrame:CGRectZero];
    [v addSubview:keyboard];
    overlay = [[HPOverlay alloc] initWithFrame:v.bounds];
    overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    overlay.delegate = self;
    overlay.layer.zPosition = 100;                    /* above the Metal layers Halo's windows attach later */
    overlay.opaque = NO;
    overlay.backgroundColor = UIColor.clearColor;
    keyboard.opaque = NO;
    [NSNotificationCenter.defaultCenter addObserverForName:UIKeyboardWillChangeFrameNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
        CGRect screenFrame = [note.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
        CGRect frame = [v convertRect:screenFrame fromCoordinateSpace:v.window.screen.coordinateSpace];
        BOOL docked = CGRectGetMinY(frame) < CGRectGetMaxY(v.bounds) &&
                      CGRectGetMaxY(frame) >= CGRectGetMaxY(v.bounds) &&
                      CGRectGetMinX(frame) <= CGRectGetMinX(v.bounds) &&
                      CGRectGetMaxX(frame) >= CGRectGetMaxX(v.bounds);
        /* iOS can leave an accessory-height guide after resignation. Use full bounds when
           the reported keyboard is offscreen, rather than retaining that stale inset. */
        if (keyboard_bottom.active != docked) {
            keyboard_bottom.active = NO;
            full_bottom.active = NO;
            (docked ? keyboard_bottom : full_bottom).active = YES;
            [v setNeedsLayout];
        }
        if (getenv("HALOPAD_TRACE_WINDOWS"))
            fprintf(stderr, "HALOPAD KEYBOARD: frame %s -> %s local %d docked %d\n",
                    [note.userInfo[UIKeyboardFrameBeginUserInfoKey] description].UTF8String,
                    [note.userInfo[UIKeyboardFrameEndUserInfoKey] description].UTF8String,
                    [note.userInfo[UIKeyboardIsLocalUserInfoKey] boolValue], docked);
    }];
    overlay.hidden = getenv("HALOPAD_NO_OVERLAY") != NULL;   /* development: the game view alone */
    [v addSubview:overlay];
    /* Frame rate and optional map diagnostics. Menu ownership follows presented frames. */
    [NSTimer scheduledTimerWithTimeInterval:0.25 repeats:YES block:^(NSTimer *t) {
        NSString *m = current_map();
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
    if (getenv("HALOPAD_ACTION_SELFTEST")) after(5, ^{ action_selftest(); });
    else if (getenv("HALOPAD_TOUCH_SELFTEST")) after(5, ^{ touch_selftest(); });
    if (getenv("HALOPAD_GAME_ROOT")) { [self startHalo]; return; }
    /* no game folder yet: the import screen, then Halo */
    HPImportViewController *imp = [HPImportViewController new];
    imp.modalPresentationStyle = UIModalPresentationFormSheet;
    imp.modalInPresentation = YES;
    __weak HPImportViewController *wimp = imp;
    imp.ready = ^{ [wimp dismissViewControllerAnimated:YES completion:^{ [self startHalo]; }]; };
    [self presentViewController:imp animated:YES completion:nil];
}
- (void)startHalo
{
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
    for (CALayer *l in game_view.layer.sublayers) l.frame = game_view.bounds;
    [self applyDisplay];
    [self.view bringSubviewToFront:overlay];
}
/* ---- the overlay's requests ---- */
- (void)applyDisplay
{
    BOOL typing = game_view.bounds.size.height < self.view.bounds.size.height - 1;
    overlay.softwareKeyboardVisible = typing;
    /* Preserve readable proportions in the short typing viewport; restore the user's
       display choice when the keyboard closes. */
    NSString *g = !typing && HPSettings.shared.aspect == HPAspectFill ? kCAGravityResize : kCAGravityResizeAspect;
    for (CALayer *l in game_view.layer.sublayers) l.contentsGravity = g;
    if (getenv("HALOPAD_TRACE_WINDOWS"))
        for (CALayer *l in game_view.layer.sublayers)
            fprintf(stderr, "HALOPAD APP:   layer %s %s hidden %d opaque %d frame %.0fx%.0f z %.0f\n", l.class.description.UTF8String,
                    l == overlay.layer ? "(overlay)" : "", l.hidden, l.opaque, l.frame.size.width, l.frame.size.height, l.zPosition);
}
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
- (void)overlayDisplayChanged:(HPOverlay *)o { [self applyDisplay]; }
- (void)overlayRequestsKeyboard:(HPOverlay *)o
{
    BOOL accepted = [keyboard becomeFirstResponder];
    if (getenv("HALOPAD_TRACE_WINDOWS")) fprintf(stderr, "HALOPAD KEYBOARD: focus %d, scene %ld, frame %s\n", accepted, (long)self.view.window.windowScene.activationState, NSStringFromCGRect(keyboard.frame).UTF8String);
}
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
    CGRect b = game_view.frame;
    if (b.size.width <= 0 || b.size.height <= 0) return NO;
    double sx = b.size.width / w, sy = b.size.height / h;
    if (b.size.height < self.view.bounds.size.height - 1 || HPSettings.shared.aspect != HPAspectFill) sx = sy = fmin(sx, sy);
    double ox = b.origin.x + (b.size.width - w * sx) / 2, oy = b.origin.y + (b.size.height - h * sy) / 2;
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
/* A direct finger touch in Halo's menus positions the game cursor before clicking.
   Keep the gesture's ownership through its end even if the menu changes underneath it. */
- (void)menuTouch:(UITouch *)touch begin:(BOOL)begin ended:(BOOL)ended
{
    int32_t x, y; double scale;
    if (![self client:[touch locationInView:self.view] x:&x y:&y scale:&scale]) return;
    uint32_t w, h;
    halopad_host_window_size(input_window, &w, &h);
    if (!w || !h) return;
    int mx = (int)lround(x * 640.0 / w), my = (int)lround(y * 480.0 / h);
    pthread_mutex_lock(&menu_touch_lock);
    if (begin) self.menuTouchToken = hp_menu_begin(&menu_touch, menu_touch_root, mx, my, x, y);
    else hp_menu_update(&menu_touch, self.menuTouchToken, mx, my, x, y, ended);
    pthread_mutex_unlock(&menu_touch_lock);
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    UITouch *t = touches.anyObject;
    self.trackingMenuTouch = overlay.haloMenuVisible && t.type == UITouchTypeDirect;
    if (self.trackingMenuTouch) { [self menuTouch:t begin:YES ended:NO]; return; }
    [self pointer:t event:event kind:HPI_MOUSEMOVE down:0];
    [self pointer:t event:event kind:HPI_BUTTON down:1];
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (self.trackingMenuTouch) { [self menuTouch:touches.anyObject begin:NO ended:NO]; return; }
    [self pointer:touches.anyObject event:event kind:HPI_MOUSEMOVE down:0];
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (self.trackingMenuTouch) {
        [self menuTouch:touches.anyObject begin:NO ended:YES]; self.trackingMenuTouch = NO; return;
    }
    [self pointer:touches.anyObject event:event kind:HPI_BUTTON down:0];
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (self.trackingMenuTouch) { cancel_menu_touch(); self.trackingMenuTouch = NO; return; }
    [self pointer:touches.anyObject event:event kind:HPI_BUTTON down:0];
}
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
- (void)sceneDidBecomeActive:(UIScene *)scene
{
    [HPOverlay setTextInputActive:YES];
    if (getenv("HALOPAD_TRACE_LIFECYCLE")) fprintf(stderr, "HALOPAD LIFECYCLE: %.3f scene active, frames %d\n", CFAbsoluteTimeGetCurrent(), atomic_load(&presented));
    hp_input e = {.kind = HPI_ACTIVATE, .down = 1}; halopad_host_post_input(&e);
}
- (void)sceneWillResignActive:(UIScene *)scene
{
    if (getenv("HALOPAD_TRACE_LIFECYCLE")) fprintf(stderr, "HALOPAD LIFECYCLE: %.3f scene inactive, frames %d\n", CFAbsoluteTimeGetCurrent(), atomic_load(&presented));
    [overlay clearTouchInput];
    cancel_menu_touch();
    [HPOverlay setTextInputActive:NO];
    hp_input e = {.kind = HPI_ACTIVATE, .down = 0}; halopad_host_post_input(&e);
}
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
    resolve_device_paths();
    /* Halo's console (the menu's Join Server, Halo Console) needs its -console switch */
    const char *args = getenv("HALOPAD_ARGS");
    if (!args || !strstr(args, "-console")) {
        char with[1024];
        snprintf(with, sizeof with, "%s%s-console", args ? args : "", args && *args ? " " : "");
        setenv("HALOPAD_ARGS", with, 1);
    }
    @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(HPAppDelegate.class)); }
}
