# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 4 / M4b — finite dry String transients + shared Rhythm audio.**

Implementation is complete and verified through desktop, ASAN/UBSAN, real JieLi target build and FM-1 emulator. PR #8 is merge-pending only; physical FM-1 hardware has not been tested.

Completed and merged baselines:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a persistent String: `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7

Active work:
- branch `m4b-string-transients`
- PR #8
- base `2ea217f09c143d30385f136e84e2b29d582987a0`
- validated implementation/test head `0749ac86e6162fab190f343730ebc9b133c13e47`
- implementation gates: core #170 PASS; staged #159 PASS
- current task: final documentation-head double gate, then squash-merge

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic authority:
- Study 98 · Performance Architecture where the acoustic feature existed there
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- later features absent from Study 98 remain Study-126 behaviour/acoustics unless explicitly source-resolved otherwise
- Study-124/125/126 de-click and pulse-isolation fixes override older engine mechanics

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

Physical FM-1: **not tested**.

## Frozen M3c runtime baseline

M3c remains the only scheduler/runtime. M4b does not add a String-private clock or event queue.

Frozen host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- canonical queue 352/512
- extreme queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Live emulator invariants:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

## Frozen M4a persistent String baseline

M4a merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0`.

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

Persistent architecture:
- 2 Study-98 Foundation voices
- plus 2 Study-98 Classic sine voices or Study-126 BODY/WIRE
- fixed 44.1 kHz stereo Q15
- transport-gated sustain
- no shared Space

Frozen host references:
- Classic peak 1477, hash `b20610c1`
- Dual peak L/R 823/787, hash `10d23cf4`

Frozen emulator reference:
- peak 317 Q15
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM SHA `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

M4b preserves these references exactly when no transient is active.

## M4b normative authority

Normative reference:
- `docs/reference/M4B_STRING_TRANSIENT_CONTRACT.md`

### M4b1 — finite transient engine

Study-98 `classicStringNote()` acoustic DNA implemented natively:
- normal harmonics 1,2,3,4
- pizz harmonics 1,2,3
- normal roll exponent `1.28 + (1-brightness)*0.58`
- pizz exponent 1.52
- 3 ms stagger per harmonic index
- normal duration multiplier `1 + hi*0.16`
- pizz duration multiplier `1 - hi*0.09`
- first two harmonics triangle, higher harmonics sine
- fundamental at requested pan; upper harmonics use inverted ×0.45 pan
- triangle LP follows `clamp(freq*2.35,1600,16000)`; sine bypasses that filter
- folded current Gravity-root shadow

Current safety overrides retained:
- 16 ms String onset instead of Study-98's older 10 ms onset
- no broadband random-noise String exciter
- no hard scene-gain Clock stepping
- audible voice steals fade with the source-derived 14 ms time constant and remain alive for 105 ms before teardown

Native pool architecture:
- browser logical ceiling: 80 voices
- native fade-tail reserve: 32 additional physical slots
- total fixed physical slots: 112
- no allocation, no `libm`
- pool state is separate from `ApsisStringDsp`; merged M4a sustain struct remains exactly 196 B
- legal 30 ms burst: logical 80 / physical 112, 480 steals, 0 drops

Frozen M4b1 host references:
- partial transient peak 1309, hash `97142d3e`
- BODY peak 947, hash `c1168570`, energy L/R `4176243 / 2564882`
- realistic Classic-composite stress: logical 80 / physical 108, 410 steals, 0 drops
- normal and ASAN/UBSAN agree

### BODY/WIRE finite topology

Later Dual/BODY behaviour remains Study-126 authority. Native BODY transient reuses the already accepted M4a seven-harmonic BODY identity so persistent and finite String voices stay in the same instrument family.

Study-126 topology preserved:
- Classic composite: left BODY at t ×0.66; right WIRE/pizz at t+4 ms ×0.78
- Dual left side remains the longer BODY/bowed role
- Dual right side remains the shorter WIRE/pizz role
- side timbre is determined by side role; BOW/PIZZ changes source level scaling, not the left/right acoustic identity
- Motion uses current Gravity chord material folded into the source range and renders as BODY

## M4b2 — shared Rhythm integration

Phase-3 transport already owned `APSIS_EVENT_STRING_RHYTHM`; M4b consumes it rather than creating another scheduler.

Transport contract:
- target remains Clock pulse +26 ms
- EVERY vs SEQUENCE trigger selection remains transport-owned
- Clock step, absolute pulse and shared flags remain event metadata
- the one source value frozen at planning is BOW/PIZZ/MOTION mode, stored in the event's otherwise-unused `sub_index`
- transport diff versus M4a is therefore only that metadata assignment; Clock/queue/timing logic is otherwise unchanged

Late-read contract at audio bind:
- String sound/level
- Classic vs Dual surface
- current String pitch
- left/right side levels
- current brightness
- current Gravity chord for Motion

Runtime observation:
- `apsis_runtime_process_observed()` passively pre-scans the existing shared heap, calls the unchanged M3c process, and publishes a copy only when M3c actually consumes a due Rhythm event
- observation state lives outside `ApsisRuntime`; the frozen 21,928 B runtime layout is unchanged
- legal Clock spacing proves no more than one Rhythm target can occur in a 256-frame callback

Target audio ordering:
1. update persistent M4a String state
2. render persistent M4a String
3. render already-active finite M4b transients
4. advance shared M3c runtime while observing a due Rhythm event
5. bind any consumed Rhythm event into the finite pool for the next callback

This policy intentionally prevents early audio. It does **not** claim sample-exact event-to-audio application; Phase 5 owns that callback architecture change.

Full-chain proof at 44.1 kHz / 256 frames:
- first source Rhythm target = sample 1147
- callback ending at 1024 contains no event/audio
- callback 1024..1279 consumes target 1147 but renders no new transient early
- next callback starts at 1280 and is the first legal audible block
- handoff = 133 samples late, less than one callback, never early
- frozen BOW event still binds after current live `state.string.rhythm` is changed to OFF, proving planned-mode freeze independently from late-read sound state

## M4b verified implementation evidence

Validated implementation/test head: `0749ac86e6162fab190f343730ebc9b133c13e47`.

Exact code-head gates:
- APSIS core #170: PASS including normal + ASAN/UBSAN and full live handoff test
- staged validation #159: PASS through source/core, unchanged Dinghy, APSIS overlay, real JieLi target and FM-1 emulator

Target #159:
- image 81,752 B / 581,564 B
- image headroom 499,812 B
- RAM 42,648 B / 98,304 B
- RAM headroom 55,656 B
- pool 30,720 B / 344,064 B
- pool headroom 313,344 B
- package bytes 610,110 B
- package SHA `8abc090ffa51efe93b21b4f59f2a40a9ab70a6983a2c5720633baba26fa7163b`

Default emulator #159:
- Rhythm is OFF by default, so the live M4b path remains acoustically idle in smoke
- raw PCM remains bit-identical to M4a: `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- peak 317
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- M3c queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

Audible Rhythm correctness is therefore proven by deterministic host/sanitizer integration tests; the default emulator smoke proves the live target path boots/builds and remains regression-transparent while Rhythm is OFF. Physical device audio is not claimed.

## Deliberately deferred from M4b

### String Attractor excitation

Attractor remains a separate String gate because Study 126 re-reads String pitch/side state near target time and has its own lead/bind/accent behaviour. Do not silently fold it into M4b.

Still unresolved for that slice:
- exact native bind lead on the shared sample timeline
- left/right/Classic Attractor dispatch at bind
- upper-octave accent rule
- target/emulator stress and reference freeze

Also deferred:
- dynamic String bus filter modulation
- shared Space/Cathedral
- physical FM-1 validation and real ISR timing

## Exact next action

1. Finish this documentation-only head (`ROADMAP.md`, `PROJECT_STATE.md`, `TESTING.md`, and contract disposition as needed).
2. Run both workflows on that exact final documentation head.
3. If both are green, update PR #8 metadata only, mark it ready and squash-merge with an expected-head SHA guard.
4. Start the next String slice from the actual M4b merge commit; keep Attractor isolated as its own late-binding gate.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority
- Study 98 is narrow acoustic authority, not a behaviour rollback
- M4a host and idle-emulator references remain immutable when M4b is inactive
- one shared Phase-3 scheduler; no module clocks
- M3c runtime stays 21,928 B unless a later explicitly scoped phase changes it
- KEEP / CHAIN / MOTHER semantics unchanged
- Spiral/Bloom M3c late binding unchanged
- `stepOneResync` OFF by default
- no broadband String noise
- no hard-stop of audible transients
- no shared Space/Cathedral during initial dry source work
- do not claim physical hardware or real ISR timing without device evidence
