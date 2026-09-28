/* HaloPad host input (G9): what the host layer (AppKit now, UIKit later) reports to the
 * Windows side. USER32 turns these into window messages and key state; DirectInput will
 * read the same stream. Mouse positions are client pixels of the game window. */
#ifndef HALOPAD_INPUT_H
#define HALOPAD_INPUT_H
#include <stdint.h>

enum { HPI_KEY, HPI_MOUSEMOVE, HPI_BUTTON, HPI_WHEEL, HPI_ACTIVATE, HPI_CLOSE, HPI_CANCEL_TOUCH };
enum { HPI_TOUCH = 1u };                 /* cancelable gameplay controls, separate from hardware/text input */

typedef struct {
    int kind;
    uint32_t vk, side_vk, scan;          /* keys: Windows virtual key, the left/right variant, set-1 scan code */
    int extended, down;                  /* keys: E0 prefix; keys, buttons, activation: pressed or active */
    uint16_t chars[4];                   /* key down: the characters typed (UTF-16) */
    int nchars;
    int32_t x, y;                        /* mouse: client pixels */
    int32_t dx, dy;                      /* mouse moves: relative counts (DirectInput) */
    int button;                          /* 0 left, 1 right, 2 middle */
    int32_t wheel;                       /* WHEEL_DELTA units (120 per notch) */
    uint32_t flags;
} hp_input;

void halopad_input_event(const hp_input *e);
void halopad_dinput_event(const hp_input *e);       /* halopad_dinput.c, fed by halopad_input_event */
/* A Mac virtual key code (kVK_*) as a Windows key; 0 if there is no equivalent. */
int halopad_mac_key(uint16_t keycode, uint32_t *vk, uint32_t *side_vk, uint32_t *scan, int *extended);
/* A USB HID keyboard usage (UIKit's UIKey.keyCode) as a Windows key; 0 if there is no equivalent. */
int halopad_hid_key(uint16_t usage, uint32_t *vk, uint32_t *side_vk, uint32_t *scan, int *extended);
/* iOS: the app shell queues events from the main thread; halopad_host_pump delivers them on Halo's
   thread (port/apple/halopad_host_ios.m). */
void halopad_host_post_input(const hp_input *e);

/* Game controllers the host has now (port/apple/halopad_gamepad.m: GameController's extended
   gamepads). Sticks -1..1 with up positive, triggers 0..1; buttons: bit 0 A, 1 B, 2 X, 3 Y,
   4 left shoulder, 5 right shoulder, 6 Back (Options), 7 Start (Menu), 8 left stick, 9 right
   stick; dpad -1 centred or 0..7 clockwise from up. id stays the same while it is connected. */
typedef struct { uint32_t id; float lx, ly, rx, ry, lt, rt; uint32_t buttons; int dpad; } hp_gamepad;
int halopad_host_gamepads(hp_gamepad *out, int max);
#endif
