# Xbox engine (second HaloPad engine)

Status, 2026-09-30: **Mac proof passes to the main menu and a campaign level.** iPad, the picker and
controls are next. Nothing here is a playable iPhone or iPad build yet.

HaloPad will offer two games at launch:

| | Halo PC | Halo Xbox |
|---|---|---|
| Engine | Custom Edition 1.10, translated from the player's own `haloce.exe` | [cybersecurity/halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal), a port of the Xbox decompilation |
| Game files | Custom Edition package | The player's own Xbox disc image |
| Online | Custom Edition servers | Other copies of that port (system link, invites) |
| Campaign | Not yet | Yes, plus split-screen |

The two cannot play online with each other.

## Personal-build boundary (Chris's decision, 2026-09-30)

- The upstream engine is **fetched and built on the player's own Mac** at the revision in
  [config/xbox-engine.lock.json](../config/xbox-engine.lock.json). Its sources, its guest image and
  the translation live only under the ignored `ref/xbox-build/`.
- HaloPad's repository holds only HaloPad's own code: the translator, the host runtime and scripts.
- No IPA or other build containing the engine is published.
- Updates stay **pinned**: moving the pin is a deliberate step (upstream's saved games are memory
  snapshots and do not survive engine changes, so saves are backed up first).
- Upstream's documentation says parts of the decompilation were reconstructed with help from leaked
  Bungie material ([REVIEW-HALO1-DECOMP.md](REVIEW-HALO1-DECOMP.md)). That is why the engine stays a
  personal build, and why this document is not a rights clearance.

## How it works

Upstream's Android build compiles the game as **arm64_32** (AArch64 instructions, 32-bit pointers,
because the game's data files hold 32-bit pointers) and links it into one static image at guest
address `0x88000000`, with the Xbox memory window at `0x80000000`. Apple platforms reserve the low
4 GiB of every process, so that image cannot run where it was linked. HaloPad therefore:

1. **Builds upstream's own Android guest** with two extra compiler flags
   ([scripts/xbox/guest-cc.sh](../scripts/xbox/guest-cc.sh)): x27 and x28 are reserved, and there are no
   jump tables.
2. **Translates the linked image** ([scripts/xbox/translate.py](../scripts/xbox/translate.py)): every
   instruction becomes ordinary ARM64. Guest memory is one **4 GiB-aligned** host reservation whose base
   is held in x28, so guest address *g* is at x28 + *g*, and the low 32 bits of any host address inside it
   are the guest address. Loads and stores add the base (`add x27, x28, wN, uxtw`); adrp/adr
   become constants; direct branches go to translated labels; blr/br go through one dispatch table (one
   entry per guest instruction); calls to upstream's import stubs become direct calls into the host.
   The stack pointer is a real host address inside guest memory, and any copy of it into a register
   is cut back to 32 bits. Nothing is compiled at run time: no JIT.
3. **Runs it on a Darwin host** ([port/xbox](../port/xbox)): guest memory and the game's mmap,
   Linux system calls converted to Darwin (flags, structures, errno, futexes on
   `os_sync_wait_on_address`), threads with stacks inside guest memory, SDL3 for the window, input
   and sound, and OpenGL ES wrappers generated from upstream's own list
   ([scripts/xbox/gen-host-gl.py](../scripts/xbox/gen-host-gl.py)). The game's file and socket helpers are
   upstream's `port/linux/src/posix_*.c`, compiled for the host from the pinned checkout.

Apple devices use 16 KiB pages and the game 4 KiB ones: inside the Xbox window and the image the
game's own mapping calls are emulated, and Direct3D write tracking protects whole 16 KiB pages.

## Build and run the Mac proof

```sh
scripts/xbox/extract-maps.py "ref/Halo - Combat Evolved (USA).xiso.iso" ref/xbox-build/data
scripts/xbox/build-mac.sh
cd ref/xbox-build && ./out/halopad-xbox --image out/halo_guest.elf --data $PWD/data --angle <dir>
```

Needs Homebrew `llvm`, `lld`, `ninja` and `sdl3`. `build-mac.sh` makes a case-sensitive disk image for
the checkout (upstream has a header that includes itself on a case-insensitive disk). On the Mac,
OpenGL ES comes from ANGLE; any Chromium/Electron app's `libEGL.dylib` and `libGLESv2.dylib` work for a
local test (`--angle`), and the Metal back end is selected. `XG_FRAME_DUMP=<file.ppm>` saves the game's
own frames. `init.txt` in the data folder holds console commands, for example
`map_name levels\a10\a10`.

## Evidence

| Check | Result |
|---|---|
| Disc | `Halo - Combat Evolved (USA).xiso.iso`: all 24 maps are build `01.10.12.2276` (NTSC, upstream's reference speed); `default.xbe` SHA-256 `ed3a8e96…e3a3ac`; 1.7 GB of maps |
| Upstream pin | `b47f237d` (2026-09-30) |
| Guest image | 7.0 MB ELF; .text 2.6 MB, 657,553 instructions, no use of x27/x28, 191 import stubs |
| Translation | Assembles for arm64-apple-macos in about 4 s; links against the host with every import resolved |
| Start-up | Settings, OpenGL ES 3.0 on ANGLE's Metal renderer (M3 Max), audio, network start-up |
| Main menu | Drawn (Campaign / Multiplayer / Settings) |
| Campaign | `map_name levels\a10\a10` loads The Pillar of Autumn and plays its opening cinematic |

## Open items

- **Presentation:** in fullscreen the game draws 640 x 480 in a corner of the window; the window,
  the drawable and the game's screen size need to agree.
- **Input and play:** controller and keyboard play, a full campaign mission and a multiplayer match are
  not yet checked on the Mac.
- **Privacy defaults:** the game starts internet hosting and asks public STUN servers for its address
  at start-up. HaloPad keeps it off the clipboard; internet play should be opt-in in the app.
- **UPnP and Discord:** stubbed.
- **iPad:** a UIKit host (iOS has Apple's OpenGL ES 3.0, so no ANGLE), guest memory under the
  Extended Virtual Addressing entitlement HaloPad already uses, the PC/Xbox picker, disc import,
  separate saves and the touch overlay.
- **Update routine:** a script that moves the pin, rebuilds, runs the checks and backs up saves.
