# G2d — real Halo slices, native ARM64 vs the x86 oracle

Date: 2026-09-26. Result: **PASS for every slice the phase 2 loop lists.** Each slice is original Halo code translated by the whole-image SRW/llasm pipeline, compiled with `clang -O1` for arm64, linked as one program with HaloPad's runtime, and compared with the same function run from the original x86 bytes in the Unicorn oracle.

Reproduce:

```sh
scripts/build-lifter.sh                                   # tool build (SRW + llasm with the recorded patch)
scripts/srw-pipeline.sh generated/tool-builds/<key>        # audit -> hints -> SRW -> llasm (292 MB IR)
# compile: prepend the target triple, then
#   clang -target <triple> -c -O1 -fno-fast-math -ffp-contract=off haloce.target.ll -o haloce.o
.venv/bin/python scripts/run-slices.py
```

Evidence: `docs/artifacts/2026-09-26/G2d/slices-*/result.json` (per-case native output, oracle comparison, identities of the image, object and executable).

| Slice | Original | What it exercises | Result |
|---|---|---|---|
| `crc32` | `0x59f2a2` stdcall | table-driven integer loop | 121/121 (lengths 0–17, 255–257, 4096, random; check value `0xCBF43926`) |
| `memmove` | `0x5c83f0` cdecl (CRT) | `rep movsd`, `std`/`cld`, overlap both directions, alignment paths, nine jump tables (masked, negative-index, downward) | 300/300 |
| `strrchr` | `0x5c88c0` cdecl (CRT) | `repne scasb` forward and backward (direction flag) | 200/200 |
| `vec3_transform_coord` | `0x5834d7` stdcall | x87 multiply–add, single-precision control | 300/300 in Halo's mode (`0x007F`); 84/300 at 53-bit, as expected |
| `vec4_transform` | `0x583b65` stdcall | x87 | 300/300 in Halo's mode; 39/300 at 53-bit |
| `map_header` | `0x4434a0` (eax = name, esi = buffer) | CRT `sprintf` path building, `CreateFileA`/`ReadFile`/`CloseHandle` through HaloPad's runtime, header validation | 6/6: real `bloodgulch.map` accepted; wrong version, zeroed `foot`, oversize length, 32-character name and a missing map rejected |

Compared per case: return value (pointers as offsets into argument buffers), stack effect, and the final contents of every argument buffer. The oracle additionally checks that no other memory is written. Flags are not observable at these functions' returns.

The loop listed "the SSE routine at `0x59f397`" as slice 2. Under the plain-CPU contract (`G2D-SIMD-SCOPE.md`) SIMD routines are never selected and are translated as explicit traps, so that slice is replaced by the generic x87 routines above, which the contract actually executes.

## Defects found and fixed on the way

| Symptom | Cause | Fix |
|---|---|---|
| `memmove` jumped to garbage | MSVC's hand-written copy indexes tables backwards (`jmp [label + ecx*4]` with ecx −4..−1; `neg ecx` before a table jump) and leaves slot 0 of masked tables unused, overlapping the next instruction | Audit: `and`-mask bounds with holes, downward tables after `neg`, negative-index jumps; SRW hints: displaced labels (`displaced_labels.sci`) and index-adjusting replacements |
| Table entries read one byte off | SRW's llasm writer omitted instruction bytes from the `.text` data segment and re-aligned labels, breaking relative offsets | Patch `SR_output.c`: mirror instruction bytes into the code section's data segment, no alignment padding there |
| Jump to address 0 | Data pointers are filled at run time by llasm's `initialize_pointers` | Harness calls `ptr_initialize_pointers(offset)` before running translated code |
| String helpers dereferenced raw guest addresses | SR support compiled without `PTROFS_64BIT` | Build support with `-DPTROFS_64BIT=1 -std=c2x` |
| x87 results differed from Halo | SR models x87 as `double` and ignored precision control; Halo runs single precision (`fldcw 0x7e` after `CreateDevice` at `0x51a9f1`, or Direct3D's default) | `port/llasm-support/llasm_float.c`: round add/sub/mul/div/sqrt results per the control word |

## Known limits carried forward

- The memory model is still SR's pointer offset (code, data and stack in one 4 GiB host window). G2e replaces it.
- x87 64-bit precision control is approximated by 53 bits; values outside `float` range under single precision overflow earlier than on x87. Neither occurs in the slices; the oracle can check any function where it might.
- The runtime implements only read-only `CreateFileA`, `ReadFile` and `CloseHandle`; every other import stops with its name.

