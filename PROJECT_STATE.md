# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 5 — sounding-source structural integration: technically complete; final documentation-head double gate pending.**

Active branch: `p5-sounding-integration`  
Draft PR: #15  
Base: merged M4g `713a1e9ac746a1a4d19ba825a0e6765b337ba410`  
Technically final Phase-5 verification head before documentation freeze: `de3a0bdfc0831db502c186ca8808c46b46ee5c1a`.

Phase 5 required **no production scheduler/DSP/runtime change**. The existing M3c shared runtime plus M4f/M4g sounding bridges already satisfy the source-defined structural semantics. Phase 5 therefore closes through explicit end-to-end sounding verification rather than manufacturing a production diff.

Implementation-head evidence:
- APSIS core #357 PASS, normal + ASAN/UBSAN
- APSIS staged validation #346 PASS through pinned source/core, unchanged Dinghy, M4g single-TU overlay, real JieLi `FM-1_972` target, emulator smoke and exact M4f Bloom-bypass gate
- all frozen M3/M4 references remain unchanged

Physical FM-1 hardware is still unavailable. No real ISR-cycle-margin or hardware-listening claim is made.

## Completed sounding Phase-5 slices

- **P5a — structural late binding:** immediate structural edits between occurrence and bind reach actual sounding Spiral/Bloom DSP while occurrence-frozen acoustic data and target time remain unchanged.
- **P5b — quantized Gravity:** the deliberate source asymmetry is audible: Spiral does not project ordinary pending Gravity; Bloom may project it target-aware under the +0.5 ms rule.
- **P5c — Mother `->1`:** Mother-held Gravity remains inaudible to future-bound sources until the actual `->1` commit.
- **P5d — Chain discrete:** Chain boundary state reaches sounding Spiral/Bloom in source order.
- **P5e — Chain morph:** continuous Drift morph begins on the musical target and reaches the sounding Drift DSP.
- **P5f — step-one free-run/resync:** RESYNC OFF preserves free-running Spiral attractor phase into sounding energy; source attractor identity is read from the frozen M3c slot rather than duplicated into the M4f acoustic snapshot.

Normative starting contract: `docs/reference/P5A_SOUNDING_LATE_BINDING_CONTRACT.md`.

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural/state/timing/safety authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic exception only:
- Study 98 · Performance Architecture
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- only String features that existed there use Study 98 acoustically
- later Study-124/125/126 safety/de-click fixes override older engine mechanics

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

## Frozen shared runtime baseline

M3c remains the only musical scheduler/runtime timeline.

Host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- shared heap 512 events
- canonical queue 352/512; extreme 357/512
- Spiral slots 59/64; Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Pinned emulator invariants:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648 exactly**
- late trace `811c9dc5`

No Phase-5 slice changed these values.

## M4g / Phase-5 real target baseline

The Phase-5 staged build is byte-for-byte the merged M4g firmware because Phase 5 is verification-only:
- image **100,944 / 581,564 B**
- RAM **87,336 / 98,304 B**
- static RAM headroom **10,968 B**
- pool **30,720 / 344,064 B**
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- product `FM-1_972`

Pinned default emulator:
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856; RMS 144/144; stereo delta 125; mean -2/-2
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`

M4g Bloom bypass remains exact frozen M4f:
- image/RAM/pool 95,804 / 58,056 / 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- same PCM/runtime invariants as M4f

## Completed merged baselines

- M3a transport `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a String persistent `2ea217f09c143d30385f136e84e2b29d582987a0`
- M4b String finite/Rhythm `c96c00debf9a67948f1e874e36b090999dacadb9`
- M4c String Attractor `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`
- M4d Drift `d7e39e2481c33628f402bed51d761b8be8304bb3`
- M4e Clock `db9d0b3ecbc7931be343cec7717103059d3c5160`
- M4f Spiral `734229f4d23626a176a8366de82aab2c58c48cc9`
- M4g Bloom `713a1e9ac746a1a4d19ba825a0e6765b337ba410`

## Phase 6 constraint

The next code phase is **shared Space / Cathedral**, not more Phase-5 scheduler work.

Because only **10,968 B static RAM headroom** remains on the current target, Phase 6 must be prototype-first and memory-conscious:
- compare compact modulated FDN and nested allpass/Schroeder candidates;
- preserve one shared room with five per-source sends;
- do not allocate a large unconditional delay/reverb BSS before target memory evidence;
- do not sacrifice parity simply to hide memory use without a measured alternative.

## Exact next action

1. Finish the Phase-5 continuity/documentation freeze on top of verified head `de3a0bdfc0831db502c186ca8808c46b46ee5c1a`.
2. Run **APSIS core** and full **APSIS staged validation** on the exact final documentation head.
3. If both are green, update PR #15 metadata only, mark it ready and squash-merge with `expected_head_sha` equal to that gated documentation head.
4. Create the Phase-6 Space/Cathedral branch from the actual Phase-5 squash merge commit.
5. Do not start implementation of a room algorithm before the Phase-6 source/topology contract and target-memory prototype gate are written.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority.
- Study 98 remains narrow String acoustic authority only.
- one shared Phase-3 scheduler; no module-private clocks.
- `ApsisRuntime` remains 21,928 B unless an explicitly scoped later decision changes it.
- M3c Spiral/Bloom late binding remains authoritative.
- Phase 5 adds no production random draw, queue event, DSP pool or persistent BSS.
- inactive engines may not reduce the exact emulator cursor.
- shared Space/Cathedral is Phase 6 and must be one shared return, not per-source reverbs.
- physical FM-1 and real ISR timing remain unverified.
