/* HaloPad DirectInput 8 (G9): the system keyboard and mouse on HaloPad's host input.
 *
 * How Halo uses it (haloce.exe): DirectInput8Create, then
 *  - keyboard (0x4946b7): GUID_SysKeyboard, NONEXCLUSIVE|FOREGROUND|NOWINKEY,
 *    c_dfDIKeyboard, DIPROP_BUFFERSIZE 32; read one DIDEVICEOBJECTDATA at a time with
 *    GetDeviceData, re-Acquire on DIERR_INPUTLOST/NOTACQUIRED, flush on overflow (0x4935b0);
 *  - mouse (0x4947b2): GUID_SysMouse, EXCLUSIVE|FOREGROUND, c_dfDIMouse2; GetDeviceState of
 *    DIMOUSESTATE2 each frame; DIPROP_GRANULARITY of the wheel (0x493450);
 *  - game controllers: EnumDevices(DI8DEVCLASS_GAMECTRL, ATTACHEDONLY) (0x49492f).
 * Keys arrive as set-1 scan codes (DIK = scan | 0x80 when extended), transitions only;
 * the mouse gives relative counts from the host's deltas, the wheel in WHEEL_DELTA units.
 * Foreground devices lose acquisition when their window stops being the foreground one
 * (DIERR_INPUTLOST once, then DIERR_NOTACQUIRED). An exclusive mouse captures the host
 * pointer (hidden, detached from the cursor) while acquired. Game controllers are not
 * offered yet: enumeration finds none and says so once. */
#include "halopad_win32.h"
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

enum { KEYBOARD = 1, MOUSE = 2 };
typedef struct { uint32_t ofs, data, time, seq; } event;
#define MAXBUF 1024
typedef struct {
    uint32_t guest, kind, hwnd, coop, data_size;
    int formatted, acquired, lost;
    uint32_t bufsize, head, count;
    int overflow;
    event buf[MAXBUF];
    uint8_t keys[256];
    int32_t dx, dy, dz;
    uint8_t buttons[8];
} device;

static device *devices[8];
static uint32_t sequence;

static int guid_is(uint32_t g, uint32_t d1, uint16_t d2, uint16_t d3, const uint8_t d4[8])
{
    return g && rd32(g) == d1 && (rd32(g + 4) & 0xFFFF) == d2 && rd32(g + 4) >> 16 == d3 && !memcmp(G(g + 8), d4, 8);
}
static const uint8_t sys_tail[8] = {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

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
    uint32_t kind = guid_is(guid, 0x6F1D2B61, 0xD5A0, 0x11CF, sys_tail) ? KEYBOARD
                  : guid_is(guid, 0x6F1D2B60, 0xD5A0, 0x11CF, sys_tail) ? MOUSE : 0;
    if (!kind) return DIERR_DEVICENOTREG;
    int slot = -1;
    for (int i = 0; i < 8; i++) if (!devices[i]) { slot = i; break; }
    if (slot < 0) hp_unsupported("IDirectInput8A::CreateDevice", "more than 8 devices");
    device *d = calloc(1, sizeof *d);
    d->kind = kind;
    d->guest = halopad_com_new("IDirectInputDevice8A", 4, d, dev_destroy);
    devices[slot] = d;
    wr32(out, d->guest);
    return DI_OK;
}

uint32_t hpcom_IDirectInput8A_EnumDevices_c(uint32_t g, uint32_t type, uint32_t cb, uint32_t ref, uint32_t flags)
{
    (void)cb; (void)ref;
    halopad_com_state("IDirectInput8A", g);
    if (type == 4 /* DI8DEVCLASS_GAMECTRL */) {
        static int said;
        if (!said) { fprintf(stderr, "HALOPAD: game controllers are not offered to DirectInput yet; enumerating none\n"); said = 1; }
        return DI_OK;
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

static void set_acquired(device *d, int on)
{
    if (d->acquired == on) return;
    d->acquired = on;
    if (d->kind == MOUSE && (d->coop & 1)) halopad_host_mouse_capture(on);
    if (on) { memset(d->keys, 0, sizeof d->keys); memset(d->buttons, 0, sizeof d->buttons); d->dx = d->dy = d->dz = 0; }
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
    if (d->kind == KEYBOARD) {
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
    return r ? r : DI_NOEFFECT;                                     /* keyboard and mouse are not polled devices */
}

uint32_t hpcom_IDirectInputDevice8A_GetDeviceState_c(uint32_t g, uint32_t size, uint32_t data)
{
    device *d = D(g);
    if (!data || !d->formatted || size != d->data_size) return DIERR_INVALIDPARAM;
    uint32_t r = check_acquired(d);
    if (r) return r;
    if (d->kind == KEYBOARD) { memcpy(G(data), d->keys, 256); return DI_OK; }
    wr32(data, (uint32_t)d->dx); wr32(data + 4, (uint32_t)d->dy); wr32(data + 8, (uint32_t)d->dz);
    memcpy((uint8_t *)G(data) + 12, d->buttons, size - 12);
    d->dx = d->dy = d->dz = 0;                                      /* relative: counts since the last read */
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

/* ---- input from the host (halopad_input_event) ---- */

static void record(device *d, uint32_t ofs, uint32_t data)
{
    if (!d->bufsize) return;
    if (d->count == d->bufsize) { d->overflow = 1; return; }        /* Windows drops the newest */
    event *e = &d->buf[(d->head + d->count++) % MAXBUF];
    e->ofs = ofs; e->data = data; e->time = GetTickCount_c(); e->seq = ++sequence;
}

void halopad_dinput_event(const hp_input *e)
{
    for (int i = 0; i < 8; i++) {
        device *d = devices[i];
        if (!d || !d->acquired) continue;
        if ((d->coop & 4) && GetForegroundWindow_c() != d->hwnd) continue;
        if (d->kind == KEYBOARD && e->kind == HPI_KEY) {
            uint32_t dik = (e->scan & 0x7F) | (e->extended ? 0x80u : 0);
            uint8_t v = e->down ? 0x80 : 0;
            if (d->keys[dik] == v) continue;                        /* transitions only: no autorepeat */
            d->keys[dik] = v;
            record(d, dik, v);
        } else if (d->kind == MOUSE) {
            if (e->kind == HPI_MOUSEMOVE) {
                if (e->dx) { d->dx += e->dx; record(d, 0, (uint32_t)e->dx); }
                if (e->dy) { d->dy += e->dy; record(d, 4, (uint32_t)e->dy); }
            } else if (e->kind == HPI_WHEEL) {
                d->dz += e->wheel; record(d, 8, (uint32_t)e->wheel);
            } else if (e->kind == HPI_BUTTON && e->button >= 0 && e->button < 8) {
                uint8_t v = e->down ? 0x80 : 0;
                if (d->buttons[e->button] != v) { d->buttons[e->button] = v; record(d, 12u + (uint32_t)e->button, v); }
            }
        }
    }
}
