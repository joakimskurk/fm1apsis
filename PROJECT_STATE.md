# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 4 / M4f — Spiral dry engine.**

Implementation is complete and verified through desktop, ASAN/UBSAN, real JieLi target build and FM-1 emulator. PR #12 is merge-pending only after an exact final documentation-head double gate. Physical FM-1 hardware has not been tested.

Completed and merged baselines:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a persistent String: `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7
- M4b finite String + Rhythm: `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8
- M4c String Attractor: `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9
- M4d persistent Drift: `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10
- M4e Clock: `db9d0b3ecbc7931be343cec7717103059d3c5160` via PR #11

Active work:
- branch `m4f-spiral`
- draft PR #12
- base `db9d0b3ecbc7931be343cec7717103059d3c5160`
- technically final implementation/CI head `11569371ea18bc8a5e06f28d09ddef90f9835e29`
- implementation gates: core #260 PASS; staged #249 PASS including real JieLi target, default emulator, exact cursor invariant and Spiral-DSP bypass
- current task: freeze continuity documentation, gate that exact documentation head, then squash-merge PR #12 with expected-head guard

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic authority:
- Study 98 · Performance Architecture only where that acoustic feature existed there
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- later features absent from Study 98 remain Study-126 behaviour/acoustics unless explicitly source-resolved otherwise
- Study-124/125/126 de-click and pulse-isolation fixes override older engine mechanics

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

Physical FM-1: **not tested**.

## Frozen shared runtime baseline

M3c remains the only scheduler/runtime timeline. M4a through M4f do not add module-private clocks or queues.

Frozen host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- canonical queue 352/512
- extreme M3c queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Live emulator invariants through M4f:
- queue 28/512
- overflow 0
- late flags 0
- **cursor 123,648 exactly**
- late trace `811c9dc5`

`ci/emulator_smoke.sh` now freezes the exact 123,648-sample cursor for the pinned three-second emulator scenario. A future dry engine may not silently consume one or more 256-frame callbacks while still passing a weaker non-zero health check.

## Frozen Phase-4 references before M4f

### M4a String
- Classic peak/hash 1477 / `b20610c1`
- Dual peak L/R 823/787; hash `10d23cf4`
- pre-Drift raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

### M4b finite String
- 80 logical +32 fade-tail slots
- transient peak/hash 1309 / `97142d3e`
- BODY peak/hash 947 / `c1168570`

### M4c String Attractor
- shared heap 415/512
- pending 60/64
- advances/mutations/binds 1470 / 1470 / 2884
- random `b7a9a395`
- late `e777a771`
- merged target image/RAM 85,564 / 44,680 B

### M4d Drift
- host peak/hash 488 / `0aec6bc2`
- merged target image/RAM 86,544 / 44,760 B
- raw PCM `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`

### M4e Clock
Merged as `db9d0b3ecbc7931be343cec7717103059d3c5160` via PR #11 after exact final-head core #231 + staged #220 gates.

Frozen host Clock:
- STONE accent 2081 / `edcc4a81`
- GLASS accent 1880 / `0adf9829`
- SNAP 738 / `04bc514a`
- worst legal finite-pool high-water 23/32, 176 triggers, 0 drops

Merged M4e target/emulator baseline:
- image 90,684 B
- RAM 49,128 B
- pool 30,720 B
- package `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856; RMS 144/144; stereo delta 125; mean -2/-2
- queue 28/512; overflow 0; late flags 0; cursor 123,648; trace `811c9dc5`

## M4f normative authority

Normative reference:
- `docs/reference/M4F_SPIRAL_CONTRACT.md`

Study 126 defines Spiral acoustics as a finite prepared voice around the already-frozen M3c occurrence/bind lifecycle. M4f does not create another scheduler event or rederive pitch.

Planning-time acoustic identity freezes:
- Attractor energy
- note duration and noteLength
- Spiral source level
- Drift audible delta and width
- THREAD/GLASS/REED/HALO oscillator topology
- filter Q, attack/tail law
- main/shadow topology and +8 ms shadow offset

The existing M3c bind at target−80 ms remains authoritative for:
- frozen route index resolved against current derived sequence
- Gravity interpretation
- register octave
- tuning mode/strength/deviation

Study 126's edge behaviour is preserved: oscillator topology/Q/envelope are planning-time voice data, while a voice change before bind may change the late filter-cutoff family.

## M4f architecture

M4f adds no private clock, queue or `ApsisRuntime` field.

Native pieces:
- `ApsisSpiralAudioPlan`: source-derived planning-time acoustic snapshot
- `ApsisSpiralAudioRuntime`: external 64-slot token/snapshot bridge keyed to existing M3c Spiral slots/binds
- `ApsisSpiralDsp`: fixed 32-leg finite renderer
- `apsis_runtime_m4f`: wrapper around the frozen M4c/M3c process path; captures only existing Spiral lifecycle data

The +8 ms shadow is local finite-voice delay, not a shared event.

The target has an activation latch:
- while the source-default Spiral route is empty, the callback uses the exact frozen M4e runtime/audio path and performs no Spiral runtime/renderer work;
- the latch turns on only when Spiral has both sound and a non-empty route;
- once active it remains active so already-planned binds and finite tails survive later route/mute edits.

This fixed a measured one-callback default-path regression (123,392 instead of 123,648) without changing Spiral DSP semantics.

## M4f frozen host evidence

All references agree under normal and ASAN/UBSAN:
- THREAD: peak **1230**, hash **`aef6c83e`**, 2 legs / 4 oscillators
- GLASS: peak **1157**, hash **`9efa9bdc`**, 2 legs / 6 oscillators
- REED: peak **967**, hash **`e67a3c15`**, 2 legs / 6 oscillators
- HALO: peak **1256**, hash **`e7ed6161`**, 2 legs / 6 oscillators

Eight-second worst acoustic stress:
- 384 logical hits
- logical high-water **15**
- leg high-water **30/32**
- oscillator high-water **90**
- steals **0**
- drops **0**
- deterministic hash **`02be4bc5`**

All frozen M3c/M4a–M4e host references remain unchanged.

## M4f target/emulator evidence

Technically final head: `11569371ea18bc8a5e06f28d09ddef90f9835e29`.

Exact gates:
- core #260: PASS normal + ASAN/UBSAN + M4f planning/bridge/renderer suites
- staged #249: PASS source/core, unchanged Dinghy, APSIS overlay, real JieLi target, default emulator, exact cursor invariant and Spiral-DSP bypass

Default M4f target:
- image **95,804 / 581,564 B**
- RAM **58,056 / 98,304 B**
- pool **30,720 / 344,064 B**
- RAM headroom **40,248 B**
- package bytes 610,110 B
- package SHA **`77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`**
- product `FM-1_972`

Default emulator:
- route is source-default empty, so no Spiral voice is expected in this scenario
- frames 124,928; stereo
- peak **856**
- RMS **144/144**
- stereo delta **125**
- mean **-2/-2**
- raw PCM **`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`** (exact frozen M4e PCM)
- queue 28/512; overflow 0; late flags 0
- cursor **123,648 exactly**
- late trace `811c9dc5`

Because default `spiral.route_count == 0`, the emulator smoke proves target integration, idle callback budget and unchanged baseline, but it does **not** constitute an audible target Spiral listening test. Audible Spiral topology/timing is verified by deterministic host planning/bridge/renderer/full-chain tests plus real JieLi compile/link. Physical hardware remains unavailable.

### Exact M4e bypass proof

Staged Gate 5b rebuilds with only the Spiral DSP/runtime consumer omitted.

Bypass:
- image **90,684 B**
- RAM **49,128 B**
- pool **30,720 B**
- package **`74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`**
- PCM **`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`**
- queue 28/512; overflow 0; late flags 0; cursor 123,648; trace `811c9dc5`

This is bit-identical to frozen M4e target/PCM/runtime.

## Deliberately deferred after M4f

- Bloom dry engine (M4g)
- shared Space/Cathedral
- final Mother dry/wet integration beyond frozen state behaviour
- native Spiral UI/control mapping
- physical FM-1 validation
- real ISR worst-case timing

## Exact next action

1. Freeze root continuity docs plus the M4f contract in one documentation commit on `m4f-spiral`.
2. Gate that exact final documentation head with both `APSIS core` and `APSIS staged validation`.
3. If both pass, update PR #12 metadata only; do not move the head.
4. Mark PR #12 ready and squash-merge with `expected_head_sha` equal to the gated docs head.
5. Create M4g/Bloom from the actual M4f merge commit and source-resolve Bloom acoustics before DSP work.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority
- Study 98 remains narrow String acoustic authority only
- one shared Phase-3 scheduler; no module clocks
- `ApsisRuntime` stays 21,928 B unless an explicitly scoped later phase changes it
- M3c Spiral/Bloom late binding remains authoritative
- M4f planning-time acoustic snapshot must not consume RNG or alter queue ordering
- +8 ms Spiral shadow remains DSP-local
- inactive dry engines must not silently reduce the frozen emulator cursor
- no shared Space/Cathedral during initial dry-source work
- do not claim physical hardware or real ISR timing without device evidence
