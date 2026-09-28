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
| Time | real wall clock (`GetSystemTimeAsFileTime`); `GetTickCount` and a 10 MHz `QueryPerformanceCounter` from the monotonic clock; `rdtsc` at 2.4 GHz | |
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
- **Structured exceptions** (`port/runtime/halopad_seh.c`). `RaiseException` walks the `fs:[0]` chain as XP's `RtlDispatchException` does: records and a `CONTEXT_FULL` go on the guest stack below the raiser, and each handler is called as cdecl `(record, frame, context, dispatcher context)`. Registrations must lie on the thread's stack and be 4-byte aligned. `ExceptionRecord.ExceptionAddress` is `RaiseException` itself, as XP records it. `RtlUnwind` calls each handler with `EXCEPTION_UNWINDING` down to the target and unlinks it, and then returns `ReturnValue` (x86 Windows ignores `TargetIp`). The following stop the program with the exception code: nested and collided dispositions, a handler that changes `eip` or `esp` in the context, continuing a non-continuable exception, and an unhandled exception. Guest faults (access violations) still stop the program; they are not delivered as exceptions yet. `SetUnhandledExceptionFilter` only records the filter. `HALOPAD_TRACE_SEH=1` logs each handler and its disposition.
- **Host entry levels** (`port/runtime/halopad_callback.c`). Translated code runs in continuation style, so the host stack only grows when a service calls back into guest code. Each callback gets its own return sentinel, `0xFFFFF000 + 16·level`. An SEH handler that accepts an exception jumps to its `__except` block or catch continuation from inside `RaiseException`'s callback, which abandons the inner host frames. When the guest later returns to an outer level's sentinel, those frames are discarded with `longjmp` and the outer call returns normally. A return to a level deeper than the current one, or past level 0, stops the program.

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

## MSXML 4 and what it needs (shlwapi, urlmon, OLE Automation errors)

Keystone parses and validates every `.ksml` layout with MSXML 4.0 SP2, and has no fallback. `msxml4.dll` (4.20.9818.0, SHA-256 `9808f05f…`, base `0x69b10000`) and its message DLL `msxml4r.dll` come from Halo's own installer (`redist/msxmlenu.msi`, extracted by `scripts/extract-reference-components.py` into `ref/inputs/reference-machine/system32/`). `msxml4.dll` is translated like Keystone (87,094 procedures). `msxml4r.dll` is only loaded as data (`LoadLibraryExA(LOAD_LIBRARY_AS_DATAFILE)`, whose handle has its low bit set, as on XP) for its message table.

- **In-process COM servers.** `CoCreateInstance` looks the class up in `config/runtime/com-servers.txt` (the twelve MSXML 4.0 classes), loads the translated DLL, and calls `DllGetClassObject` and then `IClassFactory::CreateInstance`.
- **SHLWAPI** (`port/runtime/halopad_shlwapi.c`). This covers the string, path and URL services msxml4 imports, plus the ordinal "wrap" exports, bound to what they forward to on XP (Wine's `shlwapi.spec` ordinals). These are: `PathIsURLW`, `PathIsRelativeW`, `UrlIsW` (`URLIS_URL`, `URLIS_FILEURL`), `UrlUnescapeW`, `PathSearchAndQualifyW` (fully qualified paths), `UrlCreateFromPathW` (`C:\a b\x` becomes `file:///C:/a%20b/x`), `PathCreateFromUrlW` (the slash forms Wine's tests record), `UrlCanonicalizeW` (strings without a scheme, as msxml4 canonicalizes `xs:anyURI` values), the `StrCmp*` family (locale compares go through `CompareStringW`; `StrCmpCW`/`StrCmpICW` are ordinal), `CharUpperBuffW`, `CharLowerW`, `IsCharSpaceW`, `IsCharAlphaNumericW`, `GetAcceptLanguagesW` (`en-us`), `CreateFileW` and `OutputDebugStringA/W` (`HALOPAD_DEBUG_OUTPUT=1` copies the text to stderr). URL schemes other than plain paths and `file:`, UNC paths, non-ASCII paths and escaping flags stop with the value. `HALOPAD_TRACE_SHLWAPI=1` prints the URLs and paths.
- **Internet security manager** (`port/runtime/halopad_urlmon.c`). MSXML creates `CLSID_InternetSecurityManager` for each document. Local files and `file:` URLs are in the Local Machine zone. Any other URL, a site object, and the zone mapping and policy methods stop.
- **OLE Automation errors** (`port/runtime/halopad_errorinfo.c`). `CreateErrorInfo` returns one object that answers both `ICreateErrorInfo` and `IErrorInfo` with one reference count. Each thread holds one current error object: `SetErrorInfo` replaces it, and `GetErrorInfo` hands it over and clears it. MSXML's error text comes from `msxml4r.dll` through `FormatMessage`.
- **VARIANTs**: `VariantClear` releases `VT_DISPATCH` and `VT_UNKNOWN` and leaves `VT_BYREF` values alone.
- **Test** (`tests/halo_msxml_test.c`, 18 checks): a DOM from a string (`nodeName`, `text`, `xml`), a malformed document rejected with MSXML's own message ("End tag 'a' does not match the start tag 'b'."), and `content/480editbox.ksml` validated against `KSML.xsd`. It is loaded both as a string and exactly as Keystone loads it: schema cache, `putref_schemas`, `validateOnParse`, no external resolution, and the file's UTF-16 text with its byte-order mark skipped. An element the schema lacks is rejected ("Element content is invalid according to the DTD/Schema. Expecting: font, b, i, …").

## Direct3D 9 device

- **`Reset`** follows Direct3D 9. It refuses while default-pool resources, state blocks, or application references to the implicit back buffer or depth buffer are alive (the runtime counts them per device). Otherwise it releases every binding, returns every state to its default, and recreates the back buffer and depth buffer from the new present parameters.

## Direct3D splash (Halo's first frame after the device)

Right after it creates the device, Halo draws its splash with `0x519080` (case in `eax`: 1 the picture, 0 black):

- `CreateOffscreenPlainSurface(640, 480, X8R8G8B8, D3DPOOL_DEFAULT)`, then the statically linked D3DX's `D3DXLoadSurfaceFromResourceA` (`0x582dfc`) for bitmap `0x86` (a 640×480 24-bit DIB) in `strings.dll`, whose handle Halo keeps at `0x6bde88` (loaded at `0x582590`);
- `GetRenderTarget(0)`, `StretchRect` onto the back buffer, Halo's present wrapper `0x51ba30` (`Present(NULL, NULL, NULL, NULL)` plus its frame counter `0x637d00`), and `StretchRect` again so the next back buffer holds the picture as well.

D3DX's loader has a staging path for surfaces it cannot lock (`0x58d1d9`: a system-memory texture, `CreateRenderTarget` plus `StretchRect` as a fallback, `UpdateSurface` on release at `0x58d123`). On HaloPad the offscreen surface is lockable, so D3DX writes it directly and that path is not taken; its methods stay loud traps until something reaches them.

**Test** (`tests/halo_splash_test.c`, the 15th suite): Halo's own `0x519080` on a 640×480 device. The test decodes bitmap `0x86` itself from `strings.dll`'s resource directory, independently of the runtime's resource services and of D3DX. Every one of the back buffer's 307,200 pixels equals it, the splash presents once, and the device is not marked lost; case 0 leaves the back buffer black. 12 checks, passing on macOS and on the iPad Simulator (evidence `docs/artifacts/2026-09-27/G3/core-arm64-apple-ios17.0-simulator-20260927T122311Z/`).

**Unreachable device method.** Halo's only `ProcessVertices` call (`0x51ff05`) is in `0x51fe90`, which nothing calls: there is no direct call, and its address appears nowhere in the image. With that, every `IDirect3DDevice9` method in the static inventory (41) is implemented.

## Halo's main and the main menu

`main` (`0x4ca9c0`) does the following, in order:

- initializes the game's systems (`0x4c96d0`, `0x4dad80`, `0x4ad8f0`, `0x45b330`, `0x4980e0`, `0x4e6730`, `0x4c9790`, `0x4cd1d0`, `0x4cd330`, `0x4cd080`);
- plays the intro movies (`0x43ed20`: `bungie.bik`, `gearbox.bik`, `mgs.bik`), unless `-novideo`, `-timedemo`, `-connect` or safe mode is set;
- runs its frame loop (`0x4cab41`), written inline in `main`. The loop loads the main menu through `0x4cbc90`: `levels\ui\ui` through `0x45b810`, `0x45b920` and `0x4cc960`, which starts a map from a 0x10c-byte options block. It draws each frame, and exits when `0x6b47eb` is set (Halo's quit), shutting down through `0x4cd290`.

**Tests** (component tests; the core runner remains the only path through `WinMain`, gated by the license and the product ID):

- `tests/halo_game_test.c`: `main`'s initialization step by step, then `0x4cbc90`. The main menu's map loads with no dialog.
- `tests/halo_menu_test.c`: `WinMain`'s set-up values and `0x5442e0`, then `main` itself. A test-only `Present` hook (`halopad_d3d9_present_hook`) counts frames and saves frame 120 as `menu.ppm`. After 150 frames it sets the quit flag, and `main` returns. There is no player input.
  - The main menu is drawn at 800 × 600: the ring and ship in 3D, the Halo logo, and Multiplayer, Profiles, Settings, Credits, Quit.
  - It passes on macOS and on the iPad Simulator.

**Services these needed:**

- `TranslateAcceleratorA` with a null table (Keystone's `KsTranslateAccelerator`: nothing translated, `ERROR_INVALID_ACCEL_HANDLE`);
- `CreateFileA` with the C runtime's `SECURITY_ATTRIBUTES` (length 12, no descriptor; inheritance is moot without child processes);
- DXT textures on GPUs without BC compression.

**DXT decoding.** The iPad Simulator and iPads before the M1 have no BC texture formats (`supportsBCTextureCompression`). There, Direct3D's DXT1–DXT5 textures are decoded to BGRA8 on upload (`halopad_d3d9_draw.c`): BC1 in 4- and 3-colour modes, BC2 explicit alpha, BC3 interpolated alpha. DXT2 and DXT4 decode as DXT3 and DXT5, as Metal's BC2 and BC3 read them. `HALOPAD_NO_BC=1` forces this path on a Mac. The main-menu frame decoded this way differs from the GPU-decoded one by 1.5/255 on average, and by under 1/255 in the static menu-text area. The rest is the animated background and Halo's cursor.

## Map loading (Halo's cache files)

Custom Edition keeps shared resources in `maps\bitmaps.map`, `maps\sounds.map` and `maps\loc.map`, which `0x442ff0` opens during the game's system start-up (`0x5442e0`). It also starts Halo's I/O thread (`0x4441c0`: alertable waits, `ReadFileEx`).

- **Opening a scenario:** `0x443c50`(path in `eax`, fatal flag) reads the 2 KB header through the I/O thread and validates it into a cache slot (`0x644318`, `0x80c` bytes each; `0x443ed0`). The checks are "head" and "foot", the size, the name, and version 609.
- **Loading it:** `0x442290` copies the header into the current map's (`0x643044`) and reads the tag data into tag memory at `0x40440000`. It sets the tag index (`0x817144`, 32-byte entries), fixes up every tag's references, including sounds and bitmaps held in the shared resource maps, and returns the scenario tag's datum handle, or −1.

**Test** (`tests/halo_maps_test.c`): a component test. It sets what `WinMain` has set up by then (window class values, the machine measurement, memory, Keystone, the DirectX library pointers), runs `0x5442e0`, and loads two maps:

- the systems start with no dialog, and the texture and sound caches exist;
- `ui.map` loads with 1,412 tags, scenario `scnr` "levels\ui\ui";
- `bloodgulch.map` loads with 2,455 tags, scenario `scnr` "levels\test\bloodgulch\bloodgulch".

It passes on macOS and on the iPad Simulator.

**Translation gaps this closed** (lifter `161d2b412a4a-89ccc7fb`, run `20260927T140130Z-31169`). The system start-up formats a number with the C runtime's `printf`, whose long-double conversion (`0x5d70c7`, `0x5da220` and on) and math intrinsics (`_trandisp`, `fmod`) use forms the translator lacked:

- **80-bit loads and stores** (`fld m80`, `fstp m80`: 146 sites). The address goes to a register, and helpers convert between the ten bytes and the stack's `double`. A load rounds the 64-bit significand once; a store is exact.
- **Byte rotates and mixed-byte tests:** `rol`/`ror` of a low byte by a constant, and `test` between a low and a high byte (`test ch, cl`, `test dl, ch`, `test bl, ah`). The missing `test` forms had produced no code, so the next `jz` read a stale condition. LLVM made that a trap at `0x5cd153`, and the translator had reported the other two as "unprocessed flags". A scan of the whole translation finds no other "flags not needed" site followed by a flag read.
- **`fprem`** (`fmod`'s loop; always complete on the `double` stack, so C2 is clear) and **`fsubr`/`fdivr st(i), st(0)`**.
- **The audit's data-like instruction heuristic** now accepts the x87 status idiom (`wait` after `fnstsw ax` or before an x87 instruction, `sahf` after `fnstsw ax`). It had rejected `fmod`'s body (`0x5cdc14`), which is reached only from its descriptor table at `0x61fac0`. The audit gains 16 functions (the math thunks and dispatchers) and 15 relocations, and loses nothing reachable.
- **Still untranslated:** 3 sites (6 at the time; the four `rcl bl` sites are translated since, see "Blood Gulch in play"). One is `imul byte [ecx+0x1d]` (`0x598198`); two are `fnstenv`/`fldenv` (`0x5dac13`, `0x5dac22`) in the CRT's Pentium FDIV workaround, which runs only when `0x63e304` is set.

## Blood Gulch in play (Halo's start-up script)

Custom Edition runs a console script at start-up: `main`'s `0x4c9790` reads the file named by `-exec` (otherwise `init.txt`) and runs each line as a console command through `0x4c9dc0`. `WinMain` has already split the command line into arguments with `0x545a00` (the vector at `0x6bd160`, the count at `0x6bd164`).

**Test** (`tests/halo_bloodgulch_test.c`, a component test): it writes a one-line script, `map_name levels\test\bloodgulch\bloodgulch`, through HaloPad's file layer, passes `-exec` with that file, sets what `WinMain` has set up by then, starts the systems (`0x5442e0`) and calls `main`. A test-only `Present` hook saves frame 300 as `bloodgulch.ppm` and sets the quit flag after 330 frames. There is no player input. Checks: `main` returns when asked to quit, the loaded map is "bloodgulch", the frame is not blank, and no dialog appeared. The script file is removed afterwards.

- Halo spawns the player in the red base and draws the level in first person at 800 × 600: the assault rifle with its ammunition counter, the HUD (health, shield, grenades, motion tracker) and the reticle.
- It passes on macOS and on the iPad Simulator.

**What it took:**

- **An audit fix for tables of code pointers that read as text.** A `.text` address whose three low bytes are printable reads as a short string (`0x566d20` is " mV" and a NUL), so a run of such pointers was taken for text and dropped. One was the callback table at `0x636b18`, whose `0x566d20` had no compiled procedure ("indirect transfer to 0x00566d20"). A text-like dword is now kept as a pointer when a neighbouring dword also points into `.text`. The three-letter language codes that used to pass as addresses ("FRB", "ZHH", "DEL" and one UTF-16 pair) now drop out, because their neighbours point into `.rdata`.
- **An audit fix for `__except` bodies.** MSVC puts `wait` before a `__try` state change (`mov dword ptr [ebp-4], n`), and the audit's data-like instruction heuristic rejected that. It dropped the `__except` handlers at `0x54651a`, `0x546a7e` and `0x546c28` once their scope tables were read as pointers, and had found two of them only by gap probing. The `wait` before `C7 45 FC` now counts as code. Net: the same 7,230 function entries, +6 real ones and −6 bogus ones, 1,764 found from data pointers, 3 more jump tables and 1,432 fewer unclassified bytes.
- **`rcl` of a byte register by a constant** (translator). Halo's ADPCM sample step `0x551d10` (a code, a predictor and a step size in; a sample clamped to 16 bits out) reads the code's bits out of CF with `rcl bl, 6` / `rcl bl, 1` and `sbb`. The translator rotates the 9-bit value CF:byte and sets CF, and OF for one-bit rotates. The `adpcm_step` slice runs the function over all 256 code bytes and random predictors and steps, and matches the x86 oracle.

Translation in use: run `20260927T145850Z-51456`, lifter `24c44b99cfac-05b596a6`. Untranslated sites: 3.

## Playing Blood Gulch: keyboard, mouse and trigger

**Test** (`tests/halo_play_test.c`, a component test). It loads Blood Gulch as above, then acts like a player at the keyboard, giving host input the way the app shells deliver it (`halopad_input_event`). The input reaches Halo through USER32 and DirectInput 8: the buffered keyboard (`0x4946b7`) and the exclusive mouse (`0x4947b2`). Every effect is checked in Halo's own game state:

- **Where the state is.** Halo's set-up code names the tables. `players` (`0x476170`) has 0x200-byte entries, with the table pointer at `0x815920`. `object` (`0x4f83e0`) has 12-byte entries whose `+8` is the object's address, with the table pointer at `0x7fb710`. A data table keeps its capacity at `+0x20` and its first entry at `+0x34`. A handle is salt << 16 | index. Player `+0x34` is the player's unit.
- **Offsets read from the unit and weapon.** Object `+0x5c` is the position and `+0x68` the velocity. Unit `+0x23c` is where it looks, and `+0x118` is the weapon in hand. Weapon `+0x2b8` is the rounds in the magazine.
- **Standing still:** without input, the player stays put.
- **Walking:** holding W for 100 frames walks the player 7.3 units forward, along +x and down the base ramp. Letting go stops it.
- **Turning:** 300 mouse counts to the right turn the view about 25° right.
- **Firing:** holding the left button fires the assault rifle; the magazine goes from 60 to 50.
- **The frame while firing** (`play.ppm`) shows the world, the muzzle flash, the rifle's counter and the shot on the motion tracker. Fewer than 10% of its pixels may be the bare clear colour.

It passes on macOS and on the iPad Simulator.

**A miscompile the trigger exposed: flags across a call.** Before the fix, firing made the camera's view-projection NaN, and every world vertex was discarded. The frame was the bare clear colour (Blood Gulch's fog, `0xffe6c4`) with only the HUD on it. The chain was:

- the C runtime's `acos` (`0x5ccd00`) calls `0x5d7318`, which classifies the argument's exponent with `cmp eax, 0x7ff00000`;
- it then calls `0x5ccd1d`, which begins with `je` on that ZF. A `call` does not change flags, so hand-written code can pass flags this way.
- The translator fused the `cmp` with the local `je` and never materialised ZF.
- `scripts/srw-flags.py` had noted the consumer at `0x5ccd1d`, but it treated flags at a function entry as undefined and followed only its post-call path. So `acos` read a stale ZF.
- For arguments whose double has a zero low word (floats whose last three mantissa bits are zero), it took the domain-error path and returned the 80-bit indefinite NaN from `0x620230`.
- Halo's vector-angle routine `0x4d08f0`, called for the first-person weapon, passed that NaN into the camera.

Fix: the hint generator now follows every path into a label that reads flags: the callee's returns after a call, **the direct call sites of a function entry**, and direct predecessors. That adds 24 hints, nearly all in the C runtime's math helpers (`0x5d727c`–`0x5d7364`, `0x5cd1fb`). It also resolves the CPUID check `0x5441a0` (it reads the caller's flags with `pushfd`), leaving one unresolved label (`0x5a142e`). Translation run `20260927T162224Z-63690`. Keystone links the same C runtime math code and gains 22 hints; it is retranslated as run `20260927T163514Z-67223` (65,819 procedures). The hints for ksimeui, Controls and MSXML 4 are unchanged.

How it was found (the tools stay available):

- `HALOPAD_TRACE_DRAWS` showed the same draws with the camera constants turned NaN.
- A scan for new NaNs in Halo's data narrowed it to the first-person weapon (`[0x64dcc8]`, 0x1ea0 bytes per player) and the camera globals.
- A temporary check in the x87 helpers found the first NaN: the 80-bit load in `acos`'s domain-error path.


## Hosting a game from Halo's menus (G4)

**Test** (`tests/halo_host_test.c`, run with `scripts/run-core.py --fresh-state`). Halo starts at its main menu with an empty profile folder. The test presses keys as a player would:

1. Multiplayer. Halo asks for a profile name, and Enter accepts "New001".
2. Create Game > LAN.
3. Select Map: the first map, Battle Creek.
4. Select Gametype: Slayer, then focus down to OK.
5. Server Setup > Start Game.

Halo then hosts the game itself: the Slayer rules screen, "Welcome New001", the map `beavercreek`. That makes it a second stock map, with Blood Gulch as the first.

**Checks, each in Halo's game state:**

- **Fire:** the trigger adds a projectile object.
- **Melee:** F swings, and unit `+0x2ac` is set during the swing.
- **Look down:** the mouse turns the look vector's k below −0.5.
- **Grenades:** the right button throws a frag. Unit `+0x31e`, the frag count, goes from 2 to 1.
- **Damage:** two frags at the player's feet take the shields to 0 and the health down.
- **Death:** "New001 committed suicide", then "Rejoin in 5".
- **Respawn:** Slayer spawns the player again as a new unit at a spawn point.

It passes on macOS and on the iPad Simulator, and it passed on repeated runs. The frames are saved as `host-01` to `host-06.ppm`.

**Fixes this needed:**

- **A jump table with a hole.** `0x4a77a4` (`jmp [edx*4+0x4a7810]`, index from a loop count, no bound) has a NULL fourth slot. The audit stopped there and missed the cases `0x4a77dd` and `0x4a77fa`, so Create Game > LAN trapped with "indirect transfer to 0x004a77fa".
  - The audit now skips a NULL slot in an unbounded table when a code pointer follows and it is not in known code.
  - That adds 2 relocations and changes nothing else. Translation run `20260927T182330Z-93072`.
- **`GlobalReAlloc` with `GMEM_MOVEABLE`** on a fixed block (the create-game screen grows a list this way). With the flag the block may move; without it, it is resized in place or the call fails.
- **What WinMain sets up for GameSpy.** The test calls WinMain's GameSpy set-up (`0x5797e0`: game name "halom", the port, and a key read from WinMain's own instructions at `0x544d27`). That set-up registers Halo's query keys (`qr2_register_key` `0x5c0850`). Without it, the lobby's server query read a NULL key name.
  - The key string at `[0x6e1468]` comes from Halo's own `0x5829e0` as in the join test. It is empty without a `DigitalProductID`.
  - Opening the Internet lobby shows "An error has occurred trying to contact the GameSpy master server.", as it would offline.
- **Fault reports name the translated procedure.** A fault now prints the host program counter and return addresses. `run-core.py` turns them into procedure names with `atos`, for example `loc_5BBE18`. Translated code has no guest program counter, so this is how a fault's location is found.
- **`run-core.py --fresh-state`:** the run starts from an empty state folder, kept in the evidence.

Vehicles and pickups, sound, menu return, map reload and relaunch are covered in the next two sections.

## Vehicles and pickups (G4)

**Vehicles** (`tests/halo_vehicle_test.c`). On Blood Gulch the test steers with the mouse and W: out of the red base, then to the driver's side of the Warthog outside it (`vehicles\warthog\mp_warthog` near (102.3, −144.7)). Checks, in Halo's game state:

- up close, Halo offers the seat: player `+0x24` is the Warthog and `+0x28` is 8 (enter a seat), shown on screen as "Press "E" to enter driver seat of Warthog";
- E puts the player in the driver's seat (the unit's parent, `+0x11c`, becomes the Warthog);
- W drives it about 10 units, and the player is still in it when it stops;
- E gets out beside it.

Driving on uphill leaves the driver out of the seat. W held for 3 s drives the Warthog up the canyon wall, and at about 3 units up the player falls out (parent `-1`), about 177 units away: the game's own flip behaviour, not an input fault. That is why the test drives for 3 s and stops before the wall.

Earlier attempts that did not enter the Warthog stood 1.5 units from it. Halo offers a vehicle only within its search radius (`0x4fa8f0` with the unit's radius): at 0.63 units the offer appears.

**Pickups** (in `tests/halo_host_test.c`, after the respawn):

- The player turns to the nearest loose weapon on its level that it does not already hold. It walks until Halo offers it (player `+0x24`, type 7, "Hold E to pick up"), then holds E.
- The weapon joins the unit's weapons (`+0x2f8`), and Halo shows "Picked up a plasma rifle" (or whichever weapon it was).
- If geometry blocks the way (no progress for 60 frames), it tries the next-nearest weapon. Slayer respawns at a random point.

The host test is in the suites with one retry. Frags can bounce away from the player's feet in some runs, because Halo's ticks follow real time while the test's inputs follow frames. In the last seven runs with the final settings, every run passed on the first try. The look-down amount is kept at 1,200 counts: 1,800 or 3,200 counts turned the look vector back up (k 0.27, 0.22), and the throws went up.

**A graphics limit removed.** Halo's fixed-function draws produced more than 256 distinct stage cascades once the Warthog and its effects were in view, and a draw trapped. The generated programs are now kept in a hash table that grows.

## Sound, menu return and relaunch (G4)

**Sound, captured** (in `tests/halo_host_test.c`). Tests have no audio device, so a thread in the test plays the device's part. Every 10 ms it pulls from DirectSound's mix (`halopad_dsound_mix`) as many frames as real time has used, just as Core Audio's callback would. It records the whole session and saves it as `session.wav` next to the other evidence. Three checks run on the recording:

- **Menu music:** the loudest 50 ms window between 2 s and the first shot is above −40 dBFS. It measured about −11 dBFS.
- **Gunfire:** the loudest window from 0.25 s before to 0.5 s after the first projectile appears is at least 15 dB above the game's ambience. Ambience is the median 100 ms level over the 8.5 s before the shot, so short spawn and announcer sounds do not raise it. Measured: shot about −10 dBFS, ambience about −40 dBFS.
- **The explosion that kills:** the loudest window around the death is at least 20 dB above the same ambience. It measured about −8 dBFS.

The recording is a real session and can be listened to. The combat sequence stays timed in frames. A version timed in milliseconds threw the frags at the wrong moment for the player's look and speed, and the player stopped dying on both platforms.

**Menu return, map reload, quitting and relaunch** (`tests/halo_lifecycle_test.c`, run with `scripts/run-core.py --fresh-state --relaunch`). `--relaunch` runs the build twice, one run after the other, on one state folder (`HALOPAD_LAUNCH` 1 and 2), with evidence under `launch-1/` and `launch-2/`. The test only presses keys:

1. **Launch 1:**
   - Multiplayer, which creates profile "New001", then Create Game > LAN > Battle Creek > Slayer > Start Game.
   - In the game, Escape > Leave Game back to the main menu.
   - The same game again, so the map loads a second time, then Leave Game.
   - Quit > OK. Halo's own `main` returns by itself. The test sets the quit flag only if a 20,000-frame guard runs out, and that counts as a failure.
2. **Launch 2:**
   - Multiplayer goes straight to the multiplayer menu with no name prompt, because Halo loads the profile it saved.
   - The same game starts with the player named New001.
   - Leave Game, then Quit > OK.

The checks cover the order of maps loaded, the player's name, `main` returning by itself, and the saved profile files between the launches.

## Joining a server (G5, step 1)

HaloPad's Halo joins the original Custom Edition 1.10 dedicated server and plays on it. The server runs as `haloceded.exe` in the project's CrossOver bottle: private, `sv_public 0`, bound to 127.0.0.1:2310.

`scripts/reference-join.sh` does the whole run:

1. it starts the server and waits for its status answer;
2. it runs `tests/halo_connect_test.c` on HaloPad;
3. it checks the server's own log for the join from HaloPad's address;
4. it stops everything.

It passes on macOS and on the iPad Simulator. A typical server log line is `JOIN SUCCESS "Whicker" player 1 machine 1 (127.0.0.1:2305)`. The client loads the server's Blood Gulch within a few frames, and the server spawns the player at a base with the multiplayer HUD (`join.ppm`).

**Halo's path.** `main`'s `0x4cd080` reads `-connect <address>` (with optional `-name` and `-password`) and calls `0x4cb800`. The network start-up follows:

- `0x4415c0` calls `WSAStartup`, then `gethostname` on a thread, then `gethostbyname` of that name. It uses `h_addr_list[0]` unchecked.
- GameSpy's transport handshake (`fe fe 01` request, `02` challenge, `03` answer, `04` accept) then game data. In the answer, `0x5bc980` hashes the key string at `[0x6e1468]` into the response.

**What the test sets for WinMain** (as the other component tests do):

- its `-cport` value (`0x544c93`: `[0x6337fc]` and the flag `0x6b7360`). It is 2305, because the server holds 127.0.0.1:2302–2303 on the default ports.
- `[0x6e1468]`, set from **Halo's own** `0x5829e0`. That reads the installer's `DigitalProductID`. There is none on this machine, so it returns Halo's empty string (`0x5f363c`). WinMain itself would stop at that point with "Your product key is invalid".

Nothing is written to the registry, no key is made up, and nothing in Halo is patched. **The private server accepts a client without a key**: it logs the key hash as `d41d8cd98f00b204e9800998ecf8427e`, the MD5 of "". That is the original server's behaviour on a LAN. Public servers check keys, which is why online play as a real player still needs the key.

**Ports on one machine.** Both processes bind the Halo ports: the client binds 0.0.0.0:2302 and :2303, and the server binds 127.0.0.1:2302 and :2303 by default. Datagrams to 127.0.0.1 then reach the more specific binding. At first HaloPad's join request went to its own socket, and later the server's challenge went to the server's own 2303. Separate ports (server 2310, client 2305) are what two installations on one PC need too.

**Services this needed:**

- **Winsock ordinals.** `WS2_32` and `WSOCK32` are delay-imported by ordinal. `pefile` names well-known ordinals, but the delay-load helper asks `GetProcAddress` for `#115`. `va-model.py` now registers both the name and `#<ordinal>` for ordinal imports of the executable.
- **Own host name.** `gethostbyname` of the machine's own name (short or full) returns its IPv4 interface addresses, or 127.0.0.1 without a network, as Windows does. macOS resolves only the `.local` form.
- **WinInet and WinHTTP** (`halopad_wininet.c`, `winhttp.dll` added as loadable). Halo's version check (`0x57a620`, on its own thread) looks for a proxy. The reference machine has none:
  - `InternetQueryOptionA(NULL, INTERNET_OPTION_PROXY)` reports direct access;
  - `WinHttpGetProxyForUrl` fails with `ERROR_WINHTTP_AUTODETECTION_FAILED` (no WPAD server).
- **Network policy** (`HALOPAD_NET`, set to `lan` by `run-core.py` for every test). It models the reference machine on a LAN with no internet:
  - only loopback, private, link-local and broadcast destinations are reachable, and others fail with `WSAENETUNREACH`;
  - only the machine's own name and `localhost` resolve, and others fail with `WSAHOST_NOT_FOUND`.
  So the version check's lookup of `hpcup.bungie.net` fails as it would offline, and no test reaches a public host.

**Two HaloPad clients in one game** (`tests/halo_match_test.c`, run with `scripts/reference-join.sh --test tests/halo_match_test.c --clients 2`).

`scripts/run-core.py --clients N` builds once and starts N instances. Each has `HALOPAD_CLIENT=<i>`, its own state folder (`generated/halopad-disk-client<i>`) and its own evidence folder (`client-<i>/`). Two installations on one machine need their own ports: `-cport 2305+i` and `-port 2320+i` (`[0x6337f8]`, WinMain `0x544c63`). With the same `-port` the second client's bind fails, and Halo gives up before sending anything.

Client 0 walks and fires. Client 1 is meant to watch the other player's unit move through the server.

**Result: the original server accepts only one of them.** Both complete the handshake and are logged as `JOIN SUCCESS`. About a second later the server drops the later one (`QUIT <No Player> machine 2`), which shows "Your CD Key is invalid.". Both present the same key hash (the MD5 of the empty key string), and Custom Edition refuses a second player with a key already in the game. Which client stays is decided by arrival order.

So a two-player match needs two different legitimate keys: the parked "second legitimately provisioned player". No key is generated to get around this. The test and runner are ready for that day, and they are not in the regression suites because they cannot pass without it. The client that stays is spawned, walks (10.7 units in 200 frames of W) and fires, as in the single-client join.

## Public servers (G5): the games people play

HaloPad's Halo joins the public Custom Edition servers people play on today, through Halo's own
`-connect` path.

**Finding them.** The community master server `s1.master.hosthpc.com` lists the servers. Sigmmma's
HaloQuery (`ref/tools/HaloQuery`, private; its native GameSpy decoder needs
`GCC_ENABLE_CPP_EXCEPTIONS` in `binding.gyp` to build on macOS) lists them, and a status query
reads each one's name, map, players and version. On 2026-09-27 the list held 252 servers and
193 answered. All ran 01.00.10.0621, most with SAPP 10.2.1; the busiest held 15 of 16 players.

**Joining them** (`scripts/public-join.sh [--list N] [ADDR:PORT ...]`). For each server it reads
the current map, runs `tests/halo_connect_test.c` with the host's network (`HALOPAD_NET=internet`),
stays about 25 seconds (1,200 frames) and quits. The test checks, in Halo's own state, that the
server's map loaded and that the server spawned the player's unit. The client sends Halo's own
key value, the MD5 of the empty string from `0x5829e0`; nothing is made up.

| server | map, mode, players | result |
| --- | --- | --- |
| AUSSIES MADNESS 5 (216.245.177.89:2308) | Blood Gulch CTF, 4/16 | spawned (twice); the server's "GIDAY **BE NICE OR BE GONE" on screen |
| DEADLY ZOMBIES (102.129.137.87:2302) | wizard Slayer, 7/16 | spawned |
| POQclan (74.91.125.79:2302) | Ice Fields CTF, 4/16 | spawned; "Become a member! JOIN at poqclan.com" |
| POQclan CE17: Massacre Island (74.91.124.220:2302) | Death Island CTF, 5/16 | spawned; also from the iPad app (next section) |
| 74.91.125.111:2302 | Sidewinder CTF, 16/16 | full: 2 packets, no join |

No server refused the key value. A server can check keys (SAPP's `sv_cdkeycheck`); a server that
does, or that already holds a player with the same key hash, will refuse it, and only a
legitimate key typed into Halo's installer changes that. Tests other than this script keep the
LAN policy (`run-core.py` sets `HALOPAD_NET=lan` unless it is set).

## The iOS and iPadOS app: Halo on screen, touch controls, the menu

**On screen.** Halo's frames now reach the app's view. Halo's thread creates its windows'
CAMetalLayers and changes them (drawable size, visibility), and that thread has no run loop, so
Core Animation never committed those changes: every frame was presented to a layer that showed
nothing (a red test background filled the area, and Halo's back buffer, read back, held the
menu). The host now commits its layer changes itself (`[CATransaction flush]` after creating,
showing or resizing). Component tests never saw it: they read the back buffer.

**Registry values under paths with spaces.** The registry file's reader split lines at spaces,
so a value under `HKCU\Software\Microsoft\Microsoft Games\Halo CE` (Halo's `gamma`) stopped the next
launch. The type, name and data are now read from the end of the line.

**Touch controls and the three-dot menu** (`port/ios/HaloPadOverlay.m`), adapted from SunPad (see
[SUNPAD-TRANSFER.md](SUNPAD-TRANSFER.md)). In a game (Halo's map is not "ui") a move stick, a look
area and Halo's PC controls appear; in Halo's menus touches reach the game view as clicks. The
menu adds what Halo's menus lack: joining a server by address through Halo's console, recent
servers, the keyboard, console and chat, aspect ratio, an FPS counter, touch-control settings
and a problem report. The app is landscape on iPad and iPhone (it asks the scene for landscape,
since iPadOS 26 no longer holds apps to Info.plist's list).

**Development scene** (`tests/halo_app_scene.c`, `scripts/build-ios-app.py --scene ...`): Halo from
its main menu in the app, prepared as the component tests prepare it (the key string from Halo's
own `0x5829e0`; nothing is written or made up). With `HALOPAD_ARGS='-connect 74.91.124.220:2302'` the
iPad Simulator app joined POQclan's Death Island CTF game and played at about 26 frames per
second with the controls over Halo's HUD.

**Still open here:** the Simulator's audio output unit does not start in the app ("audio output
unit would not start"); the touch controls are checked by screenshot only (no unattended touch
test yet); iPhone layouts are unchecked; the menu's console path (`connect` typed into Halo's
console) needs a test.

## Rasterizer initialization (Halo's graphics start-up)

`0x51a240` (reached from `WinMain` through `0x5442e0` and `0x515610`) is Halo's whole graphics start-up. In order:

- the display settings (`0x51a140`), with a default of 800 × 600 at 60 Hz, or 640 × 480 on a machine at 1000 MHz or less, with 128 MB or less, or in safe mode;
- the game window (`0x5191d0`);
- the adapter and its caps, and `0x580a00` again (`config.txt`);
- the shader path from the card's caps and the `-use*` switches;
- the NVIDIA control-panel query (`NVCPL.dll`, absent on the reference machine), the desktop's `GetDeviceCaps`, and the presentation parameters (`0x519860`);
- `CreateDevice` and the splash (`0x519080`);
- the vertex shaders and effect collection (`0x51a0b0`: `shaders\vsh.enc`, `shaders\EffectCollection_ps_2_0.enc`);
- the rasterizer's subsystems (`0x532bb0`, `0x51f290`, `0x518a60`, `0x5180c0`, `0x51ea70`, `0x52fb00`, `0x53a260`, `0x525860`, `0x4d3730`, `0x444e30`, `0x51adb0`, `0x51b440`);
- the chat windows (`0x51cdb0`).

**Test** (`tests/halo_raster_test.c`): a component test that sets only what `0x51a240` reads from `WinMain` (the window class values, the Direct3D library pointers, the machine measurement, `strings.dll` and the Keystone loader), then runs it. The results:

- It succeeds with no dialog: the game window, and a device with an 800 × 600 back buffer, the reference machine's default resolution.
- It reads `config.txt`, `shaders\vsh.enc` and the pixel shader 2.0 effect collection. That is the path Halo picks for the Radeon 9700 PRO.
- It lays out the chat from `content/600editbox.ksml` with its schema.
- The splash is left stretched over the back buffer.

It passes on macOS and on the iPad Simulator. The core runner is still the only path through `WinMain`, and it stops at the license and the product ID.

**The time-stamp counter.** HaloPad's `rdtsc` runs at the reference machine's 2.4 GHz (the host's nanoseconds × 12/5), so `0x580e70` measures 2400 MHz. It first ran at 1 GHz. Halo reads exactly 1000 MHz as its low-spec class (`0x51a2ad`: a 640 × 480 default; `0x53d70a`, `0x53e3c0`, `0x53e5ac`: lower detail defaults for new profiles), which contradicts the Radeon 9700 PRO machine the other answers describe.

## Dialog boxes (Halo's warnings and errors)

Halo reports problems with `0x582060(text id, link id, fatal)`. It loads the texts from `strings.dll` (in the player's language, falling back to English), then shows template `0x66` ("Halo - Warning") with `DialogBoxIndirectParamA` through `0x5817e0`, and its dialog procedure `0x581b90` runs it:

- `WM_INITDIALOG`: centres the dialog (`GetWindowRect`, the desktop's `GetClientRect`, `MoveWindow`), sets the title and the problem text (control 1006) and, for a fatal error (`0x68b3ac`), disables **Continue Anyway** (1004), **Continue in 'Safe Mode'** (3) and "Don't show this warning again" (1001). `0x581ab0` then subclasses the dialog (`0x581890`: the link's colour in `WM_CTLCOLORSTATIC`, blue `0xC00000`, red while hovered) and the link (`0x581940`: `SS_NOTIFY`, an underlined copy of its font through `WM_GETFONT`, `GetObjectA` and `CreateFontIndirectA`, the hand cursor, capture while hovered). Machine info (1009) is "%dMHz, %dMB" from start-up's measurements.
- `WM_COMMAND`: 1004 ends the dialog with 0 (continue), 3 with 1 (safe mode), 1005, `IDCANCEL` and `WM_CLOSE` with 2 (exit); the checkbox is read into `0x6bde90`. The link (1007) opens the support URL with `ShellExecuteA`.
- Afterwards `0x582060` records "don't show again" under `HKCU\Software\Microsoft\Microsoft Games\Halo CE` (a warning so marked is not shown again), and after a fatal error or Exit it shuts down and calls `ExitProcess(1)`.

HaloPad's dialog manager (`halopad_user32.c` part 3) builds real Win32 state from the template (`DLGTEMPLATE` and `DLGTEMPLATEEX`): the dialog window (class `#32770`, `DefDlgProcA` calling the dialog procedure), and Button and Static controls as child windows with ids, text, visibility, enabled and check state, fonts, and window procedures that have guest addresses (`HaloPadButtonWndProc`, `HaloPadStaticWndProc`, not visible to `GetProcAddress`) so Halo's subclassing and `CallWindowProcA` work. Messages follow Windows: `WM_SETFONT` to the dialog and each control, `WM_INITDIALOG` with the first tab stop, `BN_CLICKED` and `STN_CLICKED` notifications, `DefDlgProc`'s `WM_CLOSE` → `IDCANCEL`, children destroyed after their parent's `WM_DESTROY`. Dialog units use Windows XP's MS Shell Dlg 2 (Tahoma 8 pt) at 96 DPI, 6 × 13 base units: Halo's dialog is 458 × 166 client pixels.

The modal loop hands the dialog's current state to the host (`port/runtime/halopad_dialog.h`) and turns the player's action into what Windows would send: `BM_CLICK` to a button or checkbox, button down and up to the link, `SC_CLOSE` to the dialog. The iPadOS app shows it as a sheet laid out from the template (links open in Safari); the macOS runner has no dialog screen, so a dialog there prints its contents and stops unless `HALOPAD_DIALOG_ACTIONS` scripts the answers (control ids or `close`). Pop-up windows created with `CW_USEDEFAULT` get Windows' (0, 0) and 0 × 0, as Halo's temporary owner window for errors before its main window exists.

**Test** (`tests/halo_dialog_test.c`, 26 checks, passing on macOS and on the iPad Simulator): Halo's own code, with the test as the host. The Ctrl-key warning shows the right title, text, buttons, checkbox, icon, link colour and machine info; the link opens `http://www.microsoft.com/games/halo/support.asp`; the ticked checkbox makes Halo record the choice and not show the warning again; the product-key error (fatal) leaves only Exit enabled and returns 2. **Evidence on the iPad Simulator:** `build-ios-app.py --scene tests/halo_dialog_scene.c --launch` runs a scene that calls `0x582060` for the Ctrl-key warning; the screenshot shows Halo's dialog waiting for the player (`docs/artifacts/2026-09-27/G3/ios-app-20260927T124014Z/screen.png`). `--scene` is for evidence only: it replaces the app's core start (`halopad_app_entry`).

## Start-up checks after the license, and the product ID

After the license, `WinMain` checks the machine (`0x5449c7`–`0x544c38`) and reports each problem through `0x582060`: Direct3D 9 (`d3d9.dll`, then `0x580a00` on the `IDirect3D9`), the Ctrl key held (a warning offering safe mode), DirectSound, DirectInput, `shfolder.dll`, an unclean last exit (`0x581e40`), physical memory, CPU speed (`0x580e70`), free space in the temporary folder, and then **the product ID**: `0x5829e0` reads `DigitalProductID` (REG_BINARY, 164 bytes, version 3) from `HKLM\Software\Microsoft\Microsoft Games\Halo CE`, checks it and derives a string with CryptoAPI; when it is missing, Halo shows string `0xa0`, "Your product key is invalid. We recommend removing and reinstalling the game.", as a **fatal** error and exits. The value is written by Halo's installer when the player types a product key. HaloPad's reference registry deliberately has none (`config/runtime/registry-machine.txt`), and the loop's rules forbid writing product IDs, so without Chris's key entered through the original installer the core will stop there, right after the license, with that dialog.

**What start-up concludes on HaloPad** (`tests/halo_startup_test.c`, Halo's own code in `WinMain`'s order, the C runtime started as the entry point starts it):

| Check | Halo's code | HaloPad's answer | Result |
|---|---|---|---|
| Memory, CPU speed | `0x580e70`: `GlobalMemoryStatus` (capped at 1 GB), `rdtsc` over a quarter second of `QueryPerformanceCounter` | 1024 MB; 2400 MHz (HaloPad's time-stamp counter runs at the reference machine's 2.4 GHz) | above the minimums 128 MB and 733 MHz, and above the 1000 MHz and 128 MB Halo requires for its normal defaults |
| Video memory | `0x580e70`: DirectDraw 7 (`DirectDrawEnumerateExA`, `DirectDrawCreateEx`, `GetAvailableVidMem` for four surface kinds) | one device, "Primary Display Driver", 128 MB | stored as the card's memory |
| Direct3D 9 and `config.txt` | `0x580a00`: `GetAdapterIdentifier`, `GetDeviceCaps`, the game's `config.txt` card database | Radeon 9700 PRO (ATI, `0x1002`/`0x4E44`) | accepted: "ATI" "Radeon 9700 PRO", 128 MB, no error text |
| DirectSound, DirectInput, `shfolder.dll` | `LoadLibraryA` + `GetProcAddress` | provided | pass |
| Unclean last exit | `0x581e40` (HKCU `ExitFlag`) | none | pass |
| Temporary folder space | `GetDiskFreeSpaceExA` | the host volume's free space | above 100 MB |
| Product ID | `0x5829e0` | none (HaloPad never writes one) | **fatal: "Your product key is invalid"** |

No dialog appears before the product-ID check, so the key is the only thing between the license and the rest of start-up. The test passes on macOS and on the iPad Simulator.

**DirectDraw 7** (`halopad_ddraw.c`): only what `0x580e70` asks. One device (the primary, NULL GUID); `IDirectDraw7` with `QueryInterface`, `AddRef`, `Release`, `SetCooperativeLevel(DDSCL_NORMAL)` and `GetAvailableVidMem` (128 MB of local video memory, the card class `GetAvailableTextureMem` also reports). Every drawing method (surfaces, clippers, palettes, modes) stops with its name. `GetLastActivePopup` was added for the C runtime's message box, which reports runtime errors such as R6002.

## GDI text (Keystone's glyph cache)

Keystone draws every character it shows through GDI and copies the coverage into Direct3D textures (`0x1021a7b5`). The sequence is: a memory DC in `MM_TEXT`, white text on black, `CreateFontA(-MulDiv(points, 96, 72), …, ANTIALIASED_QUALITY, VARIABLE_PITCH, face)`, `GetTextMetricsA` and `GetTextExtentPoint32W(L"?")` for the cell, and a 32-bit top-down DIB section. Each glyph is then drawn with `ExtTextOutW(ETO_OPAQUE)` and read back into A4R4G4B4 texels, which go to a system-memory texture and then to the GPU with `UpdateTexture`. HaloPad implements this in `port/runtime/halopad_gdi.c` on CoreText.

- **Fonts on the reference machine.** The machine has Windows XP SP3's fonts. A face it lacks goes through GDI's mapper: for a variable-pitch or default-pitch request with no particular family and an ANSI or default character set, that is Arial. Keystone's `.ksml` files ask for "Arial Narrow", an Office font that XP does not install, so on XP they render in Arial, and so they do here. Faces are drawn with the host's copy of the same font (`ArialMT` and its bold and italic faces, present on macOS and iOS).
- **Metrics as GDI reports them, from the font's own tables:**
  - the em height is `-lfHeight`. For a positive height, it is the largest size whose `VDMX` cell fits;
  - ascent and descent are the `VDMX` table's hinted values at that size, or else `usWinAscent`/`usWinDescent` scaled and rounded;
  - external leading is `max(0, lineGap - ((winAscent + winDescent) - (ascender - descender)))`;
  - `tmAveCharWidth` is `xAvgCharWidth` scaled, and `tmMaxCharWidth` is `advanceWidthMax` scaled;
  - advances come from `hdmx` (GDI's hinted widths) at that size, or else are the linear advance rounded.
  - Checked against Windows' own values, as Wine's gdi32 tests record them: Arial 12 gives 12/9/3, Arial −34 gives 39/32/7, and Arial −16 gives 18/15/3.
- **Drawing.** `ExtTextOutW` supports `ETO_OPAQUE` and `ETO_CLIPPED`, fills the text cell in `OPAQUE` background mode, honors the `TA_*` alignments and `lpDx`, and draws underline and strikeout. Glyphs go through CoreGraphics' grayscale antialiasing at GDI's integer advances, with no subpixel positioning and no smoothing. A DIB section's fourth byte stays 0, as GDI leaves it. Glyph outlines are not hinted, so a pixel's coverage can differ slightly from GDI's; positions and cell sizes follow GDI.
- **Objects.** `CreateDIBSection` supports 32-bit `BI_RGB` (or 8-8-8 `BI_BITFIELDS`), with top-down bits from `VirtualAlloc`. A new DC has black text on white, `OPAQUE` mode, `TA_TOP | TA_LEFT` and the System font; the System font itself is not provided, and drawing with it stops. Deleting a selected font succeeds and takes effect when the font is released. `DeleteDC` is supported, and so is `MulDiv`, with kernel32's rounding.
- **Stops:** faces XP has that are not provided yet, other character sets, escapement, explicit widths, `NONANTIALIASED_QUALITY`, bottom-up DIBs, other depths, text on window DCs, palette colors, `TA_UPDATECP`, and right-to-left reading.
- **Direct3D pieces Keystone needed:**
  - `UpdateTexture`: system memory to the default pool, level-granular dirty tracking. Default-pool textures now keep a copy of their contents for upload, and they still refuse `LockRect`.
  - `d3d9.dll!DebugSetMute`, which D3DX looks up.
  - The SDK debug runtime `d3d9d.dll` is absent.
- **Tests:**
  - the GDI test gained 30 checks: metrics, extents, a `?` drawn into a DIB section, and deferred deletion;
  - the Keystone test checks Keystone's CRT `floor`/`ceil`;
  - the UI test adds a chat line through Halo's own `0x4ae8a0` and opens the chat input as `0x4ada50` does. It then reads the Metal back buffer (`chat.ppm` in the evidence directory) and checks the text pixels.

## Apple hosts: macOS and iOS

The Metal core behind `IDirect3DDevice9` (`port/apple/halopad_metal.m`: back buffer, clears, draws, present, test readback) is shared. The platform host supplies the application, windows, events, the pointer, the clipboard and the display size (`port/apple/halopad_host.h`):

- **macOS** (`halopad_host_macos.m`): AppKit windows with a `CAMetalLayer`, and AppKit events turned into Windows input (keys with their characters, mouse, wheel, activation, close).
- **iOS and iPadOS** (`halopad_host_ios.m`): each Windows top-level window is a `CAMetalLayer`. An app shell shows it with `halopad_host_attach_view`; until then the layer is off screen. Direct3D still renders into its back buffer, which tests read, and `Present` shows nothing, as for a window nobody can see.
  - Input reaches the Windows side through `halopad_input_event` from the shell's UIKit callbacks.
  - The exclusive mouse and cursor visibility are recorded for the shell to apply with UIKit's pointer lock and hiding.
  - The clipboard is `UIPasteboard`, and the desktop is the main screen's native size, in landscape.
- **The iPadOS app shell** (`port/ios/HaloPadApp.m`, built by `scripts/build-ios-app.py`):
  - A UIKit scene app runs the core (`halopad_core_run`: Halo from its PE entry point) on its own 16 MiB-stack thread. It shows each window Halo creates by attaching the window's layer to its view, letterboxed.
  - At Halo's first-run license check, it presents the game's own `Eula.rtf` in a form sheet with **Decline** and **I Accept**, which cannot be dismissed any other way. It returns the player's choice to `EBUEula`, which records an acceptance in `HALOPAD_STATE_ROOT/eula-acceptance.txt` (the SHA-256 of the license, and the time).
  - Automation only launches the app and screenshots it; it never taps.
  - Input: a hardware keyboard (USB HID usages mapped by `halopad_hid_key`), touch as the left button, the iPad pointer (secondary button, hover, scrolling) and scene activation are queued from UIKit's thread with `halopad_host_post_input` and delivered on Halo's thread by `halopad_host_pump`. Positions are mapped through the letterboxed layer to client pixels.
  - Simulator builds are ad-hoc signed and get their data paths from `simctl launch`. The app's state lives in `generated/halopad-disk-ios/`.
  - Evidence: `docs/artifacts/2026-09-27/G3/ios-app-20260927T113055Z/screen.png`, the license screen shown by the native core running on the iPad Simulator.
- **Builds.** `scripts/run-core.py --target arm64-apple-ios17.0-simulator --run-prefix xcrun simctl spawn <device>` builds against the iOS Simulator SDK and links UIKit. Translated modules are compiled with the same SDK. The `HALOPAD_*` variables reach the process as `SIMCTL_CHILD_*`.
- **Evidence (2026-09-27).** On the project's "HaloPad iPad Pro 13" Simulator:
  - all 15 suites pass (the splash suite since 2026-09-27), including the Direct3D 9 suite's Metal readbacks and the chat UI with its frame (`chat.ppm` matches the Mac's);
  - all 16 slices and fault cases pass;
  - the vorbis test reads the fixtures its macOS run leaves in `/tmp`, because iOS cannot start the reference encoder or ffmpeg.
  - The physical-device row (a 4 GiB reservation on a real device, signing) remains parked.

## Game controllers (DirectInput)

Halo reads gamepads through DirectInput 8 (`0x494840`).

- **Halo's side.** It builds an 80-object data format at run time at `0x815400`: 32 axes, 16 hats and 32 buttons, all `DIDFT_OPTIONAL`, 224 bytes. It then enumerates `DI8DEVCLASS_GAMECTRL` with its callback `0x494b30`. For each device the callback runs `CreateDevice`, `SetCooperativeLevel(EXCLUSIVE|FOREGROUND)`, `SetDataFormat`, `GetCapabilities`, and `EnumObjects`, whose callback `0x494a10` sets `DIPROP_RANGE` to −4096…4096 and `DIPROP_DEADZONE` to 1000 on every axis.
- **What HaloPad offers.** Every controller the host has is presented as Windows XP presents an Xbox 360 controller to DirectInput: "Controller (XBOX 360 For Windows)", VID 045E, PID 028E, a HID game pad. The host is GameController's extended gamepads on macOS and iOS (`port/apple/halopad_gamepad.m`).
- **Objects:**
  - X/Y are the left stick and Rx/Ry the right stick;
  - Z is both triggers, with the left toward the maximum and the right toward the minimum;
  - buttons 0–9 are A, B, X, Y, LB, RB, Back, Start, and the left and right stick;
  - one hat switch reports hundredths of a degree, 0xFFFFFFFF when centred.
- **Data formats** match object by object as DirectInput does: type class, instance or any, GUID or any, and optional entries. Unmatched hat entries read centred, and other unmatched entries read 0.
- **Axes** honour `DIPROP_RANGE` (default 0…65535), `DIPROP_DEADZONE` and `DIPROP_SATURATION`, by device, by offset or by ID. Values inside the dead zone read as centre, and the rest are rescaled.
- **Polling.** It is a polled device (`DIDC_POLLEDDEVICE`): `Poll` takes the snapshot that `GetDeviceState` reports.
- **Unplugging.** When a controller goes away, `Poll` reports `DIERR_INPUTLOST`, then `DIERR_NOTACQUIRED`, and `Acquire` reports `DIERR_UNPLUGGED`.
- **Test** (`tests/halo_dinput_test.c`, 22 new checks). Halo's own `0x494840` and callbacks set up an injected controller. The checks cover:
  - the device count, the name Halo keeps (UTF-16), and its 5 axes, 10 buttons and 1 hat;
  - the range and dead zone Halo set;
  - the state through Halo's format: full deflection, dead zone, half deflection rescaled to 1820, triggers, hats and buttons;
  - unplugging.
## Diagnostics

These are environment switches that only print; none of them changes behavior.

- `HALOPAD_TRACE_FILES=1`: file opens, reads, writes and completion routines.
- `HALOPAD_TRACE_BSTR=1`: every BSTR allocated.
- `HALOPAD_WATCH=va[,va…]`: every indirect transfer to those addresses, with the return address, arguments, `eax` and `esp`.
- `HALOPAD_WATCH_RANGE=to_lo:to_hi:from_lo:from_hi`: indirect calls from one module into another, for example Keystone into msxml4.
- `HALOPAD_TRACE_LAST=1`: the last 32 indirect transfer targets, printed with any trap or fault. Translated code has no program counter, so this shows where it was.
- `HALOPAD_TRACE_NET=1`: Winsock binds, datagrams sent and received (address, port, size, first 64 bytes).
- `HALOPAD_TRACE_DRAWS=first:last`: every Direct3D draw, clear, render-target change and `StretchRect` in those frames (counted by `Present`, from 1). Each line gives the primitive, shaders, blend, depth, alpha test, textures and viewport, depth range, bias, stencil, fog, and the guest return addresses on the stack. Up to four vertices are printed for small `*UP` draws. `HALOPAD_TRACE_DRAWS_VSCONSTS=1` adds vertex shader constants c0–c11, and `HALOPAD_TRACE_DRAWS_CONSTS=1` pixel shader constants c0–c7.
- A missing import or COM method now prints the guest stack, so its caller and arguments are visible.


## x87 additions

The backend now implements `fldpi`, `fldl2e`, `fpatan`, `frndint` (by the control word's rounding field), `fscale`, `f2xm1`, `fsincos` and `fxam` (`port/llasm-support/llasm_float.c`). SR's x87 model has no exception flags and no tag word: the status word is the condition codes plus TOP. So `fnclex` and `ffree` change no modelled state, `fnstsw` never reports a pending exception, and `fxam` never reports an empty register. Code that depends on those would diverge. Where a function might, the oracle can check it.

- **Double results (fix, 2026-09-27).** `fst`/`fstp qword` return their double through the per-thread guest slot (`halopad_x87_result`). The first version of that change copied the 64-bit integer scratch field instead of ST0, so every double stored to memory read back as 0. Keystone's CRT `floor`/`ceil` (`fstp qword`, then `fld qword`) exposed it. It is fixed and covered by the Keystone test.

## Resources

`FindResourceA/ExA`, `LoadResource`, `LockResource`, `SizeofResource` and `LoadStringA` walk the PE resource directory in guest memory. For `LANG_NEUTRAL` they try neutral, en-US, English with neutral sublanguage, then the first language present. `strings.dll` (localized strings, dialogs and bitmaps) is mapped read-only at its preferred base `0x3f800000` from the game directory. Its `DllMain` is not run: that only initializes the DLL's own static C runtime, the DLL exports nothing, and its code has no dispatch entries.

## Registry

A persistent registry (`port/runtime/halopad_registry.c`) holds case-insensitive keys, Windows return codes and `ERROR_MORE_DATA` sizing, and stores values exactly as written. It is seeded from `config/runtime/registry-machine.txt`: only `HKLM\Software\Microsoft\Direct3D`, which every XP machine with DirectX 9 has. Halo's install keys, including the product-key-derived `PID`, are **never** created by HaloPad. Their absence behaves as on a machine where setup did not run with a key. `run-core.py` starts every run from the seed and keeps the final state as evidence.

## Kernel objects

Mutexes (owner and recursion, `ERROR_NOT_OWNER`), events (manual or auto-reset), timed and untimed waits, and named objects (`ERROR_ALREADY_EXISTS`) run on host synchronization with one lock and condition variable. Handles are `0x1000 + 4·i`; file handles sit below `0x1000`. Alertable waits work like plain ones until APC delivery exists, because nothing queues APCs yet.

## Page protection

Protection is tracked per 4 KiB guest page. `VirtualProtect` returns the previous protection of the first page, and `VirtualQuery` reports runs of equal protection. Image pages carry Windows' protections (`.text` `PAGE_EXECUTE_READ`, headers and `.rdata` `PAGE_READONLY`). On the host, restricting access rounds inward to 16 KiB pages. So `WinMain`'s one-page `PAGE_NOACCESS` stack sentinel (`0x2fe000`) is recorded but not enforced, and a stack smash that reaches it would go undetected instead of faulting.


## Callbacks and windows

Runtime services call translated Halo code (window procedures) through `halopad_call_guest`, which checks Windows' stdcall callback convention. Windows are host-side records for now: classes, the desktop (the Mac's main display), and `CreateWindowExA` with the documented creation messages (`WM_GETMINMAXINFO`, `WM_NCCREATE`, `WM_NCCALCSIZE`, `WM_CREATE`). `DefWindowProcA` handles those messages and traps on any other message number. Frame metrics are XP classic (caption 19, sizing frame 4). Host windows are described under "Apple hosts" above.

## Paths

Relative guest paths and paths under the install directory map into the game directory, with `\` becoming `/`. Any other absolute path is refused.

## First-run license

`EBUEula` replaces the game's `Eula.dll`. It accepts only when the player has accepted this exact `Eula.rtf` (SHA-256) with `scripts/accept-eula.sh` at an interactive terminal, then writes REG_DWORD `FIRSTRUN=1` under `HKCU\Software\Microsoft\Microsoft Games\Halo CE` as the real DLL does. Otherwise it declines and Halo exits with code 1. HaloPad never accepts on the player's behalf.

The macOS runner has no screen to ask on, so it declines unless `scripts/accept-eula.sh` has recorded the player's acceptance. In the iPadOS app the player chooses on the license screen (see "Apple hosts"). Either record must name the exact license file's SHA-256.
