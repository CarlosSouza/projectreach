# Acceptance matrix

All original acceptance conditions remain in `HaloPad-PRD.md` Section 10. No row has passed. Supporting guard tests are not the complete acceptance matrix.

| Row | Test | Status | Evidence / unmet requirement |
|---|---|---|---|
| M01 | Workspace and private-rights state | NOT_RUN | Protection and license inventory checks executed; complete environment gate still open. |
| M02 | Custom Edition identity | PARTIAL (engineering tier) | G1a: reproducible derived 1.0.10.621 client accepted (`INPUTS.md`, `docs/artifacts/2026-09-26/G1a/`). Original-client baseline (G1b) and independent hash confirmation outstanding. |
| M03 | Retail identity | BLOCKED_EXTERNAL | Separate retail input and campaign baseline missing. |
| M04 | Static translation coverage | NOT_RUN | Required behavior has not been executed. |
| M05 | CPU/ABI differential tests | PARTIAL | Seven real Halo slices match the x86 oracle natively on macOS and the iPad Simulator (G2D-SLICES.md, G2E-ADDRESS-MODEL.md): integer, string/memory with direction flag, jump tables, x87 in Halo's single-precision mode, map-header validation over real data through the file API, and a guest callback. Whole-program coverage, the strict-mode gap list and 304 untranslated entries (including the PE entry point) remain. |
| M06 | Memory, callback and thread tests | PARTIAL | G2e: checked original-address guest memory, finite dispatch, bound imports; callback slice 200/200; null/unmapped faults name the guest address and match the oracle; unknown targets and import misuse trap by name (macOS + iPad Simulator). Threads not yet tested (G3 runtime). |
| M07 | Early physical architecture capsule | BLOCKED_EXTERNAL | Needs a physical iPhone/iPad and signing from Chris. The Simulator equivalent passes (G2E-ADDRESS-MODEL.md). First measurement on device: whether the 4 GiB guest reservation is allowed. |
| M08 | Native process/core | NOT_RUN | Required behavior has not been executed. |
| M09 | Menus and loading | NOT_RUN | Required behavior has not been executed. |
| M10 | Stock local gameplay | NOT_RUN | Required behavior has not been executed. |
| M11 | Controlled handshake | NOT_RUN | Required behavior has not been executed. |
| M12 | Controlled shared match | NOT_RUN | Required behavior has not been executed. |
| M13 | Map transition/reconnect | NOT_RUN | Required behavior has not been executed. |
| M14 | Existing-community session | NOT_RUN | Required behavior has not been executed. |
| M15 | Discovery and failures | NOT_RUN | Required behavior has not been executed. |
| M16 | Multiplayer asset/mod boundary | NOT_RUN | Required behavior has not been executed. |
| M17 | Campaign first-play | NOT_RUN | Required behavior has not been executed. |
| M18 | Campaign a30–b30 | NOT_RUN | Required behavior has not been executed. |
| M19 | Campaign b40–c20 | NOT_RUN | Required behavior has not been executed. |
| M20 | Campaign c40–d40 + credits | NOT_RUN | Required behavior has not been executed. |
| M21 | Fresh-save campaign audit | NOT_RUN | Required behavior has not been executed. |
| M22 | Saves and profile isolation | NOT_RUN | Required behavior has not been executed. |
| M23 | Rendering correctness | NOT_RUN | Required behavior has not been executed. |
| M24 | Audio correctness | NOT_RUN | Required behavior has not been executed. |
| M25 | iPad Simulator core | NOT_RUN | Required behavior has not been executed. |
| M26 | iPhone Simulator core | NOT_RUN | Required behavior has not been executed. |
| M27 | Touch and native menu | NOT_RUN | Required behavior has not been executed. |
| M28 | Controller/pointer/keyboard | NOT_RUN | Required behavior has not been executed. |
| M29 | Import and settings | NOT_RUN | Required behavior has not been executed. |
| M30 | Lifecycle and network interruption | NOT_RUN | Required behavior has not been executed. |
| M31 | Performance and soak | NOT_RUN | Required behavior has not been executed. |
| M32 | Diagnostics/package safety | NOT_RUN | Required behavior has not been executed. |
| M33 | Clean-checkout reproducibility | NOT_RUN | Required behavior has not been executed. |
| M34 | Physical iPad acceptance | NOT_RUN | Required behavior has not been executed. |
| M35 | Physical iPhone acceptance | NOT_RUN | Required behavior has not been executed. |
| M36 | Public candidate | NOT_RUN | Publication not authorized; all prerequisite gates remain open. |
