# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 3 / M3b - structural/runtime consumers on the shared transport.**

Phase 2 musical/state core is merged to `main` at `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

M3a deterministic transport is complete and squash-merged to `main` at:
- merge commit `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- PR #4: `M3a: deterministic sample-referenced transport core`

Active work now lives on:
- branch `m3b-runtime-consumers`
- base: merged M3a commit `fe2911c43fd69e63839220482e50bb10c7a2e4c2`

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

Final M3a PR head after rebase:
- `cb0319185a483ff2972aae104da6b2a86c202add`
- `APSIS core` run #60: PASS
- `APSIS staged validation` run #50: PASS through real target and emulator

M3a target evidence:
- app image: 56,680 B / 581,564 B
- RAM `.data + .bss`: 14,104 B / 98,304 B
- pool: 30,720 B / 344,064 B
- package bytes: 610,110 B
- package SHA-256 `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`
- emulator WAV peak 0, expected because M3a is intentionally silent

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

## Current M3b goal

M3b turns the proven event generator into the real silent FM-1 runtime timeline.

Required work:
- instantiate `ApsisTransport` and `ApsisEventQueue` in the FM-1 runtime;
- schedule enough future events for the current audio block / bounded lookahead;
- consume structural events deterministically without producing audio;
- apply Mother horizon/next-1 commits on the shared timeline;
- apply Chain pulse/cell transitions on the shared timeline;
- commit Gravity immediate/quantized pending changes at the correct boundary;
- progress Spiral attractor state and create symbolic Spiral occurrences;
- create Bloom occurrence/voice-plan state at the correct boundaries;
- preserve ordering needed for later M3c late binding;
- measure real live queue RAM and queue high-water on target/emulator.

M3b must remain allocation-free and must not create separate per-module clocks.

## Current risks to prove

- callback/block scheduling must never skip or double-consume a boundary;
- same-sample state priority must be deterministic;
- Mother and Chain commits must occur before/after dependent events exactly according to source intent;
- pending Gravity must not become visible before its quantized commit;
- Spiral/Bloom must schedule symbolic occurrences now but defer final pitch binding to M3c;
- 256-entry queue must retain adequate high-water margin after live runtime instantiation;
- random decisions must remain explicit/injected/deterministic;
- `app_audio()` remains silent through M3b/M3c despite becoming the timeline driver.

## Exact next action

1. Inspect the public APIs and current tests for `apsis_transport`, `apsis_mother`, `apsis_chain`, `apsis_gravity`, `apsis_spiral`, `apsis_bloom`, `apsis_keep`, and `overlay/dinghy.c`.
2. Design the smallest allocation-free runtime coordinator that owns transport + queue and consumes structural events; prefer a dedicated `apsis_runtime.[ch]` over embedding state-machine logic in `overlay/dinghy.c`.
3. Add source-derived desktop regression tests for event consumption/state ordering **before** wiring it into the FM-1 overlay.
4. Implement Mother/Chain/Gravity structural consumption first, then Spiral/Bloom symbolic occurrence state.
5. Keep `app_audio()` sample output silent; use it only to advance the deterministic timeline.
6. Run root tests + ASAN/UBSAN.
7. Add the runtime module to the overlay and run the complete real target/emulator gate.
8. Record live RAM/pool/image deltas and queue high-water in `TESTING.md`.
9. Only after M3b is green proceed to M3c late binding; Phase 4 DSP remains blocked until M3c is complete.

## Do not regress

- Do not begin DSP during M3b/M3c.
- Do not replace the shared scheduler with per-module clocks.
- KEEP is partial musical memory, not a full preset.
- CHAIN retains lane scopes/chance/transpose/morph/sync semantics.
- Mother mixer/master remain live while `->1` holds structural edits.
- Spiral and Bloom retain late binding against committed Gravity/Tuning.
- `stepOneResync` is OFF by default.
- Shared Cathedral/SPACE comes later as one shared room with per-source sends.
- String future DSP uses Study 98 as an acoustic target only; Study 126 remains the current behavioural authority.
