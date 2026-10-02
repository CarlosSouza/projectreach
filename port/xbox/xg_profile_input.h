/* Versioned guest mapping applies only to HaloPad's canonical touch pad.
 * Physical controllers and saved profiles never pass through this adapter. */
#ifndef XG_PROFILE_INPUT_H
#define XG_PROFILE_INPUT_H
#include <stdint.h>
#include <SDL3/SDL_gamepad.h>
#include "xg_touch_input.h"

struct xg_profile_input {
    int received, valid;
    uint32_t menu, low, high, sticks;
    struct xg_touch_pad last, blocked;
};

static inline int xg_profile_valid(uint32_t menu, uint32_t low, uint32_t high, uint32_t sticks)
{
    if (menu > 1 || high > 0xffff || sticks > 3) return 0;
    unsigned seen = 0;
    for (unsigned i = 0; i < 12; i++) {
        unsigned button = ((i < 8 ? low : high) >> (4 * (i % 8))) & 15;
        if (seen & (1u << button)) return 0;
        seen |= 1u << button;
    }
    /* Eight analog buttons, Start/Back and stick clicks; no D-pad bindings. */
    return seen == 0xf0ff && (high & 0xff) == 0xdc;
}

/* Returns true on a boundary; discard queued input and wait for held touches
 * to release, including the overlay's still-held state in later publications. */
static inline int xg_profile_context(struct xg_profile_input *p, struct xg_touch_input *buffer,
                                     uint32_t menu, uint32_t low, uint32_t high, uint32_t sticks)
{
    if (p->received && p->menu == menu && p->low == low && p->high == high && p->sticks == sticks)
        return 0;
    p->received = 1;
    p->valid = xg_profile_valid(menu, low, high, sticks);
    p->menu = menu; p->low = low; p->high = high; p->sticks = sticks;
    p->blocked = p->last;
    xg_touch_clear(buffer);
    return 1;
}

static inline int xg_profile_sdl_button(unsigned xbox)
{
    static const int buttons[16] = {
        SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST,
        SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
        SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
        -1, -1, SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN,
        SDL_GAMEPAD_BUTTON_DPAD_LEFT, SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
        SDL_GAMEPAD_BUTTON_START, SDL_GAMEPAD_BUTTON_BACK,
        SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK
    };
    return buttons[xbox];
}

static inline void xg_profile_publish(struct xg_profile_input *p, struct xg_touch_input *buffer,
                                      const struct xg_touch_pad *raw)
{
    struct xg_touch_pad clean = *raw, mapped = {0};
    p->last = *raw;
    p->blocked.buttons &= raw->buttons;
    clean.buttons &= ~p->blocked.buttons;
    for (unsigned i = 0; i < 6; i++) {
        if (raw->axes[i] == 0) p->blocked.axes[i] = 0;
        if (p->blocked.axes[i] != 0) clean.axes[i] = 0;
    }
    if (!p->received || (p->valid && p->menu)) {
        xg_touch_publish(buffer, &clean); /* unadapted guest or raw menu controls */
        return;
    }
    if (p->valid) {
        static const unsigned defaults[12] = {0,4,2,3,1,5,6,7,12,13,14,15};
        for (unsigned i = 0; i < 12; i++) {
            unsigned from = defaults[i];
            int active = from == 6 || from == 7 ? clean.axes[from - 2] > 0 :
                !!(clean.buttons & (1u << xg_profile_sdl_button(from)));
            if (!active) continue;
            unsigned to = ((i < 8 ? p->low : p->high) >> (4 * (i % 8))) & 15;
            if (to == 6 || to == 7) mapped.axes[to - 2] = 1;
            else mapped.buttons |= 1u << xg_profile_sdl_button(to);
        }
        /* Inverse stick routing. Relative touch look uses the separate mouse
         * path. Legacy diagonal response remains the guest's own processing. */
        static const unsigned axes[4][4] = {{0,1,2,3},{2,3,0,1},{2,1,0,3},{0,3,2,1}};
        for (unsigned i = 0; i < 4; i++) mapped.axes[axes[p->sticks][i]] = clean.axes[i];
    }
    xg_touch_publish(buffer, &mapped); /* invalid bridge data fails closed */
}

static inline void xg_profile_cancel(struct xg_profile_input *p, struct xg_touch_input *buffer)
{
    memset(&p->last, 0, sizeof(p->last));
    memset(&p->blocked, 0, sizeof(p->blocked));
    xg_touch_clear(buffer);
}
#endif
