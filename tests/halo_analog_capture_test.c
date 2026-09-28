/* Replay a private input-only development capture through translated 0x48f850.
   Compare printed bits with scripts/replay-analog-capture.py. No game loop. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
extern uint64_t halopad_guest_base;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *, uint32_t);
void halopad_thread_init(uint32_t, uint32_t, uint32_t);
void halopad_vm_mark(uint32_t, uint32_t);
void *halopad_guest_ptr(uint32_t);
uint32_t halopad_call_guest_ex(uint32_t, uint32_t, const uint32_t *, uint32_t, int);

int main(void)
{
    const char *path = getenv("HALOPAD_ANALOG_CAPTURE");
    if (!path || !getenv("HALOPAD_IMAGE")) return 2;
    uint32_t top = halopad_guest_init(getenv("HALOPAD_IMAGE"), 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42c000);
    _cpu cpu = {0};
    cpu._esp = top; cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x023f; halopad_cpu = &cpu;
    FILE *file = fopen(path, "rb");
    if (!file) return 2;
    const uint32_t ranges[][2] = {{0x612000, 0x1000}, {0x64c000, 0x2000}, {0x6ab000, 0x3000}, {0x68c000, 0x2000}, {0x815000, 0x2000}};
    for (unsigned i = 0; i < sizeof ranges / sizeof ranges[0]; i++) {
        uint32_t header[2];
        if (fread(header, sizeof header, 1, file) != 1 || memcmp(header, ranges[i], sizeof header) ||
            fread(halopad_guest_ptr(header[0]), header[1], 1, file) != 1) return 2;
    }
    if (fgetc(file) != EOF) return 2;
    fclose(file);
    halopad_call_guest_ex(0x48f850, 0, NULL, 0, 0);
    float forward, strafe;
    uint32_t bits[2];
    memcpy(&forward, halopad_guest_ptr(0x6ad4b8), 4);
    memcpy(&strafe, halopad_guest_ptr(0x6ad4bc), 4);
    memcpy(bits, halopad_guest_ptr(0x6ad4b8), 8);
    printf("CAPTURE REPLAY: %.9f %.9f bits %08x %08x\n", forward, strafe, bits[0], bits[1]);
    return 0; /* comparison belongs to the original-x86 report, not a guessed constant */
}
