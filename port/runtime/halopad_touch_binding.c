/* Configure the host's distinct MOVE device through original Halo commands.
   Never write guest binding tables, commandeer a physical slot, or clear a
   binding that no longer equals the value this session installed. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "halopad_input.h"
#include "halopad_touch_binding.h"

extern _Thread_local _cpu *halopad_cpu;
void *halopad_guest_ptr(uint32_t);
uint32_t halopad_heap_alloc(uint32_t, int);
void halopad_heap_free(uint32_t);
uint32_t halopad_call_guest_ex(uint32_t, uint32_t, const uint32_t *, uint32_t, int);

static int owned_device = -1, owned_slot = -1;
static int activated_here;
static int tried, cleanup_pending, cleanup_in_game;
static uint64_t cleanup_configuration;
static uint64_t last_configuration;
static const uint16_t actions[4] = {22, 21, 20, 19};
static uint32_t read32(uint32_t p) { uint32_t v; memcpy(&v, halopad_guest_ptr(p), 4); return v; }
static uint16_t read16(uint32_t p) { uint16_t v; memcpy(&v, halopad_guest_ptr(p), 2); return v; }
static uint32_t axis_address(int slot, int axis) { return 0x6ab536 + slot * 128 + axis * 2; }
static int is_touch(int device)
{
    if (device < 0 || device >= 8 || (unsigned)device >= read32(0x64c774) || !read32(0x64c778 + device * 4)) return 0;
    const uint16_t *name = halopad_guest_ptr(0x64c798 + 0x240 * device);
    for (unsigned i = 0; i < sizeof "HaloPad Touch Move"; i++)
        if (name[i] != (uint8_t)"HaloPad Touch Move"[i]) return 0;
    return 1;
}
static int associated(void)
{
    return owned_slot >= 0 && is_touch(owned_device) &&
        read32(0x64dc18 + owned_slot * 4) == (uint32_t)owned_device &&
        read32(0x64c9c8 + owned_device * 0x240) == (uint32_t)owned_slot;
}
static int unbound(uint32_t p, unsigned size)
{
    for (unsigned i = 0; i < size; i += 2) if (read16(p + i) != 0x7fff) return 0;
    return 1;
}
static int slot_empty(int slot, int allow_owned_axes)
{
    return unbound(0x6ab426 + slot * 64, 64) &&
        unbound(axis_address(slot, allow_owned_axes ? 4 : 0), allow_owned_axes ? 120 : 128) &&
        unbound(0x6ab736 + slot * 256, 256) && read32(0x6ab526 + slot * 4) == UINT32_MAX;
}
static int configured(void)
{
    if (!associated() || !slot_empty(owned_slot, 1)) return 0;
    for (int i = 0; i < 4; i++) if (read16(axis_address(owned_slot, i)) != actions[i]) return 0;
    return 1;
}
static void command(const char *name, int device, int slot)
{
    char text[80];
    if (slot < 0) snprintf(text, sizeof text, "%s %d", name, device);
    else snprintf(text, sizeof text, "%s %d %d", name, device, slot);
    uint32_t p = halopad_heap_alloc((uint32_t)strlen(text) + 1, 0);
    memcpy(halopad_guest_ptr(p), text, strlen(text) + 1);
    halopad_call_guest_ex(0x487030, 1, &p, 0, 0);
    halopad_heap_free(p);
}
static int bind_axis(int slot, int axis, uint16_t action)
{
    uint16_t descriptor[6] = {3, slot, 1, axis / 2, axis % 2 + 1, 0};
    uint32_t p = halopad_heap_alloc(sizeof descriptor, 0);
    memcpy(halopad_guest_ptr(p), descriptor, sizeof descriptor);
    halopad_cpu->_ebx = action;
    int ok = halopad_call_guest_ex(0x48e360, 0, NULL, p, 0) & 0xff;
    halopad_heap_free(p);
    return ok && read16(axis_address(slot, axis)) == action;
}
static int release_owned(void)
{
    /* Clear both already-delivered input and anything queued by UIKit before
       dispatching a readiness change. No old hold may follow a new profile. */
    hp_input cancel = {.kind = HPI_CANCEL_TOUCH};
    halopad_host_post_input(&cancel);
    halopad_input_event(&cancel);
    if (associated()) {
        int cleared = 1;
        for (int i = 0; i < 4; i++)
            if (read16(axis_address(owned_slot, i)) == actions[i])
                cleared &= bind_axis(owned_slot, i, 0x7fff);
        /* Keep ownership if rollback fails, so an orphaned mapping cannot become
           a supposedly free slot. Retry only after configuration/phase changes. */
        if (cleared && activated_here) {
            /* A player binding makes this assignment theirs, even if we first
               activated it. Removing the device would disable that binding. */
            if (slot_empty(owned_slot, 0)) command("input_deactivate_joy", owned_device, -1);
            else activated_here = 0;
        }
        cleanup_pending = !cleared || (activated_here && associated());
        fprintf(stderr, "HALOPAD TOUCH BINDING: %s device %d slot %d\n",
                cleanup_pending ? "cleanup pending" : "released", owned_device, owned_slot);
        if (cleanup_pending) return 0;
    }
    cleanup_pending = 0;
    activated_here = 0;
    owned_device = owned_slot = -1;
    return 1;
}
/* Suppress repeated script calls when all slots are occupied or a command fails.
   A changed mapping/enumeration permits a new attempt. No input samples in this hash. */
static uint64_t configuration(void)
{
    uint64_t h = 14695981039346656037ull;
    const uint32_t ranges[][2] = {{0x64c774, 0x24}, {0x64dc18, 16}, {0x6ab426, 0x710}};
    for (unsigned i = 0; i < sizeof ranges / sizeof *ranges; i++) {
        const uint8_t *p = halopad_guest_ptr(ranges[i][0]);
        for (unsigned j = 0; j < ranges[i][1]; j++) h = (h ^ p[j]) * 1099511628211ull;
    }
    for (unsigned i = 0; i < read32(0x64c774) && i < 8; i++) {
        h = (h ^ read32(0x64c9c8 + i * 0x240)) * 1099511628211ull;
        h = (h ^ (uint64_t)is_touch((int)i)) * 1099511628211ull;
    }
    return h;
}
static void trace_configuration(void)
{
    if (!getenv("HALOPAD_TRACE_TOUCH_BINDING")) return;
    unsigned count = read32(0x64c774);
    fprintf(stderr, "HALOPAD TOUCH CONFIG: %u devices\n", count);
    for (unsigned i = 0; i < count && i < 8; i++)
        fprintf(stderr, "HALOPAD TOUCH CONFIG: device %u touch %d present %d slot %d\n",
                i, is_touch((int)i), !!read32(0x64c778 + i * 4), (int)read32(0x64c9c8 + i * 0x240));
    for (int i = 0; i < 4; i++)
        fprintf(stderr, "HALOPAD TOUCH CONFIG: slot %d device %d empty buttons %d axes %d hats %d menu %08x\n",
                i, (int)read32(0x64dc18 + i * 4), unbound(0x6ab426 + i * 64, 64),
                unbound(axis_address(i, 0), 128), unbound(0x6ab736 + i * 256, 256), read32(0x6ab526 + i * 4));
}
int halopad_touch_binding_slot(void) { return owned_slot; }
int halopad_touch_binding_update(int in_game)
{
    _cpu saved = *halopad_cpu;
    int ready = 0;
    if (owned_slot >= 0) {
        if (in_game && !cleanup_pending && configured()) return 1;
        if (cleanup_pending && cleanup_configuration == configuration() && cleanup_in_game == !!in_game) goto done;
        tried = !release_owned();
        cleanup_configuration = configuration();
        cleanup_in_game = !!in_game;
        /* Yield a false frame so the overlay revokes the previous finger before
           any reconfiguration. Its queued main-thread updates preserve order. */
        goto done;
    }
    if (!in_game) { tried = 0; goto done; }
    uint64_t fingerprint = configuration();
    if (tried && fingerprint == last_configuration) goto done;
    tried = 1;
    for (unsigned i = 0; i < read32(0x64c774) && i < 8; i++) {
        if (!is_touch((int)i)) continue;
        uint32_t slot = read32(0x64c9c8 + i * 0x240);
        if (slot == UINT32_MAX) { owned_device = (int)i; break; }
        /* Fresh profiles can auto-assign our distinct device without bindings.
           Borrow only that reciprocal, empty assignment; cleanup must keep it. */
        if (slot < 4 && read32(0x64dc18 + slot * 4) == i && slot_empty((int)slot, 0)) {
            owned_device = (int)i;
            owned_slot = (int)slot;
            break;
        }
    }
    if (owned_device >= 0 && owned_slot < 0) {
        for (int slot = 3; slot >= 0; slot--)
            if (read32(0x64dc18 + slot * 4) == UINT32_MAX && slot_empty(slot, 0)) { owned_slot = slot; break; }
    }
    if (owned_slot >= 0) {
        if (!associated()) {
            command("input_activate_joy", owned_device, owned_slot);
            activated_here = associated();
        }
        if (associated()) {
            ready = 1;
            for (int i = 0; i < 4 && ready; i++) ready = bind_axis(owned_slot, i, actions[i]);
            ready = ready && configured();
        }
        if (!ready) {
            release_owned();
            cleanup_configuration = configuration();
            cleanup_in_game = !!in_game;
        }
    }
    if (!ready && !cleanup_pending) owned_device = owned_slot = -1;
    last_configuration = configuration();
    fprintf(stderr, "HALOPAD TOUCH BINDING: %s device %d slot %d\n", ready ? "ready" : "unavailable", owned_device, owned_slot);
    if (!ready) trace_configuration();
done:
    *halopad_cpu = saved;
    return ready;
}
