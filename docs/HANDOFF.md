# HaloPad next-step handoff

Updated 2026-09-26. The 2026-09-06 `BLOCKED_EXTERNAL` state is lifted. Follow [HaloPad-GOAL-LOOP-PHASE2.md](HaloPad-GOAL-LOOP-PHASE2.md).

Next bounded steps, in order:

1. **G0′:** `scripts/check-repo-safety.sh`, then a local commit on `codex/halopad-phase2`. No push.
2. **G1a:** write `scripts/prepare-patched-client.sh`; reproduce the patched files twice in fresh `halopad-*` CrossOver bottles; expect `haloce.exe` SHA-256 `feea46fce285ec071016cf5534abe47ecf36f6cfac8f1973ee6919851ea5a037`. Then build `ref/inputs/custom-original/` with a manifest and accept the hash in the profile with a provenance block. Pass: `inspect-inputs.py` passes for the patched file and fails for 1.00.
3. **G2a/G2b** start as soon as G1a passes: executable audit with relocation recovery, and the Unicorn oracle seeded with CRC32 at `0x59f2a2`.
4. **G1b** (CrossOver baseline) runs alongside: dedicated server first, then the client. If the client needs a key, park G1b and continue.

Stop conditions and external needs are listed in the phase 2 loop. The previous handoff's requirement for an installed, key-activated Windows copy before any translation work no longer applies.

