# HaloPad copy of SR llasm-support

Copied from M-HT/SR revision ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3, SR/llasm-support (MIT; notices retained in each file).

Changes:

- `llasm_float.c`: added `x87_pc_round` and applied it to every result of the operations the x87 precision-control field governs (add, subtract, multiply, divide in all forms; square root). Halo runs with single-precision control; SR's upstream model ignored the field and computed everything in double. See docs/G2C-SRW-CAPABILITY.md.
