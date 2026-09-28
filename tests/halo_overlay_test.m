/* UIKit overlay input boundary tests. Uses the real timer/handlers, capturing the events
   sent to the host; it does not claim actual multi-touch routing or game acceptance. */
#import "../port/ios/HaloPadOverlay.h"
#include "../port/runtime/halopad_input.h"
#include <stdio.h>

static hp_input events[512];
static int count, failures;
static BOOL held[256];
static void check(const char *name, BOOL ok);

@interface HPTestOverlay : HPOverlay
@property(nonatomic) BOOL simulatedPhone;
@property(nonatomic) UIEdgeInsets simulatedInsets;
@end
@implementation HPTestOverlay
- (BOOL)phone { return self.simulatedPhone; }
- (UIEdgeInsets)safeAreaInsets { return self.simulatedInsets; }
@end

static void check_layouts(void)
{
    HPSettings *settings = HPSettings.shared;
    CGFloat savedSize = settings.controlSize;
    BOOL savedHand = settings.leftHanded;
    NSInteger savedSpacing = settings.ringSpacing;
    CGSize screens[] = {{667, 375}, {760, 354}, {844, 390}, {1024, 768}, {1376, 1032}};
    CGFloat sizes[] = {0.7, 1, 1.35};
    int cases = 0, bad = 0, badReach = 0;
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
}

void halopad_host_post_input(const hp_input *e)
{
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
        check("fire fixture finds the actual control", [overlay driveControl:@"fire" down:YES]);
        [overlay clearTouchInput];
        check("clearing touch input releases movement and fire",
              all_released() && count >= 4 && events[count - 2].kind == HPI_BUTTON &&
              !events[count - 2].down && events[count - 1].vk == 'W' && !events[count - 1].down);
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
        [overlay clearTouchInput];
        int afterRelease = count;
        run_for(0.15);
        check("clearing the LOOK stick stops its continuous motion", count == afterRelease);
        [overlay driveAimX:0.05 y:0.05];
        run_for(0.15);
        check("LOOK dead zone causes no drift", count == afterRelease);
        [overlay clearTouchInput];
        overlay.inGame = YES;
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
        check_layouts();

        fprintf(stderr, "OVERLAY INPUT: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
        return failures ? 1 : 0;
    }
}
