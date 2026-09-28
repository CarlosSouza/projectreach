/* HaloPad DirectInput 8 (G9): the system keyboard and mouse on HaloPad's host input.
 *
 * How Halo uses it (haloce.exe): DirectInput8Create, then
 *  - keyboard (0x4946b7): GUID_SysKeyboard, NONEXCLUSIVE|FOREGROUND|NOWINKEY,
 *    c_dfDIKeyboard, DIPROP_BUFFERSIZE 32; read one DIDEVICEOBJECTDATA at a time with
 *    GetDeviceData, re-Acquire on DIERR_INPUTLOST/NOTACQUIRED, flush on overflow (0x4935b0);
 *  - mouse (0x4947b2): GUID_SysMouse, EXCLUSIVE|FOREGROUND, c_dfDIMouse2; GetDeviceState of
 *    DIMOUSESTATE2 each frame; DIPROP_GRANULARITY of the wheel (0x493450);
 *  - game controllers: EnumDevices(DI8DEVCLASS_GAMECTRL, ATTACHEDONLY) (0x49492f); for each,
 *    CreateDevice, SetCooperativeLevel(EXCLUSIVE|FOREGROUND), SetDataFormat with an 80-object
 *    format it builds at run time (224 bytes), GetCapabilities, and EnumObjects, whose callback
 *    (0x494a10) sets DIPROP_RANGE -4096..4096 and a 10% DIPROP_DEADZONE on every axis.
 * Keys arrive as set-1 scan codes (DIK = scan | 0x80 when extended), transitions only;
 * the mouse gives relative counts from the host's deltas, the wheel in WHEEL_DELTA units.
 * Foreground devices lose acquisition when their window stops being the foreground one
 * (DIERR_INPUTLOST once, then DIERR_NOTACQUIRED). An exclusive mouse captures the host
 * pointer (hidden, detached from the cursor) while acquired.
 *
 * Game controllers: each controller the host has (halopad_host_gamepads) is offered as Windows
 * XP offers an Xbox 360 controller to DirectInput, "Controller (XBOX 360 For Windows)" (VID 045E,
 * PID 028E, a HID gamepad): X/Y the left stick, Rx/Ry the right stick, Z the triggers (the left
 * one toward the maximum, the right one toward the minimum), buttons 0-9 A, B, X, Y, LB, RB,
 * Back, Start, left and right stick, and one hat switch (hundredths of a degree, 0xFFFFFFFF
 * centred). Any data format is matched object by object as DirectInput does; axes honour
 * DIPROP_RANGE (default 0..65535), DIPROP_DEADZONE and DIPROP_SATURATION. It is a polled device:
 * Poll takes a snapshot that GetDeviceState reports. A controller that goes away makes the
 * device report DIERR_INPUTLOST, then DIERR_NOTACQUIRED, and Acquire DIERR_UNPLUGGED. */
#include "halopad_win32.h"
#include <math.h>
#include "halopad_input.h"

uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *));
uint32_t halopad_com_addref(uint32_t g);
uint32_t halopad_com_release(uint32_t g);
void *halopad_com_state(const char *iface, uint32_t g);
uint32_t GetForegroundWindow_c(void);
uint32_t GetTickCount_c(void);
void halopad_host_mouse_capture(int on);

#define DI_OK 0u
#define DI_NOEFFECT 1u                  /* S_FALSE, also DI_BUFFEROVERFLOW */
#define DIERR_INVALIDPARAM 0x80070057u
#define DIERR_NOTACQUIRED 0x8007000Cu
#define DIERR_INPUTLOST 0x8007001Eu
#define DIERR_ACQUIRED 0x800700AAu
#define DIERR_OTHERAPPHASPRIO 0x80070005u
#define DIERR_NOTBUFFERED 0x80040207u
#define DIERR_DEVICENOTREG 0x80040154u
#define DIERR_OLDDIRECTINPUTVERSION 0x8007047Eu
#define DIERR_BETADIRECTINPUTVERSION 0x80070481u
#define DIERR_NOAGGREGATION 0x80040110u
#define DIERR_UNSUPPORTED 0x80004001u
#define E_NOINTERFACE 0x80004002u
#define DIERR_UNPLUGGED 0x80040209u
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void halopad_heap_free(uint32_t a);

enum { KEYBOARD = 1, MOUSE = 2, GAMEPAD = 3 };
#define NOBJ 16                         /* gamepad objects: 5 axes, 10 buttons, 1 hat */
typedef struct { uint32_t ofs, data, time, seq, flags; } event;
#define MAXBUF 1024
typedef struct {
    uint32_t guest, kind, hwnd, coop, data_size;
    int formatted, acquired, lost;
    uint32_t bufsize, head, count;
    int overflow;
    event buf[MAXBUF];
    uint8_t keys[256], key_physical[256], key_touch[256], key_read[256], key_touch_dirty[256];
    int32_t dx, dy, dz;
    int32_t touch_dx, touch_dy;         /* unread look contribution, independently cancelable */
    uint8_t buttons[8], touch_raw[8], touch_read[8];
    uint32_t touch_edges[8]; /* alternating edges, consumed by successful unbuffered reads */
    /* game controllers */
    uint32_t pad_id;
    hp_gamepad snap;                    /* the state at the last Poll or Acquire */
    int32_t lo[5], hi[5];               /* DIPROP_RANGE per axis */
    uint32_t dead[5], sat[5];           /* DIPROP_DEADZONE, DIPROP_SATURATION (1/10000) */
    int32_t ofs[NOBJ];                  /* each object's offset in the data format, -1 unmatched */
    uint32_t hat_fill[16], nhat_fill;   /* hat entries of the format no object matched */
    int unplugged;
} device;

static device *devices[8];
static uint32_t sequence;
static int touch_move_enabled, touch_move_active = 1;
static hp_gamepad touch_move = {.id = HP_TOUCH_MOVE_ID, .dpad = -1};

static void reset_touch_move(void)
{
    touch_move = (hp_gamepad){.id = HP_TOUCH_MOVE_ID, .dpad = -1};
    /* Cancel even a snapshot that was polled before a native menu opened. */
    for (int i = 0; i < 8; i++)
        if (devices[i] && devices[i]->kind == GAMEPAD && devices[i]->pad_id == HP_TOUCH_MOVE_ID)
            devices[i]->snap = touch_move;
}
void halopad_touch_move_enable(int enabled)
{
    touch_move_enabled = !!enabled;
    reset_touch_move();
}

static int guid_is(uint32_t g, uint32_t d1, uint16_t d2, uint16_t d3, const uint8_t d4[8])
{
    return g && rd32(g) == d1 && (rd32(g + 4) & 0xFFFF) == d2 && rd32(g + 4) >> 16 == d3 && !memcmp(G(g + 8), d4, 8);
}
static const uint8_t sys_tail[8] = {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

static uint32_t enum_pads(uint32_t cb, uint32_t ref);
static uint32_t pad_for_guid(uint32_t guid);
static void pad_defaults(device *d);
static int pad_now(uint32_t id, hp_gamepad *out);
static uint32_t pad_format(device *d, uint32_t flags, uint32_t size, uint32_t n, uint32_t fobjs);
static void pad_state(device *d, uint32_t data);
static uint32_t pad_set_property(device *d, uint32_t prop, uint32_t hdr);
static uint32_t pad_get_property(device *d, uint32_t prop, uint32_t hdr);

/* ---- DirectInput8Create and IDirectInput8A ---- */

static void di_destroy(void *p) { free(p); }

uint32_t DirectInput8Create_c(uint32_t instance, uint32_t version, uint32_t riid, uint32_t out, uint32_t outer)
{
    (void)instance;
    static const uint8_t iid_tail[8] = {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00};
    if (!out) return DIERR_INVALIDPARAM;
    wr32(out, 0);
    if (outer) return DIERR_NOAGGREGATION;
    if (version < 0x0800) return DIERR_OLDDIRECTINPUTVERSION;
    if (version > 0x0800) return DIERR_BETADIRECTINPUTVERSION;
    if (!guid_is(riid, 0xBF798030, 0x483A, 0x4DA2, iid_tail)) hp_unsupported("DirectInput8Create", "an interface other than IDirectInput8A");
    uint32_t *state = calloc(1, sizeof *state);
    wr32(out, halopad_com_new("IDirectInput8A", 4, state, di_destroy));
    return DI_OK;
}

uint32_t hpcom_IDirectInput8A_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    (void)out; halopad_com_state("IDirectInput8A", g);
    hp_unsupported("IDirectInput8A::QueryInterface", "interface %08x-...", rd32(iid));
}
uint32_t hpcom_IDirectInput8A_AddRef_c(uint32_t g) { halopad_com_state("IDirectInput8A", g); return halopad_com_addref(g); }
uint32_t hpcom_IDirectInput8A_Release_c(uint32_t g) { halopad_com_state("IDirectInput8A", g); return halopad_com_release(g); }

static void dev_destroy(void *p)
{
    device *d = p;
    if (d->acquired && d->kind == MOUSE && (d->coop & 1)) halopad_host_mouse_capture(0);
    for (int i = 0; i < 8; i++) if (devices[i] == d) devices[i] = NULL;
    free(d);
}

uint32_t hpcom_IDirectInput8A_CreateDevice_c(uint32_t g, uint32_t guid, uint32_t out, uint32_t outer)
{
    halopad_com_state("IDirectInput8A", g);
    if (!out || !guid) return DIERR_INVALIDPARAM;
    wr32(out, 0);
    if (outer) return DIERR_NOAGGREGATION;
    uint32_t pad = 0;
    uint32_t kind = guid_is(guid, 0x6F1D2B61, 0xD5A0, 0x11CF, sys_tail) ? KEYBOARD
                  : guid_is(guid, 0x6F1D2B60, 0xD5A0, 0x11CF, sys_tail) ? MOUSE
                  : (pad = pad_for_guid(guid)) ? GAMEPAD : 0;
    if (!kind) return DIERR_DEVICENOTREG;
    int slot = -1;
    for (int i = 0; i < 8; i++) if (!devices[i]) { slot = i; break; }
    if (slot < 0) hp_unsupported("IDirectInput8A::CreateDevice", "more than 8 devices");
    device *d = calloc(1, sizeof *d);
    d->kind = kind;
    if (kind == GAMEPAD) { d->pad_id = pad; pad_defaults(d); }
    d->guest = halopad_com_new("IDirectInputDevice8A", 4, d, dev_destroy);
    devices[slot] = d;
    wr32(out, d->guest);
    return DI_OK;
}

uint32_t hpcom_IDirectInput8A_EnumDevices_c(uint32_t g, uint32_t type, uint32_t cb, uint32_t ref, uint32_t flags)
{
    halopad_com_state("IDirectInput8A", g);
    if (!cb) return DIERR_INVALIDPARAM;
    if (type == 4 /* DI8DEVCLASS_GAMECTRL */) {
        if (flags & ~0x1u) hp_unsupported("IDirectInput8A::EnumDevices", "game controllers with flags 0x%x", flags);   /* ATTACHEDONLY */
        return enum_pads(cb, ref);
    }
    hp_unsupported("IDirectInput8A::EnumDevices", "device class %u (flags 0x%x)", type, flags);
}

/* ---- IDirectInputDevice8A ---- */

static device *D(uint32_t g) { return halopad_com_state("IDirectInputDevice8A", g); }

uint32_t hpcom_IDirectInputDevice8A_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    (void)out; D(g);
    hp_unsupported("IDirectInputDevice8A::QueryInterface", "interface %08x-...", rd32(iid));
}
uint32_t hpcom_IDirectInputDevice8A_AddRef_c(uint32_t g) { D(g); return halopad_com_addref(g); }
uint32_t hpcom_IDirectInputDevice8A_Release_c(uint32_t g) { D(g); return halopad_com_release(g); }

static void record_source(device *d, uint32_t ofs, uint32_t data, uint32_t flags);

/* Remove unread virtual keys, then reconcile what the buffered reader has seen
   with surviving physical input. Never flush unrelated keyboard events. */
static void cancel_touch_keys(device *d)
{
    if (d->kind != KEYBOARD) return;
    uint8_t projected[256];
    memcpy(projected, d->key_read, sizeof projected);
    uint32_t kept = 0;
    for (uint32_t i = 0; i < d->count; i++) {
        event e = d->buf[(d->head + i) % MAXBUF];
        if (e.flags & HPI_TOUCH) continue;
        d->buf[(d->head + kept++) % MAXBUF] = e;
        projected[e.ofs] = (uint8_t)e.data;
    }
    d->count = kept;
    for (uint32_t k = 0; k < 256; k++) {
        if (d->key_touch_dirty[k] && projected[k] != d->key_physical[k])
            record_source(d, k, d->key_physical[k], 0);
        d->keys[k] = d->key_physical[k];
    }
    memset(d->key_touch, 0, sizeof d->key_touch);
    memset(d->key_touch_dirty, 0, sizeof d->key_touch_dirty);
}

static void clear_touch(device *d)
{
    if (d->kind == MOUSE) {
        d->dx -= d->touch_dx; d->dy -= d->touch_dy;
        d->touch_dx = d->touch_dy = 0;
        uint32_t kept = 0;
        for (uint32_t i = 0; i < d->count; i++) {
            event e = d->buf[(d->head + i) % MAXBUF];
            if ((e.flags & HPI_TOUCH) && (e.ofs == 0 || e.ofs == 4)) continue;
            d->buf[(d->head + kept++) % MAXBUF] = e;
        }
        d->count = kept;
    }
    memset(d->touch_raw, 0, sizeof d->touch_raw);
    memset(d->touch_read, 0, sizeof d->touch_read);
    memset(d->touch_edges, 0, sizeof d->touch_edges);
}

static void set_acquired(device *d, int on)
{
    if (d->acquired == on) return;
    d->acquired = on;
    if (!on && d->kind == GAMEPAD && d->pad_id == HP_TOUCH_MOVE_ID) reset_touch_move();
    if (!on) cancel_touch_keys(d);
    clear_touch(d);
    if (d->kind == MOUSE && (d->coop & 1)) halopad_host_mouse_capture(on);
    if (on) { memset(d->key_physical, 0, sizeof d->key_physical); memset(d->key_touch, 0, sizeof d->key_touch); memset(d->keys, 0, sizeof d->keys); memset(d->buttons, 0, sizeof d->buttons); d->dx = d->dy = d->dz = 0; }
}

/* A foreground device whose window lost the foreground is no longer acquired. */
static uint32_t check_acquired(device *d)
{
    if (d->acquired && (d->coop & 4) && GetForegroundWindow_c() != d->hwnd) { set_acquired(d, 0); d->lost = 1; }
    if (d->acquired) return DI_OK;
    if (d->lost) { d->lost = 0; return DIERR_INPUTLOST; }
    return DIERR_NOTACQUIRED;
}

uint32_t hpcom_IDirectInputDevice8A_SetCooperativeLevel_c(uint32_t g, uint32_t hwnd, uint32_t flags)
{
    device *d = D(g);
    if (flags & ~0x1Fu) return DIERR_INVALIDPARAM;
    int excl = flags & 1, nonexcl = flags & 2, fg = flags & 4, bg = flags & 8;
    if (!!excl == !!nonexcl) return DIERR_INVALIDPARAM;              /* exactly one of each pair */
    if (!!fg == !!bg) return DIERR_INVALIDPARAM;
    if ((flags & 0x10) && d->kind != KEYBOARD) return DIERR_INVALIDPARAM;
    if (d->kind == KEYBOARD && excl && bg) return DIERR_UNSUPPORTED;
    if (!hwnd && fg) return DIERR_INVALIDPARAM;
    if (d->acquired) return DIERR_ACQUIRED;
    d->hwnd = hwnd; d->coop = flags;
    return DI_OK;
}

/* Only the standard formats Halo links: c_dfDIKeyboard, c_dfDIMouse, c_dfDIMouse2. */
uint32_t hpcom_IDirectInputDevice8A_SetDataFormat_c(uint32_t g, uint32_t fmt)
{
    device *d = D(g);
    if (!fmt || rd32(fmt) != 24 || rd32(fmt + 4) != 16) return DIERR_INVALIDPARAM;
    if (d->acquired) return DIERR_ACQUIRED;
    uint32_t flags = rd32(fmt + 8), size = rd32(fmt + 12), n = rd32(fmt + 16), objs = rd32(fmt + 20);
    if (d->kind == GAMEPAD) {
        uint32_t r = pad_format(d, flags, size, n, objs);
        if (r) return r;
    } else if (d->kind == KEYBOARD) {
        if (size != 256 || n != 256) hp_unsupported("IDirectInputDevice8A::SetDataFormat", "keyboard format of %u bytes, %u objects", size, n);
        for (uint32_t i = 0; i < n; i++)
            if (rd32(objs + 16 * i + 4) != i || (rd32(objs + 16 * i + 8) & 0xFFFF) != ((i << 8) | 0x0C))
                hp_unsupported("IDirectInputDevice8A::SetDataFormat", "a keyboard format other than c_dfDIKeyboard (object %u)", i);
    } else {
        if (!((size == 16 && n == 7) || (size == 20 && n == 11)) || (flags & 1))
            hp_unsupported("IDirectInputDevice8A::SetDataFormat", "mouse format of %u bytes, %u objects, flags 0x%x", size, n, flags);
        for (uint32_t i = 0; i < n; i++) {
            uint32_t ofs = rd32(objs + 16 * i + 4), type = rd32(objs + 16 * i + 8) & 0xFF;
            if (ofs != (i < 3 ? 4 * i : 12 + (i - 3)) || type != (i < 3 ? 0x03u : 0x0Cu))
                hp_unsupported("IDirectInputDevice8A::SetDataFormat", "a mouse format other than c_dfDIMouse(2) (object %u)", i);
        }
    }
    d->data_size = size;
    d->formatted = 1;
    return DI_OK;
}

uint32_t hpcom_IDirectInputDevice8A_SetProperty_c(uint32_t g, uint32_t prop, uint32_t hdr)
{
    device *d = D(g);
    if (!hdr || rd32(hdr + 4) != 16) return DIERR_INVALIDPARAM;
    if (d->kind == GAMEPAD && prop != 1) return pad_set_property(d, prop, hdr);
    switch (prop) {
    case 1:                                                         /* DIPROP_BUFFERSIZE */
        if (rd32(hdr) != 20 || rd32(hdr + 12) != 0) return DIERR_INVALIDPARAM;   /* DIPH_DEVICE */
        if (d->acquired) return DIERR_ACQUIRED;
        if (rd32(hdr + 16) > MAXBUF) hp_unsupported("DIPROP_BUFFERSIZE", "%u events", rd32(hdr + 16));
        d->bufsize = rd32(hdr + 16); d->head = d->count = 0; d->overflow = 0;
        return DI_OK;
    case 2:                                                         /* DIPROP_AXISMODE */
        if (d->kind != MOUSE || rd32(hdr + 16) != 0) hp_unsupported("DIPROP_AXISMODE", "absolute axes");
        return DI_OK;
    }
    hp_unsupported("IDirectInputDevice8A::SetProperty", "property %u", prop);
}

uint32_t hpcom_IDirectInputDevice8A_GetProperty_c(uint32_t g, uint32_t prop, uint32_t hdr)
{
    device *d = D(g);
    if (!hdr || rd32(hdr + 4) != 16) return DIERR_INVALIDPARAM;
    if (d->kind == GAMEPAD && prop != 1) return pad_get_property(d, prop, hdr);
    switch (prop) {
    case 1: wr32(hdr + 16, d->bufsize); return DI_OK;
    case 2: wr32(hdr + 16, 0); return DI_OK;                        /* relative */
    case 3:                                                         /* DIPROP_GRANULARITY */
        if (d->kind != MOUSE) break;
        if (rd32(hdr + 12) == 1 && rd32(hdr + 8) == 8) { wr32(hdr + 16, 120); return DI_OK; }   /* the wheel: WHEEL_DELTA */
        if (rd32(hdr + 12) == 1 && rd32(hdr + 8) < 8) { wr32(hdr + 16, 1); return DI_OK; }
        break;
    }
    hp_unsupported("IDirectInputDevice8A::GetProperty", "property %u (object %u, how %u)", prop, rd32(hdr + 8), rd32(hdr + 12));
}

uint32_t hpcom_IDirectInputDevice8A_Acquire_c(uint32_t g)
{
    device *d = D(g);
    if (!d->formatted) return DIERR_INVALIDPARAM;
    if ((d->coop & 4) && GetForegroundWindow_c() != d->hwnd) return DIERR_OTHERAPPHASPRIO;
    if (d->kind == GAMEPAD && !pad_now(d->pad_id, &d->snap)) { d->unplugged = 1; return DIERR_UNPLUGGED; }
    d->lost = 0;
    if (d->acquired) return DI_NOEFFECT;
    set_acquired(d, 1);
    return DI_OK;
}

uint32_t hpcom_IDirectInputDevice8A_Unacquire_c(uint32_t g)
{
    device *d = D(g);
    if (!d->acquired) return DI_NOEFFECT;
    set_acquired(d, 0);
    return DI_OK;
}

uint32_t hpcom_IDirectInputDevice8A_Poll_c(uint32_t g)
{
    device *d = D(g);
    uint32_t r = check_acquired(d);
    if (r) return r;
    if (d->kind != GAMEPAD) return DI_NOEFFECT;                     /* keyboard and mouse are not polled devices */
    if (!pad_now(d->pad_id, &d->snap)) { set_acquired(d, 0); d->lost = 0; return DIERR_INPUTLOST; }   /* unplugged */
    return DI_OK;
}

uint32_t hpcom_IDirectInputDevice8A_GetDeviceState_c(uint32_t g, uint32_t size, uint32_t data)
{
    device *d = D(g);
    if (!data || !d->formatted || size != d->data_size) return DIERR_INVALIDPARAM;
    uint32_t r = check_acquired(d);
    if (r) return r;
    if (d->kind == KEYBOARD) { memcpy(G(data), d->keys, 256); return DI_OK; }
    if (d->kind == GAMEPAD) { pad_state(d, data); return DI_OK; }
    wr32(data, (uint32_t)d->dx); wr32(data + 4, (uint32_t)d->dy); wr32(data + 8, (uint32_t)d->dz);
    for (uint32_t b = 0; b < size - 12; b++) {
        if (!d->bufsize && d->touch_edges[b]) {
            d->touch_edges[b]--;
            d->touch_read[b] ^= 0x80;
        }
        ((uint8_t *)G(data))[12 + b] = d->buttons[b] | (d->bufsize ? d->touch_raw[b] : d->touch_read[b]);
    }
    d->dx = d->dy = d->dz = 0;                                      /* relative: counts since the last read */
    d->touch_dx = d->touch_dy = 0;
    return DI_OK;
}

uint32_t hpcom_IDirectInputDevice8A_GetDeviceData_c(uint32_t g, uint32_t objsize, uint32_t out, uint32_t inout, uint32_t flags)
{
    device *d = D(g);
    if ((objsize != 16 && objsize != 20) || !inout || (flags & ~1u)) return DIERR_INVALIDPARAM;
    if (!d->bufsize) return DIERR_NOTBUFFERED;
    uint32_t r = check_acquired(d);
    if (r) return r;
    uint32_t want = rd32(inout), n = want < d->count ? want : d->count;
    for (uint32_t i = 0; out && i < n; i++) {
        const event *e = &d->buf[(d->head + i) % MAXBUF];
        if (d->kind == KEYBOARD && e->ofs == 0x29 && getenv("HALOPAD_TRACE_INPUT"))
            fprintf(stderr, "HALOPAD INPUT: console key read by DirectInput: %s (flags %u)\n", e->data ? "down" : "up", flags);
        if (d->kind == KEYBOARD) d->key_read[e->ofs] = (uint8_t)e->data;
        uint32_t o = out + i * objsize;
        wr32(o, e->ofs); wr32(o + 4, e->data); wr32(o + 8, e->time); wr32(o + 12, e->seq);
        if (objsize == 20) wr32(o + 16, 0);                         /* uAppData */
    }
    if (!(flags & 1)) { d->head = (d->head + n) % MAXBUF; d->count -= n; }   /* not DIGDD_PEEK */
    wr32(inout, n);
    uint32_t res = d->overflow ? DI_NOEFFECT : DI_OK;               /* DI_BUFFEROVERFLOW */
    if (!(flags & 1)) d->overflow = 0;
    return res;
}

/* ---- game controllers: the host's pads as Windows XP's Xbox 360 controller ---- */

hp_gamepad halopad_gamepad_test[4];     /* tests: when count >= 0, these replace the host's */
int halopad_gamepad_test_count = -1;

static int pads(hp_gamepad *out, int max)
{
    int n;
    if (halopad_gamepad_test_count >= 0) {
        n = halopad_gamepad_test_count < max ? halopad_gamepad_test_count : max;
        memcpy(out, halopad_gamepad_test, sizeof *out * (size_t)n);
    } else n = halopad_host_gamepads(out, max);
    if (touch_move_enabled && n < max) out[n++] = touch_move;
    return n;
}
static int pad_now(uint32_t id, hp_gamepad *out)
{
    hp_gamepad all[8];
    int n = pads(all, 8);
    for (int i = 0; i < n; i++) if (all[i].id == id) { *out = all[i]; return 1; }
    return 0;
}

/* instance GUID {2A7F6B10+id-3E7C-11EF-"HaloPad"}; product GUID {028E045E-0000-0000-"PIDVID"} */
static const uint8_t pad_tail[8] = {0x00, 'H', 'a', 'l', 'o', 'P', 'a', 'd'};
static const uint8_t pidvid[8] = {0x00, 0x00, 'P', 'I', 'D', 'V', 'I', 'D'};
static void put_guid(uint32_t at, uint32_t d1, uint16_t d2, uint16_t d3, const uint8_t d4[8])
{
    wr32(at, d1); wr16(at + 4, d2); wr16(at + 6, d3); memcpy(G(at + 8), d4, 8);
}
#define PAD_DEVTYPE 0x00010215u         /* DI8DEVTYPE_GAMEPAD, standard, HID */
static const char pad_name[] = "Controller (XBOX 360 For Windows)";

static void put_instance(uint32_t at, uint32_t id)
{
    memset(G(at), 0, 580);
    wr32(at, 580);                                          /* DIDEVICEINSTANCEA */
    put_guid(at + 4, 0x2A7F6B10u + id, 0x3E7C, 0x11EF, pad_tail);
    if (id == HP_TOUCH_MOVE_ID) put_guid(at + 0x14, 0x3A7F6B11u, 0x3E7C, 0x11EF, pad_tail);
    else put_guid(at + 0x14, 0x028E045Eu, 0, 0, pidvid);
    wr32(at + 0x24, PAD_DEVTYPE);
    const char *name = id == HP_TOUCH_MOVE_ID ? "HaloPad Touch Move" : pad_name;
    strcpy(G(at + 0x28), name);                            /* instance name */
    strcpy(G(at + 0x12C), name);                           /* product name */
    wr16(at + 0x240, 1); wr16(at + 0x242, 5);               /* HID usage page 1 (generic desktop), usage 5 (game pad) */
}

/* The objects, in DirectInput's order for this device: native offsets are c_dfDIJoystick's. */
typedef struct { uint32_t guid_d1, type, native_ofs; uint16_t page, usage; const char *name; } pad_object;
static const pad_object objs[NOBJ] = {
    {0xA36D02E0, 0x002 | 0 << 8, 0x00, 1, 0x30, "X Axis"}, {0xA36D02E1, 0x002 | 1 << 8, 0x04, 1, 0x31, "Y Axis"},
    {0xA36D02E2, 0x002 | 2 << 8, 0x08, 1, 0x32, "Z Axis"}, {0xA36D02F4, 0x002 | 3 << 8, 0x0C, 1, 0x33, "X Rotation"},
    {0xA36D02F5, 0x002 | 4 << 8, 0x10, 1, 0x34, "Y Rotation"},
    {0xA36D02F0, 0x004 | 0 << 8, 0x30, 9, 1, "Button 0"}, {0xA36D02F0, 0x004 | 1 << 8, 0x31, 9, 2, "Button 1"},
    {0xA36D02F0, 0x004 | 2 << 8, 0x32, 9, 3, "Button 2"}, {0xA36D02F0, 0x004 | 3 << 8, 0x33, 9, 4, "Button 3"},
    {0xA36D02F0, 0x004 | 4 << 8, 0x34, 9, 5, "Button 4"}, {0xA36D02F0, 0x004 | 5 << 8, 0x35, 9, 6, "Button 5"},
    {0xA36D02F0, 0x004 | 6 << 8, 0x36, 9, 7, "Button 6"}, {0xA36D02F0, 0x004 | 7 << 8, 0x37, 9, 8, "Button 7"},
    {0xA36D02F0, 0x004 | 8 << 8, 0x38, 9, 9, "Button 8"}, {0xA36D02F0, 0x004 | 9 << 8, 0x39, 9, 10, "Button 9"},
    {0xA36D02F2, 0x010 | 0 << 8, 0x20, 1, 0x39, "Hat Switch"}};
static const uint8_t obj_tail[8] = {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};   /* GUID_XAxis etc.: -C9F3-11CF-BFC7-... */
static int obj_guid_is(uint32_t g, uint32_t d1) { return guid_is(g, d1, 0xC9F3, 0x11CF, obj_tail); }

/* an axis in 0..1 (0.5 centre) through the dead zone and saturation, into the range */
static int32_t axis_value(const device *d, int a, double v01)
{
    double x = v01 * 2 - 1, m = fabs(x), dz = d->dead[a] / 10000.0, sat = d->sat[a] / 10000.0;
    if (m <= dz) x = 0;
    else if (m >= sat || sat <= dz) x = x < 0 ? -1 : 1;
    else x = (x < 0 ? -1 : 1) * (m - dz) / (sat - dz);
    double lo = d->lo[a], hi = d->hi[a];
    return (int32_t)floor(lo + (x + 1) / 2 * (hi - lo) + 0.5);
}

static void pad_defaults(device *d)
{
    for (int a = 0; a < 5; a++) { d->lo[a] = 0; d->hi[a] = 65535; d->dead[a] = 0; d->sat[a] = 10000; }
    for (int i = 0; i < NOBJ; i++) d->ofs[i] = (int32_t)objs[i].native_ofs;
}

static uint32_t enum_pads(uint32_t cb, uint32_t ref)
{
    hp_gamepad all[8];
    int n = pads(all, 8);
    uint32_t inst = halopad_heap_alloc(580, 1);
    for (int i = 0; i < n; i++) {
        put_instance(inst, all[i].id);
        uint32_t a[2] = {inst, ref};
        if (!halopad_call_guest(cb, 2, a)) break;           /* DIENUM_STOP */
    }
    halopad_heap_free(inst);
    return DI_OK;
}

static uint32_t pad_for_guid(uint32_t guid)
{
    hp_gamepad all[8];
    int n = pads(all, 8);
    for (int i = 0; i < n; i++)
        if (guid_is(guid, 0x2A7F6B10u + all[i].id, 0x3E7C, 0x11EF, pad_tail)) return all[i].id;
    return 0;
}

/* SetDataFormat for a game controller: DirectInput's matching. Each entry takes the first unused
   object of its type class (axis, button, hat), instance (or any) and GUID (or any); an entry
   that matches nothing is an error unless DIDFT_OPTIONAL. */
static uint32_t pad_format(device *d, uint32_t flags, uint32_t size, uint32_t n, uint32_t fobjs)
{
    if (flags & 2) hp_unsupported("IDirectInputDevice8A::SetDataFormat", "relative axes on a game controller");
    if (size & 3) return DIERR_INVALIDPARAM;
    int32_t ofs[NOBJ];
    uint32_t fill[16], nfill = 0;
    for (int i = 0; i < NOBJ; i++) ofs[i] = -1;
    for (uint32_t e = 0; e < n; e++) {
        uint32_t at = fobjs + 16 * e, pg = rd32(at), o = rd32(at + 4), t = rd32(at + 8);
        uint32_t cls = t & 0xFF, inst = (t >> 8) & 0xFFFF;
        int any = inst == 0xFFFF, found = -1;
        for (int i = 0; i < NOBJ && found < 0; i++) {
            if (ofs[i] >= 0 || !(objs[i].type & cls & 0x1F)) continue;
            if (!any && ((objs[i].type >> 8) & 0xFFFF) != inst) continue;
            if (pg && !obj_guid_is(pg, objs[i].guid_d1)) continue;
            found = i;
        }
        uint32_t need = (objs[found < 0 ? 0 : found].type & 0x10) ? 4 : (objs[found < 0 ? 0 : found].type & 0x2) ? 4 : 1;
        if (found >= 0 && o + need > size) return DIERR_INVALIDPARAM;
        if (found >= 0) ofs[found] = (int32_t)o;
        else if (!(t & 0x80000000u)) return DIERR_INVALIDPARAM;
        else if ((cls & 0x10) && nfill < 16) fill[nfill++] = o;   /* an unused hat reads centred */
    }
    memcpy(d->ofs, ofs, sizeof ofs);
    memcpy(d->hat_fill, fill, sizeof fill);
    d->nhat_fill = nfill;
    return DI_OK;
}

static void pad_state(device *d, uint32_t data)
{
    const hp_gamepad *p = &d->snap;
    memset(G(data), 0, d->data_size);
    double v[5] = {(p->lx + 1) / 2, (1 - p->ly) / 2, 0.5 + (p->lt - p->rt) / 2, (p->rx + 1) / 2, (1 - p->ry) / 2};
    for (int a = 0; a < 5; a++) {
        double x = v[a] < 0 ? 0 : v[a] > 1 ? 1 : v[a];
        if (d->ofs[a] >= 0) wr32(data + (uint32_t)d->ofs[a], (uint32_t)axis_value(d, a, x));
    }
    for (int b = 0; b < 10; b++)
        if (d->ofs[5 + b] >= 0) ((uint8_t *)G(data))[d->ofs[5 + b]] = (p->buttons >> b & 1) ? 0x80 : 0;
    if (d->ofs[15] >= 0) wr32(data + (uint32_t)d->ofs[15], p->dpad < 0 ? 0xFFFFFFFFu : (uint32_t)p->dpad * 4500u);
    for (uint32_t i = 0; i < d->nhat_fill; i++) wr32(data + d->hat_fill[i], 0xFFFFFFFFu);
}

/* the object a DIPROPHEADER names (by offset or by ID), or -1 for the whole device */
static int prop_object(device *d, uint32_t hdr, int *err)
{
    uint32_t obj = rd32(hdr + 8), how = rd32(hdr + 12);
    *err = 0;
    if (how == 0) { if (obj) *err = 1; return -1; }         /* DIPH_DEVICE */
    for (int i = 0; i < NOBJ; i++) {
        if (how == 1 && d->ofs[i] >= 0 && (uint32_t)d->ofs[i] == obj) return i;   /* DIPH_BYOFFSET */
        if (how == 2 && (objs[i].type & 0xFFFF1F) == (obj & 0xFFFF1F)) return i;  /* DIPH_BYID */
    }
    *err = 2;                                                /* DIERR_OBJECTNOTFOUND */
    return -1;
}

static uint32_t pad_set_property(device *d, uint32_t prop, uint32_t hdr)
{
    int err, o = prop_object(d, hdr, &err);
    if (err == 1) return DIERR_INVALIDPARAM;
    if (err == 2) return 0x80070002u;                        /* DIERR_OBJECTNOTFOUND */
    if (o >= 5) return DIERR_UNSUPPORTED;                    /* not an axis */
    int first = o < 0 ? 0 : o, last = o < 0 ? 4 : o;
    switch (prop) {
    case 4: {                                                /* DIPROP_RANGE */
        if (rd32(hdr) != 24) return DIERR_INVALIDPARAM;
        int32_t lo = (int32_t)rd32(hdr + 16), hi = (int32_t)rd32(hdr + 20);
        if (lo >= hi) return DIERR_INVALIDPARAM;
        for (int a = first; a <= last; a++) { d->lo[a] = lo; d->hi[a] = hi; }
        return DI_OK;
    }
    case 5: case 6: {                                        /* DIPROP_DEADZONE, DIPROP_SATURATION */
        if (rd32(hdr) != 20) return DIERR_INVALIDPARAM;
        uint32_t v = rd32(hdr + 16);
        if (v > 10000) return DIERR_INVALIDPARAM;
        for (int a = first; a <= last; a++) (prop == 5 ? d->dead : d->sat)[a] = v;
        return DI_OK;
    }
    }
    hp_unsupported("IDirectInputDevice8A::SetProperty", "property %u on a game controller", prop);
}

static uint32_t pad_get_property(device *d, uint32_t prop, uint32_t hdr)
{
    int err, o = prop_object(d, hdr, &err);
    if (err == 2) return 0x80070002u;
    if (prop == 4 || prop == 5 || prop == 6) {
        if (o < 0 || o >= 5) return DIERR_INVALIDPARAM;      /* ranges and zones are per axis */
        if (prop == 4) { wr32(hdr + 16, (uint32_t)d->lo[o]); wr32(hdr + 20, (uint32_t)d->hi[o]); }
        else wr32(hdr + 16, (prop == 5 ? d->dead : d->sat)[o]);
        return DI_OK;
    }
    hp_unsupported("IDirectInputDevice8A::GetProperty", "property %u on a game controller", prop);
}

uint32_t hpcom_IDirectInputDevice8A_GetCapabilities_c(uint32_t g, uint32_t caps)
{
    device *d = D(g);
    if (!caps || (rd32(caps) != 0x2C && rd32(caps) != 0x18)) return DIERR_INVALIDPARAM;
    if (d->kind != GAMEPAD) hp_unsupported("IDirectInputDevice8A::GetCapabilities", "the system keyboard or mouse");
    uint32_t size = rd32(caps);
    memset(G(caps + 4), 0, size - 4);
    wr32(caps + 4, 0x1u | 0x2u);                             /* DIDC_ATTACHED | DIDC_POLLEDDEVICE */
    wr32(caps + 8, PAD_DEVTYPE);
    wr32(caps + 12, 5); wr32(caps + 16, 10); wr32(caps + 20, 1);   /* axes, buttons, POVs */
    return DI_OK;
}

uint32_t hpcom_IDirectInputDevice8A_GetDeviceInfo_c(uint32_t g, uint32_t inst)
{
    device *d = D(g);
    if (!inst || (rd32(inst) != 580 && rd32(inst) != 0x224)) return DIERR_INVALIDPARAM;
    if (d->kind != GAMEPAD) hp_unsupported("IDirectInputDevice8A::GetDeviceInfo", "the system keyboard or mouse");
    uint32_t tmp = halopad_heap_alloc(580, 1);
    put_instance(tmp, d->pad_id);
    memcpy(G(inst + 4), G(tmp + 4), rd32(inst) - 4);         /* the DirectX 3 size stops before the HID usage */
    halopad_heap_free(tmp);
    return DI_OK;
}

uint32_t hpcom_IDirectInputDevice8A_EnumObjects_c(uint32_t g, uint32_t cb, uint32_t ref, uint32_t flags)
{
    device *d = D(g);
    if (d->kind != GAMEPAD) hp_unsupported("IDirectInputDevice8A::EnumObjects", "the system keyboard or mouse");
    if (!cb) return DIERR_INVALIDPARAM;
    if (flags & ~0x1Fu) hp_unsupported("IDirectInputDevice8A::EnumObjects", "flags 0x%x", flags);
    uint32_t inst = halopad_heap_alloc(0x13C, 1);
    for (int i = 0; i < NOBJ; i++) {
        if (flags && !(objs[i].type & flags)) continue;      /* DIDFT_ALL, or the classes asked for */
        memset(G(inst), 0, 0x13C);
        wr32(inst, 0x13C);                                   /* DIDEVICEOBJECTINSTANCEA */
        put_guid(inst + 4, objs[i].guid_d1, 0xC9F3, 0x11CF, obj_tail);
        wr32(inst + 0x14, d->formatted && d->ofs[i] >= 0 ? (uint32_t)d->ofs[i] : objs[i].native_ofs);
        wr32(inst + 0x18, objs[i].type);
        wr32(inst + 0x1C, (objs[i].type & 2) ? 0x100u : 0);  /* DIDOI_ASPECTPOSITION */
        memcpy(G(inst + 0x20), objs[i].name, strlen(objs[i].name) + 1);
        wr16(inst + 0x130, objs[i].page); wr16(inst + 0x132, objs[i].usage);
        uint32_t a[2] = {inst, ref};
        if (!halopad_call_guest(cb, 2, a)) break;
    }
    halopad_heap_free(inst);
    return DI_OK;
}

/* ---- input from the host (halopad_input_event) ---- */

static void record_source(device *d, uint32_t ofs, uint32_t data, uint32_t flags)
{
    if (!d->bufsize) return;
    if (d->count == d->bufsize) { d->overflow = 1; return; }        /* Windows drops the newest */
    event *e = &d->buf[(d->head + d->count++) % MAXBUF];
    e->ofs = ofs; e->data = data; e->time = GetTickCount_c(); e->seq = ++sequence; e->flags = flags;
}

static void record(device *d, uint32_t ofs, uint32_t data) { record_source(d, ofs, data, 0); }

void halopad_dinput_event(const hp_input *e)
{
    if (e->kind == HPI_CANCEL_TOUCH) reset_touch_move();
    if (e->kind == HPI_ACTIVATE) {
        touch_move_active = !!e->down;
        if (!touch_move_active) reset_touch_move();
    }
    if (e->kind == HPI_TOUCH_MOVE) {
        if (touch_move_enabled && touch_move_active) {
            float x = isfinite(e->move_x) ? fmaxf(-1, fminf(1, e->move_x)) : 0;
            float y = isfinite(e->move_y) ? fmaxf(-1, fminf(1, e->move_y)) : 0;
            float magnitude = hypotf(x, y);
            if (magnitude > 1) { x /= magnitude; y /= magnitude; }
            touch_move.lx = x; touch_move.ly = y;
        }
        return;
    }
    static int trace = -1;
    if (trace < 0) trace = getenv("HALOPAD_TRACE_INPUT") != NULL;
    if (trace && e->kind == HPI_BUTTON) fprintf(stderr, "HALOPAD INPUT: button %d %s\n", e->button, e->down ? "down" : "up");
    for (int i = 0; i < 8; i++) {
        device *d = devices[i];
        if (!d) continue;
        if (d->kind == GAMEPAD && d->pad_id != HP_TOUCH_MOVE_ID) continue;
        if (d->kind == GAMEPAD && e->kind != HPI_ACTIVATE) continue;
        if (e->kind == HPI_CANCEL_TOUCH) {
            cancel_touch_keys(d);
            for (uint32_t b = 0; b < 8; b++)
                if (d->touch_raw[b] && !d->buttons[b]) record(d, 12u + b, 0);
            clear_touch(d);
            continue;
        }
        if (e->kind == HPI_ACTIVATE && !e->down && (d->coop & 4)) {
            if (d->acquired) { set_acquired(d, 0); d->lost = 1; }
            continue;
        }
        if (!d->acquired) { if (trace && e->kind == HPI_BUTTON) fprintf(stderr, "HALOPAD INPUT:   device %d (kind %d) not acquired\n", i, d->kind); continue; }
        if ((d->coop & 4) && GetForegroundWindow_c() != d->hwnd) { if (trace && e->kind == HPI_BUTTON) fprintf(stderr, "HALOPAD INPUT:   device %d (kind %d) not foreground\n", i, d->kind); continue; }
        if (trace && e->kind == HPI_BUTTON && d->kind == MOUSE) fprintf(stderr, "HALOPAD INPUT:   mouse %d: button %d was %02x\n", i, e->button, d->buttons[e->button & 7]);
        if (d->kind == KEYBOARD && e->kind == HPI_KEY) {
            uint32_t dik = (e->scan & 0x7F) | (e->extended ? 0x80u : 0);
            uint8_t v = e->down ? 0x80 : 0;
            if (e->flags & HPI_TOUCH) { d->key_touch[dik] = v; d->key_touch_dirty[dik] = 1; }
            else d->key_physical[dik] = v;
            v = d->key_physical[dik] | d->key_touch[dik];
            if (d->keys[dik] == v) continue;                        /* transitions only: no autorepeat */
            d->keys[dik] = v;
            record_source(d, dik, v, e->flags & HPI_TOUCH);
        } else if (d->kind == MOUSE) {
            if (e->kind == HPI_MOUSEMOVE) {
                if (e->flags & HPI_TOUCH) { d->touch_dx += e->dx; d->touch_dy += e->dy; }
                if (e->dx) { d->dx += e->dx; record_source(d, 0, (uint32_t)e->dx, e->flags & HPI_TOUCH); }
                if (e->dy) { d->dy += e->dy; record_source(d, 4, (uint32_t)e->dy, e->flags & HPI_TOUCH); }
            } else if (e->kind == HPI_WHEEL) {
                d->dz += e->wheel; record(d, 8, (uint32_t)e->wheel);
            } else if (e->kind == HPI_BUTTON && e->button >= 0 && e->button < 8) {
                uint8_t v = e->down ? 0x80 : 0;
                uint32_t b = (uint32_t)e->button;
                uint8_t before = d->buttons[b] | d->touch_raw[b];
                if (e->flags & HPI_TOUCH) {
                    if (d->touch_raw[b] == v) continue;
                    d->touch_raw[b] = v;
                    if (!d->bufsize) {
                        /* Bound backlog; dropping a complete pair preserves parity and release. */
                        if (d->touch_edges[b] >= MAXBUF) d->touch_edges[b] -= 2;
                        d->touch_edges[b]++;
                    }
                } else d->buttons[b] = v;
                uint8_t after = d->buttons[b] | d->touch_raw[b];
                if (before != after) record(d, 12u + b, after);
            }
        }
    }
}
