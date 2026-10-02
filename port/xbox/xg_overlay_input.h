/* HaloPad's shared action surface -> upstream SDL gamepad. No upstream edits.
 * USE and RELOAD share X; keep their ownership independent until both release. */
#ifndef XG_OVERLAY_INPUT_H
#define XG_OVERLAY_INPUT_H
#include <SDL3/SDL_gamepad.h>
#include "../runtime/halopad_input.h"
#include "xg_touch_input.h"

struct xg_overlay_input { uint32_t actions; int pause; struct xg_touch_pad pad; };
static inline void xg_overlay_event(struct xg_overlay_input *input, const hp_input *event)
{
    if (event->kind == HPI_CANCEL_TOUCH) { memset(input, 0, sizeof(*input)); return; }
    if (event->kind == HPI_TOUCH_MOVE) {
        input->pad.axes[0] = fmaxf(-1, fminf(1, event->move_x));
        input->pad.axes[1] = -fmaxf(-1, fminf(1, event->move_y));
    }
    if (event->kind == HPI_ACTION && event->action < 29) {
        uint32_t bit = 1u << event->action;
        if (event->down) input->actions |= bit; else input->actions &= ~bit;
    }
    if (event->kind == HPI_KEY && event->vk == 0x1b) input->pause = event->down;
    static const struct { int action, button; } bindings[] = {
        {0, SDL_GAMEPAD_BUTTON_SOUTH}, {1, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
        {2, SDL_GAMEPAD_BUTTON_WEST}, {3, SDL_GAMEPAD_BUTTON_NORTH},
        {4, SDL_GAMEPAD_BUTTON_EAST}, {5, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
        {10, SDL_GAMEPAD_BUTTON_LEFT_STICK}, {11, SDL_GAMEPAD_BUTTON_RIGHT_STICK},
        {12, SDL_GAMEPAD_BUTTON_BACK}, {13, SDL_GAMEPAD_BUTTON_WEST},
    };
    input->pad.buttons = input->pause ? 1u << SDL_GAMEPAD_BUTTON_START : 0;
    for (unsigned i = 0; i < sizeof(bindings) / sizeof(bindings[0]); i++)
        if (input->actions & (1u << bindings[i].action)) input->pad.buttons |= 1u << bindings[i].button;
    input->pad.axes[4] = (input->actions & (1u << 6)) ? 1 : 0;
    input->pad.axes[5] = (input->actions & (1u << 7)) ? 1 : 0;
}
#endif
