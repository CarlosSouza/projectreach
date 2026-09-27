# HaloPad goal-based loop

Operating loop for the autonomous build of HaloPad. Requirements live in [PRD.md](PRD.md); source evidence and acquisition live in [INPUTS-AND-RESEARCH.md](INPUTS-AND-RESEARCH.md). Written 6 September 2026.

**Authorization:** private gated implementation. Continue automatically when a goal passes; no routine confirmation is needed for ordinary local engineering. Original-input acquisition, credentials, physical-device actions and public release remain explicit operator boundaries. A source-level GO is not evidence of working Halo.

## The goal stack

Work the **lowest unmet goal**. A goal is met only when its required evidence exists under `docs/`. A regression reopens the lowest affected goal. Independent diagnostic or preparation work is allowed while blocked; it must not be reported as proof that the blocked goal passed.

- **G0. Environment and protected state ready.** Mac/SDK/tools and original-client reference environment identified; root state recorded; reference sources pinned/read; push disabled on references; safety checks and ignores active; `RIGHTS-STATUS.md` records private-only authorization and unresolved distribution questions. No unknown local work overwritten.
- **G1. Exact Custom Edition input and original-client baseline.** English `haloce.exe` version 1.0.10.0621 is acquired through the accepted original-PC-key route and hashed; PE/import/resource inventory and prepared-data recipe exist. The original client boots, plays locally and reaches a compatible PC multiplayer destination. Baseline evidence and a legitimately provisioned second reference player are identified. Retail input may be recorded as missing here, but remains mandatory by G7. (D1; M01–M02)
- **G2. Native execution architecture proven.** One bounded SRW/llasm experiment, and at most one xboxrecomp-component/PE alternate when needed, establish a chosen lifter. Self-authored differential fixtures and nontrivial actual Halo function slices pass on ARM64 macOS. The selected guest-memory/callback/thread/dispatch contract and a real Halo slice run in an iOS capsule, including a signed physical-device run without debugger/JIT dependence. Input/profile and all remaining code-coverage gaps are explicit. (D2; M04–M07)
- **G3. Real native macOS core initializes.** Compiled ARM64 Halo code reaches meaningful actual initialization, loads required prepared data, uses real platform shims and tears down cleanly. No hidden original x86 code execution, unimplemented-call success stubs or guest Windows process. (D3; M08)
- **G4. Custom Edition is locally playable on macOS.** Actual menus/loading, Blood Gulch and another stock map, collision, player control, weapons, grenades, melee, pickups, vehicle, death/respawn, HUD and meaningful audio work. Menu return/map reload and clean relaunch pass. A map viewer or empty scene does not qualify. (D4; M09–M10)
- **G5. A complete controlled match with an original PC client.** Through an unchanged compatible Custom Edition dedicated server, native and original clients complete the Section 8.4 sequence: authentication/version/resource validation, shared Slayer and CTF, combat/score/vehicle behavior, match end, natural map transition, disconnect and reconnect. Observe both sides. This is the first substantial end-to-end feasibility proof. (D5; M11–M13)
- **G6. Existing-community interoperability.** Join an accepted, currently usable community service with an established desktop player and complete real play. Discovery/direct connect, failure states, exact edition/map/mod boundaries and redacted evidence are recorded. Server listing or ping alone is insufficient. (D6; M14–M16)
- **G7. Complete original retail campaign.** Acquire and lock the separate English `halo.exe` 1.0.10.0621 retail profile; reuse the proven platform/translation machinery without mixing code or resources. Complete a fresh-save native macOS run of all ten original chapters, ending/credits, checkpoint/death/reload, save/relaunch and profile isolation. Regress Custom multiplayer after shared changes. (D7; M03, M17–M22)
- **G8. Both Simulator cores work.** iPad Simulator first, then iPhone Simulator, each using the same AOT cores through a real multiplayer loop and retail campaign first-play/checkpoint loop. Minimal test input is permitted; a synthetic replacement core is not. One project Simulator at a time. (D8; M25–M26)
- **G9. Apple shell and input complete.** UTP-derived game-neutral mechanisms are adapted for Halo: phone/tablet touch layout, controller and pointer ownership, three-dot menu, prepared-data import, profile/settings/save management, lifecycle and privacy-bounded diagnostics. No stuck inputs or placeholder controls. (D9; M27–M30)
- **G10. Technical matrix and clean reproduction green.** All M01–M33 pass, including CPU/ABI correctness, complete campaign, graphics/audio comparison, network transitions, redacted diagnostics, soak/performance data and scripted clean-checkout rebuild. No silent fallback path remains. (D10)
- **G11. Exact physical candidates accepted.** Physical iPad and iPhone pass M34–M35 against the exact signed artifacts: accepted existing-PC match, campaign first-play and later fixture, touch/controller/lifecycle and separate sustained thermal/memory/audio sessions. Record device/OS/input/render settings and candidate identities. (D11)
- **G12. Public release specifically authorized.** Source and binary/data rights outcomes, notices, repo/history/package audits and Chris's explicit approval for the exact artifact are recorded. Only then may the specifically authorized publication occur. If public rights remain unresolved, deliver the completed private/local-build state and stop at that boundary. (D12; M36)

G2 earns further native-core investment. G5 earns the full-product implementation commitment; it does not mark the app complete. G10 is technical completion; G11 is exact-device acceptance; G12 alone permits publication. There is no fallback to an empty-map demo, campaign-only Xbox port, mobile-only server, omitted campaign or “asset-free therefore cleared” release.

`private-only` rights state does not automatically block lawful authorized local engineering; it blocks publication. Missing retail input does not block the already validated Custom work through G6, but cannot be waived for G7. Missing physical hardware is `BLOCKED_EXTERNAL`, not a passed device test.

## The loop

Repeat until the authorized terminal goal is reached or a real external/foundational blocker requires a handoff:

1. **Pick** the lowest unmet goal and the smallest experiment that reduces its most important uncertainty. Write a hypothesis and a falsifiable expected result.
2. **Check state.** Read `STATUS.md`, the latest journal entries, the relevant inventory, root/reference Git status, input profile/hash, source locks, generated-code cache identity, save fixtures, candidate/reference processes and booted Simulators. Reuse verified caches only when all relevant identities match.
3. **Execute one bounded step.** Keep game inputs and evidence read-only or backed up as appropriate. Use a scriptable command; update the script if a manual step is essential.
4. **Test immediately.** Verify the actual behavior changed. A compiler output is not execution; a screen is not gameplay; a handshake is not a match; a match is not a complete campaign or an accepted phone app.
5. **Capture evidence.** Store logs, captures, profiles and hashes in an ignored dated directory. Record both sides for network tests and the original behavior for differential tests.
6. **Interpret and update.** Append goal, hypothesis, commands, observations, result, evidence, rejected explanations and next step to `JOURNAL.md`. Update `STATUS.md` and the corresponding technical inventory.
7. **Continue or unblock.** On failure, use the ladder below before retrying. On a passing goal, advance automatically to the next unmet goal. On regression, reopen the earliest affected goal and reduce scope until it is repaired.

## Session-start checklist

1. Read the three supplied documents on first entry; subsequently read current status, latest journal and the lowest-goal inventory.
2. Record root revision and dirty state. Isolate unknown work rather than resetting it.
3. Verify pinned reference revisions and source safety. Check the exact selected game profile before using addresses, patches or generated artifacts.
4. Run `xcrun simctl list devices booted`; identify ownership. Shut down only project-owned stray Simulators. Never disrupt unrelated user sessions.
5. Inspect candidate processes. Terminate only stale HaloPad instances/test harnesses from this workspace. Reference client/server processes used by the current controlled test are deliberate exceptions.
6. Confirm the chosen save/profile and back it up before write tests. Confirm native and reference instances use distinct valid identities when required.
7. Confirm the current network-test environment is controlled or explicitly approved community play. Public services are not debugging/fuzzing targets.
8. State the hypothesis, smallest next command, required evidence and stop condition. Enter the loop.

## Process hygiene — hard rules

- **One candidate at a time.** One active HaloPad core/profile, one candidate app instance and one project-owned Simulator. A named original reference client and dedicated server are allowed simultaneously for interoperability; this is not a license to leave orphan copies running.
- **Terminate before relaunch.** Collect the causal crash/error evidence, then clean only identified project processes, locked fixtures or transient captures. Do not layer a new candidate on a half-dead process.
- **Change one causal variable.** Especially for instruction translation, address model, threading, shader state, timing and networking. Repeat the same test before changing the next variable.
- **No destructive cleanup.** Never erase original installs, source references, ignored data or evidence with blanket resets/clean commands. Inspect exact paths before deletion.
- **Protect original inputs.** Work from ignored copies; hash originals again when identity is uncertain. Do not “repair” a mismatch by replacing the recorded expected hash.
- **No data or credential leakage.** Game binaries/maps/generated source/IR/initialized sections, keys/key hashes, saves, packet secrets and memory dumps are never committed or uploaded.
- **No undeclared runtime CPU execution.** A fallback interpreter/JIT/Wine/x86 process means the gate failed. Explicitly authorized reference environments are not the native deliverable.
- **No silent stubs.** Implement real file, event, worker, save, collision, physics, renderer, network and progression semantics. Only a named optional external feature may return its documented absence/failure after inspection and tests.
- **No numeric-progress fiction.** Percent translated, functions compiled and titles rendered are diagnostics, not percentages of a working port. Record reachable unresolved paths and behavior instead.
- **No unchanged third failure.** The same command failing the same way twice requires a new diagnosis or reduced experiment before another run.
- **No unfunded expansion.** Do not create cloud machines, buy licenses, recruit testers, message maintainers, change public servers or alter security settings without authorization.
- **No publication by momentum.** A working local `.app` or `.ipa` remains private until G12. A PRD is not permission to announce a release.

## Executable and native-boundary discipline

- Every generated module is keyed to its exact input SHA-256, source locks, translator/config version and patch set. Custom and retail modules must not share unnamed guest globals, addresses or resource roots.
- An unresolved indirect call/jump is a coverage incident. Log guest target, calling site, current profile, last state transition and expected code region. Add analysis/metadata and regenerate; do not ignore it.
- Every imported/dynamically resolved host call has an inventory entry and tested marshalling. Treat arguments, return values, packed structures, pointers, vtables and callbacks separately.
- Use 32-bit guest values plus checked address translation. Never leak native host pointers into the game by truncation. Test the actual physical-iOS address strategy early, not after a desktop-only implementation becomes entrenched.
- Native worker threads have separate guest CPU/TLS/stack state where required. Events and timeouts must work under actual contention; an immediate-success wait is not a synchronization implementation.
- Use original-client differential evidence for x87/SSE/flags/ABI behavior. No “looks close enough” floating-point patch in collision, damage or serialization.
- Guest code bytes may exist only as inert data where required. All executed CPU control flow goes to statically compiled functions/replacements. GPU pipeline compilation and bounded original game-script interpretation are separate documented subsystems.
- A native replacement requires a named original behavior, profile-specific mapping, provenance/license decision, differential or equivalent regression, and a reason it is smaller/safer than fixing the translation.
- Compiler warnings affecting pointer width, implicit declarations, aliasing, alignment or floating-point behavior are causal evidence. Do not suppress them globally to achieve a green build.

## Network discipline — protect the actual differentiator

- First reproduce the original client/server path. Then compare native behavior against it; never debug a broken original install as if the native client caused it.
- G5 uses an unchanged compatible original dedicated server and at least one original desktop client. Do not replace either with a permissive mock, modified validation or a second HaloPad instance.
- Declare the edition at every step: Custom Edition client/server versus retail client/server. Shared branding, map format support or identical-looking menus do not prove protocol compatibility.
- Store normal authentication credentials privately. Do not randomize key identities, bypass bans/anti-cheat, forge map CRCs or disable integrity checks to pass.
- “Online works” requires observed movement/combat/objectives/vehicles/score, a completed match, map change and reconnect from both perspectives. Server list, ping and handshake are lower-level milestones only.
- Use automation and malformed-packet tests only in owned/controlled environments. On a public service, use normal approved gameplay and respect its rules. No repeated automated join/leave loops or load tests.
- Public server endpoint and player population must be checked when tested. An archived operator page is a destination lead, not proof of today's busy lobby.
- A native UI overlay does not pause an online server. Suspension may disconnect; clear input and report/recover honestly rather than replaying stale actions.

## Campaign and data discipline

- The retail campaign profile is required by G7. A successful Custom Edition match does not remove this requirement. An alternate campaign packaging strategy needs explicit documentation and the same original-content tests.
- A baseline user acquisition is the original Windows Halo PC copy/key plus the correct patch. Do not quietly introduce a Digsite executable, PAL debug build, original Xbox SDK, Mac binary or MCC data because another repo expects it.
- Validate the whole prepared package before replacing a known-good import. Enforce path, size, content and profile boundaries. Do not execute anything from the user archive.
- Track every chapter and checkpoint transition in the fresh-save run. Preserve local fixtures for fast regressions, but never mark the fresh-run requirement complete from a collection of unrelated late-game saves.
- Save tests use disposable backed-up profiles; verify in-game state after relaunch, not only that a file was written. No save data shared across incompatible profiles.

## Unblocking ladder

When blocked, escalate in order and journal each rung used:

1. **Read the first causal failure.** Full tool/build output, runtime log, guest call/branch trace, native crash backtrace, worker state, graphics validation, audio counters and network phase. Do not fix the last cascading error while ignoring the first.
2. **Verify state and identity.** Original input, exact profile, source/dependency locks, generated metadata, build architecture, active data/save and known-good artifact. Eliminate mixed retail/Custom and stale-cache mistakes first.
3. **Check the original behavior.** Reproduce the equivalent operation in the original authorized client. For packet or save differences, capture only the private evidence needed to explain semantics.
4. **Check the relevant reference, not every repo.** SRW/llasm and its actual generated output for lifting; the audited guest-memory mechanism for pointers; version-mapped Halo code for semantics; UTP for Apple surface/lifecycle. A reference must address the named failure.
5. **Read actual implementation paths.** PE frontend, CFG recovery, emitted instruction helper, dispatcher, ABI bridge, file/worker/socket shim, renderer state or save transaction. Inspect the exact pinned source and the generated output together.
6. **Research one unresolved question.** Primary source/code/issues or platform documentation. End the research with an experiment. Do not replace building with repeated broad searches for “Halo iOS.”
7. **Minimize.** One real function; one callback; one indirect branch; one file read; one texture/shader; one controlled packet exchange; one stock map; one checkpoint. Retain real semantics and input identity.
8. **Route around narrowly.** Implement one documented native replacement or finite platform adaptation with a regression test. Never substitute an empty engine, forced success result, skipped objective or forgiving server.
9. **Use the bounded alternate when justified.** At G2, test the second lifter against the same fixtures and actual Halo slices. Keep a comparison record. Do not loop through new frameworks indefinitely or quietly change the target edition/platform.
10. **Park only the blocked path.** Write the repro and take useful independent work that does not falsify the lowest goal: importer tests while awaiting data, native ABI fixtures while awaiting a reference machine, or shell extraction while awaiting physical access. Do not mark a later goal passed if its dependency remains broken.
11. **Stop with a precise handoff for a real external/foundational boundary.** Wrong/unavailable authorized input; unavailable original-client comparison; missing physical/signing action; absent rights/permission decision; both candidate routes failing the native execution contract without a bounded repair; essential unbounded runtime CPU generation; or no accepted compatible community destination. Record what would change the decision.

Ordinary crashes, compile errors, unsupported-but-implementable imports, misrendered effects, save bugs and packet mistakes are not terminal merely because they are hard. Equally, a no-go gate is real: do not keep claiming progress after abandoning the required CPU/network contract.

## Testing rhythm

- **Per instruction/ABI change:** differential fixtures plus actual Halo slice; recheck all relevant guest-width/FP/control-flow cases.
- **Per memory/callback/thread change:** ARM64 host suite, worker contention/shutdown and physical capsule regression where behavior could differ on device.
- **Per renderer change:** the same reference scene/camera, state trace, image and GPU validation. Include first-person/UI and a depth/transparency case, not only static terrain.
- **Per audio change:** speech, weapon/shield, ambient/vehicle loop and transition with clock/underrun evidence.
- **Per network change:** controlled original-client match subset, then transition/reconnect. Re-run public compatibility only when the candidate is stable and testing is appropriate.
- **Per profile/shared-runtime change:** Custom multiplayer regression plus the highest completed retail campaign/save fixture.
- **Per save/import change:** wrong-profile/unsafe-input tests, backup, simulated interrupted write, relaunch and game-visible validation.
- **Per goal claim:** complete the exact PRD rows and evidence before marking it passed.
- **Per session:** run the smallest meaningful suite plus highest known-good end-to-end smoke. Finish with the actual known-good command and next step.
- **Per candidate:** run the applicable full matrix against the exact source/payload/signing identity. Do not combine evidence from different candidates into a fictitious green release.

Use Xcode/Simulator screenshot tools for actual screenshots, not recreated images. Input automation drives the real compiled core through its normal input boundary; it cannot set game objectives or connection results directly. Hands-on requirements remain hands-on.

## Using the existing Apple machinery

Read UTP's pinned source before copying. Port its mechanisms for focus/input clearing, controller ownership, pointer/text routing, safe imports, scoped paths, diagnostics, clean exit and package checks where applicable. Keep Unreal-specific runtime loading, rendering, keys, audio and network assumptions out of HaloPad.

Preserve the supplied SnapPad templates' good operational rules: source locks, local original inputs, generated-output exclusion, scripts rather than terminal folklore, evidence tiers, one candidate instance, clean reproduction and separate public gates. Do **not** port N64 overlays, RSP behavior, RT64 or FlashRAM settings as if they were Halo systems.

Every experiment has a named mode in logs, explicit configuration, a rollback path and a default-stable behavior. A risky patch may not become the default merely because it makes the first screenshot more attractive.

## Session-end checklist

1. Save the causal evidence and stop project candidate processes/Simulators. Stop only reference services this session explicitly owns and no longer needs.
2. Run the relevant regression and highest known-good smoke; otherwise state the exact reason they could not run.
3. Record source/core/input/artifact identity, active save fixture, matrix status, remaining processes and whether any external action is required.
4. Update the technical inventories and `STATUS.md`; append the journal without rewriting earlier failures.
5. Run repository safety checks before a local commit. No push or public upload without authorization.
6. Leave **one unambiguous next experiment** for the lowest unmet goal, including command/interface, expected evidence and fail condition.

## Terminal reports

Use one of these honest terminal states:

- **PRIVATE_TECHNICAL_COMPLETE:** G0–G10 pass; physical/public gates not yet complete. List exact remaining evidence/actions.
- **PHYSICAL_CANDIDATE_ACCEPTED:** G11 passes; publication is still blocked until G12.
- **PUBLIC_RELEASE_AUTHORIZED:** G12 passes for the specifically approved surfaces/artifacts; execute only that authorization.
- **BLOCKED_EXTERNAL:** a named input/reference/device/credential/rights action is needed. Provide a reproducible handoff, not a fabricated pass.
- **NO_GO_CURRENT_CONSTRAINTS:** the documented experiments invalidate the selected native/online route under this brief. Include reproductions, both candidate outcomes where relevant, the smallest changed assumption that would reopen it, and preserved reusable work.

Do not label the initial documents themselves as a completed technical gate. **If it was not run and observed, it is not done.**
