/* UIKit overlay input boundary tests. Uses the real timer/handlers, capturing the events
   sent to the host; it does not claim actual multi-touch routing or game acceptance. */
#import "../port/ios/HaloPadOverlay.h"
#include "../port/runtime/halopad_input.h"
#include <stdio.h>

static hp_input events[512];
static int count, failures;
static BOOL held[256];

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

        fprintf(stderr, "OVERLAY INPUT: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
        return failures ? 1 : 0;
    }
}
