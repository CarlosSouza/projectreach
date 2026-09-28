/* Development only: original controller activation and binding, with no direct
   writes to Halo's input tables or player movement. Use a backed-up profile.
   Production profile ownership/persistence remains a separate acceptance gate. */
#define halopad_app_entry scene_main
#include "halo_app_scene.c"
#undef halopad_app_entry
#include "../port/runtime/halopad_input.h"

void halopad_heap_free(uint32_t address);
static int ready, attempted;

static void capture_partial_input(void)
{
    const char *path = getenv("HALOPAD_ANALOG_CAPTURE");
    static int partial_frames, captured;
    if (!path || captured) return;
    int16_t axis; memcpy(&axis, halopad_guest_ptr(0x64d9ba + 3 * 0xa0), 2);
    if (axis != -683 || ++partial_frames != 5) return;
    captured = 1;
    FILE *file = fopen(path, "wb");
    if (!file) { fprintf(stderr, "HALOPAD ANALOG: capture open failed\n"); return; }
    const uint32_t ranges[][2] = {{0x612000, 0x1000}, {0x64c000, 0x2000}, {0x6ab000, 0x3000}, {0x68c000, 0x2000}, {0x815000, 0x2000}};
    for (unsigned i = 0; i < sizeof ranges / sizeof ranges[0]; i++) {
        fwrite(ranges[i], sizeof ranges[i], 1, file);
        fwrite(halopad_guest_ptr(ranges[i][0]), ranges[i][1], 1, file);
    }
    fclose(file);
    fprintf(stderr, "HALOPAD ANALOG: captured partial input, control word %04x flags %08x\n", halopad_cpu->_st_cw, halopad_cpu->_eflags);
}

int halopad_app_entry(void)
{
    halopad_touch_move_enable(1);
    return scene_main();
}

static int all_unbound(uint32_t address, unsigned bytes)
{
    const uint16_t *values = halopad_guest_ptr(address);
    for (unsigned i = 0; i < bytes / 2; i++) if (values[i] != 0x7fff) return 0;
    return 1;
}

int halopad_app_touch_move_ready(void)
{
    const char *map = halopad_guest_ptr(0x643064);
    if (attempted) { if (ready) capture_partial_input(); return ready; }
    if (!map[0] || !strcmp(map, "ui")) return ready;
    attempted = 1;
    int enumerated = -1, slot = -1;
    for (unsigned i = 0; i < rd(0x64c774) && i < 8; i++) {
        const uint16_t *name = halopad_guest_ptr(0x64c798 + 0x240 * i);
        int match = 1;
        for (unsigned j = 0; j < sizeof "HaloPad Touch Move"; j++)
            match &= name[j] == (uint8_t)"HaloPad Touch Move"[j];
        if (match && rd(0x64c9c8 + 0x240 * i) == 0xffffffff) enumerated = (int)i;
    }
    for (int i = 3; i >= 0; i--) {
        int unbound = all_unbound(0x6ab426 + i * 64, 64) &&
            all_unbound(0x6ab536 + i * 128, 128) && all_unbound(0x6ab736 + i * 256, 256);
        fprintf(stderr, "HALOPAD ANALOG: slot %d device %d unbound %d\n", i, (int32_t)rd(0x64dc18 + i * 4), unbound);
        if (slot < 0 && unbound && rd(0x64dc18 + i * 4) == 0xffffffff) slot = i;
    }
    if (enumerated < 0 || slot < 0) {
        fprintf(stderr, "HALOPAD ANALOG: unavailable: device %d free slot %d\n", enumerated, slot);
        return 0;
    }
    /* The same original script evaluator called by the console. Preserve the
       interrupted Present's CPU state around these nested original calls. */
    _cpu saved = *halopad_cpu;
    char command[80];
    snprintf(command, sizeof command, "input_activate_joy %d %d", enumerated, slot);
    uint32_t string = str(command);
    uint32_t accepted = halopad_call_guest_ex(0x487030, 1, &string, 0, 0);
    halopad_heap_free(string);
    fprintf(stderr, "HALOPAD ANALOG: original activation returned %u, slot now %d\n", accepted & 0xff, (int32_t)rd(0x64dc18 + slot * 4));
    if (rd(0x64dc18 + slot * 4) == (uint32_t)enumerated) {
        ready = 1;
        uint32_t descriptor = halopad_heap_alloc(12, 0);
        for (int i = 0; i < 4; i++) {
            uint16_t fields[6] = {3, slot, 1, i / 2, i % 2 + 1, 0};
            memcpy(halopad_guest_ptr(descriptor), fields, 12);
            halopad_cpu->_ebx = (uint32_t[]){22, 21, 20, 19}[i];
            ready &= halopad_call_guest_ex(0x48e360, 0, NULL, descriptor, 0) & 0xff;
        }
        halopad_heap_free(descriptor);
    }
    *halopad_cpu = saved;
    float thresholds[2]; memcpy(thresholds, halopad_guest_ptr(0x6abb58), 8);
    fprintf(stderr, "HALOPAD ANALOG: %s, original thresholds %.3f %.3f\n", ready ? "ready" : "failed", thresholds[0], thresholds[1]);
    return ready;
}
