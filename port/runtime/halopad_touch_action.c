/* Translate a touch action at the host boundary using the current CE 1.10
   keyboard/mouse bindings. No guest state or player profile is modified.
   All calls run on Halo's input thread, before USER32 and DirectInput. */
#include <string.h>
#include "halopad_input.h"

void *halopad_guest_ptr(uint32_t);
/* Original action names: 0x5f9d10, 29 entries of 16 bytes (0x492c20).
   Binding tables verified against original setter 0x48e360. */
#define ACTION_COUNT 29
static hp_input held[ACTION_COUNT];
static unsigned char pressed[ACTION_COUNT];
static uint16_t read16(uint32_t p)
{
    uint16_t v; memcpy(&v, halopad_guest_ptr(p), 2); return v;
}
static int resolve(uint32_t action, hp_input *out)
{
    *out = (hp_input){.kind = HPI_KEY, .flags = HPI_TOUCH, .down = 1};
    for (uint16_t usage = 0; usage < 0xe8; usage++) {
        if (!halopad_hid_key(usage, &out->vk, &out->side_vk, &out->scan, &out->extended)) continue;
        uint32_t dik = (out->scan & 127) | (out->extended ? 128 : 0);
        uint16_t index = read16(0x5fa358 + dik * 2);
        if (index < 109 && read16(0x6ab330 + index * 2) == action) return 1;
    }
    for (int b = 0; b < 8; b++) if (read16(0x6ab40a + b * 2) == action) {
        *out = (hp_input){.kind = HPI_BUTTON, .flags = HPI_TOUCH, .down = 1,
                          .button = b, .x = 400, .y = 300};
        return 1;
    }
    /* Unbound/controller-only/wheel-only actions cannot synthesize a keyboard
       hold. Never fall back to a key that might now trigger another action. */
    return 0;
}
static int same_control(const hp_input *a, const hp_input *b)
{
    return a->kind == b->kind && (a->kind == HPI_KEY ?
        a->scan == b->scan && a->extended == b->extended : a->button == b->button);
}
int halopad_touch_action_event(const hp_input *e)
{
    if (e->kind == HPI_CANCEL_TOUCH) {
        /* The ordinary cancellation path drops unread edges and preserves
           physical holds; synthetic up events here would retain stale taps. */
        memset(pressed, 0, sizeof pressed);
        memset(held, 0, sizeof held);
        return 0;
    }
    if (e->kind != HPI_ACTION) return 0;
    uint32_t action = e->action;
    if (!(e->flags & HPI_TOUCH) || action >= ACTION_COUNT) return 1;
    if (!!e->down == !!pressed[action]) return 1;
    pressed[action] = !!e->down;
    if (e->down) {
        held[action] = (hp_input){0};
        if (!resolve(action, &held[action])) { held[action].flags = 0; return 1; }
    }
    hp_input mapped = held[action];
    if (!mapped.flags) return 1;
    if (!e->down) held[action] = (hp_input){0};
    /* A remap during a hold can make two fingers share a control. Release it
       only after its last touch owner lets go. Physical ownership is downstream. */
    for (unsigned i = 0; i < ACTION_COUNT; i++)
        if (i != action && pressed[i] && held[i].flags && same_control(&held[i], &mapped)) return 1;
    mapped.down = !!e->down;
    halopad_input_event(&mapped);
    return 1;
}
