/* Absolute menu touches expressed as ordinary relative mouse input. No guest writes.
   The caller serializes UIKit requests with steps on the game's frame thread. */
#ifndef HALOPAD_MENU_TOUCH_H
#define HALOPAD_MENU_TOUCH_H
#include "../runtime/halopad_input.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int x, y, client_x, client_y, ended;
    int press_x, press_y, press_client_x, press_client_y;
    unsigned token;
    uint32_t root;
} hp_menu_gesture;
typedef struct {
    hp_menu_gesture gestures[8];
    unsigned head, count, serial;
    int phase, cancel, stalled, last_x, last_y; /* 0 positioning, 1 pressed, 2 released */
} hp_menu_touch;

static unsigned hp_menu_begin(hp_menu_touch *s, uint32_t root, int x, int y, int cx, int cy)
{
    if (!root || x < 0 || y < 0 || x > 640 || y > 480 || s->count == 8) return 0;
    unsigned token = ++s->serial;
    if (!token) token = ++s->serial;
    s->gestures[(s->head + s->count++) % 8] = (hp_menu_gesture){
        .x=x, .y=y, .client_x=cx, .client_y=cy,
        .press_x=x, .press_y=y, .press_client_x=cx, .press_client_y=cy, .token=token, .root=root};
    return token;
}
static void hp_menu_update(hp_menu_touch *s, unsigned token, int x, int y, int cx, int cy, int ended)
{
    for (unsigned i = 0; i < s->count; i++) {
        hp_menu_gesture *g = &s->gestures[(s->head + i) % 8];
        if (!token || g->token != token) continue;
        g->x = x < 0 ? 0 : x > 640 ? 640 : x;
        g->y = y < 0 ? 0 : y > 480 ? 480 : y;
        g->client_x = cx; g->client_y = cy; g->ended |= ended;
    }
}
/* CE's menu acceleration is d * (1 + abs(d) * sensitivity * .05).
   Choose the nearest raw count, then use the next frame's observed cursor to correct
   rounding. Gameplay sensitivity and relative aiming are unaffected. */
static int hp_menu_delta(int distance, float sensitivity)
{
    double a = (isfinite(sensitivity) ? fmax(0, sensitivity) : 0) * .05, d = abs(distance);
    double raw = a > 0 ? 2 * d / (1 + sqrt(1 + 4 * a * d)) : d;
    int n = (int)lround(raw);
    if (!n && distance) n = 1;
    return distance < 0 ? -n : n;
}
static int hp_menu_step(hp_menu_touch *s, uint32_t root, int x, int y,
                        float sx, float sy, hp_input *event)
{
    memset(event, 0, sizeof *event);
    if (s->cancel || (s->count && s->gestures[s->head].root != root)) {
        int release = s->phase == 1;
        s->head = s->count = 0; s->phase = s->cancel = s->stalled = 0;
        if (release) { event->kind = HPI_BUTTON; return 1; }
        return 0;
    }
    if (!s->count) return 0;
    if (s->phase == 2) { s->head = (s->head + 1) % 8; s->count--; s->phase = s->stalled = 0; return 0; }
    hp_menu_gesture *g = &s->gestures[s->head];
    event->x = s->phase ? g->client_x : g->press_client_x;
    event->y = s->phase ? g->client_y : g->press_client_y;
    int dx = (s->phase ? g->x : g->press_x) - x, dy = (s->phase ? g->y : g->press_y) - y;
    if (abs(dx) > 1 || abs(dy) > 1) {
        s->stalled = x == s->last_x && y == s->last_y ? s->stalled + 1 : 0;
        s->last_x = x; s->last_y = y;
        if (s->stalled >= 8) { /* An inactive/unresponsive game must never click later. */
            s->cancel = 1;
            return hp_menu_step(s, root, x, y, sx, sy, event);
        }
        event->kind = HPI_MOUSEMOVE;
        event->dx = hp_menu_delta(dx, sx); event->dy = hp_menu_delta(dy, sy);
        return 1;
    }
    if (!s->phase) { event->kind = HPI_BUTTON; event->down = 1; s->phase = 1; return 1; }
    if (g->ended) { event->kind = HPI_BUTTON; s->phase = 2; return 1; }
    return 0;
}
#endif
