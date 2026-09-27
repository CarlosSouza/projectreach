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
| Modules | handles are the XP SP3 base addresses of the DLLs; `mscoree.dll` and `nvcpl.dll` are absent (no .NET, no NVIDIA control panel) | see `port/runtime/halopad_modules.c` |

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

## x87 additions

The backend now implements `fldpi`, `fldl2e`, `fpatan`, `frndint` (by the control word's rounding field), `fscale`, `f2xm1`, `fsincos` and `fxam` (`port/llasm-support/llasm_float.c`). SR's x87 model has no exception flags and no tag word: the status word is the condition codes plus TOP. So `fnclex` and `ffree` change no modelled state, `fnstsw` never reports a pending exception, and `fxam` never reports an empty register. Code that depends on those would diverge. Where a function might, the oracle can check it.

