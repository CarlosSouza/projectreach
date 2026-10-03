"""The fault reporter walks guest and host frame records without faulting."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class XboxFaultFrameTests(unittest.TestCase):
    def test_native_walk_translates_guest_frames_and_never_faults(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'fault-frames-test'
            subprocess.run(['clang', '-x', 'c', '-', '-std=c11', '-Wall', '-Werror',
                            '-fsanitize=address,undefined', '-I', str(ROOT / 'port/xbox'),
                            '-o', str(binary)], input=r'''
#include <assert.h>
#include <string.h>
#include "xg_fault_frames.h"

/* A fake 4 GiB guest reservation: only a small stack window is "mapped". */
#define BASE 0x300000000ull
#define STACK 0x11013800ull
static uint64_t stack[64];
static int fake_reads;
static int fake(uint64_t address, uint64_t record[2]) {
    fake_reads++;
    if (address < BASE + STACK || address + 16 > BASE + STACK + sizeof(stack)) return 0;
    memcpy(record, (char *)stack + (address - BASE - STACK), 16);
    return 1;
}
static void put(uint64_t guest, uint64_t fp, uint64_t lr) {
    stack[(guest - STACK) / 8] = fp;
    stack[(guest - STACK) / 8 + 1] = lr;
}

int main(void) {
    uint64_t returns[8], record[2], host[4][2];
    assert(xg_frame_host_address(0x11013840, BASE) == BASE + 0x11013840);
    assert(xg_frame_host_address(BASE + 0x40, BASE) == BASE + 0x40);

    /* The observed chain: a 32-bit guest fp, whose saved records hold guest
       fps and translated (host) return addresses. */
    put(0x11013840, 0x11013880, 0x10460aeecull);
    put(0x11013880, 0x110138c0, 0x1046087c8ull);
    put(0x110138c0, 0, 0x104500000ull);
    assert(xg_walk_frames(0x11013840, BASE, returns, 8, fake) == 3);
    assert(returns[0] == 0x10460aeecull && returns[1] == 0x1046087c8ull && returns[2] == 0x104500000ull);

    /* A guest frame continuing in a host frame (full address) is followed. */
    put(0x110138c0, BASE + 0x11013900, 0x104500000ull);
    put(0x11013900, 0, 0x1048d0000ull);
    assert(xg_walk_frames(0x11013840, BASE, returns, 8, fake) == 4 && returns[3] == 0x1048d0000ull);

    /* Stops: the bound, a loop, a backwards link, misalignment, unmapped. */
    assert(xg_walk_frames(0x11013840, BASE, returns, 2, fake) == 2);
    put(0x11013880, 0x11013880, 1);
    assert(xg_walk_frames(0x11013840, BASE, returns, 8, fake) == 2);
    put(0x11013880, 0x11013840, 1);
    assert(xg_walk_frames(0x11013840, BASE, returns, 8, fake) == 2);
    assert(xg_walk_frames(0x11013844, BASE, returns, 8, fake) == 0);
    assert(xg_walk_frames(0, BASE, returns, 8, fake) == 0);
    fake_reads = 0;
    assert(xg_walk_frames(0x10, BASE, returns, 8, fake) == 0 && fake_reads == 1);

    /* The real reader: the crashing value was read as a host pointer. In a
       Mac process that address is unmapped; the kernel copy reports failure
       instead of faulting, and with base 0 the walk simply ends. */
    assert(!xg_read_frame_record(0x11013848, record));
    assert(xg_walk_frames(0x11013840, 0, returns, 8, xg_read_frame_record) == 0);

    /* ...while readable host records are followed. */
    host[0][0] = (uint64_t)host[1]; host[0][1] = 0xaa;
    host[1][0] = (uint64_t)host[2]; host[1][1] = 0xbb;
    host[2][0] = 0x11013840;        host[2][1] = 0xcc; /* guest fp, base 0: unmapped */
    assert(xg_walk_frames((uint64_t)host[0], 0, returns, 8, xg_read_frame_record) == 3);
    assert(returns[0] == 0xaa && returns[1] == 0xbb && returns[2] == 0xcc);
    return 0;
}
''', text=True, check=True, capture_output=True)
            subprocess.run([str(binary)], check=True, capture_output=True)

    def test_reporter_uses_the_safe_walk_and_cannot_recurse(self):
        source = (ROOT / 'port/xbox/xg_memory.c').read_text()
        self.assertIn('xg_walk_frames(uc->uc_mcontext->__ss.__fp, xg_base', source)
        self.assertNotIn('(uint64_t *)fp', source)
        handler = source[source.index('static void fault('):source.index('void xg_install_signal_handlers')]
        self.assertLess(handler.index('if (reporting)'), handler.index('report(number'))
        self.assertIn('SA_NODEFER', source)  # recursion is prevented by the guard, not the mask


if __name__ == '__main__':
    unittest.main()
