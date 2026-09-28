/* Development-only, one-shot original-call binding ownership test. Run against
   a private server with a backed-up profile; never combine with UIKit selftests.
   No guest input tables or player state are written by this experiment. */
#define halopad_app_touch_move_ready normal_touch_ready
#include "halo_touch_move_scene.c"
#undef halopad_app_touch_move_ready
void halopad_heap_free(uint32_t);

static int checks;
static void require(const char *what, int ok)
{
    fprintf(stderr, "HALOPAD BINDING EDIT: %s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) abort();
    checks++;
}
static uint16_t edit_read16(uint32_t p)
{
    uint16_t v; memcpy(&v, halopad_guest_ptr(p), 2); return v;
}
static void edit_command(const char *name, int device, int slot)
{
    char text[80];
    if (slot < 0) snprintf(text, sizeof text, "%s %d", name, device);
    else snprintf(text, sizeof text, "%s %d %d", name, device, slot);
    uint32_t p = str(text);
    halopad_call_guest_ex(0x487030, 1, &p, 0, 0);
    halopad_heap_free(p);
}
static int edit_axis(int slot, int action)
{
    uint16_t descriptor[6] = {3, slot, 1, 0, 1, 0};
    uint32_t p = halopad_heap_alloc(sizeof descriptor, 0);
    memcpy(halopad_guest_ptr(p), descriptor, sizeof descriptor);
    halopad_cpu->_ebx = action;
    int ok = halopad_call_guest_ex(0x48e360, 0, NULL, p, 0) & 0xff;
    halopad_heap_free(p);
    return ok;
}

int halopad_app_touch_move_ready(void)
{
    static int phase, frames, original_slot, device, slot;
    int ready = normal_touch_ready();
    if (phase == 6) return ready;
    if (!phase && !ready) return ready;
    _cpu saved = *halopad_cpu;
    if (++frames >= 600) require("bounded original-call experiment", 0);
    switch (phase) {
    case 0:
        original_slot = halopad_touch_binding_slot();
        device = rd(0x64dc18 + original_slot * 4);
        halopad_touch_binding_update(0);
        /* Release the empty borrowed assignment to exercise activated-here
           ownership too. Restore this exact association at the end. */
        edit_command("input_deactivate_joy", device, -1);
        require("original deactivation leaves device unassigned", rd(0x64c9c8 + device * 0x240) == UINT32_MAX);
        phase = 1; ready = 0; frames = 0;
        break;
    case 1:
        if (!ready) break;
        /* UIKit clears touch on the readiness transition. Let that asynchronous
           cancellation settle before beginning a new test hold. */
        if (frames < 10) break;
        slot = halopad_touch_binding_slot();
        require("manager activates its temporary assignment", rd(0x64dc18 + slot * 4) == (uint32_t)device);
        halopad_host_post_input(&(hp_input){.kind = HPI_TOUCH_MOVE, .move_y = .5f});
        phase = 2; frames = 0;
        break;
    case 2:
        if (frames < 10) break;
        fprintf(stderr, "HALOPAD BINDING EDIT: observed slot %d Y %d\n", slot, (int16_t)edit_read16(0x64d9ba + slot * 0xa0));
        require("held MOVE reaches original polled axis", (int16_t)edit_read16(0x64d9ba + slot * 0xa0) == -1820);
        require("original setter accepts player forward binding on X+", edit_axis(slot, 19));
        require("original setter installed player binding", edit_read16(0x6ab536 + slot * 128) == 19);
        phase = 3; frames = 0;
        break;
    case 3:
        require("changed binding revokes touch readiness", !ready && halopad_touch_binding_slot() == -1);
        require("player binding keeps reciprocal device assignment", rd(0x64dc18 + slot * 4) == (uint32_t)device && rd(0x64c9c8 + device * 0x240) == (uint32_t)slot);
        require("player binding survives temporary-axis cleanup", edit_read16(0x6ab536 + slot * 128) == 19);
        for (int i = 1; i < 4; i++) require("temporary axis removed", edit_read16(0x6ab536 + slot * 128 + 2 * i) == 0x7fff);
        phase = 4; frames = 0;
        break;
    case 4:
        if (frames < 10) break;
        require("player-owned assignment is not commandeered", !ready && halopad_touch_binding_slot() == -1 && rd(0x64dc18 + slot * 4) == (uint32_t)device);
        require("old MOVE hold cleared by original polling", edit_read16(0x64d9ba + slot * 0xa0) == 0);
        require("original setter clears the test player binding", edit_axis(slot, 0x7fff));
        phase = 5; frames = 0;
        break;
    case 5:
        if (!ready) break;
        require("empty assignment recovers neutrally", halopad_touch_binding_slot() == slot && edit_read16(0x64d9ba + slot * 0xa0) == 0);
        halopad_touch_binding_update(0);
        edit_command("input_deactivate_joy", device, -1);
        edit_command("input_activate_joy", device, original_slot);
        require("original assignment restored", rd(0x64dc18 + original_slot * 4) == (uint32_t)device && rd(0x64c9c8 + device * 0x240) == (uint32_t)original_slot);
        phase = 6; ready = 0;
        fprintf(stderr, "HALOPAD BINDING EDIT: COMPLETE %d checks\n", checks);
        break;
    }
    *halopad_cpu = saved;
    return ready;
}
