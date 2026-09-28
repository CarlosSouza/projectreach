/* Transaction/ownership policy, using a simulated guest command boundary.
   Actual CE activation/unbinding is separately exercised by the iPad scene. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "../port/llasm-support/llasm_cpu.h"
#include "../port/runtime/halopad_input.h"
#include "../port/runtime/halopad_touch_binding.h"
static uint8_t memory[0x400000];
static _cpu cpu;
_Thread_local _cpu *halopad_cpu = &cpu;
static int failures, assertions, commands, bindings, cancels, fail_binding;
static uint32_t allocation;
void *halopad_guest_ptr(uint32_t p) { if (p < 0x400000 || p >= 0x800000) abort(); return memory + p - 0x400000; }
static uint32_t rd(uint32_t p) { uint32_t v; memcpy(&v, halopad_guest_ptr(p), 4); return v; }
static void wr(uint32_t p, uint32_t v) { memcpy(halopad_guest_ptr(p), &v, 4); }
static uint16_t r16(uint32_t p) { uint16_t v; memcpy(&v, halopad_guest_ptr(p), 2); return v; }
static void w16(uint32_t p, uint16_t v) { memcpy(halopad_guest_ptr(p), &v, 2); }
uint32_t halopad_heap_alloc(uint32_t size, int zero) { uint32_t p = allocation; allocation += (size + 15) & ~15u; if (zero) memset(halopad_guest_ptr(p), 0, size); return p; }
void halopad_heap_free(uint32_t p) { (void)p; }
void halopad_host_post_input(const hp_input *e) { if (e->kind != HPI_CANCEL_TOUCH) abort(); cancels++; }
void halopad_input_event(const hp_input *e) { if (e->kind != HPI_CANCEL_TOUCH) abort(); cancels++; }
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t n, const uint32_t *args, uint32_t ecx_value, int pops)
{
    (void)n; (void)pops;
    if (va == 0x487030) {
        int d, slot; const char *text = halopad_guest_ptr(args[0]); commands++;
        if (sscanf(text, "input_activate_joy %d %d", &d, &slot) == 2) {
            if (d < 0 || d >= 8 || slot < 0 || slot >= 4) abort();
            if (rd(0x64dc18 + slot * 4) != UINT32_MAX || rd(0x64c9c8 + d * 0x240) != UINT32_MAX) return 0;
            wr(0x64dc18 + slot * 4, d); wr(0x64c9c8 + d * 0x240, slot); return 1;
        }
        if (sscanf(text, "input_deactivate_joy %d", &d) != 1) abort();
        slot = (int32_t)rd(0x64c9c8 + d * 0x240);
        if (slot >= 0) wr(0x64dc18 + slot * 4, UINT32_MAX);
        wr(0x64c9c8 + d * 0x240, UINT32_MAX); return 1;
    }
    if (va != 0x48e360) abort();
    bindings++;
    if (bindings == fail_binding) return 0;
    const uint16_t *desc = halopad_guest_ptr(ecx_value);
    if (desc[0] != 3 || desc[1] >= 4 || desc[2] != 1 || desc[3] >= 2 || desc[4] < 1 || desc[4] > 2) abort();
    w16(0x6ab536 + desc[1] * 128 + desc[3] * 4 + (desc[4] - 1) * 2, (uint16_t)cpu._ebx);
    return 1;
}
static void check(const char *what, int ok) { assertions++; failures += !ok; printf("%s %s\n", ok ? "PASS" : "FAIL", what); }
static void reset(void)
{
    halopad_touch_binding_update(0); halopad_touch_binding_update(0);
    memset(memory, 0, sizeof memory); memset(&cpu, 0x35, sizeof cpu);
    allocation = 0x700000; commands = bindings = cancels = fail_binding = 0;
    for (uint32_t p = 0x6ab330; p < 0x6abb36; p += 2) w16(p, 0x7fff);
    for (int i = 0; i < 4; i++) { wr(0x64dc18 + i * 4, UINT32_MAX); wr(0x6ab526 + i * 4, UINT32_MAX); }
    for (int i = 0; i < 8; i++) wr(0x64c9c8 + i * 0x240, UINT32_MAX);
    wr(0x64c774, 2); wr(0x64c778, 1); wr(0x64c77c, 2);
    const char *name = "HaloPad Touch Move";
    for (unsigned i = 0; i <= strlen(name); i++) w16(0x64c798 + 0x240 + i * 2, name[i]);
    wr(0x64dc18, 0); wr(0x64c9c8, 0); /* Physical controller already owns slot zero. */
    w16(0x6ab536, 31); w16(0x6ab330, 19);
}
int main(void)
{
    reset(); uint8_t before[0x806]; memcpy(before, halopad_guest_ptr(0x6ab330), sizeof before);
    _cpu saved = cpu;
    check("original physical slot is preserved; touch chooses spare slot three", halopad_touch_binding_update(1) && halopad_touch_binding_slot() == 3 && rd(0x64dc18) == 0 && r16(0x6ab536) == 31);
    check("nested original calls preserve interrupted CPU", !memcmp(&saved, &cpu, sizeof cpu));
    int c = commands, b = bindings;
    int stayed_ready = 1;
    for (int i = 0; i < 30; i++) stayed_ready &= halopad_touch_binding_update(1);
    check("unchanged frames remain ready", stayed_ready);
    check("unchanged frames do not reactivate or rebind", c == commands && b == bindings);
    check("menu transition revokes readiness", !halopad_touch_binding_update(0));
    check("owned bindings and association restored on menu return", !memcmp(before, halopad_guest_ptr(0x6ab330), sizeof before) && rd(0x64dc24) == UINT32_MAX && rd(0x64cc08) == UINT32_MAX);
    check("revocation clears delivered and queued touch input", cancels == 2);
    check("next map configures again", halopad_touch_binding_update(1));
    w16(0x6ab536 + 3 * 128, 31); /* Profile replaced one owned axis. */
    check("changed profile revokes the old finger before reconfiguration", !halopad_touch_binding_update(1));
    check("new profile binding is retained", r16(0x6ab536 + 3 * 128) == 31);
    check("reserved old slot is avoided when rebuilding", halopad_touch_binding_update(1) && halopad_touch_binding_slot() == 2);
    reset(); wr(0x6ab526 + 3 * 4, 5);
    check("menu/button binding reserves an otherwise empty slot", halopad_touch_binding_update(1) && halopad_touch_binding_slot() == 2);
    reset(); fail_binding = 3;
    memcpy(before, halopad_guest_ptr(0x6ab330), sizeof before);
    check("partial bind failure leaves the source unready", !halopad_touch_binding_update(1));
    check("partial transaction is rolled back", !memcmp(before, halopad_guest_ptr(0x6ab330), sizeof before) && rd(0x64dc24) == UINT32_MAX);
    c = commands; b = bindings;
    halopad_touch_binding_update(1);
    check("unchanged failed configuration is not retried every frame", commands == c && bindings == b);
    halopad_touch_binding_update(0); fail_binding = 0;
    check("later map retries a transient failure", halopad_touch_binding_update(1));
    reset(); wr(0x64c774, 5);
    for (int i = 1; i < 4; i++) { wr(0x64dc18 + i * 4, i + 1); wr(0x64c9c8 + (i + 1) * 0x240, i); }
    check("full physical slots fall back without changing mappings", !halopad_touch_binding_update(1) && commands == 0 && bindings == 0);
    wr(0x64dc20, UINT32_MAX); wr(0x64c9c8 + 3 * 0x240, UINT32_MAX);
    check("a newly freed slot triggers recovery", halopad_touch_binding_update(1) && halopad_touch_binding_slot() == 2);
    reset(); halopad_touch_binding_update(1);
    wr(0x64dc18, UINT32_MAX); wr(0x64dc24, 0); wr(0x64c9c8, 3); wr(0x64cc08, UINT32_MAX);
    c = commands; b = bindings;
    check("reassigned physical slot revokes touch without editing it", !halopad_touch_binding_update(1) && commands == c && bindings == b && r16(0x6ab536 + 3 * 128) == 22);
    check("touch recovers elsewhere and leaves reassigned physical slot intact", halopad_touch_binding_update(1) && halopad_touch_binding_slot() == 2 && rd(0x64dc24) == 0);
    reset(); halopad_touch_binding_update(1); fail_binding = bindings + 1;
    check("failed cleanup stays unready with ownership retained", !halopad_touch_binding_update(0) && halopad_touch_binding_slot() == 3 && rd(0x64dc24) == 1);
    c = commands; b = bindings;
    halopad_touch_binding_update(0);
    check("failed cleanup is not retried on unchanged frames", commands == c && bindings == b);
    fail_binding = 0;
    check("next phase completes pending cleanup before reacquiring", !halopad_touch_binding_update(1) && halopad_touch_binding_slot() == -1 && rd(0x64dc24) == UINT32_MAX);
    check("cleaned transaction can configure the next frame", halopad_touch_binding_update(1));
    reset(); wr(0x64dc1c, 1); wr(0x64cc08, 1);
    memcpy(before, halopad_guest_ptr(0x6ab330), sizeof before);
    check("fresh profile's empty touch slot is reused without activation", halopad_touch_binding_update(1) && halopad_touch_binding_slot() == 1 && commands == 0 && rd(0x64dc18) == 0);
    check("menu cleanup preserves the profile's original touch assignment", !halopad_touch_binding_update(0) && commands == 0 && rd(0x64dc1c) == 1 && rd(0x64cc08) == 1 && !memcmp(before, halopad_guest_ptr(0x6ab330), sizeof before));
    reset(); wr(0x64dc1c, 1); wr(0x64cc08, 1); fail_binding = 3;
    memcpy(before, halopad_guest_ptr(0x6ab330), sizeof before);
    check("partial failure rolls back axes but preserves borrowed assignment", !halopad_touch_binding_update(1) && bindings >= 3 && commands == 0 && rd(0x64dc1c) == 1 && !memcmp(before, halopad_guest_ptr(0x6ab330), sizeof before));
    reset(); wr(0x64dc1c, 1); wr(0x64cc08, 1); w16(0x6ab536 + 128, 31);
    check("pre-existing bound touch assignment is not commandeered", !halopad_touch_binding_update(1) && commands == 0 && bindings == 0);
    reset(); wr(0x64dc1c, 0); wr(0x64cc08, 1);
    check("inconsistent assignment cannot borrow a physical slot", !halopad_touch_binding_update(1) && commands == 0 && bindings == 0);
    reset(); wr(0x64dc1c, 1); wr(0x64cc08, 1);
    halopad_touch_binding_update(1); w16(0x6ab536 + 128, 31);
    check("changed player binding survives cleanup of borrowed slot", !halopad_touch_binding_update(1) && commands == 0 && rd(0x64dc1c) == 1 && r16(0x6ab536 + 128) == 31 && r16(0x6ab536 + 130) == 0x7fff);
    check("borrowed slot with player binding stays unavailable", !halopad_touch_binding_update(1) && commands == 0 && rd(0x64dc1c) == 1);
    printf("TOUCH BINDING POLICY: %d assertions, %d failures\n", assertions, failures);
    return failures != 0;
}
