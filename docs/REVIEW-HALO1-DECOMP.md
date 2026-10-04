# Review: bnunu/halo-1 and the halo-ce-universal ports (2026-09-27)

> **Superseded for the Xbox engine (2026-09-30).** Chris chose to offer the
> halo-ce-universal port as a second engine, strictly as a personal build: it
> is fetched and built on the player's own Mac and never committed or
> published. The port no longer needs the Xbox SDK or the PAL pre-release data
> (any retail disc works). See [XBOX-ENGINE.md](XBOX-ENGINE.md). The provenance
> notes below still apply, and the PC engine still uses nothing from it.

Chris asked whether [bnunu/halo-1](https://github.com/bnunu/halo-1) can advance HaloPad. It can't replace the HaloPad route, but it is useful as a reference for the engine code the two builds share. Both repositories are pinned read-only under the ignored `ref/decomp/` and nothing from them is linked into HaloPad.

| Repository | Pinned commit | What it is |
|---|---|---|
| `bnunu/halo-1` | `8036fb82430fb0969a773abdf0f456dc3d669a38 (first review: f88c8919)` | Fork of `punpckhdq/halo`: a C decompilation (about 410,000 lines, 477 `.c` files) that aims to byte-match `cachebeta.exe`, **Xbox build 2342** (a PAL pre-release debug build). CC0 license text, but the code reconstructs Bungie/Microsoft code. |
| `bnunu/halo-ce-universal` | `8b4c73aa91de6e6032c762541e181f6e5d5101c3 (first review: 522dcf6f)` | That decompilation ported natively to Linux, Windows and Android (SDL3, OpenGL/GLES, POSIX sockets), plus 128-player system link. |

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

## Second look (2026-09-27, later)

Chris posted the repository again. Both pins were moved to the current heads: `bnunu/halo-1` `8036fb82430fb0969a773abdf0f456dc3d669a38` (8 more matching commits: collision, HUD, bitmaps) and `bnunu/halo-ce-universal` `8b4c73aa91de6e6032c762541e181f6e5d5101c3` (2 more commits, system-link bot tooling). The upstream port `cybersecurity/halo-ce-universal` was also read at `5d1ee75` (a Linux/Android system-link join fix). The verdict is unchanged. New findings:

- **Custom Edition support still does not exist.** Neither repository has a commit that loads Custom Edition maps or speaks the PC protocol.
- **The native ports cannot talk to the Xbox game either.** Their multiplayer is the Xbox's lockstep system link (the host sends every player's input to every machine each tick), with enlarged messages under "protocol version 2" (`port/linux/include/halo_port_limits.h`). A Custom Edition 1.10 server speaks a different, server-authoritative protocol behind a GameSpy lobby.
- **Measured overlap with our binary.** 1,223 of the decompilation's 5,602 string literals (12+ characters) occur in `haloce.exe`, most of them script function names HaloPad already reads from the game's own tables. By area: networking 37 of 830, rasterizer 0 of 1,440, interface 4 of 1,996, saved games 0 of 404. The retail PC build keeps no assertion text, so a name map built from strings would be thin; matching would still have to be structural.
- **Provenance.** The repository's own documentation says some bodies were reconstructed with the help of files described as original Bungie source (`docs/user_source_reconstruction_map_20260906.md`: `network_server_manager.c`, `network_client_manager.c`, `random_math.c` from a "haloleak2024" folder), a leaked Halo CEA source tree ("halocea full blobs") and CEA beta debug symbols. The CC0 notice cannot license that material. This is a further reason to keep its code, headers and names out of HaloPad's tree, runtime and generated translation.
- **Inputs that cannot be bought.** The ports need the August 2001 Xbox SDK headers and the PAL data of the pre-release build 01.01.14.2342 ("the game rejects cache files from any other build"). Neither is sold, so other players could not legitimately run a port built from it. That fails the same test that ruled out other editions.
- **Nothing here unblocks HaloPad's parked items.** The license choice, the product key, a physical device and a second player are still what the next goals wait on.

The reference-only use described above stands, restricted to reading, in the ignored `ref/decomp/` checkout.
