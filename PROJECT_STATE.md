# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 4 / M4g — Bloom dry engine.**

M4f / Spiral is merged. M4g is active on `m4g-bloom`, draft PR #13, based directly on the actual M4f squash merge `734229f4d23626a176a8366de82aab2c58c48cc9`.

Current M4g head after source freeze:
- `7b404fdeb8c117d68b57381270077c867083cd0f`
- normative reference: `docs/reference/M4G_BLOOM_CONTRACT.md`
- current task: implement and test a pure occurrence-time Bloom acoustic planner before any runtime bridge or renderer

Physical FM-1 hardware has not been tested.

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

M4f final documentation head `744559f89df733c4c167367efe30250b6dcdecdf` passed core #261 and staged #250 before the guarded squash merge.

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural/state/timing/safety authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic exception only:
- Study 98 · Performance Architecture
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- only features that actually existed in Study 98 use it acoustically
- later Study-124/125/126 de-click and pulse-isolation fixes override older engine mechanics

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

## Frozen shared runtime baseline

M3c remains the only scheduler/runtime timeline. M4 dry engines must not add module-private clocks or event queues.

Frozen host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- shared heap 512 events
- canonical queue high-water 352/512
- extreme M3c queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Pinned emulator invariants through M4f:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648 exactly**
- late trace `811c9dc5`

`ci/emulator_smoke.sh` asserts the exact 123,648-sample cursor. A new dry engine may not silently steal one or more 256-frame callbacks while still passing a weaker health check.

## Frozen dry-engine references through M4f

### M4a String
- Classic peak/hash 1477 / `b20610c1`
- Dual peak L/R 823/787; hash `10d23cf4`
- pre-Drift PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

### M4b finite String
- 80 logical + 32 fade-tail slots
- transient 1309 / `97142d3e`
- BODY 947 / `c1168570`

### M4c String Attractor
- shared heap 415/512
- pending 60/64
- advances/mutations/binds 1470 / 1470 / 2884
- random `b7a9a395`
- late `e777a771`
- merged target image/RAM 85,564 / 44,680 B

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
- 8 s worst acoustic stress: 384 hits, logical HW 15, legs 30/32, oscillator HW 90, 0 steals, 0 drops, hash `02be4bc5`

Merged M4f target/emulator baseline:
- image 95,804 B
- RAM 58,056 B
- pool 30,720 B
- package `77035ccdbbe827e80730762ac3b79dd9978f04bcf4adb2b86456198a98213898`
- default source route is empty, so PCM remains exact M4e `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- queue 28/512; cursor 123,648; late trace `811c9dc5`

M4f bypass is exact frozen M4e target/PCM/runtime.

## M4g normative authority

Normative reference:
- `docs/reference/M4G_BLOOM_CONTRACT.md`

Study 126 defines Bloom as a three-stage source:
1. Phase-3 transport plans the musical Bloom target.
2. Existing M3c occurrence at nominal target−45 ms freezes the voicing plan and M4g acoustic geometry.
3. Each existing M3c petal bind at `max(occurrence, petalTarget−50 ms)` resolves target-aware Gravity/tuning and reads current Bloom voice/attack/brightness.

### Occurrence-frozen acoustic data

For the occurrence-frozen petal count `n`:
- `duration = clamp(length, .65, 6.0) + .060*i` seconds
- `unfurl = clamp(unfurl, 0, 3.6)` seconds
- `targetOffset = i/(n−1) * unfurl` when `n>1`
- `amp = .010 * sourceLevel(bloom) * (1 − .065*i)`
- `pan = ((i/(n−1))*2 − 1) * width` when `n>1`

Length, unfurl, width, source level and these derived petal values stay frozen after occurrence.

### Bind-late acoustic data

At each petal bind, M4g must use the already-resolved `ApsisBloomLateBinding` plus current:
- Bloom voice (VEIL / CHOIR / ORGAN / GLASS)
- attack
- brightness
- source ON/OFF

A long unfurl may therefore legitimately change voice family between petals after a user edit.

### Existing M3c semantics that remain authoritative

- occurrence-frozen voicing plan
- future-aware ordinary pending Gravity projection with +0.5 ms tolerance
- no Mother-held future projection
- per-petal target-aware Gravity/tuning
- frozen petal count preserved across later DEPTH changes
- no extra random draw or queue event from M4g

## M4g implementation sequence

1. **Pure acoustic planner** — occurrence-time target offsets, duration, amp and pan only.
2. **External Bloom audio runtime** — mirror existing 64 M3c Bloom slots/tokens; capture on occurrence and pair each existing bind with frozen acoustic data plus M3c late pitch.
3. **Finite Bloom DSP** — fixed-point VEIL/CHOIR/ORGAN/GLASS renderer.
4. **Capacity stress** — measure legal worst case before freezing pool size.
5. **Target integration** — activation latch so default idle callback budget remains unchanged until Bloom actually has audible finite work.
6. **Bloom-DSP bypass** — recover frozen M4f target/PCM/runtime.
7. Freeze docs, exact final-head core + staged gate, guarded squash merge.

## M4g capacity/stress requirement

Before pool capacity is frozen, test at least:
- 180 BPM
- 1/16 division
- max swing
- Bloom cycles 1
- Bloom rate 4
- 5 petals / 9th depth
- unfurl 3.6 s
- length 6 s
- source level 1.25
- worst three-oscillator voice family

Record logical petal high-water, oscillator high-water, steals/drops, deterministic hash and target RAM/image delta. Do not choose capacity by intuition.

## Deliberately deferred after M4g

- shared Space/Cathedral
- final Mother dry/wet integration beyond frozen state behaviour
- native Bloom manual surface/gesture mapping (must later reuse the same acoustic primitive)
- physical FM-1 validation
- real ISR worst-case timing

## Exact next action

1. Implement `apsis_bloom_audio.[ch]` as a pure occurrence-time acoustic planner only.
2. Add source-vector tests for default 4-petal Bloom plus min/max edge values; no runtime or DSP yet.
3. Gate the planner normal + ASAN/UBSAN while all frozen M3/M4 references remain unchanged.
4. Only then add the external M3c Bloom token/snapshot bridge.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority
- Study 98 remains narrow String acoustic authority only
- one shared Phase-3 scheduler; no module clocks
- `ApsisRuntime` remains 21,928 B
- M3c Spiral/Bloom late binding remains authoritative
- M4g must not draw random numbers or alter shared queue ordering
- no shared Space/Cathedral during dry-engine work
- inactive audio engines must not reduce the exact emulator cursor
- no physical-hardware or real ISR timing claim without device evidence
