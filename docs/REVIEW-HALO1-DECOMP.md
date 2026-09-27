# Review: bnunu/halo-1 and the halo-ce-universal ports (2026-09-27)

Chris asked whether [bnunu/halo-1](https://github.com/bnunu/halo-1) can advance HaloPad. It can't replace the HaloPad route, but it is useful as a reference for the engine code the two builds share. Both repositories are pinned read-only under the ignored `ref/decomp/` and nothing from them is linked into HaloPad.

| Repository | Pinned commit | What it is |
|---|---|---|
| `bnunu/halo-1` | `f88c89192b67827ce6e44a5acd48322b39473c14` | Fork of `punpckhdq/halo`: a C decompilation (about 410,000 lines, 477 `.c` files) that aims to byte-match `cachebeta.exe`, **Xbox build 2342** (a PAL pre-release debug build). CC0 license text, but the code reconstructs Bungie/Microsoft code. |
| `bnunu/halo-ce-universal` | `522dcf6f3d35db73627a0ef38ddbd8f10313d16e` | That decompilation ported natively to Linux, Windows and Android (SDL3, OpenGL/GLES, POSIX sockets), plus 128-player system link. |

## What it needs and does

- **Build inputs:** the August 2001 **Xbox SDK** (its headers are required even for the native ports) and the **PAL Xbox game data** of build 01.01.14.2342.
- **Custom Edition:** it does not use `haloce.exe` or the Custom Edition maps. The README says Custom Edition map compatibility is "currently adding"; no commit implements it.
- **Networking:** Xbox system link reimplemented over UDP sockets (`port/linux/src/xnet.c`: "XNet's secure addressing collapses to plain IPv4 ... enough for system link play on a LAN"). It plays only against other copies of the port. It has no GameSpy lobby, no CD-key handshake and none of the PC's changes to the Halo PC 1.10 network protocol, so it cannot join an existing Custom Edition server.
- **Apple platforms:** there is no macOS or iOS port. The Android port runs the game as a 32-bit `arm64_32` guest inside a host loader, which iOS does not allow for apps.
- **PC-only systems** absent from the source: GameSpy, Keystone (the multiplayer chat), the Direct3D 9 rasterizer, the PC input and sound layers, and the CD-key code. These are exactly what HaloPad's current work touches.

## Fit against HaloPad's requirements

- **Joining existing Custom Edition 1.10 servers** is the reason for the project (PRD; network discipline in the goal loop). This repository cannot do it.
- **Inputs:** the loop's data discipline says "Do not quietly introduce a Digsite executable, PAL debug build, original Xbox SDK, Mac binary or MCC data because another repo expects it." Building from this repository would bring in exactly the PAL debug build and the Xbox SDK.
- **Mapping to our binary:** the PC release `haloce.exe` keeps no source file names or assertion text, so decompiled functions cannot be matched to it by string. Matching would have to be structural, function by function.

## How HaloPad uses it

- As an engine reference when reading translated code: names and layouts for tag structures, the game engine (CTF, slayer and so on), physics, AI, the scripting library and the network message definitions that predate the PC port. These help to name a function or a structure the core reaches, and to write better diagnostics.
- Not as code: nothing is copied into HaloPad's tree, runtime or generated translation. The behavior HaloPad runs is still the translated original executable, checked against the oracle.
- The ports are a useful proof that the engine runs well on ARM phones (a Pixel 9 Pro XL at 190–220 fps). Their SDL/OpenGL layers solve a different problem (an Xbox SDK surface) from HaloPad's Win32/Direct3D 9 one.
