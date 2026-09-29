/* A connected controller plays without setup (G9, M28).

   Halo CE 1.10 ships default layouts for the controllers of 2003-2004 in ui.map
   (tag class 'devc', ui\device_defaults\*), matched by DirectInput product ID. Its
   original Xbox controller default (ui\device_defaults\xbox, 045E:0285) is:
   A jump, B melee, X reload, Y switch weapon, Black switch grenade, White
   flashlight, left stick click crouch, right stick click zoom, right trigger
   fire, left trigger throw grenade, left stick move, right stick look.
   HaloPad presents every Apple game controller as an Xbox 360 controller
   (045E:028E), for which Halo has no default, so a fresh controller would do
   nothing until the player bound every control in Controls Setup.

   When a physical controller is assigned to a player slot whose bindings are
   all empty, this applies that Xbox layout once, through Halo's own binding
   setter 0x48e360 (the function its Controls Setup and bind command use). It
   maps Black/White to LB/RB's neighbours on a modern pad and adds what the
   Xbox's combined X button did: RB is action (pick up, enter vehicles), LB
   switches grenades, D-pad up is the flashlight and View shows the scores.
   An unassigned controller is assigned with Halo's input_activate_joy command.
   Nothing is applied to a slot that has any binding, so the player's own
   layout always wins, and saving the profile saves these like any binding. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "halopad_input.h"
#include "halopad_log.h"

extern _Thread_local _cpu *halopad_cpu;
void *halopad_guest_ptr(uint32_t);
uint32_t halopad_heap_alloc(uint32_t, int);
void halopad_heap_free(uint32_t);
uint32_t halopad_call_guest_ex(uint32_t, uint32_t, const uint32_t *, uint32_t, int);

enum { JUMP = 0, SWITCH_GRENADE = 1, ACTION = 2, SWITCH_WEAPON = 3, MELEE = 4, FLASHLIGHT = 5, THROW_GRENADE = 6,
       FIRE = 7, CROUCH = 10, ZOOM = 11, SHOW_SCORES = 12, RELOAD = 13, FORWARD = 19, BACKWARD = 20, LEFT = 21,
       RIGHT = 22, LOOK_UP = 23, LOOK_DOWN = 24, LOOK_LEFT = 25, LOOK_RIGHT = 26 };

/* Objects as halopad_dinput.c presents them: buttons A B X Y LB RB View Menu LS RS;
   axes X, Y (left stick, up negative), Z (LT positive, RT negative), Rx, Ry (right
   stick); one hat, directions 0..7 clockwise from up. Axis direction 1 is positive. */
static const struct { uint16_t kind, index, dir, action; } layout[] = {
    {2, 0, 0, JUMP}, {2, 1, 0, MELEE}, {2, 2, 0, RELOAD}, {2, 3, 0, SWITCH_WEAPON},
    {2, 4, 0, SWITCH_GRENADE}, {2, 5, 0, ACTION}, {2, 6, 0, SHOW_SCORES}, {2, 8, 0, CROUCH}, {2, 9, 0, ZOOM},
    {1, 0, 1, RIGHT}, {1, 0, 2, LEFT}, {1, 1, 1, BACKWARD}, {1, 1, 2, FORWARD},
    {1, 2, 1, THROW_GRENADE}, {1, 2, 2, FIRE},
    {1, 3, 1, LOOK_RIGHT}, {1, 3, 2, LOOK_LEFT}, {1, 4, 1, LOOK_DOWN}, {1, 4, 2, LOOK_UP},
    {3, 0, 0, FLASHLIGHT},
};

static uint32_t read32(uint32_t p) { uint32_t v; memcpy(&v, halopad_guest_ptr(p), 4); return v; }
static uint16_t read16(uint32_t p) { uint16_t v; memcpy(&v, halopad_guest_ptr(p), 2); return v; }
static int unbound(uint32_t p, unsigned size)
{
    for (unsigned i = 0; i < size; i += 2) if (read16(p + i) != 0x7fff) return 0;
    return 1;
}
static int slot_empty(int slot)
{
    return unbound(0x6ab426 + slot * 64, 64) && unbound(0x6ab536 + slot * 128, 128) && unbound(0x6ab736 + slot * 256, 256);
}
static int is_physical(unsigned device)
{
    if (device >= 8 || device >= read32(0x64c774) || !read32(0x64c778 + device * 4)) return 0;
    static const char touch[] = "HaloPad Touch Move";
    const uint16_t *name = halopad_guest_ptr(0x64c798 + 0x240 * device);
    for (unsigned i = 0; i < sizeof touch; i++) if (name[i] != (uint8_t)touch[i]) return 1;
    return 0;
}
static int bind(int slot, uint16_t kind, uint16_t index, uint16_t dir, uint16_t action)
{
    /* Descriptor read by 0x48e360: device 3 (joystick), slot, kind (1 axis,
       2 hat, otherwise button), object index, then a 32-bit direction. */
    uint16_t descriptor[6] = {3, (uint16_t)slot, kind == 2 ? 0 : kind == 3 ? 2 : 1, index, dir, 0};
    uint32_t p = halopad_heap_alloc(sizeof descriptor, 0);
    memcpy(halopad_guest_ptr(p), descriptor, sizeof descriptor);
    halopad_cpu->_ebx = action;
    int ok = halopad_call_guest_ex(0x48e360, 0, NULL, p, 0) & 0xff;
    halopad_heap_free(p);
    return ok;
}
static void command(const char *text)
{
    uint32_t p = halopad_heap_alloc((uint32_t)strlen(text) + 1, 0);
    memcpy(halopad_guest_ptr(p), text, strlen(text) + 1);
    halopad_call_guest_ex(0x487030, 1, &p, 0, 0);
    halopad_heap_free(p);
}

int halopad_gamepad_defaults_apply(int slot)
{
    if (slot < 0 || slot >= 4 || !slot_empty(slot)) return 0;
    int applied = 0;
    for (unsigned i = 0; i < sizeof layout / sizeof *layout; i++)
        applied += bind(slot, layout[i].kind, layout[i].index, layout[i].dir, layout[i].action);
    return applied;
}

/* Halo's thread, once per presented frame while a profile is in play. */
void halopad_gamepad_defaults_update(int in_game)
{
    static uint32_t done;                 /* devices handled this session */
    static uint32_t logged_slot[8] = {UINT32_MAX - 1, UINT32_MAX - 1, UINT32_MAX - 1, UINT32_MAX - 1,
                                      UINT32_MAX - 1, UINT32_MAX - 1, UINT32_MAX - 1, UINT32_MAX - 1};
    static uint32_t held_slot[8] = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    static int logged_count = -1;
    uint32_t count = read32(0x64c774);
    if (count > 8) return;
    if ((int)count != logged_count) { HP_LOG("Halo: %u controller device(s) enumerated", count); logged_count = (int)count; }
    for (unsigned device = 0; device < count; device++) {
        uint32_t slot = read32(0x64c9c8 + device * 0x240);
        if (slot == logged_slot[device]) continue;
        logged_slot[device] = slot;
        if (slot == UINT32_MAX) HP_LOG("Halo: %s controller device %u is not assigned to a player", is_physical(device) ? "physical" : "touch-move", device);
        else HP_LOG("Halo: %s controller device %u assigned to player slot %u", is_physical(device) ? "physical" : "touch-move", device, slot);
    }
    if (!in_game) return;
    _cpu saved = *halopad_cpu;
    for (unsigned device = 0; device < read32(0x64c774); device++) {
        if (!is_physical(device)) continue;
        uint32_t now = read32(0x64c9c8 + device * 0x240);
        if (done & 1u << device) {
            /* Once set up, a controller that Halo later unassigns returns to its
               player slot if that slot has no controller of its own. */
            if (now == UINT32_MAX && held_slot[device] < 4 && read32(0x64dc18 + held_slot[device] * 4) == UINT32_MAX) {
                char text[48];
                snprintf(text, sizeof text, "input_activate_joy %u %u", device, held_slot[device]);
                command(text);
                HP_LOG("HaloPad: reattached controller device %u to player slot %u", device, held_slot[device]);
            }
            if (now < 4) held_slot[device] = now;
            continue;
        }
        uint32_t slot = read32(0x64c9c8 + device * 0x240);
        if (slot == UINT32_MAX) {
            for (int s = 0; s < 4; s++)
                if (read32(0x64dc18 + s * 4) == UINT32_MAX && slot_empty(s)) {
                    char text[48];
                    snprintf(text, sizeof text, "input_activate_joy %u %d", device, s);
                    command(text);
                    break;
                }
            slot = read32(0x64c9c8 + device * 0x240);
            if (slot == UINT32_MAX) { done |= 1u << device; continue; }  /* no free slot: leave it */
        }
        int n = slot < 4 ? halopad_gamepad_defaults_apply((int)slot) : 0;
        fprintf(stderr, "HALOPAD CONTROLLER: device %u slot %d: %s\n", device, (int)slot,
                n ? "applied Halo's Xbox controller layout" : "kept the profile's bindings");
        HP_LOG("HaloPad: controller device %u slot %d: %s", device, (int)slot,
                    n ? "applied Halo's Xbox controller layout" : "kept the profile's bindings");
        if (slot < 4) held_slot[device] = slot;
        done |= 1u << device;
    }
    *halopad_cpu = saved;
}
