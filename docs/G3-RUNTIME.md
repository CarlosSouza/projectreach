# G3 — HaloPad's Windows runtime: the reference machine

Translated Halo code calls Windows through HaloPad's runtime (`port/runtime/`, `port/llasm-runtime/`). The runtime presents **one fixed Windows machine**, so the game always sees the same answers. Each choice below is deliberate. A service that meets a case outside what it implements stops the program with the service name and the values it did not handle (`HALOPAD TRAP: <service>: ... is not supported by the runtime yet`). None of them returns a guessed value.

Run it: `.venv/bin/python scripts/run-core.py` (links the VA-model translation with the runtime and enters Halo at `0x5ccac7`; evidence in `docs/artifacts/<date>/G3/`).

## The machine

| Area | What Halo sees | Why |
|---|---|---|
| Windows version | XP SP3, 5.1.2600, NT platform, workstation | Custom Edition's supported era. Services that only exist later are absent, exactly as on XP (for example `FlsAlloc`, so the C runtime uses its TLS path). |
| CPU | one x86 processor, family 6, no MMX/SSE/SSE2/3DNow!, CMPXCHG8B present | the plain-CPU contract ([G2D-SIMD-SCOPE.md](G2D-SIMD-SCOPE.md)): Halo selects its generic code everywhere |
| Install path | `C:\Program Files\Microsoft Games\Halo Custom Edition\haloce.exe` (`GetModuleFileNameA`, command line) | the retail installer's default |
| Command line | the quoted path, plus `HALOPAD_ARGS` if set | Halo's own switches (`-window`, `-console`, …) pass through unchanged |
| Environment | empty block | deterministic; Halo does not depend on environment variables |
| Standard handles | none (`GetStdHandle` returns NULL) | a GUI process started from Explorer |
| Startup info | `STARTF_USESHOWWINDOW`, `SW_SHOWNORMAL` | as Explorer starts a program |
| Code pages | ANSI 1252, OEM 437; conversions, character types and case mapping come from Microsoft's `bestfit1252.txt` (pinned hash) via `scripts/gen-nls-tables.py` | US English Windows. Character types follow Unicode categories with the Windows XP-era overrides listed in the generator. |
| Time | real wall clock (`GetSystemTimeAsFileTime`); `GetTickCount` and a 10 MHz `QueryPerformanceCounter` from the monotonic clock | |
| Modules | fixed handles: XP SP3 base addresses for the system DLLs, HaloPad-chosen addresses for dinput8.dll and the game's DLLs; `mscoree.dll` and `nvcpl.dll` are absent (no .NET, no NVIDIA control panel) | see `port/runtime/halopad_modules.c` |

## Memory

- **Guest address space.** 4 GiB reserved with no access. The image sits at `0x400000`, and its headers, `.text`, `.rdata` and `.rsrc` are read-only on the host, as on Windows. Any write to Halo's code (self-modifying code, which translated code would not see) faults and names the address. The main-thread stack is `0x100000–0x300000`, the TLS block is at `0x7ffd0000` and the TEB at `0x7ffde000`.
- **`VirtualAlloc` family.** Windows granularity (64 KiB reservations, 4 KiB pages), requested addresses honoured, committed pages read as zero, and bookkeeping kept on the host. Apple Silicon hosts use 16 KiB pages, so granting access rounds outward and removing access rounds inward: a partially covered host page stays accessible. That is the one place guest memory is more permissive than Windows. Execute permission is accepted, but nothing in guest memory is ever executed: control only reaches compiled code through dispatch.
- **Heaps.** `HeapCreate`/`HeapAlloc`/`HeapFree`/`HeapReAlloc`/`HeapSize`/`HeapDestroy`, `GetProcessHeap`, and the fixed-memory forms of `GlobalAlloc`/`LocalAlloc`. Blocks are 16-byte aligned, and 512 KiB or more gets its own region. `HeapSize` returns the requested size. Bookkeeping is on the host, so guest overflows cannot corrupt it, and an unknown pointer passed to free, size or realloc stops the program. Movable global memory is not supported yet.

## Threads, TLS and synchronization

- **TEB.** Fields Halo uses: `fs:[0]` (SEH chain head, the only writable field), `fs:[4]`, `fs:[8]`, `fs:[0x18]`, `fs:[0x2c]`. Any other `fs:` offset stops with the offset. The PEB is not provided.
- **Static TLS.** From the image's TLS directory: the template is copied, the index is set to 0, and there are no TLS callbacks (checked).
- **`TlsAlloc` slots** live in the TEB at `+0xE10` (64 slots).
- **Critical sections** are host recursive mutexes keyed by the guest address. `InterlockedExchange` is a host atomic exchange on guest memory.
- **Structured exceptions** are not delivered. A guest fault stops the program with its address, and `SetUnhandledExceptionFilter` only records the filter.

## Imports, dynamic lookups and delay-loading

Every import is reached through the stable symbol `hpimp_<name>`, which resolves at link time to HaloPad's service or to a stub that stops with the name, so adding a service needs only a relink. `scripts/va-model.py` gives each static import, each delay-load import and each name in `config/runtime/dynamic-exports.txt` a guest address in a never-mapped page (`0xFFFE0000 + 16·i`), and that address is what `GetProcAddress` returns. Exports known to be missing on XP SP3 return NULL with `ERROR_PROC_NOT_FOUND`. Unknown names stop the program.

## Translated game DLLs (Keystone.dll, ksimeui.dll)

Halo loads `keystone.dll` in WinMain (`0x545f9d`, skipped only with `-safemode`) and resolves 17 exports. Keystone is Microsoft's UI library. Halo creates two Keystone windows after every device creation or reset (`0x51cdb0`), `KeystoneEditbox` and `KeystoneChatLog`, from `content/<height>editbox.ksml` and `content/<height>log.ksml`. These are the multiplayer chat input and chat log, and Keystone draws them each frame through Halo's own `IDirect3DDevice9` (`KsUpdate`, between `BeginScene` and `EndScene`). Keystone statically links D3DX9 and libpng and imports `ksimeui.dll`. Both DLLs ship with the game, so they are translated like the executable instead of being reimplemented.

- **Pipeline per module.** `scripts/hpmodule.py` lists the modules with their accepted hashes (the profile's `modules`). Every hint script takes `--module`, and `scripts/srw-pipeline.sh <build> --module keystone` runs the whole chain. A DLL's own relocation table is ground truth: the audit takes data pointers from it, roots the exports and the entry point, and cross-checks its code-operand addresses against it (none are missing). Functions that never return (`_CxxThrowException`, which raises a non-continuable C++ exception from a template; process and thread exit) become SRW `noret_procedures.sci`. SRW names WS2_32 and OLEAUT32 ordinals, and an unnamed `WINSPOOL.DRV` ordinal 203 becomes `WINSPOOL_ord203` (an explicit trap).
- **Results.** Keystone: 312,283 instructions, 65,756 procedures, 143 MB of LLVM IR. ksimeui: 7,188 procedures. D3DX's SSE/MMX/3DNow! routines are explicit traps, as in the executable, because the plain-CPU contract selects the generic paths. Remaining named traps: two fused-flag `test` forms, one 8-bit `imul`, one `and` with an address-like immediate, one 80-bit load, and `bt`/`bts` on memory in ksimeui's CRT.
- **One address space.** `scripts/va-model.py` rewrites each module into `va/<module>/` with its procedures at their original addresses in the shared dispatch table (193,611 entries). A module's imports from another translated module are bound to that module's real export addresses, as the Windows loader does. Everything else goes through the shared import registry.
- **Loader** (`port/runtime/halopad_modules.c`):
  - `LoadLibraryA` maps the image at its preferred base and loads static imports first (ksimeui before Keystone). It then binds the import table, write-protects the image and calls `DllMain(DLL_PROCESS_ATTACH)`. A FALSE return is followed by `DLL_PROCESS_DETACH` and an unload.
  - `GetProcAddress` reads the export directory in guest memory.
  - `FreeLibrary` runs `DLL_PROCESS_DETACH`, unmaps the image and releases the imports. New threads get `DLL_THREAD_ATTACH`/`DETACH`.
  - Until a DLL loads, its range reads as free, but automatic placement never uses it; an explicit reservation there stops the program, because translated code cannot be relocated.
  - HaloPad's handles for the DLLs it replaces natively (vorbisfile, binkw32, eula) moved to `0x6E000000`+, clear of `ksimeui.dll` at `0x10000000`.
- **Test** (`tests/halo_keystone_test.c`, 18 checks): the load, both DllMains in translated code, all 17 names Halo resolves matching the file's export table, the ordinal and missing-name cases, reference counting, unload through `DLL_PROCESS_DETACH` (which needed `TryEnterCriticalSection`), the range free again, and a clean reload.

## x87 additions

The backend now implements `fldpi`, `fldl2e`, `fpatan`, `frndint` (by the control word's rounding field), `fscale`, `f2xm1`, `fsincos` and `fxam` (`port/llasm-support/llasm_float.c`). SR's x87 model has no exception flags and no tag word: the status word is the condition codes plus TOP. So `fnclex` and `ffree` change no modelled state, `fnstsw` never reports a pending exception, and `fxam` never reports an empty register. Code that depends on those would diverge. Where a function might, the oracle can check it.


## Resources

`FindResourceA/ExA`, `LoadResource`, `LockResource`, `SizeofResource` and `LoadStringA` walk the PE resource directory in guest memory. For `LANG_NEUTRAL` they try neutral, en-US, English with neutral sublanguage, then the first language present. `strings.dll` (localized strings, dialogs and bitmaps) is mapped read-only at its preferred base `0x3f800000` from the game directory. Its `DllMain` is not run: that only initializes the DLL's own static C runtime, the DLL exports nothing, and its code has no dispatch entries.

## Registry

A persistent registry (`port/runtime/halopad_registry.c`) holds case-insensitive keys, Windows return codes and `ERROR_MORE_DATA` sizing, and stores values exactly as written. It is seeded from `config/runtime/registry-machine.txt`: only `HKLM\Software\Microsoft\Direct3D`, which every XP machine with DirectX 9 has. Halo's install keys, including the product-key-derived `PID`, are **never** created by HaloPad. Their absence behaves as on a machine where setup did not run with a key. `run-core.py` starts every run from the seed and keeps the final state as evidence.

## Kernel objects

Mutexes (owner and recursion, `ERROR_NOT_OWNER`), events (manual or auto-reset), timed and untimed waits, and named objects (`ERROR_ALREADY_EXISTS`) run on host synchronization with one lock and condition variable. Handles are `0x1000 + 4·i`; file handles sit below `0x1000`. Alertable waits work like plain ones until APC delivery exists, because nothing queues APCs yet.

## Page protection

Protection is tracked per 4 KiB guest page. `VirtualProtect` returns the previous protection of the first page, and `VirtualQuery` reports runs of equal protection. Image pages carry Windows' protections (`.text` `PAGE_EXECUTE_READ`, headers and `.rdata` `PAGE_READONLY`). On the host, restricting access rounds inward to 16 KiB pages. So `WinMain`'s one-page `PAGE_NOACCESS` stack sentinel (`0x2fe000`) is recorded but not enforced, and a stack smash that reaches it would go undetected instead of faulting.


## Callbacks and windows

Runtime services call translated Halo code (window procedures) through `halopad_call_guest`, which checks Windows' stdcall callback convention. Windows are host-side records for now: classes, the desktop (the Mac's main display), and `CreateWindowExA` with the documented creation messages (`WM_GETMINMAXINFO`, `WM_NCCREATE`, `WM_NCCALCSIZE`, `WM_CREATE`). `DefWindowProcA` handles those messages and traps on any other message number. Frame metrics are XP classic (caption 19, sizing frame 4). The AppKit/Metal view attaches with the graphics work.

## Paths

Relative guest paths and paths under the install directory map into the game directory, with `\` becoming `/`. Any other absolute path is refused.

## First-run license

`EBUEula` replaces the game's `Eula.dll`. It accepts only when the player has accepted this exact `Eula.rtf` (SHA-256) with `scripts/accept-eula.sh` at an interactive terminal, then writes REG_DWORD `FIRSTRUN=1` under `HKCU\Software\Microsoft\Microsoft Games\Halo CE` as the real DLL does. Otherwise it declines and Halo exits with code 1. HaloPad never accepts on the player's behalf.

