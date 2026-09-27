/* HaloPad host input (G9): what the host layer (AppKit now, UIKit later) reports to the
 * Windows side. USER32 turns these into window messages and key state; DirectInput will
 * read the same stream. Mouse positions are client pixels of the game window. */
#ifndef HALOPAD_INPUT_H
#define HALOPAD_INPUT_H
#include <stdint.h>

enum { HPI_KEY, HPI_MOUSEMOVE, HPI_BUTTON, HPI_WHEEL, HPI_ACTIVATE, HPI_CLOSE };

typedef struct {
    int kind;
    uint32_t vk, side_vk, scan;          /* keys: Windows virtual key, the left/right variant, set-1 scan code */
    int extended, down;                  /* keys: E0 prefix; keys, buttons, activation: pressed or active */
    uint16_t chars[4];                   /* key down: the characters typed (UTF-16) */
    int nchars;
    int32_t x, y;                        /* mouse: client pixels */
    int button;                          /* 0 left, 1 right, 2 middle */
    int32_t wheel;                       /* WHEEL_DELTA units (120 per notch) */
} hp_input;

void halopad_input_event(const hp_input *e);
/* A Mac virtual key code (kVK_*) as a Windows key; 0 if there is no equivalent. */
int halopad_mac_key(uint16_t keycode, uint32_t *vk, uint32_t *side_vk, uint32_t *scan, int *extended);
#endif
