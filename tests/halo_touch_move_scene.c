/* Development only: original controller activation and binding, with no direct
   writes to Halo's input tables or player movement. Use a backed-up profile.
   Production profile ownership/persistence remains a separate acceptance gate. */
#define halopad_app_entry scene_main
#include "halo_app_scene.c"
#undef halopad_app_entry
#include "../port/runtime/halopad_input.h"

#include "../port/runtime/halopad_touch_binding.h"

static void capture_partial_input(void)
{
    const char *path = getenv("HALOPAD_ANALOG_CAPTURE");
    static int partial_frames, captured;
    if (!path || captured) return;
    int slot = halopad_touch_binding_slot();
    if (slot < 0) return;
    int16_t axis; memcpy(&axis, halopad_guest_ptr(0x64d9ba + slot * 0xa0), 2);
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

int halopad_app_touch_move_slot(void) { return halopad_touch_binding_slot(); }

int halopad_app_touch_move_ready(void)
{
    const char *map = halopad_guest_ptr(0x643064);
    static char previous_map[256];
    if (strncmp(map, previous_map, sizeof previous_map - 1)) {
        snprintf(previous_map, sizeof previous_map, "%s", map);
        fprintf(stderr, "HALOPAD TOUCH BINDING: map transition, revoking touch ownership\n");
        halopad_touch_binding_update(0);
        return 0;
    }
    int ready = halopad_touch_binding_update(map[0] && strcmp(map, "ui"));
    if (ready) capture_partial_input();
    return ready;
}
