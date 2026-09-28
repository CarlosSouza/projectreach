/* HaloPad touch controls and the three-dot menu: see HaloPadOverlay.h. Structure, colours, sizes
 * and the layout editor follow SunPad's SunPadGameOverlay.mm (ref/sunpad at e43f0ea, GPL-3.0). */
#import "HaloPadOverlay.h"
#import <GameController/GameController.h>
#include <math.h>

#include "../runtime/halopad_input.h"

/* ---- settings (NSUserDefaults, HaloPad-owned keys) ---- */

@implementation HPSettings
+ (instancetype)shared
{
    static HPSettings *s;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        s = [HPSettings new];
        [NSUserDefaults.standardUserDefaults registerDefaults:@{
            @"HaloPad.controlOpacity": @0.8, @"HaloPad.controlSize": @1.0, @"HaloPad.lookSensitivity": @1.0,
            @"HaloPad.hideWithController": @YES, @"HaloPad.hideTouchControls": @NO, @"HaloPad.showFPS": @NO,
            @"HaloPad.aspect": @0, @"HaloPad.recentServers": @[], @"HaloPad.leftHanded": @NO, @"HaloPad.showCaptions": @YES,
            @"HaloPad.ringSpacing": @1}];
    });
    return s;
}
#define HP_SETTING(type, get, set, key, box, unbox) \
    - (type)get { return (type)[(NSNumber *)[NSUserDefaults.standardUserDefaults objectForKey:key] unbox]; } \
    - (void)set:(type)v { [NSUserDefaults.standardUserDefaults setObject:box forKey:key]; }
HP_SETTING(CGFloat, controlOpacity, setControlOpacity, @"HaloPad.controlOpacity", @(fmin(1, fmax(0.25, v))), doubleValue)
HP_SETTING(CGFloat, controlSize, setControlSize, @"HaloPad.controlSize", @(fmin(1.35, fmax(0.7, v))), doubleValue)
HP_SETTING(CGFloat, lookSensitivity, setLookSensitivity, @"HaloPad.lookSensitivity", @(fmin(3, fmax(0.25, v))), doubleValue)
HP_SETTING(BOOL, hideWithController, setHideWithController, @"HaloPad.hideWithController", @(v), boolValue)
HP_SETTING(BOOL, hideTouchControls, setHideTouchControls, @"HaloPad.hideTouchControls", @(v), boolValue)
HP_SETTING(BOOL, showFPS, setShowFPS, @"HaloPad.showFPS", @(v), boolValue)
HP_SETTING(HPAspectMode, aspect, setAspect, @"HaloPad.aspect", @(v), integerValue)
HP_SETTING(BOOL, leftHanded, setLeftHanded, @"HaloPad.leftHanded", @(v), boolValue)
HP_SETTING(BOOL, showCaptions, setShowCaptions, @"HaloPad.showCaptions", @(v), boolValue)
HP_SETTING(NSInteger, ringSpacing, setRingSpacing, @"HaloPad.ringSpacing", @(MIN(2, MAX(0, v))), integerValue)
- (NSArray<NSString *> *)recentServers { return [NSUserDefaults.standardUserDefaults stringArrayForKey:@"HaloPad.recentServers"] ?: @[]; }
- (void)setRecentServers:(NSArray<NSString *> *)v { [NSUserDefaults.standardUserDefaults setObject:v forKey:@"HaloPad.recentServers"]; }
@end

/* ---- Windows input ---- */

static void post_key(uint32_t vk, uint32_t side, uint32_t scan, int ext, int down, unichar ch)
{
    hp_input e = {.kind = HPI_KEY, .vk = vk, .side_vk = side ? side : vk, .scan = scan, .extended = ext, .down = down};
    if (down && ch) { e.chars[0] = ch; e.nchars = 1; }
    halopad_host_post_input(&e);
}
static void post_mouse(int32_t dx, int32_t dy)
{
    hp_input e = {.kind = HPI_MOUSEMOVE, .x = 400, .y = 300, .dx = dx, .dy = dy};
    halopad_host_post_input(&e);
}
static void post_button(int b, int down)
{
    hp_input e = {.kind = HPI_BUTTON, .x = 400, .y = 300, .button = b, .down = down};
    halopad_host_post_input(&e);
}

/* Typed keys (text, the console key, chat keys) wait in a queue and go to Halo one event every
   50 ms, a little over one of Halo's frames: its keyboard is read once a frame, and a key that
   goes down and up between two reads is never seen. Held controls post at once. */
static NSMutableArray<NSValue *> *typed_queue;
static NSMutableDictionary<NSNumber *, NSValue *> *typed_held;
static BOOL typed_active = YES;
static void queue_key(uint32_t vk, uint32_t side, uint32_t scan, int down, unichar ch)
{
    if (!typed_active) return;
    hp_input e = {.kind = HPI_KEY, .vk = vk, .side_vk = side ? side : vk, .scan = scan, .down = down};
    if (down && ch) { e.chars[0] = ch; e.nchars = 1; }
    if (!typed_queue) {
        typed_queue = [NSMutableArray array];
        typed_held = [NSMutableDictionary dictionary];
        [NSTimer scheduledTimerWithTimeInterval:0.05 repeats:YES block:^(NSTimer *t) {
            if (!typed_active || !typed_queue.count) return;
            hp_input next;
            [typed_queue.firstObject getValue:&next size:sizeof next];
            [typed_queue removeObjectAtIndex:0];
            NSNumber *key = @(next.side_vk);
            if (next.down) typed_held[key] = [NSValue valueWithBytes:&next objCType:@encode(hp_input)];
            else [typed_held removeObjectForKey:key];
            halopad_host_post_input(&next);
        }];
    }
    [typed_queue addObject:[NSValue valueWithBytes:&e objCType:@encode(hp_input)]];
}

/* A character on a US keyboard: virtual key, scan code, whether Shift is held. */
static int us_key(unichar c, uint32_t *vk, uint32_t *scan, int *shift)
{
    static const char row1[] = "1234567890", qrow[] = "qwertyuiop", arow[] = "asdfghjkl", zrow[] = "zxcvbnm";
    static const char shifted1[] = "!@#$%^&*()";
    *shift = 0;
    if (c >= 'A' && c <= 'Z') { *shift = 1; c = c - 'A' + 'a'; }
    const char *p;
    if (c < 128 && (p = strchr(shifted1, (char)c)) && c) { *shift = 1; c = row1[p - shifted1]; }
    if (c < 128 && c && (p = strchr(row1, (char)c))) { *vk = (uint32_t)c; *scan = 0x02 + (uint32_t)(p - row1); return 1; }
    if (c < 128 && c && (p = strchr(qrow, (char)c))) { *vk = (uint32_t)(c - 32); *scan = 0x10 + (uint32_t)(p - qrow); return 1; }
    if (c < 128 && c && (p = strchr(arow, (char)c))) { *vk = (uint32_t)(c - 32); *scan = 0x1e + (uint32_t)(p - arow); return 1; }
    if (c < 128 && c && (p = strchr(zrow, (char)c))) { *vk = (uint32_t)(c - 32); *scan = 0x2c + (uint32_t)(p - zrow); return 1; }
    static const struct { unichar c; uint32_t vk, scan; int shift; } other[] = {
        {' ', 0x20, 0x39, 0}, {'.', 0xBE, 0x34, 0}, {',', 0xBC, 0x33, 0}, {';', 0xBA, 0x27, 0}, {':', 0xBA, 0x27, 1},
        {'-', 0xBD, 0x0c, 0}, {'_', 0xBD, 0x0c, 1}, {'=', 0xBB, 0x0d, 0}, {'+', 0xBB, 0x0d, 1}, {'/', 0xBF, 0x35, 0},
        {'?', 0xBF, 0x35, 1}, {'\'', 0xDE, 0x28, 0}, {'"', 0xDE, 0x28, 1}, {'[', 0xDB, 0x1a, 0}, {']', 0xDD, 0x1b, 0},
        {'\\', 0xDC, 0x2b, 0}, {'<', 0xBC, 0x33, 1}, {'>', 0xBE, 0x34, 1}, {'\n', 0x0D, 0x1c, 0}, {'\b', 0x08, 0x0e, 0}};
    for (unsigned i = 0; i < sizeof other / sizeof other[0]; i++)
        if (other[i].c == c) { *vk = other[i].vk; *scan = other[i].scan; *shift |= other[i].shift; return 1; }
    return 0;
}

/* ---- controls ---- */

typedef NS_ENUM(NSInteger, HPControlKind) { HPKey, HPMouseButton };

@interface HPStickView : UIView
@property(nonatomic, copy) void (^valueChanged)(float x, float y);
@property(nonatomic) BOOL editing;
@property(nonatomic, strong) UILabel *caption;
- (void)reset;
- (void)track:(UITouch *)t;
@end

@implementation HPStickView {
    UIView *_thumb;
    float _x, _y;
}
- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = NO;
        self.backgroundColor = [UIColor colorWithWhite:0.05 alpha:0.30];
        self.layer.borderColor = [UIColor colorWithWhite:1 alpha:0.30].CGColor;
        self.layer.borderWidth = 1.5;
        _thumb = [UIView new];
        _thumb.backgroundColor = [UIColor colorWithWhite:1 alpha:0.55];
        _thumb.userInteractionEnabled = NO;
        [self addSubview:_thumb];
        _caption = [UILabel new];
        _caption.textAlignment = NSTextAlignmentCenter;
        _caption.textColor = [UIColor colorWithWhite:1 alpha:0.65];
        _caption.font = [UIFont systemFontOfSize:10 weight:UIFontWeightSemibold];
        _caption.userInteractionEnabled = NO;
        [self addSubview:_caption];
    }
    return self;
}
- (void)layoutSubviews
{
    [super layoutSubviews];
    CGFloat side = fmin(self.bounds.size.width, self.bounds.size.height), t = side * 0.42;
    self.layer.cornerRadius = side / 2;
    _thumb.bounds = CGRectMake(0, 0, t, t);
    _thumb.layer.cornerRadius = t / 2;
    _caption.frame = CGRectMake(0, side - 23, side, 14);
    [self place];
}
- (void)place
{
    CGFloat half = self.bounds.size.width / 2, travel = half - _thumb.bounds.size.width / 2 - 3;
    _thumb.center = CGPointMake(half + _x * travel, half - _y * travel);
}
- (void)reset { _x = _y = 0; [self place]; if (self.valueChanged) self.valueChanged(0, 0); }
- (void)track:(UITouch *)t
{
    CGPoint p = [t locationInView:self];
    CGFloat r = fmax(1, fmin(self.bounds.size.width, self.bounds.size.height) / 2);
    CGFloat dx = (p.x - CGRectGetMidX(self.bounds)) / r, dy = (p.y - CGRectGetMidY(self.bounds)) / r, l = hypot(dx, dy);
    if (l > 1) { dx /= l; dy /= l; }
    _x = (float)dx; _y = (float)-dy;                    /* +y up, as SunPad */
    [self place];
    if (self.valueChanged) self.valueChanged(_x, _y);
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { if (!self.editing) [self track:touches.anyObject]; }
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { if (!self.editing) [self track:touches.anyObject]; }
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self reset]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self reset]; }
@end

/* A Halo control: a key or a mouse button, held while touched; a translucent glass circle with an
   SF Symbol and a small caption. Fire and grenade also look while the finger moves, as a mobile
   shooter's fire button does. */
@interface HPControlButton : UIView
@property(nonatomic) HPControlKind kind;
@property(nonatomic) uint32_t vk, side, scan, button;
@property(nonatomic) BOOL looks, held, primary;
@property(nonatomic, strong) UILabel *label;
@property(nonatomic, strong) UIImageView *icon;
@property(nonatomic, copy) void (^lookBy)(CGFloat dx, CGFloat dy);
@property(nonatomic) BOOL editing;
- (void)release_;
- (void)press:(int)down;
- (void)setSymbol:(NSString *)name;
@end

@implementation HPControlButton
- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = NO;
        self.layer.borderWidth = 1.5;
        _icon = [UIImageView new];
        _icon.contentMode = UIViewContentModeScaleAspectFit;
        _icon.tintColor = [UIColor colorWithWhite:1 alpha:0.95];
        _icon.layer.shadowColor = UIColor.blackColor.CGColor;
        _icon.layer.shadowOpacity = 0.55; _icon.layer.shadowRadius = 1.5; _icon.layer.shadowOffset = CGSizeMake(0, 1);
        [self addSubview:_icon];
        _label = [UILabel new];
        _label.textAlignment = NSTextAlignmentCenter;
        _label.textColor = [UIColor colorWithWhite:1 alpha:0.86];
        _label.layer.shadowColor = UIColor.blackColor.CGColor;
        _label.layer.shadowOpacity = 0.7; _label.layer.shadowRadius = 1; _label.layer.shadowOffset = CGSizeMake(0, 1);
        _label.adjustsFontSizeToFitWidth = YES;
        _label.minimumScaleFactor = 0.6;
        [self addSubview:_label];
        [self paint];
    }
    return self;
}
- (void)setSymbol:(NSString *)name { _icon.image = [UIImage systemImageNamed:name]; [self setNeedsLayout]; }
- (void)setPrimary:(BOOL)primary { _primary = primary; [self paint]; }
/* glass: dark and translucent at rest, brighter while held; the primary (FIRE) a muted red */
- (void)paint
{
    UIColor *rest = self.primary ? [UIColor colorWithRed:0.72 green:0.12 blue:0.16 alpha:0.50] : [UIColor colorWithWhite:0.05 alpha:0.42];
    UIColor *held = self.primary ? [UIColor colorWithRed:0.92 green:0.20 blue:0.24 alpha:0.75] : [UIColor colorWithWhite:1 alpha:0.30];
    self.backgroundColor = self.held ? held : rest;
    if (!self.editing) self.layer.borderColor = [UIColor colorWithWhite:1 alpha:self.held ? 0.8 : 0.32].CGColor;
}
- (void)layoutSubviews
{
    [super layoutSubviews];
    CGFloat d = fmin(self.bounds.size.width, self.bounds.size.height);
    self.layer.cornerRadius = d / 2;
    BOOL caption = self.label.text.length && d >= 44 && HPSettings.shared.showCaptions;
    CGFloat iconSide = d * (caption ? 0.40 : 0.50);
    _icon.frame = CGRectMake((self.bounds.size.width - iconSide) / 2, self.bounds.size.height / 2 - iconSide / 2 - (caption ? d * 0.08 : 0), iconSide, iconSide);
    _icon.preferredSymbolConfiguration = [UIImageSymbolConfiguration configurationWithPointSize:iconSide * 0.8 weight:UIImageSymbolWeightSemibold];
    _label.hidden = !caption;
    _label.font = [UIFont systemFontOfSize:fmax(10, d * 0.15) weight:UIFontWeightSemibold];
    _label.frame = CGRectMake(d * 0.12, CGRectGetMaxY(_icon.frame) + d * 0.02, self.bounds.size.width - d * 0.24, d * 0.2);
}
- (void)press:(int)down
{
    if (down == self.held) return;
    self.held = down;
    if (self.kind == HPMouseButton) post_button((int)self.button, down);
    else post_key(self.vk, self.side, self.scan, 0, down, 0);
    self.transform = down ? CGAffineTransformMakeScale(0.92, 0.92) : CGAffineTransformIdentity;
    [self paint];
}
- (void)release_ { [self press:0]; }
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { if (!self.editing) [self press:1]; }
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (self.editing || !self.looks || !self.lookBy) return;
    UITouch *t = touches.anyObject;
    CGPoint a = [t locationInView:self.superview], b = [t previousLocationInView:self.superview];
    self.lookBy(a.x - b.x, a.y - b.y);
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self press:0]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self press:0]; }
@end

/* Halo's PC bindings. Layout gives the sticks their own space and places actions in
   rows above/beside them. Size 0 is a utility, 1 an action, 2 the fire target. */
typedef struct {
    const char *ident, *caption, *symbol;
    HPControlKind kind;
    uint32_t vk, side, scan, button;
    int looks, size;
} hp_control_def;

static const hp_control_def CONTROLS[] = {
    {"fire",     "FIRE",   "scope",                    HPMouseButton, 0, 0, 0, 0,    1, 2},
    {"action",   "USE",    "hand.tap.fill",            HPKey, 'E', 0, 0x12, 0,       0, 1},
    {"switch",   "SWAP",   "arrow.left.arrow.right",   HPKey, 0x09, 0, 0x0f, 0,      0, 1},
    {"zoom",     "ZOOM",   "plus.magnifyingglass",     HPKey, 'Z', 0, 0x2c, 0,       0, 1},
    {"grenade",  "THROW",  "flame.fill",               HPMouseButton, 0, 0, 0, 1,    1, 1},
    {"melee",    "MELEE",  "hand.raised.fill",         HPKey, 'F', 0, 0x21, 0,       0, 1},
    {"reload",   "RELOAD", "arrow.clockwise",          HPKey, 'R', 0, 0x13, 0,       0, 1},
    {"crouch",   "CROUCH", "arrow.down.to.line",       HPKey, 0x11, 0xA2, 0x1d, 0,   0, 1},
    {"jump",     "JUMP",   "arrow.up",                 HPKey, 0x20, 0, 0x39, 0,      0, 1},
    {"flash",    "LIGHT",  "flashlight.on.fill",       HPKey, 'Q', 0, 0x10, 0,       0, 0},
    {"nadetype", "NADE",   "arrow.triangle.2.circlepath", HPKey, 'G', 0, 0x22, 0,    0, 0},
    {"scores",   "",       "list.number",              HPKey, 0x70, 0, 0x3b, 0,      0, 0},
    {"menu",     "",       "pause.fill",               HPKey, 0x1B, 0, 0x01, 0,      0, 0},
};
#define NCONTROLS (int)(sizeof CONTROLS / sizeof CONTROLS[0])

@interface HPOverlay () <UIGestureRecognizerDelegate>
@end

@implementation HPOverlay {
    UIButton *_menuButton;
    HPStickView *_move, *_aim;
    CADisplayLink *_aimClock;
    float _aimX, _aimY;
    NSMutableArray<HPControlButton *> *_buttons;
    NSMutableArray<UIGestureRecognizer *> *_editGestures;
    UILabel *_fps;
    UIView *_panel, *_editorBar;
    UISlider *_opacity, *_size, *_look, *_selectedSize;
    UISwitch *_hideSwitch, *_editSwitch, *_leftSwitch, *_captionSwitch;
    UISegmentedControl *_spacingControl;
    UILabel *_editorHint;
    __weak UIView *_selected;
    BOOL _editing, _controllerHidden;
    int _wasd[4];                                   /* W A S D held */
    double _lookRestX, _lookRestY;
    NSMutableSet<UITouch *> *_lookTouches;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = YES;
        _lookTouches = [NSMutableSet set];
        [self buildControls];
        [self buildMenuButton];
        [self buildPanel];
        [self buildEditorBar];
        _fps = [UILabel new];
        _fps.font = [UIFont monospacedDigitSystemFontOfSize:13 weight:UIFontWeightSemibold];
        _fps.textColor = UIColor.greenColor;
        _fps.backgroundColor = [UIColor colorWithWhite:0 alpha:0.5];
        _fps.textAlignment = NSTextAlignmentCenter;
        _fps.hidden = YES;
        _fps.userInteractionEnabled = NO;
        [self addSubview:_fps];
        for (NSString *n in @[GCControllerDidConnectNotification, GCControllerDidDisconnectNotification])
            [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(refreshControllerVisibility) name:n object:nil];
        [self refreshControllerVisibility];
    }
    return self;
}
- (void)dealloc { [NSNotificationCenter.defaultCenter removeObserver:self]; }

/* ---- the controls ---- */

- (void)buildControls
{
    _buttons = [NSMutableArray array];
    _editGestures = [NSMutableArray array];
    _move = [HPStickView new];
    _move.accessibilityIdentifier = @"move";
    _move.accessibilityLabel = @"Move";
    __weak HPOverlay *weak = self;
    _move.valueChanged = ^(float x, float y) { [weak moveX:x y:y]; };
    [self addSubview:_move];
    [self addEditGestures:_move];
    _move.caption.text = @"MOVE";
    _aim = [HPStickView new];
    _aim.accessibilityIdentifier = @"aim";
    _aim.accessibilityLabel = @"Look";
    _aim.caption.text = @"LOOK";
    _aim.valueChanged = ^(float x, float y) { [weak aimX:x y:y]; };
    [self addSubview:_aim];
    [self addEditGestures:_aim];
    for (int i = 0; i < NCONTROLS; i++) {
        const hp_control_def *d = &CONTROLS[i];
        HPControlButton *b = [HPControlButton new];
        b.kind = d->kind; b.vk = d->vk; b.side = d->side; b.scan = d->scan; b.button = d->button; b.looks = d->looks;
        b.label.text = @(d->caption);
        [b setSymbol:@(d->symbol)];
        b.primary = d->size == 2;
        b.accessibilityIdentifier = @(d->ident);
        b.accessibilityLabel = strlen(d->caption) ? @(d->caption).capitalizedString : @(d->ident).capitalizedString;
        b.lookBy = ^(CGFloat dx, CGFloat dy) { [weak lookX:dx y:dy]; };
        [_buttons addObject:b];
        [self addSubview:b];
        [self addEditGestures:b];
    }
}

/* the move stick as Halo's W A S D (digital, as Halo's keyboard movement is), with hysteresis */
- (void)moveX:(float)x y:(float)y
{
    if (_editing) return;
    static const uint32_t vk[4] = {'W', 'A', 'S', 'D'}, scan[4] = {0x11, 0x1e, 0x1f, 0x20};
    float v[4] = {y, -x, -y, x};
    for (int i = 0; i < 4; i++) {
        int want = _wasd[i] ? v[i] > 0.25f : v[i] > 0.38f;
        if (want != _wasd[i]) { _wasd[i] = want; post_key(vk[i], 0, scan[i], 0, want, 0); }
    }
}
/* looking: points dragged as mouse counts (DirectInput), fractions kept */
- (void)lookX:(CGFloat)dx y:(CGFloat)dy
{
    double s = 2.2 * HPSettings.shared.lookSensitivity;
    _lookRestX += dx * s; _lookRestY += dy * s;
    int32_t ix = (int32_t)_lookRestX, iy = (int32_t)_lookRestY;
    _lookRestX -= ix; _lookRestY -= iy;
    if (ix || iy) post_mouse(ix, iy);
}
- (void)clearTouchInput
{
    for (HPControlButton *b in _buttons) [b release_];
    [_move reset];
    [_aim reset];
    [_lookTouches removeAllObjects];
    _lookRestX = _lookRestY = 0;
}

- (BOOL)driveControl:(NSString *)identifier down:(BOOL)down
{
    for (HPControlButton *b in _buttons)
        if ([b.accessibilityIdentifier isEqualToString:identifier]) { [b press:down]; return YES; }
    return NO;
}
- (void)driveMoveX:(float)x y:(float)y { if (_move.valueChanged) _move.valueChanged(x, y); }
- (void)driveLookX:(CGFloat)dx y:(CGFloat)dy { [self lookX:dx y:dy]; }

/* The fixed sticks own their touches independently. Swiping open screen or dragging
   FIRE can still aim, so the right thumb can shoot and turn without leaving FIRE. */
- (void)aimX:(float)x y:(float)y
{
    if (_editing) return;
    _aimX = x; _aimY = y;
    if (hypotf(x, y) <= 0.12f) {
        [_aimClock invalidate]; _aimClock = nil;
    } else if (!_aimClock) {
        _aimClock = [CADisplayLink displayLinkWithTarget:self selector:@selector(aimTick:)];
        [_aimClock addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    }
}
- (void)aimTick:(CADisplayLink *)clock
{
    float magnitude = hypotf(_aimX, _aimY);
    if (magnitude <= 0.12f) return;
    /* Radial dead zone, gentle near-centre aim, speed independent of refresh rate. */
    double response = pow(fmin(1, (magnitude - 0.12) / 0.88), 1.5);
    double dt = fmin(0.05, fmax(0, clock.targetTimestamp - clock.timestamp));
    double speed = 250 * response * dt / magnitude;
    [self lookX:_aimX * speed y:-_aimY * speed];
}
- (void)driveAimX:(float)x y:(float)y { if (_aim.valueChanged) _aim.valueChanged(x, y); }

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
    UIView *hit = [super hitTest:point withEvent:event];
    if (!_panel.hidden && hit && hit != _menuButton && ![hit isDescendantOfView:_menuButton] &&
        hit != _panel && ![hit isDescendantOfView:_panel]) return self;
    if (hit != self) return hit;
    return self.inGame && !self.controlsHidden && !_editing ? self : nil;
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    if (!_panel.hidden) return;
    [_lookTouches unionSet:touches];
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches) {
        if (![_lookTouches containsObject:t]) continue;
        CGPoint a = [t locationInView:self], b = [t previousLocationInView:self];
        [self lookX:a.x - b.x y:a.y - b.y];
    }
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [_lookTouches minusSet:touches]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [_lookTouches minusSet:touches]; }

- (BOOL)controlsHidden { return HPSettings.shared.hideTouchControls || _controllerHidden; }
- (void)setInGame:(BOOL)inGame
{
    if (_inGame == inGame) return;
    _inGame = inGame;
    if (!inGame) [self clearTouchInput];
    [self updateAppearance];
}

- (void)refreshControllerVisibility
{
    BOOL connected = NO;
#if !TARGET_OS_SIMULATOR
    for (GCController *c in GCController.controllers) if (c.extendedGamepad) connected = YES;   /* only real controllers, as SunPad */
#endif
    _controllerHidden = connected && HPSettings.shared.hideWithController;
    if (connected) [self clearTouchInput];
    [self updateAppearance];
}

- (void)setFramesPerSecond:(int)fps
{
    _fps.hidden = fps < 0 || !HPSettings.shared.showFPS;
    _fps.text = [NSString stringWithFormat:@"%d FPS", fps];
}

/* ---- layout: point-sized targets constrained together to the safe area, with
   separate saved phone/tablet positions and sizes ---- */

- (BOOL)phone { return self.traitCollection.userInterfaceIdiom == UIUserInterfaceIdiomPhone; }
- (NSString *)key:(NSString *)what { return [NSString stringWithFormat:@"HaloPad.%@.v3.%@", self.phone ? @"phone" : @"tablet", what]; }
- (CGRect)safe { return UIEdgeInsetsInsetRect(self.bounds, self.safeAreaInsets); }

- (void)place:(UIView *)v frame:(CGRect)f
{
    NSString *ident = v.accessibilityIdentifier;
    NSNumber *scale = [NSUserDefaults.standardUserDefaults dictionaryForKey:[self key:@"scales"]][ident];
    CGFloat s = scale ? scale.doubleValue : 1;
    v.bounds = CGRectMake(0, 0, f.size.width * s, f.size.height * s);
    NSString *saved = [NSUserDefaults.standardUserDefaults dictionaryForKey:[self key:@"origins"]][ident];
    CGRect safe = self.safe;
    CGPoint c = CGPointMake(CGRectGetMidX(f), CGRectGetMidY(f));
    if (saved) {
        CGPoint n = CGPointFromString(saved);
        c = CGPointMake(CGRectGetMinX(safe) + fmin(1, fmax(0, n.x)) * safe.size.width, CGRectGetMinY(safe) + fmin(1, fmax(0, n.y)) * safe.size.height);
    }
    CGFloat hw = v.bounds.size.width / 2, hh = v.bounds.size.height / 2;
    c.x = fmin(fmax(c.x, CGRectGetMinX(safe) + hw), CGRectGetMaxX(safe) - hw);
    c.y = fmin(fmax(c.y, CGRectGetMinY(safe) + hh), CGRectGetMaxY(safe) - hh);
    v.center = c;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    CGRect safe = self.safe;
    BOOL pad = !self.phone && safe.size.width >= 1000;
    /* Bound the whole arrangement before placing individual controls. Clamping each
       circle against an edge was what collapsed the old ring at larger sizes. */
    CGFloat k = fmin((pad ? 1.18 : 1) * HPSettings.shared.controlSize,
                     fmin(safe.size.width / 760, (safe.size.height - 68) / 260));
    k = fmax(0.65, k);
    CGFloat stick = fmax(100, 128 * k), action = fmax(44, 52 * k);
    CGFloat utility = fmax(44, 44 * k), fire = fmax(60, 72 * k);
    CGFloat gaps[] = {10, 16, 24};
    CGFloat gap = gaps[HPSettings.shared.ringSpacing] * k;
    CGFloat inset = fmax(16, 24 * k);
    CGFloat edge = fmax(inset, safe.size.width * (self.phone ? 0.17 : 0.20) - stick / 2);
    CGFloat left = CGRectGetMinX(safe) + edge;
    CGFloat right = CGRectGetMaxX(safe) - edge;
    CGFloat bottom = CGRectGetMaxY(safe) - inset;
    CGFloat rowY = bottom - stick - gap - fire / 2;
    CGFloat nearX = right - stick - gap - action / 2;
    CGFloat farX = nearX - action - gap;
    CGFloat lowY = bottom - action / 2, midY = lowY - action - gap;
    CGFloat fireX = right - fire / 2;
    CGFloat reloadX = fireX - fire / 2 - gap - action / 2;
    CGFloat zoomX = reloadX - action - gap, swapX = zoomX - action - gap;
    BOOL mirror = HPSettings.shared.leftHanded;
    CGFloat (^mx)(CGFloat) = ^CGFloat(CGFloat x) { return mirror ? CGRectGetMinX(safe) + CGRectGetMaxX(safe) - x : x; };
    [self place:_move frame:CGRectMake(mx(left + stick / 2) - stick / 2, bottom - stick, stick, stick)];
    [self place:_aim frame:CGRectMake(mx(right - stick / 2) - stick / 2, bottom - stick, stick, stick)];
    CGPoint points[NCONTROLS] = {
        {fireX, rowY}, {farX, midY}, {swapX, rowY}, {zoomX, rowY},
        {left + fire / 2, rowY}, {nearX, midY}, {reloadX, rowY},
        {farX, lowY}, {nearX, lowY},
        {left + fire + gap + utility / 2 + utility + gap, rowY},
        {left + fire + gap + utility / 2, rowY},
        {CGRectGetMidX(safe) - utility / 2 - 6, CGRectGetMinY(safe) + utility / 2 + 12},
        {CGRectGetMidX(safe) + utility / 2 + 6, CGRectGetMinY(safe) + utility / 2 + 12},
    };
    for (int i = 0; i < NCONTROLS; i++) {
        CGFloat dia = CONTROLS[i].size == 2 ? fire : CONTROLS[i].size == 1 ? action : utility;
        CGPoint c = points[i];
        if (i < NCONTROLS - 2) c.x = mx(c.x);
        [self place:_buttons[i] frame:CGRectMake(c.x - dia / 2, c.y - dia / 2, dia, dia)];
        [_buttons[i] setNeedsLayout];
    }
    CGFloat side = 44; inset = 12;
    _menuButton.frame = CGRectMake(CGRectGetMaxX(safe) - side - inset, CGRectGetMinY(safe) + inset, side, side);
    _fps.frame = CGRectMake(CGRectGetMaxX(safe) - side - inset - 86, CGRectGetMinY(safe) + inset + 8, 76, 24);
    _fps.layer.cornerRadius = 6; _fps.layer.masksToBounds = YES;
    CGFloat pw = fmin(360, safe.size.width - 32), ph = fmin(560, fmax(0, safe.size.height - 72));
    _panel.frame = CGRectMake(CGRectGetMaxX(safe) - pw - 12, CGRectGetMinY(safe) + 60, pw, ph);
    CGFloat ew = fmin(560, safe.size.width - 24);
    _editorBar.frame = CGRectMake(CGRectGetMidX(safe) - ew / 2, CGRectGetMaxY(safe) - 72, ew, 60);
    [self updateAppearance];
    [self bringSubviewToFront:_panel];
    [self bringSubviewToFront:_editorBar];
    [self bringSubviewToFront:_menuButton];
}

- (void)updateAppearance
{
    BOOL show = _editing || (self.inGame && !self.controlsHidden);
    CGFloat alpha = HPSettings.shared.controlOpacity;
    NSMutableArray<UIView *> *all = [NSMutableArray arrayWithArray:_buttons];
    [all addObjectsFromArray:@[_move, _aim]];
    for (UIView *v in all) {
        v.hidden = !show;
        BOOL picked = _editing && v == _selected;
        v.alpha = [v isKindOfClass:HPStickView.class] && !_editing ? alpha * 0.75 : alpha;
        if ([v isKindOfClass:HPStickView.class]) ((HPStickView *)v).editing = _editing;
        v.layer.borderWidth = picked ? 3 : 1.5;
        v.layer.borderColor = (picked ? UIColor.systemYellowColor : [UIColor colorWithWhite:1 alpha:0.32]).CGColor;
        if ([v isKindOfClass:HPControlButton.class]) ((HPControlButton *)v).editing = _editing;
    }
}

/* ---- the three-dot menu ---- */

- (void)buildMenuButton
{
    _menuButton = [UIButton buttonWithType:UIButtonTypeCustom];
    UIImage *dots = [UIImage systemImageNamed:@"ellipsis" withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:19 weight:UIImageSymbolWeightBold]];
    [_menuButton setImage:dots forState:UIControlStateNormal];
    _menuButton.tintColor = UIColor.whiteColor;
    _menuButton.backgroundColor = [UIColor colorWithWhite:0.06 alpha:0.72];
    _menuButton.layer.cornerRadius = 22;
    _menuButton.layer.borderWidth = 1;
    _menuButton.layer.borderColor = [UIColor colorWithWhite:1 alpha:0.3].CGColor;
    _menuButton.layer.masksToBounds = YES;
    _menuButton.accessibilityLabel = @"Menu";
    _menuButton.accessibilityIdentifier = @"HaloPadMenu";
    _menuButton.showsMenuAsPrimaryAction = YES;
    [self addSubview:_menuButton];
    [self rebuildMenu];
}

- (UIAction *)check:(NSString *)title on:(BOOL)on handler:(void (^)(void))h
{
    UIAction *a = [UIAction actionWithTitle:title image:nil identifier:nil handler:^(__kindof UIAction *x) { h(); }];
    a.state = on ? UIMenuElementStateOn : UIMenuElementStateOff;
    return a;
}

- (void)rebuildMenu
{
    __weak HPOverlay *weak = self;
    HPSettings *s = HPSettings.shared;
    NSMutableArray<UIMenuElement *> *recent = [NSMutableArray array];
    for (NSString *addr in s.recentServers)
        [recent addObject:[UIAction actionWithTitle:addr image:nil identifier:nil handler:^(__kindof UIAction *a) { [weak joinServer:addr password:@""]; }]];
    UIMenu *online = [UIMenu menuWithTitle:@"" image:nil identifier:nil options:UIMenuOptionsDisplayInline children:@[
        [UIAction actionWithTitle:@"Join Server by Address…" image:[UIImage systemImageNamed:@"network"] identifier:nil
                          handler:^(__kindof UIAction *a) { [weak promptJoin]; }],
        [UIMenu menuWithTitle:@"Recent Servers" image:[UIImage systemImageNamed:@"clock"] identifier:nil options:0
                     children:recent.count ? recent : @[[UIAction actionWithTitle:@"None yet" image:nil identifier:nil handler:^(__kindof UIAction *a) {}]]]]];
    UIMenu *text = [UIMenu menuWithTitle:@"" image:nil identifier:nil options:UIMenuOptionsDisplayInline children:@[
        [UIAction actionWithTitle:@"Show Keyboard" image:[UIImage systemImageNamed:@"keyboard"] identifier:nil
                          handler:^(__kindof UIAction *a) { [weak.delegate overlayRequestsKeyboard:weak]; }],
        [UIAction actionWithTitle:@"Halo Console" image:[UIImage systemImageNamed:@"terminal"] identifier:nil
                          handler:^(__kindof UIAction *a) { [HPOverlay tapKey:0xC0 scan:0x29]; }],
        [UIAction actionWithTitle:@"Team Chat" image:[UIImage systemImageNamed:@"bubble.left"] identifier:nil
                          handler:^(__kindof UIAction *a) { [HPOverlay tapKey:'Y' scan:0x15]; [weak.delegate overlayRequestsKeyboard:weak]; }],
        [UIAction actionWithTitle:@"All Chat" image:[UIImage systemImageNamed:@"bubble.left.and.bubble.right"] identifier:nil
                          handler:^(__kindof UIAction *a) { [HPOverlay tapKey:'T' scan:0x14]; [weak.delegate overlayRequestsKeyboard:weak]; }]]];
    UIMenu *display = [UIMenu menuWithTitle:@"Display" image:[UIImage systemImageNamed:@"display"] identifier:nil options:0 children:@[
        [UIMenu menuWithTitle:@"Aspect Ratio" image:nil identifier:nil options:UIMenuOptionsDisplayInline children:@[
            [self check:@"Original 4:3" on:s.aspect == HPAspectOriginal handler:^{ HPSettings.shared.aspect = HPAspectOriginal; [weak displayChanged]; }],
            [self check:@"Stretch to Fill" on:s.aspect == HPAspectFill handler:^{ HPSettings.shared.aspect = HPAspectFill; [weak displayChanged]; }]]],
        [self check:@"Show FPS Counter" on:s.showFPS handler:^{ HPSettings.shared.showFPS = !HPSettings.shared.showFPS; [weak displayChanged]; }]]];
    UIMenu *controls = [UIMenu menuWithTitle:@"" image:nil identifier:nil options:UIMenuOptionsDisplayInline children:@[
        [UIAction actionWithTitle:@"Touch Control Settings…" image:[UIImage systemImageNamed:@"slider.horizontal.3"] identifier:nil
                          handler:^(__kindof UIAction *a) { [weak togglePanel]; }],
        [self check:@"Hide Touch Controls" on:s.hideTouchControls handler:^{
            HPSettings.shared.hideTouchControls = !HPSettings.shared.hideTouchControls; [weak clearTouchInput]; [weak updateAppearance]; [weak rebuildMenu]; }]]];
    UIAction *report = [UIAction actionWithTitle:@"Report a Problem…" image:[UIImage systemImageNamed:@"exclamationmark.bubble"] identifier:nil
                                         handler:^(__kindof UIAction *a) { [weak report]; }];
    _menuButton.menu = [UIMenu menuWithTitle:@"HaloPad" children:@[online, text, display, controls, report]];
}

- (void)displayChanged
{
    [self.delegate overlayDisplayChanged:self];
    [self rebuildMenu];
    [self setNeedsLayout];
}

- (UIViewController *)presenter
{
    UIViewController *vc = self.window.rootViewController;
    while (vc.presentedViewController) vc = vc.presentedViewController;
    return vc;
}

/* Halo's own console command: connect <address> [password] (the same as the -connect switch) */
- (void)promptJoin
{
    UIAlertController *a = [UIAlertController alertControllerWithTitle:@"Join Server"
                                                               message:@"The server's address and port, for example 203.0.113.5:2302. Halo connects through its console command \"connect\"."
                                                        preferredStyle:UIAlertControllerStyleAlert];
    [a addTextFieldWithConfigurationHandler:^(UITextField *f) {
        f.placeholder = @"address:port";
        f.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
        f.autocorrectionType = UITextAutocorrectionTypeNo;
        f.autocapitalizationType = UITextAutocapitalizationTypeNone;
        f.text = HPSettings.shared.recentServers.firstObject;
    }];
    [a addTextFieldWithConfigurationHandler:^(UITextField *f) { f.placeholder = @"password (optional)"; f.secureTextEntry = YES; }];
    __weak HPOverlay *weak = self;
    __weak UIAlertController *wa = a;
    [a addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [a addAction:[UIAlertAction actionWithTitle:@"Join" style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) {
        NSString *addr = [wa.textFields[0].text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        if (addr.length) [weak joinServer:addr password:wa.textFields[1].text ?: @""];
    }]];
    [self.presenter presentViewController:a animated:YES completion:nil];
}

/* Halo's console: connect <address> <password> (both arguments are required; an empty password is "") */
- (void)joinServer:(NSString *)addr password:(NSString *)pw
{
    NSMutableArray *recent = [HPSettings.shared.recentServers mutableCopy];
    [recent removeObject:addr];
    [recent insertObject:addr atIndex:0];
    while (recent.count > 6) [recent removeLastObject];
    HPSettings.shared.recentServers = recent;
    [self rebuildMenu];
    [HPOverlay tapKey:0xC0 scan:0x29];                              /* open Halo's console */
    NSString *quoted = [pw stringByReplacingOccurrencesOfString:@"\"" withString:@""];
    [HPOverlay typeText:[NSString stringWithFormat:@"connect %@ \"%@\"\n", addr, quoted]];
    [HPOverlay tapKey:0xC0 scan:0x29];                              /* and close it: in a game its keys would type there */
}

- (void)report
{
    NSString *body = [NSString stringWithFormat:@"HaloPad problem report\n\n%@\n\nWhat happened:\n", [self.delegate overlayDiagnostics:self]];
    UIActivityViewController *share = [[UIActivityViewController alloc] initWithActivityItems:@[body] applicationActivities:nil];
    share.popoverPresentationController.sourceView = _menuButton;
    share.popoverPresentationController.sourceRect = _menuButton.bounds;
    [self.presenter presentViewController:share animated:YES completion:nil];
}

+ (void)setTextInputActive:(BOOL)active
{
    NSAssert(NSThread.isMainThread, @"Text input belongs to UIKit's main thread");
    typed_active = active;
    if (active) return;
    [typed_queue removeAllObjects];
    /* A canceled Shift-up (or ordinary key-up) must not leave that key held. These
       releases precede the scene's deactivation event in the host queue. */
    for (NSValue *value in typed_held.allValues) {
        hp_input up;
        [value getValue:&up size:sizeof up];
        up.down = 0;
        up.nchars = 0;
        memset(up.chars, 0, sizeof up.chars);
        halopad_host_post_input(&up);
    }
    [typed_held removeAllObjects];
}

+ (void)tapKey:(uint32_t)vk scan:(uint32_t)scan
{
    queue_key(vk, 0, scan, 1, 0);
    queue_key(vk, 0, scan, 0, 0);
}

+ (void)typeText:(NSString *)text
{
    for (NSUInteger i = 0; i < text.length; i++) {
        unichar c = [text characterAtIndex:i];
        uint32_t vk, scan; int shift;
        if (!us_key(c, &vk, &scan, &shift)) continue;
        if (c == '\n') c = '\r';
        if (shift) queue_key(0x10, 0xA0, 0x2a, 1, 0);
        queue_key(vk, 0, scan, 1, c == '\b' ? 0x08 : c);
        queue_key(vk, 0, scan, 0, 0);
        if (shift) queue_key(0x10, 0xA0, 0x2a, 0, 0);
    }
}

/* ---- the settings panel (SunPad's: opacity, size, hide with a controller, edit, reset) ---- */

- (UIView *)row:(NSString *)title control:(UIView *)c
{
    UILabel *l = [UILabel new];
    l.text = title;
    l.textColor = UIColor.whiteColor;
    l.font = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
    [l setContentHuggingPriority:UILayoutPriorityDefaultHigh forAxis:UILayoutConstraintAxisHorizontal];
    [c setContentHuggingPriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisHorizontal];
    UIStackView *r = [[UIStackView alloc] initWithArrangedSubviews:@[l, c]];
    r.spacing = 12;
    r.alignment = UIStackViewAlignmentCenter;
    return r;
}
- (UISlider *)slider:(float)lo max:(float)hi action:(SEL)sel
{
    UISlider *s = [UISlider new];
    s.minimumValue = lo; s.maximumValue = hi;
    [s addTarget:self action:sel forControlEvents:UIControlEventValueChanged];
    return s;
}
- (void)buildPanel
{
    _panel = [UIView new];
    _panel.backgroundColor = [UIColor colorWithWhite:0.035 alpha:0.94];
    _panel.layer.cornerRadius = 16;
    _panel.clipsToBounds = YES;
    _panel.layer.borderWidth = 1;
    _panel.layer.borderColor = [UIColor colorWithWhite:1 alpha:0.18].CGColor;
    _panel.hidden = YES;
    _panel.accessibilityIdentifier = @"TouchControlSettings";
    UILabel *title = [UILabel new];
    title.text = @"Touch Controls";
    title.textColor = UIColor.whiteColor;
    title.font = [UIFont systemFontOfSize:17 weight:UIFontWeightBold];
    _opacity = [self slider:0.25 max:1 action:@selector(opacityChanged:)];
    _size = [self slider:0.7 max:1.35 action:@selector(sizeChanged:)];
    _look = [self slider:0.25 max:3 action:@selector(lookChanged:)];
    _hideSwitch = [UISwitch new];
    [_hideSwitch addTarget:self action:@selector(hideChanged:) forControlEvents:UIControlEventValueChanged];
    _editSwitch = [UISwitch new];
    [_editSwitch addTarget:self action:@selector(editChanged:) forControlEvents:UIControlEventValueChanged];
    _leftSwitch = [UISwitch new];
    [_leftSwitch addTarget:self action:@selector(leftChanged:) forControlEvents:UIControlEventValueChanged];
    _captionSwitch = [UISwitch new];
    [_captionSwitch addTarget:self action:@selector(captionChanged:) forControlEvents:UIControlEventValueChanged];
    _spacingControl = [[UISegmentedControl alloc] initWithItems:@[@"Compact", @"Normal", @"Spread"]];
    [_spacingControl addTarget:self action:@selector(spacingChanged:) forControlEvents:UIControlEventValueChanged];
    _spacingControl.selectedSegmentTintColor = [UIColor colorWithWhite:1 alpha:0.9];
    _spacingControl.backgroundColor = [UIColor colorWithWhite:1 alpha:0.12];
    [_spacingControl setTitleTextAttributes:@{NSForegroundColorAttributeName: UIColor.whiteColor} forState:UIControlStateNormal];
    [_spacingControl setTitleTextAttributes:@{NSForegroundColorAttributeName: UIColor.blackColor} forState:UIControlStateSelected];
    UIButton *reset = [UIButton buttonWithType:UIButtonTypeSystem];
    [reset setTitle:@"Reset This Device Layout" forState:UIControlStateNormal];
    [reset setTitleColor:UIColor.systemRedColor forState:UIControlStateNormal];
    [reset addTarget:self action:@selector(confirmReset) forControlEvents:UIControlEventPrimaryActionTriggered];
    UIButton *done = [UIButton buttonWithType:UIButtonTypeSystem];
    [done setTitle:@"Done" forState:UIControlStateNormal];
    done.titleLabel.font = [UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];
    [done addTarget:self action:@selector(togglePanel) forControlEvents:UIControlEventPrimaryActionTriggered];
    UIStackView *header = [[UIStackView alloc] initWithArrangedSubviews:@[title, done]];
    header.alignment = UIStackViewAlignmentCenter;
    header.translatesAutoresizingMaskIntoConstraints = NO;
    [done setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
    [done.heightAnchor constraintGreaterThanOrEqualToConstant:44].active = YES;
    [_panel addSubview:header];
    UIScrollView *scroll = [UIScrollView new];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    scroll.alwaysBounceVertical = YES;
    scroll.indicatorStyle = UIScrollViewIndicatorStyleWhite;
    scroll.accessibilityIdentifier = @"TouchControlSettingsScroll";
    [_panel addSubview:scroll];
    UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:@[
        [self row:@"Opacity" control:_opacity], [self row:@"Size" control:_size], [self row:@"Look Speed" control:_look],
        [self row:@"Left-handed" control:_leftSwitch], [self row:@"Button Labels" control:_captionSwitch],
        [self row:@"Spacing" control:_spacingControl],
        [self row:@"Hide with a Controller" control:_hideSwitch], [self row:@"Move Controls" control:_editSwitch], reset]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 14;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    for (UIView *row in stack.arrangedSubviews)
        [row.heightAnchor constraintGreaterThanOrEqualToConstant:44].active = YES;
    [scroll addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [header.leadingAnchor constraintEqualToAnchor:_panel.leadingAnchor constant:18],
        [header.trailingAnchor constraintEqualToAnchor:_panel.trailingAnchor constant:-18],
        [header.topAnchor constraintEqualToAnchor:_panel.topAnchor constant:8],
        [scroll.topAnchor constraintEqualToAnchor:header.bottomAnchor constant:8],
        [scroll.leadingAnchor constraintEqualToAnchor:_panel.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:_panel.trailingAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:_panel.bottomAnchor],
        [stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:18],
        [stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-18],
        [stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor],
        [stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-16],
        [stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-36]]];
    [self addSubview:_panel];
}
- (void)togglePanel
{
    if (_editing) [self endEditing];
    _panel.hidden = !_panel.hidden;
    if (!_panel.hidden) [self clearTouchInput];
    HPSettings *s = HPSettings.shared;
    _opacity.value = s.controlOpacity; _size.value = s.controlSize; _look.value = s.lookSensitivity;
    _hideSwitch.on = s.hideWithController; _editSwitch.on = NO;
    _leftSwitch.on = s.leftHanded; _captionSwitch.on = s.showCaptions; _spacingControl.selectedSegmentIndex = s.ringSpacing;
    [self bringSubviewToFront:_panel];
}
- (void)opacityChanged:(UISlider *)s { HPSettings.shared.controlOpacity = s.value; [self updateAppearance]; }
- (void)sizeChanged:(UISlider *)s { HPSettings.shared.controlSize = s.value; [self setNeedsLayout]; }
- (void)lookChanged:(UISlider *)s { HPSettings.shared.lookSensitivity = s.value; }
- (void)hideChanged:(UISwitch *)s { HPSettings.shared.hideWithController = s.on; [self refreshControllerVisibility]; }
- (void)editChanged:(UISwitch *)s { if (s.on) [self beginEditing]; else [self endEditing]; }
- (void)leftChanged:(UISwitch *)s { HPSettings.shared.leftHanded = s.on; [self clearTouchInput]; [self setNeedsLayout]; }
- (void)captionChanged:(UISwitch *)s { HPSettings.shared.showCaptions = s.on; for (UIView *b in _buttons) [b setNeedsLayout]; }
- (void)spacingChanged:(UISegmentedControl *)c { HPSettings.shared.ringSpacing = c.selectedSegmentIndex; [self setNeedsLayout]; }
- (void)confirmReset
{
    UIAlertController *a = [UIAlertController alertControllerWithTitle:@"Reset Touch Control Layout?"
                                                               message:@"All control positions and sizes on this kind of device return to their defaults."
                                                        preferredStyle:UIAlertControllerStyleAlert];
    __weak HPOverlay *weak = self;
    [a addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [a addAction:[UIAlertAction actionWithTitle:@"Reset" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *x) {
        [NSUserDefaults.standardUserDefaults removeObjectForKey:[weak key:@"origins"]];
        [NSUserDefaults.standardUserDefaults removeObjectForKey:[weak key:@"scales"]];
        [weak setNeedsLayout];
    }]];
    [self.presenter presentViewController:a animated:YES completion:nil];
}

/* ---- the layout editor: drag to move, tap to select, a size slider, Done ---- */

- (void)buildEditorBar
{
    _editorBar = [UIView new];
    _editorBar.backgroundColor = [UIColor colorWithWhite:0.035 alpha:0.94];
    _editorBar.layer.cornerRadius = 14;
    _editorBar.hidden = YES;
    _editorHint = [UILabel new];
    _editorHint.textColor = UIColor.whiteColor;
    _editorHint.font = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
    _editorHint.text = @"Drag controls • tap one to resize";
    _selectedSize = [self slider:0.6 max:1.75 action:@selector(selectedSizeChanged:)];
    _selectedSize.enabled = NO;
    UIButton *done = [UIButton buttonWithType:UIButtonTypeSystem];
    [done setTitle:@"Done" forState:UIControlStateNormal];
    done.titleLabel.font = [UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];
    [done addTarget:self action:@selector(endEditing) forControlEvents:UIControlEventPrimaryActionTriggered];
    UIStackView *r = [[UIStackView alloc] initWithArrangedSubviews:@[_editorHint, _selectedSize, done]];
    r.spacing = 14;
    r.alignment = UIStackViewAlignmentCenter;
    r.translatesAutoresizingMaskIntoConstraints = NO;
    [_editorBar addSubview:r];
    [NSLayoutConstraint activateConstraints:@[
        [r.leadingAnchor constraintEqualToAnchor:_editorBar.leadingAnchor constant:16],
        [r.trailingAnchor constraintEqualToAnchor:_editorBar.trailingAnchor constant:-16],
        [r.centerYAnchor constraintEqualToAnchor:_editorBar.centerYAnchor],
        [_selectedSize.widthAnchor constraintGreaterThanOrEqualToConstant:140]]];
    [self addSubview:_editorBar];
}
- (void)addEditGestures:(UIView *)v
{
    UIPanGestureRecognizer *drag = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(dragged:)];
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapped:)];
    drag.enabled = tap.enabled = NO;
    [v addGestureRecognizer:drag];
    [v addGestureRecognizer:tap];
    [_editGestures addObjectsFromArray:@[drag, tap]];
}
- (void)beginEditing
{
    [self clearTouchInput];
    _editing = YES;
    _panel.hidden = YES;
    _editorBar.hidden = NO;
    _selected = nil;
    _selectedSize.enabled = NO;
    for (UIGestureRecognizer *g in _editGestures) g.enabled = YES;
    [self updateAppearance];
}
- (void)endEditing
{
    _editing = NO;
    _editorBar.hidden = YES;
    _editSwitch.on = NO;
    for (UIGestureRecognizer *g in _editGestures) g.enabled = NO;
    _selected = nil;
    [self setNeedsLayout];
    [self clearTouchInput];
    [self updateAppearance];
}
- (void)select:(UIView *)v
{
    _selected = v;
    NSNumber *s = [NSUserDefaults.standardUserDefaults dictionaryForKey:[self key:@"scales"]][v.accessibilityIdentifier];
    _selectedSize.value = s ? s.floatValue : 1;
    _selectedSize.enabled = YES;
    _editorHint.text = [NSString stringWithFormat:@"%@ size", v.accessibilityLabel];
    [self updateAppearance];
}
- (void)tapped:(UITapGestureRecognizer *)t { if (t.state == UIGestureRecognizerStateEnded) [self select:t.view]; }
- (void)dragged:(UIPanGestureRecognizer *)g
{
    UIView *v = g.view;
    if (g.state == UIGestureRecognizerStateBegan) [self select:v];
    CGPoint d = [g translationInView:self];
    [g setTranslation:CGPointZero inView:self];
    CGRect safe = self.safe;
    CGFloat hw = v.bounds.size.width / 2, hh = v.bounds.size.height / 2;
    v.center = CGPointMake(fmin(fmax(v.center.x + d.x, CGRectGetMinX(safe) + hw), CGRectGetMaxX(safe) - hw),
                           fmin(fmax(v.center.y + d.y, CGRectGetMinY(safe) + hh), CGRectGetMaxY(safe) - hh));
    if (g.state == UIGestureRecognizerStateEnded || g.state == UIGestureRecognizerStateCancelled) {
        NSMutableDictionary *o = [[NSUserDefaults.standardUserDefaults dictionaryForKey:[self key:@"origins"]] mutableCopy] ?: [NSMutableDictionary dictionary];
        o[v.accessibilityIdentifier] = NSStringFromCGPoint(CGPointMake((v.center.x - CGRectGetMinX(safe)) / safe.size.width,
                                                                       (v.center.y - CGRectGetMinY(safe)) / safe.size.height));
        [NSUserDefaults.standardUserDefaults setObject:o forKey:[self key:@"origins"]];
    }
}
- (void)selectedSizeChanged:(UISlider *)s
{
    if (!_selected) return;
    NSMutableDictionary *o = [[NSUserDefaults.standardUserDefaults dictionaryForKey:[self key:@"scales"]] mutableCopy] ?: [NSMutableDictionary dictionary];
    o[_selected.accessibilityIdentifier] = @(s.value);
    [NSUserDefaults.standardUserDefaults setObject:o forKey:[self key:@"scales"]];
    [self setNeedsLayout];
}

/* development: open a part of the overlay for an unattended screenshot (HALOPAD_OVERLAY_DEMO) */
- (void)didMoveToWindow
{
    [super didMoveToWindow];
    const char *demo = getenv("HALOPAD_OVERLAY_DEMO");
    if (!self.window || !demo) return;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (!strcmp(demo, "settings")) [self togglePanel];
        if (!strcmp(demo, "layout")) { [self beginEditing]; }
        if (!strcmp(demo, "lefthanded")) { HPSettings.shared.leftHanded = YES; [self setNeedsLayout]; }
        if (!strcmp(demo, "spread")) { HPSettings.shared.ringSpacing = 2; HPSettings.shared.showCaptions = NO; [self setNeedsLayout]; }
        fprintf(stderr, "HALOPAD OVERLAY: demo \"%s\" open\n", demo);
    });
    /* join:ADDRESS: the menu's Join Server, once Halo's menu is up */
    if (!strcmp(demo, "console") || !strcmp(demo, "console-keyboard")) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 20 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            fprintf(stderr, "HALOPAD OVERLAY: demo opens console through the menu's key handler\n");
            [HPOverlay tapKey:0xC0 scan:0x29];
            if (!strcmp(demo, "console-keyboard")) [self.delegate overlayRequestsKeyboard:self];
        });
    }
    if (!strncmp(demo, "join:", 5)) {
        NSString *addr = @(demo + 5);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 20 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            fprintf(stderr, "HALOPAD OVERLAY: demo joins %s through Halo's console\n", addr.UTF8String);
            [self joinServer:addr password:@""];
        });
    }
}
@end
