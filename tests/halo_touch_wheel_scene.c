/* Development-only wheel-binding gameplay fixture. Uses original setters to
   move JUMP from Space to wheel Z+ in memory; never saves a profile. Run with
   backed-up saves and HALOPAD_TOUCH_SELFTEST, then discard the process and
   reinstall the regular scene. This does not prove original-menu capture. */
#define halopad_app_touch_move_ready normal_touch_ready
#include "halo_touch_move_scene.c"
#undef halopad_app_touch_move_ready
void halopad_heap_free(uint32_t);

static uint16_t wheel_read16(uint32_t p)
{
    uint16_t v; memcpy(&v, halopad_guest_ptr(p), 2); return v;
}
static void wheel_require(const char *what, int ok)
{
    fprintf(stderr, "HALOPAD WHEEL FIXTURE: %s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) abort();
}
static int wheel_bind(const uint16_t descriptor[6], uint16_t action)
{
    uint32_t p = halopad_heap_alloc(12, 0);
    memcpy(halopad_guest_ptr(p), descriptor, 12);
    halopad_cpu->_ebx = action;
    int ok = halopad_call_guest_ex(0x48e360, 0, NULL, p, 0) & 255;
    halopad_heap_free(p);
    return ok;
}
int halopad_app_touch_move_ready(void)
{
    int ready = normal_touch_ready();
    static int configured;
    if (!ready || configured) return ready;
    wheel_require("explicit gameplay driver enabled", getenv("HALOPAD_TOUCH_SELFTEST") != NULL);
    wheel_require("original wheel granularity is one notch", rd(0x64c738) == 120);
    uint16_t space = wheel_read16(0x5fa358 + 0x39 * 2);
    wheel_require("fixture has default Space JUMP", space < 109 && wheel_read16(0x6ab330 + space * 2) == 0);
    int no_alternate = 1;
    for (unsigned i = 0; i < 109; i++)
        if (i != space) no_alternate &= wheel_read16(0x6ab330 + i * 2) != 0;
    for (unsigned i = 0; i < 8; i++) no_alternate &= wheel_read16(0x6ab40a + i * 2) != 0;
    no_alternate &= wheel_read16(0x6ab424) != 0;
    wheel_require("no alternate keyboard/mouse JUMP", no_alternate);
    wheel_require("wheel Z+ is unassigned", wheel_read16(0x6ab422) == 0x7fff);
    _cpu saved = *halopad_cpu;
    wheel_require("original setter unbinds Space", wheel_bind((uint16_t[]){1, 0, 0, space, 0, 0}, 0x7fff));
    wheel_require("original setter binds wheel Z+ to JUMP", wheel_bind((uint16_t[]){2, 0, 1, 2, 1, 0}, 0));
    *halopad_cpu = saved;
    wheel_require("wheel-only JUMP mapping is active", wheel_read16(0x6ab330 + space * 2) == 0x7fff && wheel_read16(0x6ab422) == 0);
    configured = 1;
    return ready;
}
