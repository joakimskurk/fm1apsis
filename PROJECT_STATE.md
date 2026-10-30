# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 6 — shared Space / Cathedral is complete and merged. Phase 7 — persistent KEEP / CHAIN storage is next.**

Merged Phase-6 commit on `main`:
- `cd321e9f7999d0568d280d89e7e3574c3da66054`
- PR #16
- guarded pre-merge head `cfdfff673204d4fa36208508e343c6297a37ff37`

Final pre-merge gates:
- core #424: **PASS**
- staged #413: **PASS all gates**
- P6 final #22: **PASS all gates**

Physical FM-1 hardware remains unavailable. Nothing is promoted to physical-hardware verification.

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behaviour/state/timing/safety authority:
- Study 126 SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic exception only:
- Study 98 LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

## Frozen shared runtime

M3c remains the only musical scheduler/runtime timeline.

Host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- heap 512 events
- canonical queue 352/512; extreme 357/512
- Spiral slots 59/64; Bloom 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Phase-5/M4g pinned emulator baseline:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648**
- late trace `811c9dc5`

Phase-6 integrated boot-to-stop scenario:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,392**
- late trace `811c9dc5`

The Phase-6 cursor is scenario-specific. Both audible and room-return-disabled P6 builds produce 123,392, while older M4g/M4f staged baselines remain 123,648. The shared scheduler itself is unchanged.

## Frozen Phase-6 target

Study 126 defines one shared room fed by five post-source/post-Mother sends:
- String 0.72
- Drift 0.20
- Clock 0.06
- Spiral 0.62
- Bloom 0.58

Shared Space defaults:
- wet 0.48
- decay 8.4 s
- predelay 72 ms
- damping 6200 Hz
- echo 362 ms / 18% feedback

Selected tail: compact 8-line modulated FDN with Hadamard feedback. Host/reference room remains 44.1 kHz. The generated FM-1 target room runs at 11.025 kHz internally; all room-time constants, delay geometry, damping and modulation are retimed for that rate. Source DSP and final output remain 44.1 kHz.

Large Space memories live in a dedicated NOLOAD `.apsis_space_pool` immediately before Dinghy's ordinary `.pool`. Space uses fill/valid counters and never requires a giant boot-time clear.

Real JieLi target build:
- image **107,352 / 581,564 B**
- RAM **87,780 / 98,304 B**
- RAM headroom **10,524 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- product `FM-1_972`

Pinned audible emulator:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- peak 856
- RMS 144/144
- stereo delta 126
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

P6 dry-only regression:
- exact Phase-5 PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,392
- late trace `811c9dc5`

## Exact next action — Phase 7

Implement persistent KEEP / CHAIN project storage without disturbing the frozen musical/runtime/audio engine:
1. freeze the exact persisted data contract for 8 KEEP slots + 32 CHAIN cells;
2. define a versioned CRC-protected record format;
3. select/validate dedicated FM-1 storage outside Dinghy's tiny settings slot;
4. implement two-slot/journal recovery and safe deferred writes outside the audio callback;
5. add desktop corruption/power-loss tests before any target integration.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output stay 44.1 kHz
- target Cathedral quarter-rate implementation preserves real-time parameter roles
- `ApsisRuntime` stays 21,928 B unless explicitly re-approved
- long Space storage stays in dedicated pre-`.pool` NOLOAD memory
- dry-only P6 preserves exact Phase-5 PCM
- M4f/M4g historical bypass gates retain their original 123,648 cursor
- Phase-6 integrated scenario baseline is 123,392 with zero late/overflow flags
- project persistence must be journaled/versioned/CRC-protected and never block audio
- physical FM-1 and real ISR timing remain unverified