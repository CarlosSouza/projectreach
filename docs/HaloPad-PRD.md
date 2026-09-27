# HaloPad PRD: Halo: Combat Evolved, native on Apple platforms

Status: **GO for gated private autonomous engineering; feasibility not yet demonstrated.** Written 6 September 2026.

Audience: an autonomous agentic system with control of a macOS Apple Silicon development machine and operator-authorized access to a Windows reference environment.

Companions: [GOAL-LOOP.md](GOAL-LOOP.md), the operating procedure, and [INPUTS-AND-RESEARCH.md](INPUTS-AND-RESEARCH.md), the acquisition and source record. Read all three before implementation. Source labels S01–S20 refer to that record.

**Decision:** begin a bounded native-client program, not an unbounded “finish a decomp” project. Continue automatically through the implementation goals when their evidence passes. Do not treat this document as evidence that Halo already works, as a completion guarantee, or as authorization to publish. A full-build commitment is earned at G5. Public release remains NO-GO until G12.

---

## 1. Objective

Build **HaloPad** (`halopad`, working name): a native ARM64 implementation of the original Halo: Combat Evolved experience for **macOS, iPadOS, and iOS**, using ahead-of-time translation of supported user-supplied Windows game executables plus a narrowly scoped native runtime. The product must play on **existing PC community servers with established PC clients**. It must not require a separate mobile player pool.

### 1.1 Selected editions

**Online-first profile:** English **Halo Custom Edition 1.0.10.0621**, input `haloce.exe`. This is the first CPU, runtime, graphics and multiplayer target.

**Full-campaign profile:** English **2003 Halo: Combat Evolved for Windows, updated to 1.0.10.0621**, input `halo.exe`, with the user's original retail campaign data. This is a separate versioned core/profile sharing the platform runtime. It preserves the original campaign objective without assuming that Custom Edition includes it or that two game formats are interchangeable. [S14–S17]

The default plan uses two explicit build profiles, not two independent platform rewrites. Each profile has its own executable identity, generated code namespace, initialized guest-memory image, resource root, saves, original-client oracle and compatibility ledger. Only one profile runs at a time. Shared fixes are regressed against both after the second exists.

A proven adaptation that safely uses one core for both profiles may be proposed later, but it must pass the same campaign, resource and network tests. It is not assumed here. **Do not silently replace the retail campaign with fan conversions, SPV3, Anniversary, or an Xbox build.**

### 1.2 Delivery order

1. Preserve inputs; pin sources; record rights and operator authorization; reproduce a normal Custom Edition PC session.
2. Compare two bounded static-translation candidates on the exact selected Windows executable. Prove CPU semantics, pointer translation and a signed physical-iOS architecture capsule before major UI work.
3. Bring up the Custom Edition core in a native ARM64 macOS process with real runtime behavior, rendering, audio and controllable gameplay.
4. Complete a controlled match with an original PC client through a compatible, unmodified dedicated server. Then complete an accepted existing-community session.
5. Extend the same tooling/runtime to the separately identified retail profile and complete the original campaign, checkpoints and fresh-save progression.
6. Run the same cores on iPad and iPhone Simulators, then complete the Apple shell and exact-artifact physical-device acceptance.
7. Produce reproducible local packages; publish only after separate source/binary rights decisions, package audits and explicit final approval.

### 1.3 Hard product boundaries

- **AOT CPU execution:** game machine code is translated and compiled on the development Mac before app signing. No runtime x86/PPC JIT, general CPU interpreter, Rosetta, Wine guest process, virtual machine or streaming service in the delivered Apple app.
- Modeling guest registers or a guest address space is permitted. Dispatching to already compiled functions is permitted. Executing HSC game-script bytecode as game data is permitted if its semantics are preserved. These do not license a general fallback interpreter for missing CPU instructions.
- Metal's normal GPU shader/pipeline compilation is distinct from generating executable ARM64 CPU code. Audit both paths, but do not confuse a GPU pipeline with a CPU JIT.
- **Same game and network behavior:** do not replace physics, movement, damage, vehicles, AI, campaign progression or packet semantics with plausible approximations just to reach a screen.
- **Existing people:** a responding server or two copies of HaloPad communicating is insufficient. An original desktop client must share actual play with the native client.
- **Complete product:** the first online match is a feasibility milestone, not permission to omit campaign, saves, lifecycle, touch usability, or real devices.

## 2. What “done” means

Every item requires the evidence contract in Section 11.

| ID | Requirement | Acceptance |
|---|---|---|
| D1 | Reproducible authorized inputs | Exact profile hashes, source pins, private-rights state, original-client baseline and deterministic data preparation recorded; originals unchanged |
| D2 | Native execution model | Selected lifter translates actual Halo routines; differential tests pass; thread/callback/pointer model runs in an ARM64 Mac and signed physical-iOS capsule without runtime CPU code generation |
| D3 | Native Mac core | Actual Halo initialization and guest/host boundary work in an ARM64 process, with no hidden original x86 execution or required Win32 guest process |
| D4 | Local playable Halo | Stock Custom Edition map loads; original player, collision, weapons, grenades, vehicles, death/respawn, HUD, menus and audio work |
| D5 | Controlled PC interoperability | Native client and original Windows client complete a match through an unchanged compatible PC dedicated server, including objective/score, map transition and reconnect |
| D6 | Existing-community play | An accepted public/community server session with an established desktop player is documented; discovery/direct-connect and edition/mod limits are honest |
| D7 | Complete original campaign | Retail profile completes all ten original levels from a fresh save through ending/credits, with correct checkpoints, reload, objectives, AI, vehicles and audio |
| D8 | Simulator cores | iPad first, then iPhone Simulator runs the same AOT cores through multiplayer and campaign first-play tests; no device performance inference |
| D9 | Apple product surface | Usable touch/controller/pointer input, three-dot menu, data import, profiles, settings, saves, diagnostics and lifecycle behavior on both screen classes |
| D10 | Technical correctness and reproducibility | Applicable matrix M01–M33 green, clean-checkout pipeline passes, timing/rendering/audio and soak requirements met |
| D11 | Exact physical candidate | M34–M35 accepted on physical iPad and iPhone, signed candidate identities recorded, no unverified hardware claims |
| D12 | Release authorization | M36 green; code/data boundary and notices audited; source and binary decisions explicit; Chris authorizes the exact public artifact |

**Not done:** a translated function count; a clean compile; a map viewer; empty Blood Gulch; a title screen; ping; a server-list screenshot; a session between two native test harnesses; an x86 Windows build; or a Simulator-only video.

### Non-goals for the baseline

App Store/TestFlight submission, a new matchmaking backend, console/MCC cross-play, Halo 2, online campaign co-op, splitscreen, multiplayer server hosting on phones, anti-cheat evasion, arbitrary executable mods, OpenSauce/HAC2/SPV3 parity, unlocked simulation speed, remastered assets, AI bots added to multiplayer, and automatic full-game acquisition inside the app.

Stock multiplayer and the original retail single-player campaign are baseline. Selected data-only custom maps may be accepted after stock compatibility. Full compatibility with every community modification is not promised. Campaign co-op is not implied by “multiplayer.”

## 3. Why proceed — and what is still unproven

### 3.1 Audited evidence

| Evidence | What it supports | What it does not support |
|---|---|---|
| [SR/SRW/llasm](https://github.com/M-HT/SR) has Windows-game and ARM64/macOS targets | Real precedent for static translation rather than mandatory runtime x86 execution | A Halo backend, a complete Win32 implementation or an existing iOS Halo port |
| SR's pointer-offset implementation [S02] | A concrete reference for preserving 32-bit values in a 64-bit host | Automatic correctness of all Halo memory/ABI paths |
| Ringworld, Chimera and related Halo code [S03, S08, S19] | Version-specific structures and bounded behavior references | A complete independently executable engine |
| Newer Xbox decomp and xboxrecomp source [S06–S07] | More reference material and actual Halo-related compiler/runtime fixes | A completed Apple port or the desired Windows network lineage |
| [UTP](https://github.com/chrissotraidis/utp) | Existing first-person Apple UI, import, lifecycle and testing mechanisms | Halo's missing CPU, renderer and platform implementation |
| Original 1.10 release and operator server pages [S14, S18] | Identifiable versions and real community destinations | A tested native client or reliable current human-player census |

### 3.2 Selected architecture hypothesis

```text
User-authorized Windows installation(s)
    -> exact hashes + PE/asset/ABI inventory
    -> build-time static translation + explicitly mapped native replacements
    -> compiled ARM64 game module(s)
    -> guest-memory / Win32-call / graphics / audio / input interfaces
    -> native macOS + UIKit host and Metal-backed rendering
    -> original-compatible network behavior
    -> existing PC dedicated servers and desktop players
```

The first lifter candidate is **SRW → llasm → LLVM → ARM64**. The bounded alternate is **a PE-specific frontend using suitable xboxrecomp lifting components**, with a HaloPad-owned Windows runtime. Neither is an already configured Halo pipeline. A PE executable must not be passed to the XBE parser and called supported.

The selection depends on G2 results: real instruction coverage, metadata requirements, indirect control flow, 64-bit guest pointers, callbacks, floating-point accuracy, traceability, license fit and generated-code performance. Do not choose by stars, advertised portability, generated line counts or one toy program.

### 3.3 Unproven conditions

No reviewed foundation demonstrates all of: a standalone Halo PC/Custom core, complete compatible networking, native ARM64 game execution, complete Metal-backed rendering, working iOS lifecycle, original campaign completion and an acceptable distribution boundary. AOT translation can preserve original networking behavior only if the surrounding execution and platform semantics are correct.

**G2 and G3 answer whether a finite native client route is emerging. G5 is the first substantial end-to-end feasibility proof.** If they fail with a documented foundational blocker after the unblocking ladder, report NO-GO under current constraints. Do not wait for 100% upstream decompilation by default; do not declare inevitable success merely because all components can be named.

This review was source-level. The numerical feasibility scores from earlier chat are superseded by these pass/fail experiments.

## 4. Environment and workspace

### 4.1 Development environment

Use an Apple Silicon Mac with a stable installed Xcode supporting the chosen iOS deployment target, command-line tools, CMake, Ninja, Python, Git, LLVM tooling, a PE inspection/disassembly tool such as Ghidra, and the selected lifter's documented build requirements. SRW's documented build uses SCons; provision only what the selected path requires. Record actual tool versions and SDK build IDs instead of assuming a future/latest Xcode is necessary.

Start the Apple shell with a provisional **iOS/iPadOS 17+** deployment target as a planning choice; confirm it against the selected shell APIs and SDK during G0. This is not a verified Halo support claim or a statement of UTP's minimum requirement. Raise the target only for an identified essential API. Record the actual supported macOS deployment target during G0. Prefer stable SDKs over unexplained beta-only dependencies.

A Windows reference environment is required for baseline game behavior and original-client/server testing. A user-authorized existing PC or appropriately configured VM may serve as the oracle. Its ability to run x86 software does not count as native Mac feasibility. Do not provision cloud machines, buy software, enable remote access, or change networking/firewalls globally without authorization.

### 4.2 Layout

```text
halopad/
  docs/
    PRD.md, GOAL-LOOP.md, INPUTS-AND-RESEARCH.md
    STATUS.md, JOURNAL.md, HANDOFF.md, RIGHTS-STATUS.md
    INPUTS.md, EXECUTION-MODEL.md, ABI-AND-MEMORY.md
    IMPORTS.md, RENDERER.md, AUDIO.md, NETWORK.md
    CAMPAIGN-AND-SAVES.md, PERF.md, TEST-MATRIX.md
    RELEASE-READINESS.md, artifacts/        # artifacts ignored
  ref/                                    # ignored; read-only references
    utp/, sr/, xboxrecomp/, halo-references/
    inputs/custom-original/, inputs/retail-original/
  config/
    profiles/custom-en-1.0.10.0621.json
    profiles/retail-en-1.0.10.0621.json
  generated/                              # entirely ignored
    prepared/, aot/custom/, aot/retail/, fixtures/, builds/
  port/
    core/, guest/, win32/, graphics/, audio/, network/, apple/
    replacements/, patches/
  scripts/
  tests/
  dependencies.lock.json                  # source/tool pins, no secrets
  INPUTS.lock.json                        # local ignored input manifest
```

Do not rename a known existing project or overwrite local changes. Create a separate integration repository unless Chris selects an existing destination. No public repo creation, branch push, tag, issue upload or release follows automatically from this PRD.

### 4.3 Safety rules

Original media, original installations, reference repos and keys are immutable. Prepare ignored copies. Disable push URLs in reference clones. No `git clean -fdx`, destructive reset, blanket deletion, or killing processes by broad names. Preserve ignored evidence and save fixtures. Never copy keys, original binaries, extracted assets, generated game code, game memory, saves or private logs into tracked files.

Use local-only test traffic until protocol behavior is understood. No probing/fuzzing public servers. No security weakening, key generation, identity forgery or integrity-check bypass to manufacture a passing test.

## 5. Inputs and repositories

### 5.1 Acquisition and validation

Follow the companion acquisition record. **The default required purchase/source is the original Windows PC release with a legitimate product key**, not the Mac release, Xbox disc or MCC. The downloadable English Custom Edition distribution supplies the first multiplayer installation, but not an automatic entitlement or complete original campaign.

G1 must:

1. Record the original installation/media provenance and operator permission locally, without publishing secrets.
2. Install/prepare the exact Custom Edition profile and apply its edition-specific 1.10 update in the reference environment.
3. Preserve before/after binary identity, patch identity, versions, sizes and SHA-256. The supported Custom `haloce.exe` hash is **not known in this PRD**; establish it from the actual accepted installation.
4. Inventory PE headers, code/data/resources, imports and delay imports, relocations, TLS, entry points, callbacks, exception behavior, executable-memory writes, multimedia modules and sockets.
5. Inventory every baseline asset and resource file by relative path, size and hash. Reject accidental mixed installations.
6. Demonstrate original-client boot, local Blood Gulch gameplay, a compatible dedicated-server session and one currently usable community destination. Capture the baseline behavior and endpoint edition.
7. Register the retail campaign profile when its owned input is available. Missing retail input need not block Custom work through G6, but it blocks D7 and completion.

Do not borrow the `cachebeta` or Digsite hashes from another repository. They identify different executables. Version strings, timestamps and file names are clues, not sufficient identity checks. Do not claim input authenticity solely because two untrusted mirrors agree.

### 5.2 Data preparation versus CPU code

Build-time tools read the user-supplied original PE and produce two distinct products:

- **AOT CPU module:** generated source/IR/object code, compiled and signed into the app before installation; never generated on the phone.
- **Prepared game-data package:** a manifest, required map/resource data and any validated original PE sections needed as inert guest initialization data. Store locally under `generated/prepared/` and import into the app container.

Some original executable bytes may need to remain readable **as data** to preserve initialization or code-reading semantics. That does not permit the app to execute those x86 bytes. Record which sections are required, never mark the guest arena CPU-executable, and never use `dlopen`, a CPU interpreter, executable trampolines or JIT to run imported input.

The baseline iOS importer accepts a prepared **`.halopad.zip`** data package created by the Mac tool. It does not run Windows installers, fetch full retail media or load arbitrary EXE/DLL files. A later direct installation-folder importer may be added only with the same exact-profile validation and inert-data boundary. The setup UI must explain the one-time preparation step rather than imply that any ISO works.

Code, data and profile manifests are tied by identity. Reject data prepared for a different AOT module. New executable support requires a new build, not merely downloading a different binary into the installed app.

### 5.3 Reference graph

| Reference | Starting revision / status | Use |
|---|---|---|
| `chrissotraidis/utp` | `2f9c9b8f815b1f691a39960bfc96c6a9b8ebda14` | Read-only FPS Apple shell, import, diagnostics, physical testing and package discipline |
| `M-HT/SR` | `ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3` | SRW/llasm experiment and pointer-offset reference |
| `sp00nznet/xboxrecomp` | `a781596e2181f8d4b15336659897ce07bdd7d56a` | Alternate lifter/component audit, not the default Windows runtime |
| Ringworld / Chimera / Invader | Resolve and record before use | Version-specific semantics, asset structure and bounded correctness references |
| Xbox decomp / Demon / Anniversary corpus | Reference-only; provenance and version review required | No automatic import of a debug executable, proprietary SDK or unreviewed reconstructed corpus |

Pins identify retrieved source snapshots, not a tested Halo dependency graph. G0 verifies full checkouts and their recursive dependencies. Never fetch a newer branch tip silently during debugging.

### 5.4 Read the machinery, not just screenshots

In UTP, inspect `README.md`, `docs/STATUS.md`, `docs/COMPLETION_AUDIT.md`, `docs/UT99_Apple_PRD.md`, `docs/NETWORK_COMPATIBILITY.md`, `docs/DATA_COMPATIBILITY.md`, `docs/PUBLIC_RELEASE_CHECKLIST.md`, build scripts/Makefile, native input and lifecycle code. Verify actual paths at the pinned revision.

Reuse game-neutral ownership and safety mechanisms. Do not inherit Unreal key bindings, binary layout, FMOD assumptions, network claims or its renderer as Halo implementations. The attached SnapPad documents supply this PRD's operating structure; their N64Recomp, RT64, RSP, overlay and FlashRAM specifics are not Halo dependencies.

## 6. Phase 0: authorization, baseline and the early decision gates

### 6.1 G0/G1 state

Create `RIGHTS-STATUS.md` with `private-engineering-authorized; publication-not-authorized`. This records Chris's direction, not a legal conclusion about every upstream artifact. Inventory licenses and permissions before copying code. A public repository is not automatic authorization for reuse.

Create safety checks before generating proprietary output. Set up the source lock, input-manifest validation, journal and matrix. Run the normal original-client network baseline before attempting to reconstruct it; otherwise a broken original install can masquerade as a port bug.

### 6.2 G2: bounded translator selection

Compare **A: SRW/llasm** and, only if needed, **B: an audited xboxrecomp-derived x86 lifter with a new PE frontend**. Do not start a general Windows emulator or complete Xbox runtime as a prerequisite.

Each candidate must produce a written capability report against the **actual selected Halo PE**, not marketing text:

- all image sections and executable regions classified;
- instruction families and unsupported operations counted by reachable call path;
- indirect calls/jumps, callbacks, function boundaries and embedded data handled explicitly;
- guest pointer strategy and host-call ABI shown with tests;
- at least one nontrivial actual Halo function slice executed as compiled ARM64 and compared with original behavior;
- at least one higher-level slice such as map-header validation, data lookup, bitstream processing or initialization executed on real authorized data;
- generated-code size, compile cost and profile of representative hot loops recorded;
- same chosen execution/memory/callback mechanism demonstrated in an iOS capsule, including a physical device before large-scale graphics/UI commitment.

Start with self-authored synthetic x86 fixtures for tool bring-up. Then move immediately to real Halo functions. Toy fixtures alone do not pass G2. All original reference executions, fixtures and traces stay private. The selection report names the winning candidate, failing cases, exact fixes, license boundary and why the remaining work is finite.

**G2 stop condition:** neither candidate produces trustworthy real Halo native slices and the required Apple memory/callback behavior after the unblocking ladder, or the only usable route violates the no-runtime-CPU-execution constraint. Produce a reproducible NO-GO report. Ordinary fixable instruction omissions are not automatic terminal failures.

### 6.3 Physical architecture capsule

The capsule is a small signed native app using the intended guest-memory layout, static dispatch, threading/callback ABI and at least a real Halo function slice. Test launch without a debugger, relaunch, background/foreground and data import. No undocumented low-address mapping entitlement, debugger-only execution permission or runtime executable allocation may be necessary.

When a physical device or signing action is unavailable, record `BLOCKED_EXTERNAL`, provide the exact install/test steps, and continue independent analysis or native-Mac work. Do not mark the capsule accepted or expand into extensive UI polish on a false premise.

## 7. Phase 1: executable, memory and platform model

### 7.1 PE and code manifest

Create `EXECUTION-MODEL.md` plus a machine-readable manifest for every image and executable region. Record virtual/RVA/file ranges, initialized/uninitialized data, protections, relocation types, imports, thunk tables, TLS callbacks, entry points, handlers, code/data overlaps and the analyzed profile hash.

Every reachable direct and indirect target must resolve to **AOT code or a named semantics-preserving native replacement**. Distinguish function returns, tail calls, jump tables and computed branches. Discover dynamically observed targets using private instrumentation, then update metadata and **regenerate at build time**. Never silently skip an unknown target or return zero to advance execution.

Audit the code paths used by title/menu, loading, campaign AI, multiplayer serialization, vehicles, sound, save/load, disconnect and shutdown. Coverage of one map is not whole-program coverage. If runtime CPU code creation is detected, classify its exact purpose and implement a bounded ahead-of-time alternative only when equivalent behavior can be tested.

### 7.2 Guest address-space contract

Represent game pointers as explicit 32-bit guest values. Convert them through a checked guest-memory API to native host pointers. Do not cast arbitrary 32-bit values directly into `void *`, truncate host pointers, or depend on mapping the original image at its numeric Windows address.

The chosen implementation may use a bounded high-address arena, segmented mapping or another audited relocation scheme. It must account for guest wraparound, null/invalid addresses, alignment, little-endian loads, overlapping copies, pointer arithmetic, pointer tables, allocation, memory protection observations and executable bytes read as data.

**Do not commit 4 GiB of physical memory just to model a 32-bit address space.** Reserve/commit only what the supported layout requires and measure actual resident/dirty memory and iOS address-space behavior. Avoid fixed-address destructive mappings. Respect the platform's actual page granularity. Fail cleanly when reservation is unavailable.

Host objects—files, sockets, mutexes, audio buffers, graphics resources and Objective-C objects—live behind typed handles or marshalled wrappers. They cannot be stored as truncated guest pointers. Callbacks use a finite, build-time dispatch/marshalling scheme, never a newly emitted executable trampoline.

Test pointers inside packed structures, pointer-to-pointer outputs, vtables, COM-like interfaces, function pointers stored as integers, null and one-past-end cases, signed/unsigned boundary arithmetic and data-buffer lifetime across async work. A working `MEM32` macro is not sufficient proof.

### 7.3 CPU/ABI fidelity

Write differential tests for every reached instruction family and calling convention before treating it as supported. Include flags, carry/borrow/overflow, partial registers, shifts/rotates, direction flag, string instructions, signed division, x87 control/status/stack, rounding, NaNs/infinities, SSE lane semantics and conversion behavior actually used by Halo.

Do not equate x87 extended precision with C `double`, nor assume `long double` has x87 behavior on Apple ARM. Determine which precision mode and operations the original uses. Use appropriately validated helper semantics where necessary. Disable fast-math and unintended contraction for the baseline. Record acceptable nondeterminism separately from numerical divergence.

Audit `cdecl`, `stdcall`, `thiscall`, fastcall-like paths and compiler helper conventions actually present. Preserve stack cleanup, implicit registers, guest return addresses when observable, exception unwinding, `setjmp`/`longjmp`, TLS, thread creation, worker shutdown, atomics and synchronization.

Per-thread execution state is required if multiple guest threads execute. A synchronous fake worker can deadlock a real work queue. Do not solve races by deleting work or making all event waits succeed. Check guest/host stack exhaustion and exception crossing explicitly.

### 7.4 Narrow Windows runtime

Create `IMPORTS.md`: original module/function or internal call site, observed use, required semantics, implementation, tests and remaining cases. Implement the actual game's needs, not a generic Wine replacement.

Required families to investigate include files/paths, memory, timing, synchronization, TLS, locale/text, process startup/shutdown, registry-like configuration, sockets/DNS, input, window events and multimedia. Add actual hidden/dynamic imports from traces. Host networking may use native sockets while preserving guest-visible ordering, error values and timeouts.

Configuration moves into a scoped app container; input/game settings persist with explicit profile ownership. Do not emulate an entire system registry or invoke legacy crash-reporting executables. Optional obsolete external integration can return a correct documented absence/failure; gameplay, saves, loading, network validation and rendering cannot be no-op stubs.

### 7.5 Build interfaces

The following are **interfaces the agent must implement**, not claims that upstream already supplies these scripts:

```text
scripts/doctor.sh
scripts/bootstrap-sources.sh
scripts/verify-sources.sh
scripts/inspect-inputs.py
scripts/prepare-inputs.py
scripts/run-reference-baseline.sh
scripts/audit-executable.py
scripts/build-lifter.sh
scripts/generate-core.sh --profile <id>
scripts/test-differential.sh --profile <id>
scripts/build-macos.sh --profile <id>
scripts/build-ios-simulator.sh --device-class ipad|iphone
scripts/build-ios-device.sh
scripts/run-network-matrix.sh --environment controlled|community
scripts/run-campaign-matrix.sh
scripts/collect-diagnostics.sh
scripts/check-repo-safety.sh
scripts/audit-package.sh
scripts/package-local.sh
```

Validate arguments, inspect current state, fail nonzero on unmet prerequisites, quote paths, and store logs/evidence automatically. Source lock + input identity + generator version + patches + SDK/config determine cache identity. Never reuse AOT output from another executable because filenames match.

## 8. Phase 2: native macOS gameplay and real networking

### 8.1 G3/G4 bring-up ladder

Test and capture each rung immediately:

1. ARM64 process and native host start. Confirm architecture using built Mach-O metadata and process evidence; no hidden x86 guest process.
2. Validated prepared data initializes the guest state and the real entry/initialization path.
3. File loads, worker threads, timers, configuration and clean shutdown work without silent fallback calls.
4. A real graphics surface presents the original menu/loading state; input and basic audio work.
5. Stock Blood Gulch loads **in the game**, not a viewer. Original player control and collision work.
6. Weapons, grenades, melee, pickup/reload/swap, zoom, damage, death/respawn and a representative vehicle behave as in the reference.
7. Exit to menu, load another stock map, return, and cleanly quit/relaunch without leaked resources or stale callbacks.

G4 requires controllable gameplay with meaningful audio and no missing-subsystem mock. A temporary diagnostic render or silent-audio mode may help debugging but cannot be the accepted path.

### 8.2 Graphics route

Inventory the actual PC executable's graphics imports/calls and shader paths. Evaluate an **API-level native graphics façade** first: preserve the original game's draw/state decisions and translate the required API subset into Metal-backed operations. Record the actual Direct3D version and interfaces from the selected executable rather than relying solely on DirectX installation requirements.

Do not substitute xboxrecomp's NV2A/Xbox-D3D8 implementation for the PC API. Do not assume UTP's renderer can render Halo. A Vulkan-to-Metal route is permissible only after its required extensions, resource behavior and physical-iOS integration are demonstrated. Otherwise implement the finite observed graphics subset directly on Metal. A high-level renderer replacement is a separately justified alternative because it risks rebuilding much more game behavior.

`RENDERER.md` inventories vertex/index formats, state transitions, shader constants, fixed/programmed shader variants, resource creation/update, surfaces, render targets, depth/stencil, blending, texture compression, cubemaps, lightmaps, fog, transparency, water/camouflage, skinned/first-person models, UI/text, particles, decals and movies actually required.

Preserve depth conventions, projection, handedness, clipping, texture coordinates, filtering and resource lifetime. Use captured reference scenes and API traces to identify defects. Metal pipeline creation and shader conversion are versioned/reproducible; unrecognized required shader behavior is an explicit failure, not an invisible fallback.

Magellanicus can inform Halo-specific rendering semantics, but its source advertises major unfinished features. A flycam port is not an accepted renderer implementation. [S10]

### 8.3 Audio and multimedia

Inspect actual imported and statically linked audio/codec paths. Reproduce PCM/compressed sample decoding, mixing, streaming, sample rates, volume/panning, looping, positional cues, music, dialogue and synchronization using native or properly licensed portable components.

No x86 DLL may remain a hidden runtime requirement. A codec/middleware wrapper must be functionally complete for the supported assets. Test weapons, shields, footsteps, vehicle loops, ambient sound, UI, speech, campaign music and transitions. Record queue depth, underruns and output-clock drift. Handle route/interruption recovery independently of the game simulation clock.

### 8.4 G5 controlled interoperability — the main feasibility gate

Use the selected **Custom Edition** original dedicated server and an established Windows Custom Edition client with matching stock data. Keep the dedicated server executable unchanged for the initial test. Necessary firewall/router changes remain operator-controlled. Automated reference play is confined to a controlled session, never public matches.

Evidence must show:

- normal identity/authentication, challenge and version/resource validation;
- native client and original client in the **same** match;
- movement/aim, authoritative hits/damage, weapons, grenades and shield recovery observed from both sides;
- death/respawn, scoreboard/team changes, one vehicle with both observer perspectives and a normal game objective;
- match end, a natural server map change, another playable map, disconnect and reconnect;
- native logs containing no unresolved CPU/API/packet path, and server/reference logs agreeing on the observed result.

Test at least Slayer and CTF with suitable stock maps. Distinguish ordinary client prediction corrections from persistent semantic drift. Do not fake version responses, map checksums, identity or match completion to pass. A ping or successful connection handshake cannot substitute for this sequence.

**When G5 passes with the early iOS capsule also accepted, continue the full implementation automatically.** This is evidence that the selected native-client approach works end to end, not proof of the finished iPhone product.

### 8.5 G6 existing community and compatibility scope

Resolve a currently available stock-compatible destination from the operator-published communities in the research record. Check rules and any required client modifications. Join normally and complete a real session with an established desktop player. Record date, version, map, server operator, duration, measured latency, disconnect behavior and redacted corroborating evidence.

The exact IP is runtime configuration, not a permanent immutable promise. The first-run app must offer direct connect and a maintained/configurable discovery path. Preserve 1.10-compatible discovery, with timeouts and rate limits. If master discovery fails while direct connection works, report that distinction rather than claiming online is entirely broken or entirely complete.

Custom Edition and retail compatibility remain separate test rows. Never send one edition's profile to the other service by accident. Source inspection of a map compatibility patch does not prove cross-edition networking. Server-side mods may need no client code changes, but this must be tested. Required unsupported binary mods are labelled unsupported; no security evasion or arbitrary mobile DLL loading.

### 8.6 G7 retail campaign

Apply the same translation and runtime process to the exact retail profile. Isolate addresses and generated namespaces; retest shared runtime changes against Custom multiplayer. The original retail client is the behavior oracle.

Record a fresh-save completion path through the following expected original levels, confirming names/identities from the actual owned data:

| Map identity | Level | Required evidence |
|---|---|---|
| `a10` | The Pillar of Autumn | New game, training/control flow, combat, checkpoint, reload and level completion |
| `a30` | Halo | Outdoor traversal, allies/encounters, vehicle operation and mission objectives |
| `a50` | The Truth and Reconciliation | Night/indoor transitions, stealth/combat and scripted objective progress |
| `b30` | The Silent Cartographer | Water/beach/island, vehicles, interiors and mission completion |
| `b40` | Assault on the Control Room | Large traversal, vehicles, doors and objective transitions |
| `c10` | 343 Guilty Spark | Encounter scripting, fog/lighting, cinematics and exit sequence |
| `c20` | The Library | Repeated encounter progression, doors/elevators, checkpoints and sustained combat |
| `c40` | Two Betrayals | Return-level state, large encounters and objective-specific progression |
| `d20` | Keyes | Indoor/organic geometry, scripting, objective pickup and completion |
| `d40` | The Maw | Final scripted sequence, vehicle escape, ending and credits |

These are the test plan's expected identities, not a preverified asset manifest. G1/G7 resolve them against the selected files. The agent may automate reproducible routes privately, but final touch/controller feel and exact physical-device acceptance require actual interaction.

At least one full fresh-save run on native macOS is mandatory. Checkpoint fixtures accelerate regression but cannot replace progression proof. Require each chapter transition, checkpoint resume, death/retry, save/relaunch, difficulty/profile persistence and ending. Keep fixtures local and test the save path under interruption and disk-pressure conditions.

## 9. Phase 3: Apple surface, correctness and stability

### 9.1 Simulation and performance

Preserve the selected original simulation/network cadence. Rendering rate and simulation ticks are different. Establish original-client baselines for menu, combat, vehicles, campaign AI, streaming and networking. Do not silently import Chimera timing patches or use higher frame rates to alter movement, collision, aim assistance or server semantics. [S19]

The engineering target is responsive **60 Hz presentation where measured hardware supports it**, with a stable **30 Hz presentation mode** if necessary; these are targets, not tested performance claims. Neither mode may change simulation speed. Record median/p95/p99 CPU/GPU frame time, stutters, resident memory, audio underruns, network corrections, power/thermal state and session duration.

Profile representative hot lifted loops early at G2/G3. AOT output is not automatically fast: memory helpers, per-instruction flags, indirect dispatch, floating-point helpers and register spills can dominate. Replace a proven hot function only with equivalent tested behavior. Do not optimize by removing game work.

### 9.2 Simulator order

Build iPad Simulator first. Confirm both supported profiles reach their acceptance loops. Shut down the project-owned Simulator, then test iPhone. Never count Simulator timing as device timing. Verify the device build and simulator build use the same core revision/patches rather than a separate synthetic implementation.

Only one HaloPad candidate process and one project Simulator run at once. An explicitly configured dedicated server and original desktop reference client are permitted companions for network testing. Do not stop unrelated user processes or simulators.

### 9.3 Native shell and input

Adapt game-neutral mechanisms from UTP into `port/apple/`; use a native lifecycle owner and Metal-backed view. Halo retains gameplay/menu logic; the host owns imports, input routing, diagnostics, device state and settings. Keep native UI actions on the appropriate thread and renderer ownership explicit.

Provide phone/tablet landscape defaults with safe areas, readable HUD/text and full multi-touch. Touch controls must cover movement, look, fire, grenade throw/select, melee, reload, weapon switch/pickup, jump, crouch, use/enter/exit vehicle, zoom, flashlight where applicable, pause/menu, scoreboard and chat. Verify exact game mappings instead of copying Unreal's ALT/USE semantics.

Controls need drag/scale editing, opacity, reset, handedness, persistent layout profiles and appropriate context states. No required simultaneous action may be physically impossible. Clear held controls whenever native menus, alerts, keyboard, controller handoff or interruptions take focus. Preserve taps/edges and analog magnitude correctly; avoid double-firing on handoff.

Physical controllers use Apple's supported controller APIs, explicit ownership, reconnect and touch auto-hide/show. Test controller present-at-launch and hot-connect. Support keyboard/pointer on macOS and compatible iPads, relative aiming, capture/release, text input and separate gameplay/menu routing. Gyro aiming and haptics are optional post-baseline features, default-off until tested. Aim assistance may not create a competitive advantage or alter server expectations.

### 9.4 Three-dot menu

Required sections:

- Resume / original game menu, with all held input cleared on transition.
- Controls: touch visibility/edit/reset/opacity/scale, controller status, pointer sensitivity/capture and routing.
- Display: verified resolution/render-scale choices, frame-presentation target and default-correct aspect behavior.
- Audio: levels and output/interruption state.
- Multiplayer: discover, favorites/direct connect, clear edition/mod compatibility, disconnect and last failure detail.
- Game Data: prepare/import instructions, imported profiles, verify/reimport/remove, supported identity and missing files.
- Campaign/Saves: correct retail profile, checkpoint status, safe backup/export/import where supported; destructive deletion separately confirmed.
- Diagnostics: share a redacted log and report a problem.
- About: version, source/core/data identities, third-party notices, unofficial status and verified limitations.

Settings persist and incompatible changes restart the relevant subsystem cleanly. No placeholder menu item that appears functional but silently does nothing.

### 9.5 Import, filesystem and secrets

Import prepared packages transactionally into profile-specific app-container directories. Validate declared type, sizes, hashes, path normalization and total expanded size. Reject traversal, absolute paths, unsafe symlinks, duplicates/case collisions, encrypted/unexpected archive types, decompression bombs and unapproved CPU-code payloads. Preserve the previous working installation until the new one validates.

All Windows executables and credentials remain outside public source/package artifacts. Prepared inert PE data is still proprietary user data. Missing/incorrect packages show actionable errors; do not guess or download substitute retail assets. Keep game profiles, saves and settings separate from immutable data. Keys are handled only through an accepted private mechanism, redacted from logs and diagnostic export.

### 9.6 Lifecycle and online interruption

Offline pause/checkpoint behavior and multiplayer interruption are different. Opening native settings cannot pause the server. On loss of focus, release all inputs and use a documented safe networking policy; on actual suspension or prolonged interruption, expect timeout/disconnect and offer a clean reconnect. Do not pretend a suspended app remained continuously active in the match.

Test app background/foreground, lock/unlock, audio interruption, route change, renderer recreation, memory pressure, controller loss, orientation/safe-area updates and clean shutdown. Never resume stale input, duplicate worker threads, replay old network actions or restore a server session from an offline save snapshot.

### 9.7 Diagnostics

Start structured breadcrumbs at G2/G3: executable/profile identity, core hash, boot stage, dispatch fault, import/callback failure, thread/worker state, memory mapping, graphics/audio initialization, map/state transition, network phase, input ownership, lifecycle, save transaction and clean exit.

Local raw traces may contain game memory or sensitive identifiers and must remain ignored. Export only a privacy-bounded log with no product keys/key hashes, tokens, personal paths, game bytes, packet payloads containing secrets, saves or dump memory. Server addresses and usernames should be minimized/redacted in public diagnostics unless explicitly approved.

## 10. Phase 4: acceptance matrix

All rows start **NOT RUN**. Record target, hardware/OS/SDK, root and dependency revisions, input profile/hash, build configuration, artifact identity, commands, logs, screenshots/captures, actual observer, result and defects. `PASS` means observed; `SOURCE_ONLY`, `NOT_RUN`, `BLOCKED_EXTERNAL` and `FAIL` are distinct states.

| Row | Test | Targets | Pass condition |
|---|---|---|---|
| M01 | Workspace and private-rights state | Repository | Inputs/ref/generated/evidence protected; no unauthorized publication; license inventory initiated |
| M02 | Custom Edition identity | Preparation + reference PC | Accepted provenance, 1.0.10.0621 identity, full hashes/imports/resource manifest, original-client baseline |
| M03 | Retail identity | Preparation + reference PC | Separate exact retail profile and original campaign baseline; no Custom/retail mixing |
| M04 | Static translation coverage | Host tools | All analyzed executable regions/targets classified; unknown paths fail explicitly; no fake coverage percentages |
| M05 | CPU/ABI differential tests | Reference + ARM64 Mac | Reached instructions, floats, flags, calling conventions and observable memory behavior agree |
| M06 | Memory, callback and thread tests | ARM64 Mac + device capsule | High-address guest model, structures, callbacks, TLS, workers and nonlocal control flow operate without executable runtime allocation |
| M07 | Early physical architecture capsule | Physical iPad or iPhone | Real compiled Halo slice runs signed without debugger, including relaunch and data import |
| M08 | Native process/core | macOS | ARM64 process reaches real initialization without hidden Windows/x86 execution |
| M09 | Menus and loading | macOS | Actual original menu/title/loading accepts input; resources/audio initialize |
| M10 | Stock local gameplay | macOS | Blood Gulch and another stock map; collision, combat, grenades, melee, pickups, vehicle, death/respawn |
| M11 | Controlled handshake | macOS + reference PC/server | Correct version/resource/authentication flow; no bypass or mocked accept |
| M12 | Controlled shared match | macOS + reference PC/server | Slayer and CTF with two-sided observed combat/objective/score and vehicle behavior |
| M13 | Map transition/reconnect | macOS + reference PC/server | Natural map change, continued play, disconnect and reconnect without stale state |
| M14 | Existing-community session | macOS + established PC player | Normal accepted client session on existing service; real play documented; no current-population extrapolation |
| M15 | Discovery and failures | macOS, both Simulators | Discovery/direct connect, timeout, refusal, wrong edition/map/password and offline state handled clearly |
| M16 | Multiplayer asset/mod boundary | All app targets | Stock compatibility proven; unsupported native plugins rejected; optional maps identified/validated |
| M17 | Campaign first-play | macOS | Retail a10 new game, combat, checkpoint, death/reload, completion, quit/relaunch persistence |
| M18 | Campaign a30–b30 | macOS | Each original level completed with objectives, AI, vehicles and transitions recorded |
| M19 | Campaign b40–c20 | macOS | Each original level completed; long encounters and checkpoints stable |
| M20 | Campaign c40–d40 + credits | macOS | Remaining original levels, final escape, ending and credits complete |
| M21 | Fresh-save campaign audit | macOS | One linked fresh-save full run; fixtures are not sole progression evidence |
| M22 | Saves and profile isolation | macOS + both Simulators | Transactional saves, relaunch, backup/recovery, wrong-profile rejection and controlled interrupted write |
| M23 | Rendering correctness | macOS + both Simulators | Representative outdoor/indoor/water/fog/camo/UI/first-person/effects/cinematic references reviewed; no gameplay-obscuring omission |
| M24 | Audio correctness | macOS + both Simulators | Dialogue, music, effects, position/looping, transitions and interruption recover correctly |
| M25 | iPad Simulator core | iPad Simulator | Actual multiplayer match and campaign first-play with the same AOT core |
| M26 | iPhone Simulator core | iPhone Simulator | Same loops after iPad Simulator is shut down; appropriate phone viewport |
| M27 | Touch and native menu | Both Simulators, later devices | Required actions, multi-touch, edit/reset, readable UI, routing and no stuck inputs; hands-on checks |
| M28 | Controller/pointer/keyboard | macOS + supported Sim/device peripherals | Ownership, launch/hot-connect/disconnect, pointer capture and text/game routing proven for named devices |
| M29 | Import and settings | macOS + both Simulators | Correct packages, unsafe/wrong packages, rollback/reimport/remove, settings persistence and useful errors |
| M30 | Lifecycle and network interruption | macOS + both Simulators | Offline pause/save and online suspend/disconnect/reconnect handled honestly; no duplicate threads or stale input |
| M31 | Performance and soak | macOS + iPad Simulator | Baseline tick fidelity; frame/memory/audio data; 60-minute mixed play and repeated map/load cycles; no unbounded growth |
| M32 | Diagnostics/package safety | Repo + local app products | Useful redacted export; source/data/code/signing boundaries verified and notices generated |
| M33 | Clean-checkout reproducibility | Fresh directory | Pinned scripts rebuild both cores/app and run regression matrix without undocumented edits |
| M34 | Physical iPad acceptance | Exact signed iPad candidate | Real accepted online PC match plus a10 and later campaign fixture; controls/lifecycle; 60-minute thermal/memory/audio session |
| M35 | Physical iPhone acceptance | Exact signed iPhone candidate | Same core/session requirements, phone ergonomics and a separate 60-minute sustained session |
| M36 | Public candidate | Exact source + each binary | D1–D11 satisfied; rights/notices/audits; candidate identity; explicit Chris approval for each release surface |

M01–M33 are the technical gate, with M07 requiring the early device capsule. M34–M35 test the exact final candidates; capsule evidence does not replace them. Some matrix conditions concern design-selected behavior and are not claims that the original game already supports a proposed convenience feature.

For each device, record actual model, OS, render scale, presentation target, input method, thermal state and network. Do not extrapolate from one iPad to all iPhones. Re-signing changes artifact identity: preserve the unsigned payload hash, record signed candidate identity and rerun required install/launch checks.

## 11. Evidence, journal and reporting

Maintain the documents listed in the workspace. `STATUS.md` gives the lowest unmet goal, current rung, accepted source/input profile, known-good commands/artifacts, matrix summary and blockers. `JOURNAL.md` is append-only: hypothesis, step, command, observed result, evidence path, interpretation and next action.

Use local `docs/artifacts/YYYY-MM-DD/goal/test/` paths. Never commit the evidence folder wholesale. Every assertion must identify its evidence level:

- **Source-level:** a file or author describes a behavior.
- **Generated/built:** a tool produced code or a binary.
- **Executed:** the named target actually performed the test.
- **Interoperable:** an established external client/server agreed with the observed gameplay.
- **Physically accepted:** the exact physical candidate passed hands-on requirements.

No source claim, screenshot, percentage, PID or AI-written report can be promoted automatically to a stronger evidence level. Preserve first causal errors and rejected hypotheses; do not erase the failed tests when a workaround succeeds.

Each gate report includes `PASS/FAIL/BLOCKED_EXTERNAL/NOT_RUN`, source/core/input identities, actual test steps and why the result answers the gate. Maintain a stable public-safe source index separately from private traces. Generated or reconstructed code must remain traceable to the selected original behavior and license boundary.

If a session ends before completion, write a useful handoff with the known-good command, precise blocker, smallest next experiment and external action required. Do not claim background work will finish later. The next agent should be able to resume without rereading the whole chat.

## 12. Public release, provenance and package boundaries

This is an engineering release gate, not legal advice. Chris's authorization to conduct private work does not establish blanket permission over all original or third-party material.

### 12.1 Default topology

Use a separate HaloPad integration repository containing original glue, permissible patches, scripts and documentation. Original game inputs, generated game source, inert PE data images and extracted resources remain private build inputs/products. Resolve exact licenses for copied components, including GPL/LGPL requirements where applicable. Do not assume a root MIT label covers every imported file.

Avoid dependence on unlicensed SDKs, leaked/prototype executables or unreviewed reconstructed corpora. The selected retail/Custom input route is deliberate. Publicly visible source is not necessarily licensed for reuse. Read and record upstream notices before copying.

### 12.2 Build-time and installed-app boundary

The prepared data package contains no signing material and is never auto-uploaded. The installed native app must not download/import new CPU modules after signing. A newly supported executable requires a new compiled candidate. Data-only imports and original game scripts remain tightly validated.

A native binary produced by translation can still contain proprietary game logic and constants. **“No maps bundled” and “user owns the game” are not automatic redistribution clearance.** Source publication, build scripts, prepared-data sharing and binary/IPA publication have different boundaries; decide each separately.

If redistribution of compiled game logic is not cleared, retain a local user-build workflow rather than silently shipping the translated engine. Do not decide legal sufficiency by technical convenience.

### 12.3 Package and repository audit

Reject tracked proprietary executables, media/images, assets, generated game source/IR, initialized guest data, saves, private packet traces, crash memory, credentials, product keys/key hashes, signing identities/profiles and machine-specific paths. Inspect Git history as well as the current tree before publication.

For local app packages inspect architecture, linked dependencies, bundle layout, private paths, entitlements, executable segments and imported-code boundary. Use Xcode's supported signing/provisioning flow for devices. No debugger/JIT permission requirement in the normal run path. [S20]

### 12.4 Release wording

Only after the matching evidence and authorization exist:

> HaloPad is an unofficial native Apple port of supported legacy Halo PC game profiles, built using ahead-of-time translation and native platform integration. Users must provide the required legally obtained game data. Online compatibility is limited to the tested PC editions, servers and map/mod configurations documented with the release. HaloPad is not affiliated with or endorsed by Microsoft, Xbox, Bungie, or Gearbox.

Do not say “official,” “works with every Halo server,” “MCC cross-play,” “complete campaign,” “60 fps,” or “works on all iPhones” without exact supporting scope/evidence. Prefer “no runtime CPU emulation/JIT” over an ambiguous “emulator-free” claim, since guest/runtime abstractions may remain.

### 12.5 Final authorization

G12 requires the complete applicable matrix, exact source and payload identities, exact physical artifacts, notices, resolved source/binary decisions and Chris's explicit final release approval. Technical completion alone never authorizes a push, release, public screenshot, installer download feature or announcement.

## 13. Risk register and go/no-go logic

| Risk | Standing | Required response |
|---|---|---|
| No ready complete PC-native Halo engine | Confirmed by reviewed routes, not an exhaustive proof that none exists | AOT client program; G2/G3 real-code proof; no pretend decomp-complete build |
| Selected lifter not Halo-ready | Unproven | Actual instruction/CFG/ABI report, differential tests, one bounded alternate |
| Wrong executable or build lineage | High risk | Exact input profiles; no debug/MCC/Xbox substitutions or address reuse |
| Guest pointers/host pointers mixed | Architecture blocker until tested | Checked relocation, typed handles, callback marshalling and early physical capsule |
| x87/flags/SSE/ABI divergence | Unproven and potentially pervasive | Differential fixtures, precise FP policy, traceable native replacements |
| Worker/TLS/exception errors | Unproven | Per-thread state and real wait/signal/shutdown tests; no synchronous fake workers |
| Renderer complexity | Major unsolved subsystem | Actual API trace, finite subset, Metal-backed proof; no map-viewer success claim |
| Audio/codec vendor dependence | Unknown until imports traced | Native/portable licensed replacement; no x86 DLL runtime dependency |
| AOT performance overhead | Unmeasured | Early hot-loop profiling; no automatic “native means fast” inference |
| Existing-server compatibility | Required and untested | Unchanged server + original-client match before community/public claims |
| Community client policy or empty sessions | Unknown | Baseline operator/rules/actual players; no evasion or fabricated population |
| Two profiles increase work | Intentional scope | Shared platform layer; separate core/input/state; regression both after retail added |
| Campaign omitted after multiplayer demo | Product failure | D7/M17–M21 required; no unilateral downgrade |
| Device memory, thermals and suspension | Unmeasured | Early capsule plus final physical iPad/iPhone sustained acceptance |
| Data acquisition unavailable | External blocker | Precise requested files/provenance handoff; no unlicensed fallback |
| Rights/code distribution unresolved | Public-release blocker | Private work topology and separate source/binary decisions |
| Agent reports synthetic progress | Process risk | Lowest unmet goal; evidence classes; no stubs or unexplained count metrics |

**Continue** when each experiment reduces a named uncertainty with reproducible evidence and the remaining work is a finite implementation problem. **Pause an affected path** for an external input/device/permission. **Report NO-GO under current constraints** when the required CPU/runtime/network contract cannot be met after both bounded candidate routes and the unblocking ladder. **Never label ordinary debugging as impossibility, and never label an architectural hypothesis as a completed port.**
