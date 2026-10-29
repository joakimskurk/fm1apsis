# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 3 / M3c — late binding and deterministic timing trace. Implementation complete; final documentation-head merge gate pending.**

Merged foundation:
- Phase 2 musical/state core: `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`
- M3a deterministic transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2` via PR #4
- M3b structural/runtime consumers: `a61a40dc83c9b52129f7d3f920da26a9a61837d9` via PR #5

Active work:
- branch: `m3c-late-binding`
- PR: #6 `M3c: late binding and deterministic reference trace`
- base: M3b merge `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- last code/test implementation head before status docs: `05ddbe2c4bed26b91d9d5701aa0965aa2bba90d2`

Phase 4 DSP is still blocked until PR #6 is green on its final documentation head and merged.

## Product/source invariant

APSIS must remain a distinct parity-first instrument. Adapt physical interaction to FM-1, but do not collapse the source architecture into a generic synth/groovebox.

Behavioural authority:
- Study 126 · String Pulse Isolation
- locked `apsis.html`
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic reference for future Phase 4 only:
- Study 98 · Performance Architecture
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`
- never use Study 98 to roll back Study 126 state/timing/routing behavior

FM-1 baseline:
- Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product identity `FM-1_972`

## Completed through M3b

Phase 2 provides the pure shared musical/state graph: scales/tuning, Gravity, String pitch model, Drift, Clock, Spiral, Bloom, KEEP, CHAIN and MOTHER.

M3a provides one sample-referenced shared transport with whole-sample + Q32 fractional time and the source-derived Clock/event grid.

M3b provides the allocation-free runtime coordinator, shared transport/heap instantiation in the real FM-1 single translation unit, Gravity/Mother/Chain structural progression, symbolic Spiral/Bloom occurrences and a silent `app_audio()` timeline driver.

M3b final target evidence:
- image 66,428 B
- RAM 22,552 B
- pool 30,720 B
- live emulator queue 7/256
- WAV peak 0

## M3c source contract now implemented

Normative audit: `docs/reference/LATE_BINDING_CONTRACT.md`.

### Spiral

Occurrence freezes musical identity such as route ordinal, trigger cell and energy. Final pitch is resolved later against the current derived sequence, current committed Gravity/frame, current register and current tuning.

Native FM-1 bind lead:
- 80 ms before target

Study 126 narrow-browser 110 ms remains a source fact in tests, but is browser safety margin rather than native musical behavior.

Important asymmetry:
- Spiral **does not** project normal pending Gravity.
- A Gravity change affects Spiral only if shared state has actually committed before the bind observation.

### Bloom

Occurrence callback:
- target − 45 ms

Occurrence freezes the voicing plan, including register/octave offsets and random choice, plus occurrence timing/shape data.

Per-petal final observation:
- nominal voice target − 50 ms on FM-1
- causally clamped to `max(occurrence, voiceTarget − 50 ms)` because Study 126 cannot create petal timers before occurrence

At every petal bind Bloom rereads target-aware Gravity and current tuning/depth while preserving every occurrence-frozen plan entry. If DEPTH changes mid-unfurl, a five-petal plan is not truncated; frozen tone ordinals are remapped modulo the new base-chord size as Study 126 does.

Bloom ordinary pending Gravity projection:
- only degree/ring may be projected
- pending target must be at or before requested petal target plus Study 126's 0.5 ms tolerance
- Mother-held state is never projected before Mother actually commits

### Shared-state pre-target observations

Study 126 mutates the shared state graph in timer callbacks before the audio target. M3c therefore represents these as explicit events on the same heap:
- normal quantized Gravity commit: target − 18 ms
- Mother structural commit: Clock target − 26 ms
- Chain discrete apply: Clock target − 24 ms
- Chain continuous morph still begins at its target

These are shared-timeline events, not additional clocks.

## M3c implementation architecture

M3c remains allocation-free and silent.

Key files:
- `apsis/apsis_late_binding.[ch]`
- `apsis/apsis_runtime_m3c.c`
- `apsis/apsis_runtime_final.c`
- `tests/test_late_binding.c`
- `tests/test_runtime.c`
- `tests/test_runtime_lifecycle.c`
- `tests/test_runtime_extreme.c`
- `docs/reference/LATE_BINDING_CONTRACT.md`
- `docs/reference/M3C_REFERENCE_TRACE.md`

Architecture:
- one shared min-heap; no per-module clocks
- Study 126 ~1.20 s planning lookahead
- 512 fixed heap entries
- 64 fixed Spiral late-bind slots
- 64 fixed Bloom occurrence/bind slots
- Bloom petals are chained one at a time on the same heap, avoiding a multi-second full-petal queue explosion
- `spiral_step` is 32-bit so routes with lengths not dividing 256 do not phase-jump after long runs
- target-safe arithmetic avoids pi32v2-unavailable `__udivdi3`

Lifecycle parity:
- Stop cancels future scheduler/Spiral/Bloom/Chain/Mother shared-heap work and resets Spiral planning state
- ordinary already-pending quantized Gravity remains independent, matching Study 126's separate `framePendingTimer`
- restart begins a fresh shared transport at the current native sample cursor

## Capacity and deterministic reference evidence

The original 256-entry queue was not retained blindly: legal M3c worst-case stress set `scheduler_starved`. Capacity was raised to 512 from measured evidence.

Canonical 30 s host replay, seed `0x31415926`:
- `sizeof(ApsisRuntime)`: 21,928 B
- queue high-water: **352/512**
- Spiral slots: **59/64**
- Bloom slots: **44/64**
- queue overflow: 0
- scheduler starvation: 0
- late-slot overflow: 0
- random trace: **`78771ff5`**
- late-binding trace: **`0e6fc422`**
- identical seed replay is identical; a different seed changes the trace

60 s extreme burst with 180 BPM, 1/4 division, 35% swing, 4x Spiral, all gates, 4x Bloom and 3.6 s unfurl:
- queue high-water: **357/512**
- Spiral slots: **59/64**
- Bloom slots: **44/64**
- no queue/starvation/slot overflow
- normal and ASAN/UBSAN both PASS

## M3c real-target/emulator evidence already obtained

A target-identical M3c implementation has passed the full staged gate. This evidence predates only documentation/extreme-test wiring; target firmware sources are unchanged by those host-only additions.

Target:
- app image: **71,608 B / 581,564 B**
- RAM `.data + .bss`: **35,272 B / 98,304 B**
- RAM headroom: **63,032 B**
- pool: **30,720 B / 344,064 B**
- pool headroom: **313,344 B**
- package bytes: **610,110 B**
- identity: `FM-1_972`
- package SHA-256: `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

Emulator:
- boot/input/M3c runtime contract: PASS
- live queue high-water: **28/512**
- queue overflow: **0**
- late-bind/starvation flags: **0**
- runtime cursor low 32 bits: **123,648**
- late trace: **`811c9dc5`**
- stereo WAV frames: 124,928
- WAV peak: **0** as required for M3c

Physical FM-1 remains **not tested**.

## Verification status

M3c currently has:
- source contract: COMPLETE
- implementation: COMPLETE
- desktop regression: PASS
- deterministic replay: PASS
- extreme capacity stress: PASS
- ASAN/UBSAN: PASS
- real JieLi target build: PASS on target-identical implementation
- FM-1 emulator: PASS on target-identical implementation
- final documentation-head core/staged gate: PENDING
- physical FM-1: NOT TESTED

## Exact next action

1. Finish documentation/status updates on PR #6 without changing runtime behavior.
2. Run `APSIS core` and full `APSIS staged validation` on that exact final documentation head.
3. Confirm `main` has not moved/rebase if required.
4. Compare PR #6 to `main` for unrelated files.
5. If both final gates are green, mark PR #6 ready and squash-merge.
6. Only then open Phase 4 / dry DSP work. First DSP priority remains String, using Study 98 as acoustic target inside the Study 126 behavioral contract.

## Do not regress

- No DSP before M3c merge.
- No per-module clocks.
- KEEP remains partial musical memory, not a full preset.
- CHAIN retains lane scopes/chance/transpose/morph/sync semantics.
- Mother mixer/master remain live while structural edits can be held.
- Preserve the deliberate Spiral/Bloom pending-Gravity asymmetry.
- `stepOneResync` remains OFF by default.
- Shared Cathedral/SPACE remains a later one-room/five-send design.
- Do not call physical hardware behavior verified without real FM-1 evidence.
