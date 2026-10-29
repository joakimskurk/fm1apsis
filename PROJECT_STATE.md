# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 4 / M4d — persistent dry Drift engine.**

Implementation is complete and verified through desktop, ASAN/UBSAN, real JieLi target build and FM-1 emulator. PR #10 is merge-pending only after an exact final documentation-head double gate. Physical FM-1 hardware has not been tested.

Completed and merged baselines:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a persistent String: `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7
- M4b finite String + Rhythm: `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8
- M4c String Attractor: `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9

Active work:
- branch `m4d-drift`
- PR #10
- base `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`
- validated implementation head `0b2a351b17adfa7556c217f24af8a08c1c708d2c`
- implementation gates: core #210 PASS; staged #199 PASS including dedicated Drift-DSP bypass regression
- current task: exact final documentation-head double gate, then squash-merge with expected-head guard

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

## Frozen M3c runtime baseline

M3c remains the only scheduler/runtime timeline. M4a/M4b/M4c/M4d do not add module-private clocks or queues.

Frozen host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- canonical queue 352/512
- extreme M3c queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Live emulator invariants through M4d:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

M4c appends `APSIS_EVENT_STRING_ATTRACTOR_BIND` after existing event values but keeps Attractor state/telemetry external to the frozen runtime layout. M4d adds no runtime event at all.

## Frozen String baselines

### M4a persistent String

Merged as `2ea217f09c143d30385f136e84e2b29d582987a0`.

Host references:
- Classic peak 1477, hash `b20610c1`
- Dual peak L/R 823/787, hash `10d23cf4`

Frozen pre-Drift emulator reference:
- peak 317
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

### M4b finite String + Rhythm

Merged as `c96c00debf9a67948f1e874e36b090999dacadb9`.

Frozen finite engine:
- 80 logical voices +32 fade-tail reserve = 112 physical slots
- transient peak/hash 1309 / `97142d3e`
- BODY peak/hash 947 / `c1168570`
- legal burst 80/112, 480 steals, 0 drops
- Classic-composite stress 80/108, 410 steals, 0 drops
- `ApsisStringDsp` remains 196 B

Rhythm uses the existing shared `APSIS_EVENT_STRING_RHYTHM`; first reference target 1147 becomes audible at callback start 1280, 133 samples late and never early.

### M4c String Attractor

Merged as `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc` via PR #9.
Final docs head `fe53bf265882369e2c8c1e38899e9cc655301dd6` passed core #202 and staged #191.

Normative reference:
- `docs/reference/M4C_STRING_ATTRACTOR_CONTRACT.md`

Frozen M4c stress:
- shared heap 415/512
- pending 60/64
- completed-bind output max 6 in deliberate 512-frame stress
- advances/mutations/binds 1470 / 1470 / 2884
- random `b7a9a395`
- late `e777a771`
- zero overflow/starvation

Full-chain proof:
- target 36,750
- bind callback ends 34,816
- remaining exact pool delay 1,934 samples
- voice age starts on target sample

Final M4c target:
- image 85,564 B
- RAM 44,680 B
- pool 30,720 B
- package SHA `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`
- Attractor OFF preserves raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

## M4d normative authority and source disposition

Normative reference:
- `docs/reference/M4D_DRIFT_CONTRACT.md`

Current Study 126 contains an explicit wiring inconsistency:
- `beatDelta()` and `beatPair()` still define the identity-bearing two-frequency field;
- `probeBeat()` can audition two sine oscillators with `.013 * sourceLevel('beat')` gain and 25 ms pitch/gain smoothing;
- the transport path no longer instantiates that pair as an independent persistent `beat` source;
- Drift delta remains cross-coupled into already-frozen String/Spiral behavior.

The project port contract already defines DRIFT as an independent paired beating field around the Gravity root. M4d therefore treats the missing transport voice as a source wiring defect and restores the persistent pair without removing the existing cross-couplings in this slice.

Earlier source history provides the surviving `STEREO WIDTH` law: pan `-0.34*width / +0.34*width`.

## M4d persistent Drift architecture

Native `ApsisDriftDsp` is separate from String and from `ApsisRuntime`:
- two fixed persistent sine voices;
- carrier = live, scale/tuning-aware current Gravity root;
- base delta `0.05 + 3.95 * amount^1.65` Hz using the existing 257-point fixed table;
- audible delta scales by `max(0.15, depth)` and current Drift level;
- each oscillator gain = `.013 * level`;
- source-derived width = ±0.34×width with equal-power panning;
- 25 ms pitch/gain/pan smoothing;
- gain smoothing uses fractional internal precision so low target gain does not shorten the time constant;
- transport-gated persistent source;
- no allocation, no `libm`, no events, no private queue, no RNG draws;
- dry only; shared Space/Cathedral remains deferred.

Frozen host reference, 2 s default Drift:
- peak **488 Q15**
- hash **`0aec6bc2`**
- normal + ASAN/UBSAN identical.

Host tests also prove 0.05/4.00-Hz endpoints, 0.15 depth floor, symmetric pair, Equal/Just/Pyth tuning response, tuning-strength behavior, exact center width, width stereo symmetry, 25 ms gain time constant and click-safe mute tail.

## M4d implementation evidence

Validated implementation head: `0b2a351b17adfa7556c217f24af8a08c1c708d2c`.

Exact gates:
- core #210: PASS normal + ASAN/UBSAN
- staged #199: PASS real JieLi target + default FM-1 emulator + dedicated Drift-DSP bypass regression

Default M4d target:
- image **86,544 / 581,564 B**
- image headroom 495,020 B
- RAM **44,760 / 98,304 B**
- RAM headroom 53,544 B
- pool **30,720 / 344,064 B**
- pool headroom 313,344 B
- package bytes 610,110 B
- package SHA `394bda661be55021b2b520c9f2490cb93a96446a118723162e60d26dd4a0a7d4`

M4d cost versus M4c:
- +980 B app image
- +80 B RAM
- +0 B pool

Default Drift-on emulator:
- frames 124,928
- peak 433
- RMS L/R 135/135
- stereo delta 125
- mean L/R -3/-3
- raw PCM `539e4192134d62d5f941ea21111157f763e92d0db5007f8157ca023db7c975f7`
- queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

The PCM change is expected because Drift is ON by default. Runtime telemetry remains frozen.

### Exact M4c bypass proof

Changing `state.drift.sound` is not an isolation test because pre-M4d String/Spiral behavior already consumes Drift state. Staged Gate 5b instead regenerates the same `0.1-dev` target with only the new `ApsisDriftDsp` layer omitted.

Bypass result:
- image **85,564 B**
- RAM **44,680 B**
- pool **30,720 B**
- package SHA **`c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`**
- raw PCM **`10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`**
- peak 317; RMS 101/100; stereo delta 63; mean -2/-2
- queue 28/512; overflow 0; late flags 0; trace `811c9dc5`.

The bypass package SHA itself equals the frozen final M4c package SHA. This proves the previous target is bit-identical when only the new Drift DSP is removed.

## Deliberately deferred after M4d

- changing/removing the historical Drift→String/Spiral cross-couplings
- shared Space/Cathedral
- physical Drift controls / final native UI mapping
- physical FM-1 validation
- real ISR worst-case timing

Next Phase-4 dry engine in roadmap order: **Clock**, after M4d is actually merged.

## Exact next action

1. Run both workflows on this documentation-only final M4d head.
2. If both are green, update PR #10 metadata only; do not move the head.
3. Mark PR #10 ready and squash-merge with `expected_head_sha` equal to that gated docs head.
4. Start Clock from the actual M4d merge commit.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority
- Study 98 remains narrow String acoustic authority, not a behaviour rollback
- one shared Phase-3 scheduler; no module clocks
- M3c runtime stays 21,928 B unless a later explicitly scoped phase changes it
- KEEP / CHAIN / MOTHER semantics unchanged
- Spiral/Bloom M3c late binding unchanged
- M4c Attractor shared-heap order/capacity unchanged
- prior String/runtime target must remain bit-identical under the M4d DSP bypass gate
- `stepOneResync` OFF by default
- no broadband String noise
- no hard-stop of audible sources
- no shared Space/Cathedral during initial dry source work
- do not claim physical hardware or real ISR timing without device evidence
