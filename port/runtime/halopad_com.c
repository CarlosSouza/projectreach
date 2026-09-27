/* HaloPad COM objects (G3): vtables and object bookkeeping for interfaces HaloPad
 * implements natively (Direct3D 9, later DirectInput 8 and DirectSound 8).
 *
 * A COM object handed to Halo is guest memory whose first word points to the
 * interface's vtable: an array of guest addresses (scripts/va-model.py) that dispatch to
 * hpcom_<Interface>_<Method>, HaloPad's implementation or a stub naming the method.
 * Vtables are built once and are read-only. Reference counts and native state live in
 * a host-side table keyed by the object's guest address. */
#include "halopad_win32.h"

extern const uint32_t halopad_com_base, halopad_com_interface_count;
extern const uint32_t halopad_com_first[], halopad_com_count[];
extern const char *const halopad_com_interfaces[];

typedef struct { uint32_t guest; int iface; uint32_t refs, binds; void *state; void (*destroy)(void *); } comobj;
static comobj objects[4096];
static uint32_t vtables[64];

static int iface_index(const char *iface)
{
    for (uint32_t i = 0; i < halopad_com_interface_count; i++)
        if (!strcmp(halopad_com_interfaces[i], iface)) return (int)i;
    hp_unsupported("COM", "interface %s missing from config/runtime/com-interfaces.txt", iface);
}

uint32_t halopad_com_vtable(const char *iface)
{
    int i = iface_index(iface);
    if (!vtables[i]) {
        uint32_t n = halopad_com_count[i];
        uint32_t vt = halopad_heap_alloc(4 * n, 0);
        for (uint32_t k = 0; k < n; k++) wr32(vt + 4 * k, halopad_com_base + 16 * (halopad_com_first[i] + k));
        vtables[i] = vt;
    }
    return vtables[i];
}

/* New object: 'size' guest bytes (at least 4, for the vtable pointer), reference count 1. */
uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *))
{
    uint32_t g = halopad_heap_alloc(size < 4 ? 4 : size, 1);
    wr32(g, halopad_com_vtable(iface));
    for (uint32_t i = 0; i < sizeof objects / sizeof objects[0]; i++)
        if (!objects[i].guest) { objects[i] = (comobj){g, iface_index(iface), 1, 0, state, destroy}; return g; }
    hp_unsupported("COM", "more than %zu live objects", sizeof objects / sizeof objects[0]);
}

static comobj *find(const char *what, uint32_t g)
{
    for (uint32_t i = 0; i < sizeof objects / sizeof objects[0]; i++) if (objects[i].guest == g) return &objects[i];
    hp_unsupported(what, "object 0x%08x that HaloPad did not create", g);
}

/* Native state of an object, checking its interface. */
void *halopad_com_state(const char *iface, uint32_t g)
{
    comobj *o = find(iface, g);
    if (strcmp(halopad_com_interfaces[o->iface], iface))
        hp_unsupported(iface, "object 0x%08x is a %s", g, halopad_com_interfaces[o->iface]);
    return o->state;
}

uint32_t halopad_com_addref(uint32_t g) { return ++find("AddRef", g)->refs; }

static void destroy_if_unused(comobj *o)
{
    if (o->refs || o->binds) return;
    if (o->destroy) o->destroy(o->state);
    halopad_heap_free(o->guest);
    *o = (comobj){0};
}

/* Public reference count, as Direct3D reports it. An object the device still has bound
   (a texture, stream, index buffer, declaration or shader) survives its last Release
   until it is unbound, as in Direct3D 9, whose device keeps internal references. */
uint32_t halopad_com_release(uint32_t g)
{
    comobj *o = find("Release", g);
    if (!o->refs) hp_unsupported("Release", "object 0x%08x whose reference count is already 0", g);
    uint32_t n = --o->refs;
    destroy_if_unused(o);
    return n;
}

void halopad_com_bind(uint32_t g) { if (g) find("bind", g)->binds++; }
void halopad_com_unbind(uint32_t g)
{
    if (!g) return;
    comobj *o = find("unbind", g);
    o->binds--;
    destroy_if_unused(o);
}

/* The interface name of a live object (resource type checks). */
const char *halopad_com_interface(uint32_t g) { return halopad_com_interfaces[find("interface", g)->iface]; }
