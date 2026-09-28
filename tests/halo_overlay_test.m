/* UIKit overlay input boundary tests. Uses the real timer/handlers, capturing the events
   sent to the host; it does not claim actual multi-touch routing or game acceptance. */
#import "../port/ios/HaloPadOverlay.h"
#include "../port/runtime/halopad_input.h"
#include <stdio.h>

static hp_input events[512];
static int count, failures, cancellations, countAtCancel;
static BOOL held[256];
static void check(const char *name, BOOL ok);
static void run_for(double seconds);
static BOOL all_released(void);

@interface HPTestOverlay : HPOverlay
@property(nonatomic) BOOL simulatedPhone;
@property(nonatomic) UIEdgeInsets simulatedInsets;
@end
@implementation HPTestOverlay
- (BOOL)phone { return self.simulatedPhone; }
- (UIEdgeInsets)safeAreaInsets { return self.simulatedInsets; }
@end

@interface HPOverlay (LayoutTesting)
- (CGPoint)alignedCenter:(CGPoint)center forView:(UIView *)view;
- (void)place:(UIView *)view frame:(CGRect)frame;
- (NSString *)key:(NSString *)what;
@end

static void check_editor_alignment(void)
{
    HPTestOverlay *view = [[HPTestOverlay alloc] initWithFrame:CGRectMake(0, 0, 1024, 768)];
    view.inGame = YES;
    [view layoutIfNeeded];
    UIView *move = [view valueForKey:@"move"], *aim = [view valueForKey:@"aim"];
    for (UIView *other in view.subviews) other.hidden = other != move && other != aim;
    move.frame = CGRectMake(40, 400, 144, 144);
    aim.frame = CGRectMake(840, 400, 144, 144);
    CGPoint near = CGPointMake(move.center.x, move.center.y + 6);
    check("editor aligns the two sticks after a near-baseline drop",
          CGPointEqualToPoint([view alignedCenter:near forView:move], move.center));
    CGPoint far = CGPointMake(move.center.x, move.center.y + 20);
    check("editor preserves deliberate offsets outside snap distance",
          CGPointEqualToPoint([view alignedCenter:far forView:move], far));
    aim.frame = CGRectMake(194, 406, 144, 144);
    near = CGPointMake(move.center.x + 6, move.center.y);
    check("editor refuses alignment that crowds another control",
          CGPointEqualToPoint([view alignedCenter:near forView:move], near));
    NSString *key = [view key:@"scales"];
    NSDictionary *saved = [NSUserDefaults.standardUserDefaults dictionaryForKey:key];
    [NSUserDefaults.standardUserDefaults setObject:@{@"move": @0.6} forKey:key];
    [view place:move frame:CGRectMake(0, 0, 52, 52)];
    check("saved small scales retain a 44-point tappable target",
          move.bounds.size.width == 44 && move.bounds.size.height == 44);
    if (saved) [NSUserDefaults.standardUserDefaults setObject:saved forKey:key];
    else [NSUserDefaults.standardUserDefaults removeObjectForKey:key];
}

@interface HPLookDrag : NSObject
@property(nonatomic, copy) void (^delta)(CGFloat dx, CGFloat dy);
- (void)begin:(id)token at:(CGPoint)point;
- (void)move:(id)token to:(CGPoint)point;
- (void)end:(id)token at:(CGPoint)point cancelled:(BOOL)cancelled;
- (void)clear;
@end

static void check_drag_tracking(void)
{
    HPLookDrag *drag = [HPLookDrag new];
    NSObject *a = [NSObject new], *b = [NSObject new];
    __block CGFloat x = 0, y = 0;
    __block int deltas = 0;
    drag.delta = ^(CGFloat dx, CGFloat dy) { x += dx; y += dy; deltas++; };
    [drag begin:a at:CGPointMake(10, 20)];
    [drag end:a at:CGPointMake(110, 15) cancelled:NO];
    check("short swipe with no move callback preserves final displacement", x == 100 && y == -5 && deltas == 1);
    x = y = 0; deltas = 0;
    [drag begin:a at:CGPointZero];
    [drag move:a to:CGPointMake(20, 0)];
    [drag move:a to:CGPointMake(50, 5)];
    [drag end:a at:CGPointMake(80, 7) cancelled:NO];
    [drag end:a at:CGPointMake(90, 9) cancelled:NO];
    check("normal swipe consumes each segment and final endpoint exactly once", x == 80 && y == 7 && deltas == 3);
    x = y = 0; deltas = 0;
    [drag begin:a at:CGPointZero];
    [drag move:a to:CGPointMake(20, 0)];
    [drag end:a at:CGPointMake(20, 0) cancelled:NO];
    check("unchanged end point adds no duplicate aim", x == 20 && y == 0 && deltas == 1);
    x = y = 0; deltas = 0;
    [drag begin:a at:CGPointZero]; [drag begin:b at:CGPointMake(100, 100)];
    [drag move:a to:CGPointMake(5, 3)]; [drag move:b to:CGPointMake(96, 104)];
    [drag end:b at:CGPointMake(500, 500) cancelled:YES];
    [drag end:a at:CGPointMake(7, 8) cancelled:NO];
    check("interleaved fingers keep independent positions and cancellation drops its endpoint", x == 3 && y == 12 && deltas == 3);
    [drag begin:a at:CGPointZero]; [drag clear];
    [drag move:a to:CGPointMake(50, 50)]; [drag end:a at:CGPointMake(80, 80) cancelled:NO];
    check("cleared drag ignores late move and end callbacks", x == 3 && y == 12 && deltas == 3);

    HPOverlay *overlay = [[HPOverlay alloc] initWithFrame:CGRectMake(0, 0, 1024, 768)];
    HPLookDrag *surface = [overlay valueForKey:@"lookDrag"];
    count = 0;
    [surface begin:a at:CGPointZero]; [surface end:a at:CGPointMake(20, 0) cancelled:NO];
    check("real overlay forwards short swipe to host mouse motion", count == 1 && events[0].kind == HPI_MOUSEMOVE && events[0].dx > 0);
    UIView *fire = nil;
    for (UIView *v in overlay.subviews) if ([v.accessibilityIdentifier isEqualToString:@"fire"]) fire = v;
    HPLookDrag *fireDrag = [fire valueForKey:@"drag"];
    count = 0;
    [fireDrag begin:a at:CGPointZero]; [fireDrag end:a at:CGPointMake(0, 20) cancelled:NO];
    check("FIRE drag forwards its final displacement to aiming", count == 1 && events[0].kind == HPI_MOUSEMOVE && events[0].dy > 0);
    count = 0;
    [surface begin:a at:CGPointZero]; [fireDrag begin:b at:CGPointZero];
    [overlay clearTouchInput];
    [surface end:a at:CGPointMake(20, 0) cancelled:NO]; [fireDrag end:b at:CGPointMake(0, 20) cancelled:NO];
    check("overlay interruption clears both surface and FIRE drag endpoints", count == 0);
}

/* Exercise the real stick geometry without fabricating UIKit touch objects. */
@interface HPStickView : UIView
@property(nonatomic, copy) void (^valueChanged)(float x, float y);
- (void)trackPoint:(CGPoint)p;
- (void)reset;
@end

/* Handler-boundary tokens, not synthesized UIKit events or proof of OS routing. */
@interface HPTestTouch : NSObject
@property(nonatomic) CGPoint point;
- (CGPoint)locationInView:(UIView *)view;
@end
@implementation HPTestTouch
- (CGPoint)locationInView:(UIView *)view { return self.point; }
- (UITouchType)type { return UITouchTypeDirect; }
- (NSTimeInterval)timestamp { return 0; }
@end
static NSSet<UITouch *> *touch_set(HPTestTouch *touch) { return (id)[NSSet setWithObject:touch]; }

static void check_stick_ownership(void)
{
    HPOverlay *overlay = [[HPOverlay alloc] initWithFrame:CGRectMake(0, 0, 1024, 768)];
    overlay.inGame = YES;
    [overlay layoutIfNeeded];
    HPStickView *move = [overlay valueForKey:@"move"], *aim = [overlay valueForKey:@"aim"];
    HPTestTouch *left = [HPTestTouch new], *right = [HPTestTouch new], *stray = [HPTestTouch new];
    left.point = CGPointMake(CGRectGetMidX(move.bounds), 0);
    right.point = CGPointMake(CGRectGetMaxX(aim.bounds), CGRectGetMidY(aim.bounds));
    stray.point = CGPointZero;
    count = 0;
    [move touchesBegan:touch_set(left) withEvent:nil];
    [aim touchesBegan:touch_set(right) withEvent:nil];
    run_for(0.08);
    check("independent MOVE and LOOK handlers can remain held together", held['W'] && count > 1);
    [move touchesEnded:touch_set(left) withEvent:nil];
    int afterMoveRelease = count;
    run_for(0.08);
    check("releasing MOVE keeps the other finger aiming", all_released() && count > afterMoveRelease);
    [move touchesBegan:touch_set(left) withEvent:nil];
    [aim touchesEnded:touch_set(right) withEvent:nil];
    int afterAimRelease = count;
    run_for(0.08);
    check("releasing LOOK keeps movement held without further aim", held['W'] && count == afterAimRelease);
    [aim touchesBegan:touch_set(right) withEvent:nil];
    [overlay clearTouchInput];
    int afterClear = count;
    [move touchesMoved:touch_set(left) withEvent:nil];
    [aim touchesMoved:touch_set(right) withEvent:nil];
    run_for(0.08);
    check("late stick moves after interruption cannot restore movement or aim", all_released() && count == afterClear);
    [move touchesEnded:touch_set(left) withEvent:nil];
    [aim touchesEnded:touch_set(right) withEvent:nil];
    [overlay clearTouchInput];

    [move touchesBegan:touch_set(left) withEvent:nil];
    int afterBegin = count;
    [move touchesBegan:touch_set(stray) withEvent:nil];
    [move touchesMoved:touch_set(stray) withEvent:nil];
    [move touchesEnded:touch_set(stray) withEvent:nil];
    check("another touch cannot steal or release an owned stick", held['W'] && count == afterBegin);
    [move touchesCancelled:touch_set(left) withEvent:nil];
    check("owner cancellation releases movement", all_released());
    afterClear = count;
    [move touchesMoved:touch_set(left) withEvent:nil];
    check("cancelled owner cannot restart movement with a late move", all_released() && count == afterClear);
    [overlay clearTouchInput];

    [move touchesBegan:touch_set(left) withEvent:nil];
    [move reset];
    [move touchesBegan:touch_set(right) withEvent:nil];
    afterBegin = count;
    [move touchesEnded:touch_set(left) withEvent:nil];
    check("old owner ending cannot release a newly started touch", count == afterBegin);
    [move touchesEnded:touch_set(right) withEvent:nil];
    check("new owner ends normally", all_released());
    [overlay clearTouchInput];
}

static void check_stick_tracking(void)
{
    HPStickView *stick = [[HPStickView alloc] initWithFrame:CGRectMake(0, 0, 128, 128)];
    [stick layoutIfNeeded];
    __block float x = 0, y = 0;
    stick.valueChanged = ^(float a, float b) { x = a; y = b; };
    UIView *thumb = stick.subviews.firstObject;
    check("stick corners leave open-screen swipes available", ![stick pointInside:CGPointMake(1, 1) withEvent:nil]);
    check("stick centre and visible rim remain touch targets", [stick pointInside:CGPointMake(64, 64) withEvent:nil] && [stick pointInside:CGPointMake(127, 64) withEvent:nil]);
    [stick trackPoint:CGPointMake(76, 54)];
    check("stick thumb follows the finger inside its travel",
          fabs(thumb.center.x - 76) < 0.01 && fabs(thumb.center.y - 54) < 0.01 && x > 0 && y > 0);
    CGFloat travel = 64 - thumb.bounds.size.width / 2 - 3;
    [stick trackPoint:CGPointMake(64 + travel, 64)];
    check("visible stick rim reaches full input without dragging past the thumb",
          fabs(x - 1) < 0.001 && fabs(y) < 0.001);
    [stick trackPoint:CGPointMake(200, -100)];
    check("drag outside the stick clamps radially and preserves direction",
          fabs(hypot(x, y) - 1) < 0.001 && x > 0 && y > 0 &&
          fabs(hypot(thumb.center.x - 64, thumb.center.y - 64) - travel) < 0.01);
    [stick reset];
    check("stick release returns input and thumb to centre", x == 0 && y == 0 && CGPointEqualToPoint(thumb.center, CGPointMake(64, 64)));
}

static void check_layouts(void)
{
    HPSettings *settings = HPSettings.shared;
    CGFloat savedSize = settings.controlSize;
    BOOL savedHand = settings.leftHanded;
    NSInteger savedSpacing = settings.ringSpacing;
    CGSize screens[] = {{667, 375}, {760, 354}, {844, 390}, {1024, 768}, {1376, 1032}};
    CGFloat sizes[] = {0.7, 1, 1.35};
    int cases = 0, bad = 0, badReach = 0, badGrid = 0;
    for (int form = 0; form < 5; form++) for (int hand = 0; hand < 2; hand++)
    for (int size = 0; size < 3; size++) for (int gap = 0; gap < 3; gap++) {
        settings.controlSize = sizes[size]; settings.leftHanded = hand; settings.ringSpacing = gap;
        HPTestOverlay *view = [[HPTestOverlay alloc] initWithFrame:(CGRect){CGPointZero, screens[form]}];
        view.simulatedPhone = form < 3;
        view.simulatedInsets = form < 3 ? UIEdgeInsetsMake(0, 44, 21, 44) : UIEdgeInsetsMake(0, 0, 20, 0);
        view.inGame = YES;
        [view setNeedsLayout]; [view layoutIfNeeded];
        /* Render the actual UIKit controls without presenting a window or loading Halo. */
        const char *renderPath = getenv("HALOPAD_OVERLAY_RENDER_DIR");
        if (renderPath && hand == 0 && size == 1 && gap == 1) {
            UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat defaultFormat];
            format.scale = 1;
            UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:view.bounds.size format:format];
            UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
                [[UIColor colorWithRed:0.12 green:0.18 blue:0.21 alpha:1] setFill];
                UIRectFill(view.bounds);
                [view.layer renderInContext:context.CGContext];
            }];
            NSString *path = [@(renderPath) stringByAppendingPathComponent:[NSString stringWithFormat:@"layout-%.0fx%.0f.png", screens[form].width, screens[form].height]];
            check("native overlay preview written", [UIImagePNGRepresentation(image) writeToFile:path atomically:YES]);
        }
        NSMutableArray<UIView *> *controls = [NSMutableArray array];
        for (UIView *v in view.subviews)
            if (v.accessibilityIdentifier && !v.hidden) [controls addObject:v];
        BOOL valid = controls.count == 16; /* 13 actions, two sticks, native menu */
        UIView *move = nil, *aim = nil, *fire = nil, *crouch = nil;
        for (UIView *v in controls) {
            if ([v.accessibilityIdentifier isEqualToString:@"move"]) move = v;
            if ([v.accessibilityIdentifier isEqualToString:@"aim"]) aim = v;
            if ([v.accessibilityIdentifier isEqualToString:@"fire"]) fire = v;
            if ([v.accessibilityIdentifier isEqualToString:@"crouch"]) crouch = v;
            valid &= CGRectContainsRect(UIEdgeInsetsInsetRect(view.bounds, view.safeAreaInsets), v.frame) && v.bounds.size.width >= 44 && v.bounds.size.height >= 44;
            for (UIView *other in controls) if (v != other) {
                CGFloat dx = MAX(0, MAX(CGRectGetMinX(v.frame) - CGRectGetMaxX(other.frame), CGRectGetMinX(other.frame) - CGRectGetMaxX(v.frame)));
                CGFloat dy = MAX(0, MAX(CGRectGetMinY(v.frame) - CGRectGetMaxY(other.frame), CGRectGetMinY(other.frame) - CGRectGetMaxY(v.frame)));
                valid &= hypot(dx, dy) >= 7.99; /* an actual gap, not just non-overlap */
            }
            if ([v.accessibilityIdentifier isEqualToString:@"move"] || [v.accessibilityIdentifier isEqualToString:@"aim"])
                valid &= [view hitTest:v.center withEvent:nil] == v;
        }
        CGRect safe = UIEdgeInsetsInsetRect(view.bounds, view.safeAreaInsets);
        NSMutableDictionary<NSString *, UIView *> *byID = [NSMutableDictionary dictionary];
        for (UIView *v in controls) byID[v.accessibilityIdentifier] = v;
        UIView *reload = byID[@"reload"], *melee = byID[@"melee"], *jump = byID[@"jump"];
        UIView *zoom = byID[@"zoom"], *use = byID[@"action"], *swap = byID[@"switch"];
        CGFloat pitch = jump.center.y - melee.center.y;
        BOOL grid = fabs(reload.center.x - melee.center.x) < 0.01 && fabs(melee.center.x - jump.center.x) < 0.01 &&
            fabs(zoom.center.x - use.center.x) < 0.01 && fabs(use.center.x - swap.center.x) < 0.01 &&
            fabs(zoom.center.y - reload.center.y) < 0.01 && fabs(use.center.y - melee.center.y) < 0.01 &&
            fabs(swap.center.y - jump.center.y) < 0.01 && fabs(melee.center.y - reload.center.y - pitch) < 0.01 &&
            fabs(fabs(use.center.x - melee.center.x) - pitch) < 0.01 &&
            fabs(melee.center.y - aim.center.y) < 0.01 &&
            fabs(byID[@"flash"].center.y - move.center.y) < 0.01 &&
            fabs(byID[@"grenade"].center.y - fire.center.y) < 0.01;
        if (!grid) badGrid++;
        BOOL reachable = move && aim && fire && crouch && fabs(move.center.y - aim.center.y) < 0.01 &&
            fabs(move.center.x + aim.center.x - CGRectGetMinX(safe) - CGRectGetMaxX(safe)) < 0.01 &&
            MIN(move.center.x, aim.center.x) - CGRectGetMinX(safe) <= 160 &&
            fabs(fire.center.x - aim.center.x) < 0.01 &&
            fabs(crouch.center.x - move.center.x) < fabs(crouch.center.x - aim.center.x);
        if (form >= 3) {
            /* Conservative bottom-left HUD box for the stock 4:3 tablet viewport. */
            CGRect radar = CGRectMake(0, screens[form].height * 0.83, screens[form].width * 0.15, screens[form].height * 0.17);
            for (UIView *v in controls) reachable &= !CGRectIntersectsRect(v.frame, radar);
        }
        if (!reachable) badReach++;
        cases++;
        if (!valid) { bad++; fprintf(stderr, "layout failed: %.0fx%.0f hand %d size %.2f gap %d controls %lu\n",
                                    screens[form].width, screens[form].height, hand, sizes[size], gap, (unsigned long)controls.count); }
    }
    settings.controlSize = savedSize; settings.leftHanded = savedHand; settings.ringSpacing = savedSpacing;
    fprintf(stderr, "LAYOUT: %d combinations, %d failures\n", cases, bad);
    check("phone/tablet defaults have separate targets, safe bounds and two reachable sticks", bad == 0);
    check("sticks have equal reach, aligned fire, movement-side crouch and tablet radar clearance", badReach == 0);
    check("action columns retain equal spacing and align around the aiming thumb", badGrid == 0);
}

void halopad_host_post_input(const hp_input *e)
{
    if (e->kind == HPI_CANCEL_TOUCH) { cancellations++; countAtCancel = count; return; }
    if (count >= 512) abort();
    events[count++] = *e;
    if (e->kind == HPI_KEY && e->side_vk < 256) held[e->side_vk] = e->down;
}

static void check(const char *name, BOOL ok)
{
    fprintf(stderr, "%s: %s\n", ok ? "PASS" : "FAIL", name);
    failures += !ok;
}

static void run_for(double seconds)
{
    NSDate *end = [NSDate dateWithTimeIntervalSinceNow:seconds];
    while (end.timeIntervalSinceNow > 0)
        [NSRunLoop.mainRunLoop runMode:NSDefaultRunLoopMode beforeDate:end];
}

static void await_events(int wanted)
{
    NSDate *end = [NSDate dateWithTimeIntervalSinceNow:2];
    while (count < wanted && end.timeIntervalSinceNow > 0)
        [NSRunLoop.mainRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
    check("timer delivers expected events before deadline", count == wanted);
}

static BOOL all_released(void)
{
    for (int i = 0; i < 256; i++) if (held[i]) return NO;
    return YES;
}

int main(void)
{
    @autoreleasepool {
        [HPOverlay setTextInputActive:YES];
        [HPOverlay typeText:@"A\n"];
        await_events(6);
        check("normal uppercase text preserves Shift, character and Enter order",
              count == 6 && events[0].side_vk == 0xA0 && events[0].down &&
              events[1].vk == 'A' && events[1].chars[0] == 'A' && events[1].down &&
              events[2].vk == 'A' && !events[2].down &&
              events[3].side_vk == 0xA0 && !events[3].down &&
              events[4].vk == 13 && events[4].chars[0] == '\r' && events[4].down &&
              events[5].vk == 13 && !events[5].down && all_released());

        count = 0;
        [HPOverlay typeText:@"BBBB\n"];
        await_events(2);  /* Shift and B held; all remaining text, including Enter, is pending. */
        check("interruption fixture holds letter and Shift", held['B'] && held[0xA0]);
        [HPOverlay setTextInputActive:NO];
        check("losing focus immediately releases the delivered keys", count == 4 && all_released());
        check("cancellation releases contain no characters",
              !events[2].down && !events[3].down && !events[2].nchars && !events[3].nchars);
        [HPOverlay setTextInputActive:NO];
        check("repeated interruption does not duplicate releases", count == 4);
        [HPOverlay typeText:@"stale\n"];
        [HPOverlay tapKey:0xC0 scan:0x29];
        run_for(0.3);
        check("inactive scene emits no queued text or console key", count == 4);
        [HPOverlay setTextInputActive:YES];
        run_for(0.3);
        check("resuming never replays the partial text or Enter", count == 4);
        [HPOverlay typeText:@"c"];
        await_events(6);
        check("fresh input works after resume without stale Shift",
              events[4].vk == 'C' && events[4].chars[0] == 'c' && events[4].down &&
              events[5].vk == 'C' && !events[5].down && all_released());

        count = 0;
        [HPOverlay typeText:@"never sent\n"];
        [HPOverlay setTextInputActive:NO];
        [HPOverlay setTextInputActive:YES];
        run_for(0.3);
        check("cancel before the first timer tick emits nothing", count == 0);

        HPOverlay *overlay = [[HPOverlay alloc] initWithFrame:CGRectMake(0, 0, 1024, 768)];
        [overlay driveMoveX:0 y:1];
        check("move fixture holds forward", held['W']);
        check("movement identifies its cancelable touch source", events[count - 1].flags & HPI_TOUCH);
        check("fire fixture finds the actual control", [overlay driveControl:@"fire" down:YES]);
        int cancelsBefore = cancellations;
        [overlay clearTouchInput];
        check("clearing touch input explicitly cancels unread virtual button edges", cancellations == cancelsBefore + 1);
        check("virtual FIRE identifies its touch source", events[count - 2].flags & HPI_TOUCH);
        check("cancel follows all gameplay releases", countAtCancel == count);
        check("movement release keeps its touch source", events[count - 1].flags & HPI_TOUCH);
        check("clearing touch input releases movement and fire",
              all_released() && count >= 4 && events[count - 2].kind == HPI_BUTTON &&
              !events[count - 2].down && events[count - 1].vk == 'W' && !events[count - 1].down);
        [overlay driveMoveX:0 y:1];
        overlay.analogMoveReady = YES;
        check("switching MOVE sources releases the old keyboard hold", all_released());
        count = 0;
        [overlay driveMoveX:.25f y:.5f];
        check("analog MOVE preserves both partial axes without keyboard events", count == 1 &&
              events[0].kind == HPI_TOUCH_MOVE && events[0].flags == HPI_TOUCH &&
              events[0].move_x == .25f && events[0].move_y == .5f && all_released());
        [overlay clearTouchInput];
        check("clearing analog MOVE sends neutral before cancel", events[count - 1].kind == HPI_TOUCH_MOVE &&
              events[count - 1].move_x == 0 && events[count - 1].move_y == 0 && countAtCancel == count);
        overlay.analogMoveReady = NO;
        [overlay driveMoveX:0 y:1];
        check("unconfigured MOVE retains the existing keyboard route", held['W']);
        [overlay clearTouchInput];
        count = 0;
        CGFloat scale = 2.2 * HPSettings.shared.lookSensitivity;
        [overlay driveLookX:0.4 / scale y:0];
        [overlay clearTouchInput];
        [overlay driveLookX:0.7 / scale y:0];
        check("canceled look fractions do not spill into the next drag", count == 0);

        [overlay driveAimX:1 y:0];
        run_for(0.15);
        check("held LOOK stick produces continued rightward mouse motion",
              count > 1 && events[0].kind == HPI_MOUSEMOVE && events[0].dx > 0 && events[0].dy == 0);
        check("LOOK identifies its cancelable touch source", events[0].flags & HPI_TOUCH);
        [overlay clearTouchInput];
        int afterRelease = count;
        run_for(0.15);
        check("clearing the LOOK stick stops its continuous motion", count == afterRelease);
        [overlay driveAimX:0.05 y:0.05];
        run_for(0.15);
        check("LOOK dead zone causes no drift", count == afterRelease);
        [overlay clearTouchInput];
        overlay.inGame = YES;
        [overlay driveMoveX:0 y:1];
        [overlay driveControl:@"fire" down:YES];
        [overlay driveAimX:1 y:0];
        UIButton *nativeMenu = [overlay valueForKey:@"menuButton"];
        cancelsBefore = cancellations;
        /* This headless harness has no UIApplication to dispatch sendActions. Check
           the real registration and invoke that registered handler directly. */
        NSArray<NSString *> *menuActions = [nativeMenu actionsForTarget:overlay forControlEvent:UIControlEventMenuActionTriggered];
        check("three-dot menu registers input cancellation before presentation",
              [menuActions containsObject:NSStringFromSelector(@selector(clearTouchInput))]);
        for (NSString *action in menuActions) [overlay performSelector:NSSelectorFromString(action)];
        int afterNativeMenu = count;
        run_for(0.15);
        check("three-dot menu opening releases holds and cancels unread mouse edges",
              all_released() && cancellations == cancelsBefore + 1 && count == afterNativeMenu);
        [overlay setNeedsLayout]; [overlay layoutIfNeeded];
        [overlay driveMoveX:0 y:1];
        [overlay driveControl:@"fire" down:YES];
        [overlay driveAimX:1 y:0];
        NSMutableArray<UIView *> *typingTargets = [NSMutableArray array];
        for (UIView *v in overlay.subviews)
            if (v.accessibilityIdentifier && !v.hidden && ![v.accessibilityIdentifier isEqualToString:@"HaloPadMenu"])
                [typingTargets addObject:v];
        int beforeKeyboard = count;
        overlay.softwareKeyboardVisible = YES;
        int afterKeyboard = count;
        BOOL fireReleased = NO;
        for (int i = beforeKeyboard; i < afterKeyboard; i++)
            fireReleased |= events[i].kind == HPI_BUTTON && !events[i].down;
        run_for(0.15);
        check("opening software keyboard releases held movement, fire and aim",
              fireReleased && all_released() && count == afterKeyboard);
        BOOL hidden = typingTargets.count == 15, restored = YES;
        for (UIView *v in typingTargets) hidden &= v.hidden;
        check("typing hides gameplay targets and disables open-space swipe capture",
              hidden && [overlay hitTest:CGPointMake(512, 384) withEvent:nil] == nil);
        overlay.softwareKeyboardVisible = NO;
        for (UIView *v in typingTargets) restored &= !v.hidden;
        check("keyboard dismissal restores gameplay targets without replaying holds",
              restored && all_released() && count == afterKeyboard);
        UIView *back = nil;
        for (UIView *v in typingTargets)
            if ([v.accessibilityIdentifier isEqualToString:@"menu"]) back = v;
        [overlay driveMoveX:0 y:1];
        [overlay driveControl:@"fire" down:YES];
        [overlay driveAimX:1 y:0];
        int beforeMenu = count;
        overlay.haloMenuVisible = YES;
        int afterMenu = count;
        fireReleased = NO;
        for (int i = beforeMenu; i < afterMenu; i++)
            fireReleased |= events[i].kind == HPI_BUTTON && !events[i].down;
        run_for(0.15);
        check("Halo menu releases movement, fire and continuous LOOK",
              fireReleased && all_released() && count == afterMenu);
        hidden = YES;
        for (UIView *v in typingTargets) if (v != back) {
            hidden &= v.hidden && [overlay hitTest:v.center withEvent:nil] == nil;
        }
        check("Halo menu passes through former gameplay targets and the blank surface",
              hidden && [overlay hitTest:CGPointMake(512, 384) withEvent:nil] == nil);
        check("Halo menu retains a reachable Back control",
              back && !back.hidden && [overlay hitTest:back.center withEvent:nil] == back &&
              [back.accessibilityLabel isEqualToString:@"Back to game or previous menu"]);
        overlay.haloMenuVisible = YES; /* changing to a child widget keeps menu ownership */
        check("child menus do not replay releases", count == afterMenu);
        overlay.softwareKeyboardVisible = YES;
        check("keyboard also hides the menu Back target", back.hidden);
        overlay.softwareKeyboardVisible = NO;
        overlay.haloMenuVisible = NO;
        restored = YES;
        for (UIView *v in typingTargets) restored &= !v.hidden;
        check("resume restores both sticks and actions without replaying input",
              restored && count == afterMenu && [back.accessibilityLabel isEqualToString:@"Pause"]);
        overlay.haloMenuVisible = YES;
        overlay.inGame = NO;
        check("main menu hides the in-game Back target", back.hidden);
        check_drag_tracking();
        check_stick_tracking();
        check_stick_ownership();
        check_layouts();
        check_editor_alignment();

        fprintf(stderr, "OVERLAY INPUT: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
        return failures ? 1 : 0;
    }
}
