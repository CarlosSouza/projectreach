# HaloPad: decision, exact inputs, and research record

Research date: **6 September 2026**. Scope: native ARM64 macOS, iPadOS, and iOS; original Halo gameplay; multiplayer with existing PC communities.

Companions: [PRD.md](PRD.md) and [GOAL-LOOP.md](GOAL-LOOP.md). This document records the evidence and acquisition decisions; it does not replace their acceptance gates.

## 1. Decision

**GO for a private, gated technical build program. NO-GO for describing Halo as a ready-to-port engine, promising completion, or publishing a release now.**

The recommended experiment is **ahead-of-time translation of the original Windows client**, combined with a narrow native platform layer and evidence-backed replacement of selected functions. Start with **English Halo Custom Edition 1.0.10.0621** because the requested differentiator is joining its existing PC multiplayer population. Preserve the full-game objective through a separately identified **English 2003 Halo PC retail 1.0.10.0621** campaign profile. The two executables, resources, and network modes must not be silently mixed.

This is an engineering judgment, not a measured success probability. The architecture is credible because Windows-game static recompilation to ARM64/macOS exists, Halo-specific reconstruction and modding references exist, and community destinations exist. No reviewed source supplies the complete combination. The unresolved question is whether a Halo-specific translation/runtime can be made correct and performant at manageable scope.

**Promotion bar:** real Halo code on native ARM64, then playable native macOS combat, then a complete match with an established Windows client through an unchanged compatible dedicated server. An early physical-iOS architecture capsule tests the CPU/memory/signing design before extensive UI investment. Only actual execution can upgrade this source-level judgment to demonstrated feasibility.

No Halo executable, licensed game installation, original reference run, Apple build, server join, or device performance test was available or executed during this review. No game-input hash below is asserted as already verified.

## 2. Corrections to earlier advice

- Earlier numerical ratings such as “4/10” were judgment calls, not measured probabilities; do not use them as planning evidence.
- **Demon does not currently target ordinary retail `halo.exe` 1.10.** Its README identifies a 2020 Digsite `halo_cache_symbols.exe` and a 32-bit replacement DLL. It is a reference, not the build root. [S03]
- The main Xbox matching-decomp routes require specific older/debug inputs and still retain original executable code. A Mac-compatible build tool does not imply a native Mac game. [S04–S05]
- A newer Xbox fork reports substantially more reconstructed code than the earlier review found. Its output is still a patched Xbox executable, and its percentages are not Apple-port completion percentages. [S06]
- Current xboxrecomp source contains explicit Halo bring-up fixes. Therefore “nobody has attempted Halo” is not supported. Conversely, those fixes do not establish completed gameplay or PC-server compatibility. [S07]
- The Apple memory issue is not solved by changing one macro. An offset-based address model is plausible, with precedent, but every pointer, callback, API structure and generated memory access must respect it. [S01–S02, S07]
- **The old Mac release is not the UTP situation.** UTP starts from an existing ARM64 engine. The audited Mac Halo material provides no equivalent complete ARM64 input. [S11–S13]
- A source archive, fly-camera renderer, matching percentage, generated-C count, or compiling translation unit is not a working game. [S06, S08–S10]

## 3. Route comparison

| Route | What actually exists | Missing work / mismatch | Decision |
|---|---|---|---|
| Windows retail / Custom Edition → static translation → ARM64 | SRW/llasm supports Windows-executable static translation; game-specific Windows ports and macOS ARM64 targets exist | Halo executable analysis, instruction/ABI coverage, platform shims, graphics/audio and exact network preservation | **Primary research/build route**; select lifter by actual tests |
| Windows client → incremental native reconstruction | Ringworld, Chimera and Demon provide useful code/metadata, but remain extensions/replacements around original executables | Complete independent engine and buildable ARM64 integration; version/provenance differences | **Supporting route** for bounded functions, not “compile the decomp” |
| Original Xbox → xboxrecomp | XBE lifting/runtime; current Halo-specific corrections; Xbox engine reconstruction references | ARM64/platform integration, graphics/audio, remaining title work; Xbox networking is not PC/Custom Edition networking | **Alternate program only**, not a silent fallback for the requested online product |
| Old Mac PPC/i386 → native translation | Historical Mac client and very early PPC decomp | Architecture translation, legacy API replacement, edition/network compatibility, input availability | **Secondary audit only** if PC route has a demonstrated blocker |
| Anniversary/MCC reconstruction | Research corpus and officially sold newer product | Different binary, runtime, assets and multiplayer ecosystem; corpus does not run Halo | **Not the selected input** |
| HaloMD / map tools / renderer-only projects | Launcher, asset utilities, renderer experiments | Not complete original game clients | **References only** |
| Streaming / Wine + CPU translation / xemu / CPU interpreter | Useful possible reference environments | Does not meet the native AOT Apple product definition | **Reference testing only**, not completion |

Do not run all routes indefinitely. PRD G2 compares two bounded lifter candidates for the **same selected Windows binary**, then locks one. Changing to Xbox, MCC, or a campaign-only product requires an explicit recorded scope decision.

## 4. Exact game inputs and acquisition

### 4.1 Recommended acquisition path

**Obtain a legitimate copy of the original 2003 Windows release of Halo: Combat Evolved, with its Halo PC product key.** An existing owned installation or a complete original boxed PC copy is the practical input route. Verify that a proposed boxed purchase includes the correct PC edition, readable media and its original key; this review did not verify a particular seller, listing, price, or key usability.

Then prepare two separate installations on a user-authorized Windows reference machine:

1. **Online profile:** install the English Halo Custom Edition distribution, then apply the **Custom Edition** 1.10 update. Record `haloce.exe` as build **1.0.10.0621**. Custom Edition's original installer requires the Halo PC key. [S14–S16]
2. **Campaign profile:** install the original English Halo PC retail game from the owned media, then apply the **retail PC** 1.10 update. Record `halo.exe` as build **1.0.10.0621**. Keep its original campaign/resource files separate from Custom Edition. [S14]

The online profile can unlock G1–G6 before the campaign input is ready. Missing campaign data remains an explicit blocker to full-game completion, not permission to call the multiplayer prototype a complete Halo CE port.

**Do not buy the Mac edition or MCC specifically for this plan.** The Steam Halo: Combat Evolved Anniversary product is the MCC-era product, not a source of the selected legacy 32-bit clients or an interchangeable license/key. [S17] No currently verified authorized full legacy-game digital storefront was found in this review. A public mirror or the removal of a historical disc check is not a finding that the original retail game is freeware.

### 4.2 Links and exactly what each supplies

| Source | Supplies | Limitation |
|---|---|---|
| [Bungie's 1.10 announcement](https://www.bungie.net/en-US/Forums/Post/64943622) | Authoritative release notes, version `1.0.10.0621`, separate client patches and dedicated-server links | Old download URLs; binary availability and integrity must be checked at acquisition |
| [CE3: English Custom Edition installer](https://haloce3.com/downloads/official-files/halo-custom-edition-game-english/) | Community archive of Gearbox's Custom Edition installer | Not Microsoft-operated; requires an original Halo PC key; not retail campaign media |
| [CE3: patch 1.10](https://haloce3.com/downloads/official-files/patch-1-10/) | Archived Custom Edition patch | Verify which edition the downloaded update targets; do not apply blindly to retail |
| [CE3: getting started](https://haloce3.com/get-it/) | Explains relationship to the original Halo PC copy | Its optional mod recommendations are **not** baseline dependencies |
| Original owned PC media / installation | Retail campaign data, original executable and legitimate product-key provenance | Operator supplies these; no automatic purchase, key collection, or unlicensed mirror fallback |

Bungie's original link identities are retained below for provenance. These are **not asserted to be working downloads today**:

```text
http://halo.bungie.net/images/games/halopc/patch/110/halopc-patch-1.0.10.exe
http://halo.bungie.net/images/games/halopc/patch/110/haloce-patch-1.0.10.exe
http://halo.bungie.net/images/games/halopc/patch/110/haloded.exe
http://halo.bungie.net/images/games/halopc/patch/110/haloceded.exe
```

Use the official source first. An archive fallback requires the operator's accepted source provenance, recorded hashes, and a baseline run. Do not invent a checksum because two websites carry a file with the same name. Do not execute an unknown download merely because it is labelled an official patch.

### 4.3 Exact build profiles

| Profile ID | Input executable | Role | Data / server boundary |
|---|---|---|---|
| `custom-en-1.0.10.0621` | `haloce.exe`, PE32/x86, actual version and hashes verified locally | First native multiplayer target | Matching Custom Edition resources and stock maps; Custom Edition dedicated server `haloceded.exe` |
| `retail-en-1.0.10.0621` | `halo.exe`, PE32/x86, actual version and hashes verified locally | Original campaign and separately tested retail multiplayer | Owned retail campaign/resources; retail server `haloded.exe` |

The version string alone is insufficient. G1 records SHA-256, size, PE machine type, entry point, image base, section layout, imports, build provenance, patch identity, DLL inventory and resource-file manifest for each profile. A known-profile build rejects any later binary whose hash differs, including LAA-patched, modded, cracked, trial, translated-language or debug variants, until deliberately audited.

A complete source reconstruction is **not** an input requirement for this route. A complete and verified description of executable code paths is an evolving engineering requirement. Original inputs are preserved; generated code and extracted data are local ignored products.

### 4.4 Product keys and online tests

The operator enters their own credentials only into the legitimate setup/accepted local mechanism. Never search for keys, reuse a posted key, fabricate an identity, suppress authentication to pass a test, or put secrets/key hashes in public evidence. Use a second legitimately provisioned reference player for simultaneous-client tests. Establish its availability before G5 rather than discovering the requirement after porting.

The public server operator decides what clients and modifications are accepted. A compatibility failure is not authorization to evade anti-cheat or identity checks. Use a controlled server for debugging and contact maintainers only after Chris authorizes contact.

## 5. Existing multiplayer destinations

These are **operator-published destinations**, not a claim that this review successfully joined them or counted human players. Recheck at execution time; the baseline networking test must happen before native port effort expands.

| Community | Published destination | Initial use |
|---|---|---|
| [Halo PC OG](https://halopc.net/) | `74.91.112.182:2302`; describes a Custom Edition server and organized matches | Candidate stock-map community test with an established PC player |
| [Liberty Gaming](https://liberty-ce.net/index) | MIX `185.107.96.224:2302`; RACING `185.107.96.219:2302` | Later compatibility testing; map/mod requirements must be inspected first |
| [Players of Quality](https://poqclan.com/) | Operator advertises PC and Custom Edition public servers | Additional destination; resolve exact edition, endpoint and rules before testing |

Prefer a stock-map, standard-protocol session for initial validation. A server with special movement/weapons, extra client plugins, custom scripts or modified maps is not an appropriate first correctness oracle. Network compatibility is proven by two-sided observed gameplay, not ping, a browser row or UDP response alone.

## 6. Source record

Source summaries distinguish **upstream claims**, **source inspection** and **our inference**. Mutable pages were read on 6 September 2026. Branch tips and percentages may change. A pinned file identifies the audited implementation, not a guarantee that it builds in this environment.

### S01 — Windows-game AOT precedent

[M-HT/SR README](https://github.com/M-HT/SR/blob/master/README.md), [SRW README](https://github.com/M-HT/SR/blob/master/SRW/README.md), [llasm](https://github.com/M-HT/SR/tree/master/llasm).

The project lists Windows-game static recompilation and macOS ARM64 targets. SRW translates Windows executables plus game-specific metadata; llasm feeds LLVM. Its Windows examples are Septerra Core and Battle Isle 3, **not Halo**. This supports the technique, not Halo coverage. SRW's default output is x86; an ARM-oriented evaluation must deliberately select the LLVM route rather than mistake x86 output for progress.

### S02 — Guest-pointer relocation precedent

[Septerra Game-Memory.c at `ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3`](https://github.com/M-HT/SR/blob/ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3/games/Septerra%20Core/SR-Septerra/Game-Memory.c).

Source includes Apple-specific paths and a `PTROFS_64BIT` pointer-offset mechanism. It is useful evidence that 32-bit guest values need not be literal host pointers. It is not proof that the entire SRW graph supports Halo callbacks, threading, structures, or iOS. Those remain G2 tests.

### S03 — Demon: exact input mismatch

[Demon README](https://github.com/Aerocatia/demon/blob/master/README.md).

Current input is a 2020 Digsite executable, with replacement C in a 32-bit Windows DLL plus Rust thunking. It is not a standalone engine and not the selected retail/Custom binary. Original code is GPLv3. The linked proprietary debug input is not an approved default acquisition source. Use only permissible, version-mapped references; never copy guessed addresses into the release profile.

### S04 — Original Xbox incremental decomp

[halo-re/halo](https://github.com/halo-re/halo).

Build instructions produce a patched XBE that runs on Xbox/xemu, using a specific `cachebeta.xbe`. The ability to run its build tools on Apple Silicon is not Apple-native game execution. Useful architecture reference; wrong default network/input lineage for this brief.

### S05 — Separate Xbox debug build decomp

[punpckhdq/halo](https://github.com/punpckhdq/halo).

The documented target is build 2342 `cachebeta.exe`; its build requires a PAL debug input and the August 2001 Xbox SDK. Do not make proprietary SDK/debug downloads an undeclared dependency of this project.

### S06 — More advanced Xbox fork

[stianeklund/halo README](https://github.com/stianeklund/halo/blob/main/README.md).

The retrieved README reports 5,307/6,805 ported functions (77.99%) and 65.12% of code bytes, with separate match/equivalence metrics. These are upstream measurements, not independently rerun in this review. The documented output still patches the original XBE. This improves reference availability, not proof of a complete PC or ARM64 client. Do not average these percentages with another project's metrics.

### S07 — Xbox AOT toolkit

[xboxrecomp at `a781596e2181f8d4b15336659897ce07bdd7d56a`](https://github.com/sp00nznet/xboxrecomp/tree/a781596e2181f8d4b15336659897ce07bdd7d56a), especially [runtime types](https://github.com/sp00nznet/xboxrecomp/blob/a781596e2181f8d4b15336659897ce07bdd7d56a/templates/runtime/recomp_types.h).

The current README identifies v0.7.1 and an XBE-to-C pipeline. Source contains guest-memory offsets, per-thread registers and Halo-related correctness fixes. It builds runtime libraries, not a ready Halo game. A PC route would need a PE frontend and Windows runtime boundary; its Xbox D3D8/NV2A runtime is not a PC D3D9 implementation. Any adopted files need their own license/provenance check, including mixed-license components.

### S08 — Custom Edition-specific reconstruction

[Ringworld](https://github.com/MangoFizz/ringworld/blob/master/README.md).

It redirects original Custom Edition functions into replacements and describes itself as an extension, not a replacement for the game. Its build is i686 MinGW. Useful for selected data/renderer semantics; no complete ARM64 client established.

### S09 — Anniversary corpus

[halocea research corpus](https://github.com/surreptitiousresearch/halocea).

The repository explicitly says the material cannot play Halo. Its described input is an Anniversary prototype; compiling source units does not establish a complete linked game. Do not silently incorporate this corpus or its proprietary input into the release foundation. Any later use requires a separate provenance decision and behavioral checks against the selected PC version.

### S10 — Renderer and data tooling

[Magellanicus](https://github.com/FishAndRips/magellanicus/blob/master/README.md), [Invader](https://github.com/SnowyMouse/invader), [c20 Halo 1 documentation](https://c20.reclaimers.net/h1/).

Magellanicus is a GPLv3 Vulkan renderer with a documented Apple M2 test, but its README marks major gameplay-rendering features, HUDs and menus unfinished. It is a renderer reference, not a ready complete replacement. Invader/c20 offer asset and format knowledge, not a game engine. A flycam opening a map cannot pass a gameplay gate.

### S11 — Old Mac route

[MacSoft Halo downloads](https://macsoftgames.de/halo/downloads.html), [early PowerPC decomp](https://github.com/ChrisNonyminus/halo_mac_decomp).

Historical publisher material describes the Universal 2.0.4 update synchronized with PC 1.09. No complete modern ARM64 input was established. The publisher page was intermittently fetchable during this review; historical version information is not confirmation of a currently usable full-game download. The PPC decomp is not a demonstrated standalone port.

### S12 — HaloMD

[HaloMD](https://github.com/foonull/HaloMD).

The project is useful for historical Mac/community integration, but was not established as a complete portable Halo engine. Do not substitute a launcher or included legacy runtime for a verified ARM64 client.

### S13 — Chris's UTP reference

[UTP README](https://github.com/chrissotraidis/utp/blob/2f9c9b8f815b1f691a39960bfc96c6a9b8ebda14/README.md).

UTP hosts an already-ARM64 OldUnreal runtime with a Metal renderer. Reuse its first-person controls, import safety, lifecycle, diagnostics and packaging discipline after inspection. It does not solve Halo's instruction translation, game implementation or graphics. Reported UTP device evidence is upstream/reference evidence, not a new Halo test.

### S14 — Official 1.10 release

[Bungie announcement](https://www.bungie.net/en-US/Forums/Post/64943622).

Identifies build `1.0.10.0621`, separate retail/Custom patches and dedicated servers, and the GameSpy replacement. Preserve edition separation. The forum's original announcement is the source; unrelated reader comments and posted product keys are not instructions or approved inputs.

### S15 — Custom Edition distribution archive

[CE3 English installer](https://haloce3.com/downloads/official-files/halo-custom-edition-game-english/), [CE3 patch archive](https://haloce3.com/downloads/official-files/patch-1-10/).

Provides an accessible archive route for the Custom Edition distribution and update. The installer page states the Halo PC key requirement. It is a community archive, not a publisher-operated storefront or automatic permission to redistribute a new native binary. Its old installer-page patch advice is superseded by the 1.10 release selection above.

### S16 — Acquisition relationship

[CE3 getting started](https://haloce3.com/get-it/).

Explains the need for the regular PC release. Optional HEK, OpenSauce and custom UI are not mandatory prerequisites for the unmodified HaloPad baseline. No current reseller stock is guaranteed by its old shopping link.

### S17 — Steam is a different target

[Halo: Combat Evolved Anniversary on Steam](https://store.steampowered.com/app/1064221/Halo_Combat_Evolved_Anniversary/).

This is the newer MCC-era product. It does not establish access to the selected 2003/2004 legacy executables or their product key. Do not buy it expecting it to satisfy G1. The [H1 editing-kit documentation](https://c20.reclaimers.net/h1/h1-ek/) also distinguishes the newer Anniversary editing kit and its standalone tag-loading tool from the legacy Custom Edition game. A downloadable modern tool is not evidence of compatibility with existing legacy servers, and its newer compiled map format is not automatically backward compatible. Keep it outside the default acquisition path unless a separate exact-version interoperability experiment justifies a change.

### S18 — Community destinations

[Halo PC OG](https://halopc.net/), [Liberty](https://liberty-ce.net/index), [PÕQ](https://poqclan.com/).

Operator pages document the communities and endpoints used in Section 5. Current human counts, regional latency, client approval and successful joins were not independently verified. G1 captures an original-client baseline; G6 proves the native client's community session.

### S19 — Chimera compatibility reference

[Chimera README](https://github.com/SnowyMouse/chimera/blob/master/README.md).

Documents 1.10 support, campaign UI in Custom Edition when campaign maps are supplied, map-format support and many rendering/timing fixes. It remains a Windows mod. Do not copy its arbitrary DLL loading, embedded Lua execution, map-download policy, identity overrides or integrity-check modifications into HaloPad. A map-compatibility feature does not establish retail/Custom network interchangeability.

### S20 — Apple execution and signing

[Apple TN3125](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles), [Apple code signature guidance](https://developer.apple.com/documentation/Xcode/using-the-latest-code-signature-format).

Use supported Xcode signing/provisioning for device builds. The design compiles CPU code before signing; runtime imports are data-only. No claim of App Store acceptance follows from a working signed development build.

## 7. Audited starting revisions

| Repository | Retrieved revision | Intended role |
|---|---|---|
| `chrissotraidis/utp` | `2f9c9b8f815b1f691a39960bfc96c6a9b8ebda14` | Read-only Apple/FPS shell reference |
| `M-HT/SR` | `ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3` | SRW/llasm candidate and pointer-offset reference |
| `sp00nznet/xboxrecomp` | `a781596e2181f8d4b15336659897ce07bdd7d56a` | Alternate lifter/reference audit, not selected PC runtime |
| Other references | Resolve and record actual commit before code reuse | Never turn a floating URL into an assumed tested pin |
| Halo inputs | **Unverified; operator-supplied hashes required** | Do not substitute hashes from the unrelated debug builds |

The three revisions were retrieved from GitHub metadata/code URLs. Fetch and audit the complete recursive source graph before use. No dependency graph was built in this research environment. Newer revisions may be evaluated deliberately with a regression report, never pulled automatically mid-debugging.

## 8. What would reverse the GO

Stop expansion and report a project-level **NO-GO under current constraints** if both bounded Windows-lifter candidates fail the actual Halo/ARM64 contract without a finite repair plan; if core behavior requires an unbounded dynamic CPU executor; if essential authorized inputs cannot be obtained; or if the compatible community rejects the necessary client and there is no accepted equivalent destination. Describe the specific failure, evidence, attempted alternatives, and what would change the decision.

Ordinary compiler errors, incomplete syscall coverage, rendering defects, wrong input mappings and network bugs are engineering tasks—not automatic no-go findings. Conversely, months of repeatedly producing a title screen, empty map or fabricated “connected” state do not justify calling the project feasible.
