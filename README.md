# APSIS for M-VAVE FM-1

Native FM-1 port/adaptation of **APSIS**, with the existing HTML/browser instrument as product and musical reference.

## Product authority

APSIS on FM-1 must preserve the browser version's musical behavior, state, timing, structure, interaction and visual intent wherever the hardware permits. Hardware-specific adaptation is allowed, but this must remain **APSIS on physical hardware**, not a generic synth/groovebox using APSIS names.

Frozen repo contracts and documented exceptions override assumptions from chat.

Primary source authorities:
- Study 126 behavior/state/timing/safety SHA-256: `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- Study 98 LF String acoustic exception SHA-256: `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- pinned Dinghy: `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- product: `FM-1_972`

## Start here

For a new chat or contributor, read in this order:

1. `PROJECT_STATE.md` — current authority, verified/unverified state and exact next action
2. `ROADMAP.md` — phase sequence and active gate
3. `TESTING.md` — what has actually been verified
4. `DECISIONS.md` — durable architecture/product decisions
5. latest phase checkpoint/handoff under `docs/`
6. `CHANGELOG.md` — milestone history

The repository, not chat history, is the durable source of project state.

## Current status

Phases 0–11 are complete. Phase 11 merged to `main` as `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c` via PR #21 after the 18/18 exact-head regression matrix and P11Z closure passed.

**Phase 12 is active and hardware-led.** Active branch: `p12-hardware-validation`. Draft PR: #23.

Phase-12 order:

P12A install/rescue → P12B physical I/O → P12C musical parity → P12D real callback timing → P12E LCD/control/audio contention → P12F stack high-water → P12G real NOR interruption/recovery → P12H DAC/listening → P12I release freeze.

Physical FM-1 hardware is not currently available, so nothing is verification level 6 yet.

## Frozen production authority entering Phase 12

Accepted Phase-11 baseline: **P11W**

- image: **140,472 / 581,564 B**
- RAM: **87,524 / 98,304 B**
- POOL: **319,176 / 344,064 B**
- package bytes: **610,110**
- package SHA-256: `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- idle PCM SHA-256: `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`

See `docs/PHASE11_FINAL_CHECKPOINT.md` and `docs/PHASE12_HARDWARE_VALIDATION_PLAN.md` for the detailed evidence and gate definitions.

## Core invariants

- parity first, optimization second
- one shared musical state; do not create shadow musical state
- one shared scheduler/clock/RNG authority unless explicitly re-approved
- frozen behavior is not rebaselined just to make new code pass
- optimization requires measured evidence
- host, sanitizer, target, emulator and physical-hardware claims remain distinct
- hardware-only claims remain unverified until tested on an actual FM-1

## Current exact next action

When a physical FM-1 is available, execute **P12A install + rescue** against the frozen P11W authority before any destructive persistence test or further production optimization.
