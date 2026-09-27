/* HaloPad dialog boxes (G3): what USER32's dialog manager hands the host to show.
 *
 * Halo's dialogs (its warning and error box, 0x582060) are real Win32 dialogs: USER32 builds
 * the dialog window and its controls from the template, Halo's dialog procedure fills them in
 * and handles the commands, and the controls keep their text, visibility, enabled and check
 * state. The host shows that state natively and reports which control the player activated;
 * USER32 turns it into the messages Windows would send (BM_CLICK, the static's button-up,
 * the close box). Positions are client pixels from the template's dialog units. */
#ifndef HALOPAD_DIALOG_H
#define HALOPAD_DIALOG_H
#include <stdint.h>

enum { HPD_BUTTON, HPD_CHECKBOX, HPD_TEXT, HPD_LINK, HPD_ICON };
#define HPD_CLOSE (-2)          /* the player closed the dialog (the close box) */
#define HPD_NO_SCREEN (-1)      /* the host has no screen to show dialogs on */
#define HPD_MAX_ITEMS 32

typedef struct {
    uint32_t id;
    int kind, enabled, checked, is_default;
    int align;                  /* text: 0 left, 1 centre, 2 right */
    uint32_t color;             /* text colour as COLORREF (0x00BBGGRR), as the dialog's WM_CTLCOLORSTATIC sets it */
    int32_t x, y, w, h;         /* client pixels */
    char text[1024];            /* Windows-1252; mnemonic ampersands removed; for HPD_ICON the resource id as "#n" */
} halopad_dialog_item;

typedef struct {
    char title[256];
    int32_t w, h;               /* client pixels */
    int count;
    halopad_dialog_item item[HPD_MAX_ITEMS];
} halopad_dialog_view;

/* Shows the dialog and waits for the player. Returns the index of the item the player
   activated (a button, the checkbox or the link), HPD_CLOSE, or HPD_NO_SCREEN. Called on
   Halo's thread; the dialog is shown again after every action until Halo ends it. */
int halopad_host_dialog(const halopad_dialog_view *v);
/* The dialog has ended: the host takes it off the screen. */
void halopad_host_dialog_done(void);
/* Opens a URL (ShellExecuteA "open"): 1 if a handler took it. */
int halopad_host_open_url(const char *url);
#endif
