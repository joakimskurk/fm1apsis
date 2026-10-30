# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 4 / M4e — Clock dry engine.**

Implementation is complete and verified through desktop, ASAN/UBSAN, real JieLi target build and FM-1 emulator. PR #11 is merge-pending only after an exact final documentation-head double gate. Physical FM-1 hardware has not been tested.

Completed and merged baselines:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a persistent String: `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7
- M4b finite String + Rhythm: `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8
- M4c String Attractor: `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9
- M4d persistent Drift: `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10

Active work:
- branch `m4e-clock`
- PR #11
- base `d7e39e2481c33628f402bed51d761b8be8304bb3`
- validated implementation head `285aacb5309fd9abaa51814e4ea6a09a3e480a10`
- implementation gates: core #225 PASS; staged #214 PASS including real JieLi target, Clock-on emulator and dedicated Clock-DSP bypass regression
- current task: freeze documentation, exact final documentation-head double gate, then squash-merge with expected-head guard

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

M3c remains the only scheduler/runtime timeline. M4a through M4e do not add module-private clocks or queues.

Frozen host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- canonical queue 352/512
- extreme M3c queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Live emulator invariants through M4e:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

M4c appends `APSIS_EVENT_STRING_ATTRACTOR_BIND` after the frozen M3c range while keeping Attractor state external to `ApsisRuntime`. M4d adds no event. M4e consumes the already-existing `CLOCK_MAIN`, `CLOCK_SNAP`, `CLOCK_INNER` and `CLOCK_GHOST` events passively and adds no scheduler event type.

## Frozen Phase-4 references before M4e

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
- final target image/RAM 85,564 / 44,680 B
- package `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`

### M4d Drift
Merged as `d7e39e2481c33628f402bed51d761b8be8304bb3` via PR #10. Final pre-merge documentation head `82106c33…` passed core #214 + staged #203.

Normative reference: `docs/reference/M4D_DRIFT_CONTRACT.md`.

Frozen host Drift:
- peak 488
- hash `0aec6bc2`

Final M4d target/emulator:
- image 86,544 B
- RAM 44,760 B
- pool 30,720 B
- package SHA `394bda661be55021b2b520c9f2490cb93a96446a118723162e60d26dd4a0a7d4`
- peak 433; RMS 135/135; stereo delta 125; mean -3/-3
- raw PCM `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`
- queue 28/512; overflow 0; late flags 0; cursor 123,648; trace `811c9dc5`

## M4e normative authority

Normative reference:
- `docs/reference/M4E_CLOCK_CONTRACT.md`

Study 126 defines four Clock timbres and one shared event topology:
- STONE
- WOOD
- GLASS
- SOFT
- pitch 0.50×..2.00× in 0.01 steps
- MAIN at pulse target
- SNAP at pulse +4 ms
- INNER at half the current swung interval
- GHOST at that same half interval when the already-frozen shared random decision schedules it

M4e corrects the old Phase-2 acoustic placeholder from STONE-only / integer pitch to four voice IDs plus `pitch_centi=50..200`, default 100. KEEP still stores only Clock geometry; timbre/pitch remain live global sound state.

## M4e Clock architecture

Native `ApsisClockDsp` is external to `ApsisRuntime`:
- fixed 32-slot finite pool, 44.1 kHz stereo Q15
- passive observation of already-planned shared Clock events
- sample delay inside the callback preserves original event target
- deterministic first-block bootstrap only for deterministic pulse-0 Clock events before the shared queue has been filled; no transport mutation and no RNG draw
- source-derived STONE/WOOD/GLASS/SOFT pitch/gain/duration profiles
- GLASS accent extra overtone
- source-derived Snap: three filtered/panned noise bursts plus pitched square edge
- fixed-point exponential pitch/gain trajectories
- Clock-local deterministic Snap-noise generator; no consumption of M3c/M4c shared musical RNG
- no dynamic allocation and no target `libm`
- no Clock dynamics routed through String, Drift or shared master gain

Study 126's browser Snap buffer is filled once with global unseeded `Math.random()` and then reused. Because that same global RNG also controls musical systems, native M4e explicitly treats Snap-buffer random consumption as unstable engine leakage rather than musical timing semantics. Native Snap noise is local and deterministic while preserving the one-buffer/reused-realization acoustic relationship.

## M4e frozen host evidence

Frozen deterministic references, identical normal + ASAN/UBSAN:
- STONE accent: peak **2081**, hash **`edcc4a81`**
- GLASS accent: peak **1880**, hash **`0adf9829`**
- SNAP: peak **738**, hash **`04bc514a`**

Worst legal Clock stress:
- GLASS
- pitch 2.00×
- source/inner/snap levels 1.25
- Snap tone 8500 Hz
- 180 BPM, 1/16 division, maximum swing
- every main/inner/snap pulse enabled
- every main treated as accent
- every eligible Ghost forced on

Measured result:
- high-water **23/32** physical voices
- 176 triggers
- 0 drops

## M4e JieLi target compatibility

The host DSP uses exact integer 64-bit division when deriving fixed-point filter/envelope coefficients. JieLi pi32v2 links the target without libgcc's DI-mode division helpers, causing the first M4e target attempt to fail on `__divdi3` even though host/ASAN were green.

M4e therefore supplies `apsis_target_div64.c` only in the generated target overlay:
- standard ABI `__divdi3` and `__udivdi3`
- shift/compare/subtract restoring division
- no `/`, `%`, allocation or state
- preserves the exact C integer-division semantics used by the frozen host DSP rather than changing Clock mathematics to satisfy the linker

This is target compatibility, not an audio-model simplification.

## M4e implementation evidence

Validated implementation head: `285aacb5309fd9abaa51814e4ea6a09a3e480a10`.

Exact gates:
- core #225: PASS normal + ASAN/UBSAN
- staged #214: PASS source/core, unchanged Dinghy, APSIS overlay, real JieLi target, default emulator and Clock-DSP bypass

Default M4e target:
- image **90,684 / 581,564 B**
- image headroom 490,880 B
- RAM **49,128 / 98,304 B**
- RAM headroom 49,176 B
- pool **30,720 / 344,064 B**
- pool headroom 313,344 B
- package bytes 610,110 B
- package SHA `74bd7f01a7fef75985350be04e721acda0e49075b9a47b8b2bd4eb170924200b`

M4e cost versus M4d default target:
- +4,140 B app image
- +4,368 B RAM
- +0 B pool

Default Clock-on emulator:
- frames 124,928
- peak **856**
- RMS L/R **144/144**
- stereo delta **125**
- mean L/R **-2/-2**
- raw PCM **`6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`**
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

The PCM change is expected because Clock sound is ON by default and all 12 main pulses are enabled by default. Runtime telemetry remains frozen.

### Exact M4d audio/runtime bypass proof

Staged Gate 5b regenerates the same target with only the new Clock DSP/consumer omitted. Source-correct Clock state representation remains present.

Bypass target/emulator:
- image **86,544 B**
- RAM **44,760 B**
- pool **30,720 B**
- package SHA `3670d58da19a1ccad01a142ff328b6faf273901450b61b361dd436ad5041990f`
- raw PCM **`539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`**
- peak 433; RMS 135/135; stereo delta 125; mean -3/-3
- queue 28/512; overflow 0; late flags 0; cursor 123,648; trace `811c9dc5`

Image/RAM/pool and PCM/runtime return exactly to the frozen M4d values. Package SHA is not required to match M4d because M4e legitimately changes Clock acoustic state representation even when the DSP layer is omitted.

## Deliberately deferred after M4e

- Spiral dry engine
- Bloom dry engine
- changing/removing historical Drift→String/Spiral cross-couplings
- shared Space/Cathedral
- physical Clock controls / final native UI mapping
- physical FM-1 validation
- real ISR worst-case timing

Next Phase-4 dry engine in roadmap order: **Spiral**, after M4e is actually merged.

## Exact next action

1. Finish/freeze continuity documentation on the M4e branch.
2. Gate that exact final documentation head with both workflows.
3. If both are green, update PR #11 metadata only; do not move the head.
4. Mark PR #11 ready and squash-merge with `expected_head_sha` equal to that gated docs head.
5. Start Spiral from the actual M4e merge commit.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority
- Study 98 remains narrow String acoustic authority, not a behaviour rollback
- one shared Phase-3 scheduler; no module clocks
- M3c runtime stays 21,928 B unless a later explicitly scoped phase changes it
- KEEP / CHAIN / MOTHER semantics unchanged
- Spiral/Bloom M3c late binding unchanged
- M4c Attractor shared-heap order/capacity unchanged
- M4e Clock consumes existing shared events and never draws another Ghost random value
- Snap-noise RNG stays Clock-local and deterministic
- Clock DSP bypass must preserve frozen M4d PCM/runtime
- `stepOneResync` OFF by default
- no broadband String noise
- no hard-stop of audible sources
- no shared Space/Cathedral during initial dry-source work
- do not claim physical hardware or real ISR timing without device evidence
