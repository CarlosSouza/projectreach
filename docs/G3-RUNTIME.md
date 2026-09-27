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
