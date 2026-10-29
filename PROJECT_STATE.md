# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 3 / M3c - late binding and long deterministic timing trace.**

Phase 2 musical/state core is merged to `main` at `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

M3a deterministic transport is complete and squash-merged to `main` at:
- merge commit `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- PR #4: `M3a: deterministic sample-referenced transport core`

M3b structural/runtime consumers are complete and squash-merged to `main` at:
- merge commit `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- PR #5: `M3b: structural runtime consumers`
- final implementation/evidence head `001d69a758369e2d14665ba2fd176c1fd116990b`
- final documentation head `6a8a182cbd0559cc2164c78bf956fbba7e0ee919`

Active work now lives on:
- branch `m3c-late-binding`
- base: merged M3b commit `a61a40dc83c9b52129f7d3f920da26a9a61837d9`

## Product invariant

APSIS must remain a distinct instrument and as close to a 1:1 port of browser `apsis.html` as practical. Adapt physical interaction to FM-1, but do not simplify APSIS into a generic synth/groovebox or collapse identity-bearing systems.

Locked browser reference:
- `apsis.html`
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- 606,747 bytes / 5,742 lines in the locked CRLF form
- user-supplied `index.html` on 2026-10-29 is this exact source with LF line endings
- see `docs/reference/SOURCE_LOCK.md` and `docs/reference/BEHAVIOR_MAP.md`

String-specific acoustic reference:
- attached `old_index.html`
- Study 98 · Performance Architecture
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- acoustic/timbre authority for future String DSP only
- current Study 126 remains the behavioural/state/timing/routing authority
- see `docs/reference/STRING_TIMBRE_REFERENCE.md` and `DECISIONS.md`

Pinned FM-1 baseline:
- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- APSIS product identity `FM-1_972`

## What is complete

### Phase 1
Pinned Dinghy build skeleton, APSIS overlay/identity, host build, real JieLi target build, emulator smoke and CI validation pipeline.

### Phase 2
Pure musical/state core:
- versioned defaults/state
- scales/tuning
- Gravity point model and voicing helpers
- String position/ratio/pitch model
- Drift delta model
- Clock structure/grouping and swing interval law
- Spiral route/derived sequence and deterministic attractor state
- Bloom harmony and voice-leading planner
- KEEP partial musical-memory semantics
- CHAIN cell/lane/morph semantics
- MOTHER pending queue/horizon semantics

### Phase 3 / M3a
Deterministic shared transport:
- fixed-capacity 256-entry event min-heap
- absolute transport time as 64-bit whole samples + Q32 fractional sample
- BPM 35..180 and 1 / 1/2 / 1/4 division semantics
- alternating source-derived swing law
- 3..24 pulse Clock cycles
- events for step-one resync, String filter clock, playhead, Mother, Clock main/accent, Spiral subdivisions, String rhythm, Bloom, snap, inner/between, ghost and Chain
- Gravity cycle division normalization and future boundary prediction
- deterministic injected ghost random value for tests
- long-running sample-counter tests
- transport compiled in the real FM-1 single translation unit

M3a final evidence:
- final rebased PR head `cb0319185a483ff2972aae104da6b2a86c202add`
- `APSIS core` #60: PASS
- `APSIS staged validation` #50: PASS through real target and emulator
- app image: 56,680 B
- RAM `.data + .bss`: 14,104 B
- pool: 30,720 B
- package SHA-256 `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`

### Phase 3 / M3b
Silent structural/runtime consumers:
- dedicated allocation-free `apsis_runtime.[ch]` coordinator
- live transport + 256-event heap instantiated in the real FM-1 overlay
- `app_audio()` drives only the deterministic sample timeline and remains silent
- Mother horizon / next-1 commit on the shared timeline
- Mother blocked-dispatcher state preserves pending edits and recovers on a later Clock-1 retry
- Chain start/loop/cell/chance/transpose/morph progression
- browser-parity Chain meter snapshot at Chain start
- Gravity immediate/quantized pending commit on the correct boundary
- Spiral attractor progression and symbolic occurrence capture
- Bloom occurrence and voicing-plan capture
- deterministic same-sample ordering and deterministic random source
- target-safe Q32/runtime arithmetic without pi32v2 `__udivdi3`
- live target-RAM queue telemetry in the emulator gate

M3b final verification:
- `APSIS core` #81 on implementation head: PASS, including ASAN/UBSAN
- `APSIS staged validation` #71 on implementation head: PASS through real target and emulator
- `APSIS core` #84 on final documentation head: PASS
- `APSIS staged validation` #74 on final documentation head: PASS through real target and emulator

M3b target evidence:
- app image: 66,428 B / 581,564 B
- image delta vs M3a: +9,748 B
- RAM `.data + .bss`: 22,552 B / 98,304 B
- RAM delta vs M3a: +8,448 B
- pool: 30,720 B / 344,064 B
- pool delta vs M3a: 0 B
- package bytes: 610,110 B
- package SHA-256 `16ff1f412649bfb392ddc7fbfabd03bb405d0a86fca96a0d37d0b60b4f19c819`

M3b queue/runtime evidence:
- host `sizeof(ApsisRuntime)`: 8,456 B
- host stress queue high-water: 14 / 256, no overflow
- emulator target-RAM live queue high-water: **7 / 256**
- emulator target-RAM overflow flag: **0**
- emulator runtime cursor: **123,648** low-32 sample value after smoke
- emulator WAV peak: 0, required because M3b is intentionally silent

## Browser timing facts already audited

Browser `tick()` schedules about 1.2 seconds ahead. Logical pulse order:
1. optional step-one resync
2. String Clock/filter event
3. playhead / String Motion pulse
4. Mother pulse / possible commit
5. main Clock percussion
6. Spiral subdivisions/events
7. String rhythm
8. Bloom occurrence
9. snap / between / ghost
10. counters advance
11. Chain pulse / possible cell transition

Browser lead timers express late-binding intent, not timing jitter to reproduce literally:
- Spiral late pitch rebind ~80 ms desktop / 110 ms narrow UI
- Bloom occurrence 45 ms; individual Bloom voice bind ~50/70 ms
- Mother pulse 26 ms
- Chain pulse 24 ms

Native transport must preserve the musical boundary/state-observation semantics on one sample-referenced timeline.

## Current M3c goal

M3c completes deterministic transport semantics before any DSP begins. It must turn the symbolic Spiral/Bloom occurrences created in M3b into explicit late-bind events that observe the correct latest committed Gravity/Tuning state at the same source-derived moments as Study 126.

Required work:
- audit the exact Study 126 Spiral occurrence, late-pitch rebind and narrow/desktop lead behaviour;
- audit the exact Study 126 Bloom occurrence and per-voice late-pitch binding behaviour;
- define which state is frozen at occurrence time versus re-read at late-bind time;
- encode those observations as source-derived regression expectations **before** implementation;
- add explicit late-bind event/state representation without adding per-module clocks;
- make random decisions traceable and deterministic across a long reference run;
- extend queue stress/high-water validation for the new late-bind events;
- produce a long deterministic timing/state trace;
- run the complete core + sanitizer + real target + emulator gate.

M3c remains allocation-free and intentionally silent. It must not start String, Drift, Clock, Spiral or Bloom DSP.

## Current risks to prove

- Spiral must not freeze pitch/harmony too early if Study 126 re-reads committed state near trigger time;
- Bloom occurrence state and each petal's final pitch may have different observation times;
- same-sample Mother/Gravity commits must be visible to whichever late-bind event comes after them in source semantics;
- late-bind events must not create unbounded queue growth;
- deterministic random decisions must remain replayable without changing source probabilities/order;
- `app_audio()` must remain silent through all M3c work.

## Exact next action

1. Read the locked Study 126 source around the central `tick()` plus the Spiral and Bloom trigger/bind functions.
2. Write down an explicit source-derived late-binding contract: occurrence time, bind time, what is frozen, what is re-read, and desktop/narrow timing differences.
3. Add that contract to repo documentation and add regression expectations/tests that fail against the current M3b symbolic-only runtime.
4. Only after the expectations exist, implement the smallest allocation-free M3c late-bind representation on the shared event heap.
5. Keep `app_audio()` output silent.
6. Run root tests + ASAN/UBSAN, then the complete real target/emulator gate.
7. Record queue/resource deltas and long deterministic trace evidence before M3c can merge.
8. Phase 4 DSP remains blocked until M3c is merged and green.

## Do not regress

- Do not begin DSP during M3c.
- Do not replace the shared scheduler with per-module clocks.
- KEEP is partial musical memory, not a full preset.
- CHAIN retains lane scopes/chance/transpose/morph/sync semantics.
- Mother mixer/master remain live while `->1` holds structural edits.
- Spiral and Bloom retain late binding against committed Gravity/Tuning.
- `stepOneResync` is OFF by default.
- Shared Cathedral/SPACE comes later as one shared room with per-source sends.
- String future DSP uses Study 98 as an acoustic target only; Study 126 remains the current behavioural authority.
