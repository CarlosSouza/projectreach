#include "../port/ios/halopad_menu_touch.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    hp_menu_touch s = {0}; hp_input e;
    assert(!hp_menu_begin(&s, 0, 10, 20, 10, 20));
    assert(!hp_menu_begin(&s, 7, -1, 20, -1, 20));
    unsigned t = hp_menu_begin(&s, 7, 320, 370, 400, 463);
    hp_menu_update(&s, t, 320, 370, 400, 463, 1); /* up arrives before the first frame */
    assert(hp_menu_step(&s, 7, 0, 0, 1, 1, &e) && e.kind == HPI_MOUSEMOVE);
    assert(hp_menu_step(&s, 7, 323, 373, 1, 1, &e) && e.kind == HPI_MOUSEMOVE);
    assert(hp_menu_step(&s, 7, 320, 370, 1, 1, &e) && e.kind == HPI_BUTTON && e.down);
    assert(hp_menu_step(&s, 7, 320, 370, 1, 1, &e) && e.kind == HPI_BUTTON && !e.down);
    assert(!hp_menu_step(&s, 7, 320, 370, 1, 1, &e) && !s.count);
    puts("PASS: short tap waits for the observed target and preserves separate press/release frames");

    t = hp_menu_begin(&s, 7, 320, 370, 400, 463);
    assert(hp_menu_step(&s, 7, 320, 370, 1, 1, &e) && e.down);
    hp_menu_update(&s, t, 400, 370, 500, 463, 0);
    assert(hp_menu_step(&s, 7, 320, 370, 1, 1, &e) && e.kind == HPI_MOUSEMOVE && e.dx > 0);
    assert(!hp_menu_step(&s, 7, 400, 370, 1, 1, &e));
    hp_menu_update(&s, t, 400, 370, 500, 463, 1);
    assert(hp_menu_step(&s, 7, 400, 370, 1, 1, &e) && e.kind == HPI_BUTTON && !e.down);
    hp_menu_step(&s, 7, 400, 370, 1, 1, &e);
    puts("PASS: dragging holds the button until the final position and release");

    t = hp_menu_begin(&s, 7, 100, 100, 125, 125);
    hp_menu_update(&s, t, 400, 370, 500, 463, 1);
    assert(hp_menu_step(&s, 7, 100, 100, 1, 1, &e) && e.kind == HPI_BUTTON && e.down && e.x == 125);
    assert(hp_menu_step(&s, 7, 100, 100, 1, 1, &e) && e.kind == HPI_MOUSEMOVE);
    assert(hp_menu_step(&s, 7, 400, 370, 1, 1, &e) && e.kind == HPI_BUTTON && !e.down);
    hp_menu_step(&s, 7, 400, 370, 1, 1, &e);
    puts("PASS: a fast drag preserves its initial press before moving to the final position");

    t = hp_menu_begin(&s, 7, 400, 370, 500, 463);
    hp_menu_update(&s, t, 400, 370, 500, 463, 1);
    hp_menu_begin(&s, 7, 50, 50, 63, 63);
    assert(hp_menu_step(&s, 7, 400, 370, 1, 1, &e) && e.down);
    assert(hp_menu_step(&s, 8, 400, 370, 1, 1, &e) && !e.down && e.kind == HPI_BUTTON);
    assert(!s.count && !hp_menu_step(&s, 8, 400, 370, 1, 1, &e));
    puts("PASS: menu change releases the button and cancels old queued clicks");

    hp_menu_begin(&s, 8, 50, 50, 63, 63);
    s.cancel = 1;
    assert(!hp_menu_step(&s, 8, 400, 370, 1, 1, &e) && !s.count);
    hp_menu_begin(&s, 8, 50, 50, 63, 63);
    assert(hp_menu_step(&s, 8, 50, 50, 1, 1, &e) && e.down);
    s.cancel = 1;
    assert(hp_menu_step(&s, 8, 50, 50, 1, 1, &e) && e.kind == HPI_BUTTON && !e.down);
    assert(!s.count && !hp_menu_step(&s, 8, 50, 50, 1, 1, &e));
    puts("PASS: interruption cancels before or after a press without replay");

    for (int i = 0; i < 8; i++) {
        t = hp_menu_begin(&s, 8, i * 10, 50, i * 10, 50);
        assert(t); hp_menu_update(&s, t, i * 10, 50, i * 10, 50, 1);
    }
    assert(!hp_menu_begin(&s, 8, 1, 1, 1, 1));
    for (int i = 0; i < 8; i++) {
        assert(hp_menu_step(&s, 8, i * 10, 50, 1, 1, &e) && e.down && e.x == i * 10);
        assert(hp_menu_step(&s, 8, i * 10, 50, 1, 1, &e) && !e.down);
        assert(!hp_menu_step(&s, 8, i * 10, 50, 1, 1, &e));
    }
    assert(!s.count);
    puts("PASS: rapid taps preserve order with a released frame between gestures; queue is bounded");

    hp_menu_begin(&s, 8, 50, 50, 63, 63);
    for (int i = 0; i < 12; i++) {
        int emit = hp_menu_step(&s, 8, 400, 370, 1, 1, &e);
        assert(!emit || e.kind == HPI_MOUSEMOVE);
    }
    assert(!s.count);
    puts("PASS: unresponsive cursor cancels instead of clicking at the wrong location");
    return 0;
}
