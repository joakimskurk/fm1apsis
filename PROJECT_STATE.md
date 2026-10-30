# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 4 / M4g — Bloom dry engine: implementation complete, final documentation-head gate pending.**

M4f / Spiral is merged as `734229f4d23626a176a8366de82aab2c58c48cc9` via PR #12 after final documentation head `744559f89df733c4c167367efe30250b6dcdecdf` passed core #261 and staged #250.

M4g is active on `m4g-bloom`, draft PR #13, based directly on that M4f merge.

Technically final M4g implementation/CI head:
- `6c54d1cd5ca354543237a14c257b3ec28ab424d1`
- core #292 PASS
- staged #281 PASS through pinned source/core, unchanged Dinghy, M4g host/single-TU overlay, real JieLi `FM-1_972` target, emulator smoke and exact M4f Bloom-bypass gate

No DSP/runtime changes are planned after `6c54d1cd…` unless the final documentation-head gate exposes a concrete defect.

Physical FM-1 hardware has not been tested. No real ISR-cycle-margin claim is made.

## Completed and merged baselines

- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a persistent String: `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7
- M4b finite String + Rhythm: `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8
- M4c String Attractor: `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9
- M4d persistent Drift: `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10
- M4e Clock: `db9d0b3ecbc7931be343cec7717103059d3c5160` via PR #11
- M4f Spiral: `734229f4d23626a176a8366de82aab2c58c48cc9` via PR #12

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural/state/timing/safety authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic exception only:
- Study 98 · Performance Architecture
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- only String features that actually existed there use Study 98 acoustically
- later Study-124/125/126 safety/de-click fixes override older engine mechanics

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

## Frozen shared runtime baseline

M3c remains the only musical scheduler/runtime timeline. M4 dry engines do not add module-private clocks or event queues.

Frozen host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- shared heap 512 events
- canonical queue high-water 352/512
- extreme M3c queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Pinned emulator invariants through M4g:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648 exactly**
- late trace `811c9dc5`

`ci/emulator_smoke.sh` asserts the exact 123,648-sample cursor. Inactive dry engines may not silently steal a 256-frame callback.

## Frozen dry-engine references through M4f

### M4a String
- Classic peak/hash 1477 / `b20610c1`
- Dual peak L/R 823/787; hash `10d23cf4`

### M4b finite String
- 80 logical +32 fade-tail slots
- transient 1309 / `97142d3e`
- BODY 947 / `c1168570`

### M4c String Attractor
- shared heap 415/512
- pending 60/64
- advances/mutations/binds 1470 / 1470 / 2884
- random `b7a9a395`
- late `e777a771`

### M4d Drift
- host 488 / `0aec6bc2`
- target image/RAM 86,544 / 44,760 B
- PCM `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`

### M4e Clock
- STONE 2081 / `edcc4a81`
- GLASS 1880 / `0adf9829`
- SNAP 738 / `04bc514a`
- finite pool stress 23/32, 176 triggers, 0 drops
- target image/RAM 90,684 / 49,128 B
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

### M4f Spiral
Frozen host normal + ASAN/UBSAN:
- THREAD 1230 / `aef6c83e`
- GLASS 1157 / `9efa9bdc`
- REED 967 / `e67a3c15`
- HALO 1256 / `e7ed6161`
- stress: 384 hits, logical high-water 15, legs 30/32, oscillator high-water 90, 0 steals, 0 drops, hash `02be4bc5`

Merged M4f target/emulator baseline:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- default PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; cursor 123,648; late trace `811c9dc5`

## M4g normative authority

Normative reference:
- `docs/reference/M4G_BLOOM_CONTRACT.md`

Study 126 Bloom remains a three-stage source:
1. Phase-3 transport plans the Bloom musical target.
2. Existing M3c occurrence at nominal target−45 ms freezes the voicing plan and occurrence-time acoustic geometry.
3. Existing M3c petal bind at `max(occurrence, petalTarget−50 ms)` resolves target-aware Gravity/tuning and reads current Bloom voice/attack/brightness.

M4g does not add another Bloom scheduler, event, random draw, harmony resolver or pitch resolver.

### Occurrence-frozen acoustic data

For the occurrence-frozen petal count `n`:
- `duration = clamp(length, .65, 6.0) + .060*i` seconds
- `unfurl = clamp(unfurl, 0, 3.6)` seconds
- `targetOffset = i/(n−1) * unfurl` when `n>1`
- `amp = .010 * sourceLevel(bloom) * (1 − .065*i)`
- `pan = ((i/(n−1))*2 − 1) * width` when `n>1`

Length, unfurl, width, source level and these derived values stay frozen after occurrence.

### Bind-late acoustic data

Each petal bind uses the already-resolved `ApsisBloomLateBinding` plus current:
- Bloom voice VEIL / CHOIR / ORGAN / GLASS
- attack
- brightness
- source ON/OFF

A long unfurl may therefore legitimately change acoustic voice family between petals after a user edit.

### Existing M3c semantics remain authoritative

- occurrence-frozen voicing plan
- future-aware ordinary pending Gravity projection with +0.5 ms tolerance
- no Mother-held future projection
- per-petal target-aware Gravity/tuning
- frozen petal count across later DEPTH changes
- no extra random draw or shared queue mutation from M4g

## M4g implemented architecture

Implemented on `6c54d1cd…`:
- pure `apsis_bloom_audio.[ch]` occurrence-time acoustic planner
- external `ApsisBloomAudioRuntime` mirroring the existing 64 M3c Bloom slots/tokens
- M4g runtime wrapper that observes existing M3c occurrence/bind lifecycle only
- fixed-point finite `ApsisBloomDsp` for VEIL / CHOIR / ORGAN / GLASS
- three oscillators maximum per petal
- fixed **136-petal** pool, evidence-driven from legal worst-case stress
- target activation latch: exact frozen M4f callback path remains active until an already-planned Bloom occurrence is due in the current block; once activated, Bloom stays live so pending petals/tails survive later edits/mutes
- no `ApsisRuntime` growth
- no shared Space/Cathedral yet

## M4g frozen host evidence

Core #292 PASS normal + ASAN/UBSAN.

Frozen voice references:
- VEIL: peak **175**, hash `6177470f`
- CHOIR: peak **143**, hash `4b2969f8`
- ORGAN: peak **122**, hash `d54a68d9`
- GLASS: peak **129**, hash `ca1c3127`

Legal worst-case Bloom stress:
- triggers 186
- petal high-water **125/136**
- oscillator high-water **375**
- drops 0
- peak 266
- hash `5881bb36`
- `sizeof(ApsisBloomDsp) = 22,864 B`

The measured 136-petal capacity leaves 11 petals of margin above the 125-petal legal high-water.

## M4g real target/emulator evidence

Staged #281 PASS all gates.

Default M4g target:
- image **100,944 / 581,564 B**
- RAM **87,336 / 98,304 B**
- RAM headroom **10,968 B**
- pool **30,720 / 344,064 B**
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- product `FM-1_972`

Default emulator:
- pinned three-second scenario does not reach a due Bloom occurrence, so the activation latch intentionally preserves the exact M4f callback/audio path
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- peak 856; RMS 144/144; stereo delta 125; mean -2/-2
- queue 28/512; overflow 0; late flags 0; cursor **123,648**; late trace `811c9dc5`

This default emulator proves target integration, boot, baseline audio/runtime preservation and callback-budget invariance. Audible Bloom is verified by source-vector, snapshot/bind, full shared-runtime handoff and finite-renderer host tests plus real target compile/link; physical listening is unverified.

### Exact M4f bypass

With only M4g Bloom DSP/runtime consumption omitted:
- image **95,804 B**
- RAM **58,056 B**
- pool **30,720 B**
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; overflow 0; late flags 0; cursor 123,648; late trace `811c9dc5`

This is exact frozen M4f target/PCM/runtime.

## Exact next action

1. Freeze the remaining root continuity docs and `M4G_BLOOM_CONTRACT.md` on top of technically final implementation head `6c54d1cd5ca354543237a14c257b3ec28ab424d1`.
2. Run **APSIS core** and full **APSIS staged validation** on that exact final documentation head.
3. If both are green, update PR #13 metadata without moving the head, mark it ready and squash-merge with `expected_head_sha` equal to the gated docs head.
4. Create the Phase-5 sounding-source structural-integration branch from the actual M4g merge commit. Do not start shared Space/Cathedral before Phase 5 structural/audio behaviour is gated.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority
- Study 98 remains narrow String acoustic authority only
- one shared Phase-3 scheduler; no module clocks
- `ApsisRuntime` remains 21,928 B
- M3c Spiral/Bloom late binding remains authoritative
- no extra M4g random draws or shared queue ordering changes
- no shared Space/Cathedral during dry-engine work
- inactive audio engines must not reduce the exact emulator cursor
- physical FM-1 and real ISR timing remain unverified
