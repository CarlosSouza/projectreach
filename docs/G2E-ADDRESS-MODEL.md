# G2e — original-address memory, dispatch and callback contract

Date: 2026-09-26. Result: **PASS on macOS and the iPad Simulator. The physical-device row is parked** (needs Chris's iPhone or iPad and signing).

Translated Halo code now runs in Halo's own 32-bit address space. Every address the game can see is the address it would have on Windows: data, stack, return addresses, function pointers and imports. Every indirect jump, call and return goes through one finite table built at compile time, and anything outside that table stops with the guest address.

Reproduce (after the G2d pipeline and compile):

```sh
.venv/bin/python scripts/va-model.py --work generated/srw/custom-en-1.0.10.0621/run-<id> \
    --llasm generated/tool-builds/<key>/llasm/llasm
.venv/bin/python scripts/run-slices.py                                  # macOS
SDKROOT=$(xcrun --sdk iphonesimulator --show-sdk-path) .venv/bin/python scripts/run-slices.py \
    --target arm64-apple-ios17.0-simulator --run-prefix xcrun simctl spawn <HaloPad simulator UDID>
```

Evidence: `docs/artifacts/2026-09-26/G2e/slices-va-arm64-apple-macosx14.0.0-20260927T034247Z/` and `.../slices-va-arm64-apple-ios17.0-simulator-20260927T034413Z/`. Each `result.json` records per-case output, the contract runs, the binary's platform (`IOSSIMULATOR`, minos 17.0) and the image, object and executable hashes. The Simulator is a project-owned device, "HaloPad iPad Pro 13" (iOS 26.5).

## The model

| Piece | Where | What it does |
|---|---|---|
| Guest memory | `port/runtime/halopad_guest.c` | Reserves 4 GiB with no access, so every 32-bit guest address has a home and unmapped addresses fault. Loads the prepared image at `0x400000` and maps a stack (`0x100000–0x300000`) and a harness heap (`0x10000000–0x20000000`) read/write. Nothing is mapped executable. A SIGSEGV/SIGBUS handler reports faults as guest addresses. |
| Code values | `scripts/va-model.py` | Rewrites SRW's llasm so that label values are original addresses (64,857 lines). Loads and stores through a label go through the guest base. Data segments are dropped because the image supplies the bytes. |
| Dispatch | generated `va/dispatch.ll` | A sorted table of 119,706 entries: 119,439 translated procedures plus 267 imports. Each of the 15,550 register transfers (`call eax`, `jmp [table]`, `ret`) becomes `push target; tcall halopad_dispatch`, followed by a binary search and a tail call. An unknown address stops the program. |
| Entry and exit | `halopad_enter` | The host pushes the sentinel return address `0xFFFFF000` and enters an original address. A guest `ret` to the sentinel returns to the host. |
| Imports | `va-model.py` + `halopad_guest.c` | Each of Halo's 267 static imports gets an address in a page that is never mapped (`0xFFFE0000 + 16·i`). At load time these addresses are written into Halo's import address table (`0x5df000–0x5df43c`), and the 174 sites where code loads an import as a value (for example `mov esi, [__imp_CloseHandle]`) get the same address. `call esi` then reaches the import through dispatch. Direct import calls (1,031) remain direct. |
| No host addresses leak | `va-model.py` | llasm's C entry wrappers (`c_<alias>`) are SR's pointer-offset entry path, which stores a host function address minus the offset as a guest return address. The VA model removes them (7). The build now fails if any `ptrtoint` of translated code remains; all linked modules have zero. |

The remaining use of llasm's `_pointer_offset` field is the guest base itself: the host address of guest memory. Guest code never sees it.

## Results

Every G2d slice passes unchanged under the new model on both targets, and there is one new callback slice.

| Slice | macOS | iPad Simulator |
|---|---|---|
| CRC32 `0x59f2a2` | 121/121 | 121/121 |
| `memmove` `0x5c83f0` (nine jump tables) | 300/300 | 300/300 |
| `strrchr` `0x5c88c0` | 200/200 | 200/200 |
| vec3 transform `0x5834d7` (x87, Halo precision) | 300/300 | 300/300 |
| vec4 transform `0x583b65` (x87) | 300/300 | 300/300 |
| map header `0x4434a0` over real `bloodgulch.map` via `CreateFileA`/`ReadFile`/`CloseHandle` | 6/6 | 6/6 |
| **vector iterator `0x582d1a` with Halo callbacks** | **200/200** | **200/200** |

**Callback slice.** `0x582d1a` is MSVC's vector iterator, `(array, element size, count, fn)`, and it runs `call [ebp+0x14]` once per element, last to first, with `ecx` = element. The callbacks are real Halo functions: `0x589484` zeroes a 12-byte element (it sits next to `0x589491`, the 12-byte-element destructor Halo itself passes to this iterator at `0x58c221`; that destructor frees memory through the C runtime heap, which the test runtime does not provide yet), and `0x5cc982` stores the vtable pointer `0x005ed490`. So the path is host → translated guest → dispatch → translated guest callback → return through dispatch → host. The random arrays (sizes 12–36, counts 0–39) sit inside larger buffers with random guard bytes on both sides. Buffers, return value and stack effect (16 bytes, `ret 0x10`) equal the x86 oracle in every case.

**Contract runs.** Each runs in its own process.

| Case | Expected | macOS and Simulator |
|---|---|---|
| Callback address inside a function (`0x582d1b`) | trap naming `0x00582d1b` | `HALOPAD TRAP: indirect transfer to 0x00582d1b, which has no compiled procedure` |
| Null array: the callback's first store goes to 0 | fault naming guest `0x00000000`; the oracle faults at the same address | PASS, oracle `0x0` |
| Unmapped array `0x7f000000` | fault at the first element visited, `0x7f00000c`; the oracle agrees | PASS, oracle `0x7f00000c` |
| Enter through Halo's `CloseHandle` slot `0x5df2f0` | bound import → HaloPad `CloseHandle`; an unknown handle returns FALSE and 4 bytes are popped | `00000000 4` |
| Enter through Halo's `Sleep` slot | stop naming the import | `Windows import Sleep has no implementation in this runtime` |
| Jump to `0xfffe0004` (inside an import's slot) | trap naming the import | `... inside import SetSecurityDescriptorGroup's address but not its entry` |

Sizes: 283 MB of LLVM IR; the object is 21.5 MB and the linked test executable 21.1 MB, the same for both targets. The object compiles in about 70–90 s.

## Limits and what remains

- **Physical iPhone/iPad (M07): parked.** Needs a device and signing from Chris. The Simulator runs on the macOS kernel, so it does not show whether an iOS app can reserve 4 GiB of address space. On a device this likely needs the `com.apple.developer.kernel.extended-virtual-addressing` entitlement. If it is unavailable, the fallback is a smaller reservation covering only the ranges Halo actually uses (image `0x400000–0x82c000` plus the heap and stack the runtime chooses), with the same fault reporting. That is the first thing the device capsule has to measure.
- **Dynamic imports.** The 46 delay-loaded imports start out pointing at Halo's own delay-load thunks (for example `0x582b7e`), which are in the dispatch table. Their loader calls `LoadLibraryA`/`GetProcAddress`, so HaloPad's `GetProcAddress` must hand out addresses the same way (a registered guest address per implemented export). That is G3 work, and until it exists these calls trap by name.
- **Threads** (the rest of M06) are not covered yet; they arrive with G3's runtime.
- Dispatch is a binary search on every indirect transfer. That is fine for correctness work and is measured in G3/G4 before any change.

