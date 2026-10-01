/* Actual Xbox UIKit handlers + shared input buffer, with inert touch tokens.
 * No window, game data, physical events or multi-touch-routing claim. */
#import "../port/xbox/xg_ios.h"
#include <SDL3/SDL_gamepad.h>
#include <stdio.h>

static struct xg_touch_input input;
static int failures, checks, connected;
void xg_ios_set_touch_pad(const struct xg_touch_pad *state) { xg_touch_publish(&input, state); }
void xg_ios_clear_touch_pad(void) { xg_touch_clear(&input); }
int xg_ios_controller_connected(void) { return connected; }

@interface XGTouchPad (Testing)
- (void)tick;
- (void)smallDown:(UIButton *)sender;
- (void)smallUp:(UIButton *)sender;
- (void)smallCancel:(UIButton *)sender;
@end

/* Handler-boundary tokens, not fabricated UITouch instances or OS events. */
@interface XGTestTouch : NSObject
@property(nonatomic) CGPoint point;
@property(nonatomic) NSTimeInterval timestamp;
- (CGPoint)locationInView:(UIView *)view;
@end
@implementation XGTestTouch
- (CGPoint)locationInView:(UIView *)view { return self.point; }
@end

static XGTestTouch *token(CGFloat x, CGFloat y)
{
    XGTestTouch *touch = [XGTestTouch new];
    touch.point = CGPointMake(x, y); touch.timestamp = CACurrentMediaTime();
    return touch;
}
static NSSet<UITouch *> *tokens(NSArray *array) { return (id)[NSSet setWithArray:array]; }
static void check(const char *label, BOOL passed)
{
    checks++; failures += !passed;
    printf("%s %s\n", passed ? "PASS" : "FAIL", label);
}
static BOOL neutral(void)
{
    struct xg_touch_input zero = {0};
    return !memcmp(&input, &zero, sizeof(zero));
}
static UIButton *button(XGTouchPad *pad, NSString *title)
{
    for (UIView *view in pad.subviews)
        if ([view isKindOfClass:UIButton.class] &&
            [((UIButton *)view).currentTitle isEqualToString:title]) return (id)view;
    return nil;
}

int main(void)
{
    @autoreleasepool {
        setenv("XG_TOUCH_SHOW", "1", 1);
        XGTouchPad *pad = [[XGTouchPad alloc] initWithFrame:CGRectMake(0, 0, 1024, 768)];
        [pad layoutIfNeeded];
        XGTestTouch *left = token(100, 400), *right = token(600, 380);
        XGTestTouch *rt = token(924, 518), *lt = token(834, 468), *a = token(934, 698);
        NSSet *held = tokens(@[left, right, rt, lt, a]);
        [pad touchesBegan:held withEvent:nil];
        left.point = CGPointMake(130, 350);
        right.point = CGPointMake(645, 400); right.timestamp += .1;
        [pad touchesMoved:tokens(@[left, right]) withEvent:nil];
        UIButton *start = button(pad, @"Start"), *back = button(pad, @"Back");
        [pad smallDown:start];
        [pad smallDown:back];
        check("real handlers hold move/look, both triggers, A, Start and Back",
              input.current.axes[0] > 0 && input.current.axes[1] < 0 &&
              input.current.axes[2] > 0 && input.current.axes[3] > 0 &&
              input.current.axes[4] == 1 && input.current.axes[5] == 1 &&
              (input.current.buttons & (1u << SDL_GAMEPAD_BUTTON_SOUTH)) &&
              (input.current.buttons & (1u << SDL_GAMEPAD_BUTTON_START)) &&
              (input.current.buttons & (1u << SDL_GAMEPAD_BUTTON_BACK)));

        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        [center postNotificationName:UIApplicationWillResignActiveNotification object:nil];
        check("deactivation clears live and unread Xbox input", neutral());
        [pad tick];
        check("display tick cannot replay held input after deactivation", neutral());
        [pad touchesBegan:tokens(@[token(924, 518)]) withEvent:nil];
        [pad smallDown:start];
        check("inactive pad refuses new presses", neutral());
        right.point = CGPointMake(700, 420); right.timestamp += .1;
        [pad touchesMoved:tokens(@[left, right]) withEvent:nil];
        [pad touchesEnded:held withEvent:nil];
        [pad smallUp:start];
        [pad smallUp:back];
        check("late interrupted touch/control callbacks stay neutral", neutral());

        [center postNotificationName:UIApplicationDidBecomeActiveNotification object:nil];
        check("activation starts neutral", neutral());
        [pad tick];
        [pad touchesMoved:tokens(@[left, right]) withEvent:nil];
        [pad touchesEnded:held withEvent:nil];
        check("old touch identities cannot move the resumed pad", neutral());
        XGTestTouch *fresh = token(924, 518);
        [pad touchesBegan:tokens(@[fresh]) withEvent:nil];
        check("fresh trigger works after activation", xg_touch_axis(&input, 5) == 1);
        [pad touchesEnded:tokens(@[fresh]) withEvent:nil];
        check("consumed resumed trigger is released", xg_touch_axis(&input, 5) == 0);

        fresh = token(100, 400);
        [pad touchesBegan:tokens(@[fresh]) withEvent:nil];
        fresh.point = CGPointMake(100, 330);
        [pad touchesMoved:tokens(@[fresh]) withEvent:nil];
        check("fresh movement works after activation", xg_touch_axis(&input, 1) == -1);
        [pad touchesCancelled:tokens(@[fresh]) withEvent:nil];
        check("ordinary UIKit cancellation still clears live and unread input", neutral());
        [pad smallDown:start];
        [pad smallCancel:start];
        check("Start control cancellation clears its unread press", neutral());

        unsetenv("XG_TOUCH_SHOW"); connected = 1;
        [pad touchesBegan:tokens(@[token(924, 518)]) withEvent:nil];
        [pad tick];
        check("controller hiding clears touch input", pad.hidden && neutral());
        [pad touchesBegan:tokens(@[token(924, 518)]) withEvent:nil];
        [pad smallDown:start];
        check("hidden pad refuses new presses", neutral());
        connected = 0; [pad tick];
        check("controller removal shows a neutral pad", !pad.hidden && neutral());
        [pad smallDown:start];
        [pad smallUp:start];
        check("active short Start press survives exactly one poll",
              xg_touch_button(&input, SDL_GAMEPAD_BUTTON_START) == 1 &&
              xg_touch_button(&input, SDL_GAMEPAD_BUTTON_START) == 0);
        printf("%d checks, %d failures\n", checks, failures);
    }
    return failures != 0;
}
