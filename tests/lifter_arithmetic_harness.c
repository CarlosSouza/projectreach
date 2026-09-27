#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
extern void c_fixture_arithmetic(_cpu *);
static uint32_t stack_words[64];
int main(void) {
    uintptr_t fn = (uintptr_t)&c_fixture_arithmetic;
    uintptr_t stack = (uintptr_t)&stack_words[32];
    uintptr_t offset = fn & ~(uintptr_t)UINT32_MAX;
    if (stack < offset || stack + 128 - offset > UINT32_MAX) {
        fprintf(stderr, "FAIL: upstream offset model cannot represent this code/data layout\n");
        return 2;
    }
    const uint32_t values[] = {0, 1, 0x7fffffff, 0x80000000, 0xffffffff, 0x12345678};
    unsigned count = 0;
    for (unsigned i = 0; i < 6; ++i) for (unsigned j = 0; j < 6; ++j) {
        _cpu state = {0};
        state._esp = (uint32_t)(stack - offset);
        state._pointer_offset = offset;
        stack_words[32] = values[i];
        stack_words[33] = values[j];
        c_fixture_arithmetic(&state);
        uint32_t expected = (values[i] + values[j]) ^ UINT32_C(0x5a5a5a5a);
        if (state._eax != expected || state._esp != (uint32_t)(stack - offset)) {
            fprintf(stderr, "FAIL: case %u,%u result=%08" PRIx32 " expected=%08" PRIx32 "\n", i,j,state._eax,expected);
            return 1;
        }
        count++;
    }
    printf("PASS: %u self-authored PE arithmetic cases through SRW/llasm/ARM64, offset=%" PRIxPTR "\n",count,offset);
    puts("NOT Halo differential evidence; upstream code/data window restriction remains.");
    return 0;
}
