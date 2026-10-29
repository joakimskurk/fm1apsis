# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 4 / M4c — String Attractor excitation on the shared timeline.**

Implementation is complete and verified through desktop, ASAN/UBSAN, real JieLi target build and FM-1 emulator. PR #9 is merge-pending only; physical FM-1 hardware has not been tested.

Completed and merged baselines:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a persistent String: `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7
- M4b finite String + Rhythm: `c96c00debf9a67948f1e874e36b090999dacadb9` via PR #8

Active work:
- branch `m4c-string-attractor`
- PR #9 (draft until final docs gate)
- base `c96c00debf9a67948f1e874e36b090999dacadb9`
- validated implementation head `e2171caac10eb402e97a6d566eff71a27544e3fc`
- implementation gates: core #198 PASS; staged #187 PASS
- current task: final documentation-head double gate, then squash-merge with expected-head guard

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

M3c remains the only scheduler/runtime timeline. M4a/M4b/M4c do not add module-private clocks or queues.

Frozen host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- canonical queue 352/512
- extreme M3c queue 357/512
- Spiral slots 59/64
- Bloom slots 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Live default emulator invariants:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`

M4c appends `APSIS_EVENT_STRING_ATTRACTOR_BIND` after existing event values but does not extend the frozen `ApsisRuntime.event_count[]`; Attractor state/telemetry is external.

## Frozen String audio baselines

### M4a persistent String

Merged as `2ea217f09c143d30385f136e84e2b29d582987a0`.

Host references:
- Classic peak 1477, hash `b20610c1`
- Dual peak L/R 823/787, hash `10d23cf4`

Default emulator reference:
- peak 317
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

### M4b finite String + Rhythm

Merged as `c96c00debf9a67948f1e874e36b090999dacadb9`.

Frozen finite engine:
- 80 logical voices +32 fade-tail physical reserve = 112 slots
- M4b partial transient peak/hash 1309 / `97142d3e`
- BODY peak/hash 947 / `c1168570`
- legal burst 80/112, 480 steals, 0 drops
- Classic-composite stress 80/108, 410 steals, 0 drops
- M4a sustain struct remains 196 B

Rhythm integration:
- consumes existing `APSIS_EVENT_STRING_RHYTHM`
- event target remains Clock pulse +26 ms
- BOW/PIZZ/MOTION mode freezes in event metadata
- pitch/surface/levels/brightness and Motion Gravity harmony are late-read at bind
- passive observer stays outside `ApsisRuntime`
- first reference target 1147 becomes audible at callback start 1280: 133 samples late and never early

M4b target/emulator reference:
- image 81,752 B
- RAM 42,648 B
- pool 30,720 B
- package SHA `8abc090ffa51efe93b21b4f59f2a40a9ab70a6983a2c5720633baba26fa7163b`
- default Rhythm OFF remains exact M4a PCM `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`

Normative reference:
- `docs/reference/M4B_STRING_TRANSIENT_CONTRACT.md`

## M4c normative authority

Normative reference:
- `docs/reference/M4C_STRING_ATTRACTOR_CONTRACT.md`

M4c is a distinct late-bind String slice because Study 126 freezes some values during Clock lookahead but re-reads acoustic state close to target time.

### Planning-time contract

Frozen when `clockStringFilter()` plans the occurrence:
- target sample
- S&H position/value/gate
- base strum level
- Classic vs Dual route
- LEFT/RIGHT/ALTERNATE/BOTH route
- BOTH ×0.78 side scaling

Planner behavior:
- seeded local LCG reset from `filterAttractorSeed`
- max 16 cells
- legal rates 0.5/1/2/4/8
- 0.5 creates pulse + half-pulse targets on the same shared heap
- FILTER-only still advances/mutates cells
- SUSTAIN produces no Attractor draw/event
- shared RNG order when active: Attractor -> Spiral -> Clock ghost
- inactive/default Attractor consumes no extra shared draw/event

### Native bind/runtime contract

Native fixed lead: 45 ms, matching Study-126 normal/wide source path.

At bind re-read:
- String sound
- source level
- selected side level
- selected side frequency from current String/Gravity/tuning state
- current brightness as actually consumed by the selected acoustic path
- Foundation/root state used by WIRE

External runtime:
- 64 pending side slots
- six completed-binding outputs per process for legal 512-frame stress burst
- Attractor-specific telemetry outside `ApsisRuntime`
- no private scheduler or queue

Worst legal 60-second M4c1 stress:
- 180 BPM, 1/4, max swing, 0.5 rate, BOTH, all gates, maximal mutation
- concurrent Rhythm + Spiral rate4 + Bloom + Clock
- shared heap 415/512
- pending 60/64
- advances 1470
- mutations 1470
- binds 2884
- random trace `b7a9a395`
- late trace `e777a771`
- zero pending/binding/heap/Spiral/Bloom overflow/starvation
- normal + ASAN/UBSAN agree

### M4c acoustic topology

Dual left/BODY:
- 720 ms
- pan -0.30
- 30 ms attack
- current brightness −0.04 into bowed BODY path
- no audible root shadow; Study-126 `rootGain=.08` option is unused by bowed `stringSolo()`

Dual right/WIRE:
- 420 ms
- pan +0.30
- input ×0.92 into frozen M4b pizz primitive
- Foundation root contribution .05
- actual audible brightness remains current base `state.string.brightness`; the source-computed +.24 `opts.brightness` argument is unused by `classicStringNote()`

Classic:
- BODY at target ×.66
- WIRE at target+4 ms ×.78

Right accent for frozen `norm > .52`:
- sine at 2× frequency
- right input level ×.15 before WIRE ×.92
- +6 ms from right leg; Classic total +10 ms
- 176.4 ms duration
- pan +.216
- 16 ms String onset
- same global 80+32 pool

### Full-chain timing proof

`test_string_attractor_live` proves shared lookahead -> bind -> late-read -> pool delay:
- frozen target 36,750
- bind-processing callback ends 34,816
- remaining delay 1,934 samples
- voice age remains zero for that full delay
- first voice-age sample is the target sample itself
- planned LEFT route survives later live change to Classic/RIGHT while pitch/level re-read at bind

## M4c implementation evidence

Validated implementation head: `e2171caac10eb402e97a6d566eff71a27544e3fc`.

Exact gates:
- core #198: PASS normal + ASAN/UBSAN
- staged #187: PASS all five gates, real JieLi target + FM-1 emulator

Target #187:
- image 85,564 / 581,564 B
- image headroom 496,000 B
- RAM 44,680 / 98,304 B
- RAM headroom 53,624 B
- pool 30,720 / 344,064 B
- pool headroom 313,344 B
- package bytes 610,110 B
- package SHA `c609e3cf4dfb4338e933b978ef7be038c0979880a989e28ae6d437f783b61bf8`

Default emulator #187:
- Attractor is OFF by default
- raw PCM is bit-identical to M4a/M4b: `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- peak 317
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

A prior integrated target head was deliberately rejected because raw smoke PCM changed while Attractor was OFF. The accepted head preserves the frozen M4b observer/startup path until first actual Attractor activation, restoring the exact baseline hash. This is a regression fix, not a relaxed reference.

## Deliberately deferred after M4c

- audible dynamic String filter S&H modulation / filter sequence
- manual physical Attractor controls and native visual cursor
- shared Space/Cathedral
- physical FM-1 validation
- real ISR worst-case timing

Next Phase-4 dry engine in roadmap order: **Drift**, after M4c is actually merged.

## Exact next action

1. Finish documentation-only M4c head.
2. Run both workflows on that exact head.
3. If both are green, update PR #9 metadata only.
4. Mark PR #9 ready and squash-merge with `expected_head_sha` equal to the gated docs head.
5. Start Drift from the actual M4c merge commit.

## Do not regress

- Study 126 remains behavioural/state/timing/safety authority
- Study 98 is narrow acoustic authority, not a behaviour rollback
- M4a/M4b default emulator PCM remains immutable while new String features are inactive
- one shared Phase-3 scheduler; no module clocks
- M3c runtime stays 21,928 B unless a later explicitly scoped phase changes it
- KEEP / CHAIN / MOTHER semantics unchanged
- Spiral/Bloom M3c late binding unchanged
- `stepOneResync` OFF by default
- no broadband String noise
- no hard-stop of audible transients
- no shared Space/Cathedral during initial dry source work
- do not claim physical hardware or real ISR timing without device evidence
