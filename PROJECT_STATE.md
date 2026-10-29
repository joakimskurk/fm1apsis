# PROJECT_STATE

Last updated: 2026-10-29

## Current milestone

**Phase 4 / M4a — dry persistent String DSP core.**

Phase 3 is complete and merged:
- M3a transport: `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b runtime: `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c late binding: `ec0dc0b350793572f19040efc938f00121128dc6` via PR #6

Active work:
- branch `m4-string-dsp`
- base `ec0dc0b350793572f19040efc938f00121128dc6`
- first slice: pure dry persistent String core before transient/live-target integration

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural authority:
- Study 126 · String Pulse Isolation
- locked SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic authority only:
- Study 98 · Performance Architecture
- raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

## Phase 3 final verification baseline

Exact M3c final PR head `4fa7d25920523b9e5d8591ae6185e1b5f910c083` passed:
- APSIS core #123 including ASAN/UBSAN
- APSIS staged validation #113 through real JieLi target and FM-1 emulator

M3c target:
- image 71,608 B / 581,564 B
- RAM 35,272 B / 98,304 B
- pool 30,720 B / 344,064 B
- package SHA-256 `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

M3c emulator:
- queue 28/512
- overflow 0
- late flags 0
- cursor low32 123,648
- late trace `811c9dc5`
- WAV peak 0, intentionally silent

Physical FM-1: **not tested**.

## Phase 4 String authority

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`

The native String must combine two authorities deliberately:

### Keep from Study 126
- current String state/defaults and Classic/Dual interaction;
- independent Dual BODY/WIRE positions, anchors, levels and Gravity-follow semantics;
- current String geometry, quantisation and tuning behavior;
- shared Clock/transport and all M3 late-binding/state-ordering semantics;
- transport ownership of persistent sustain;
- Motion/Attractor behavior;
- source/mute/Mother routing model;
- de-click, transient teardown and pulse-isolation safety fixes.

### Recover acoustically from Study 98
- persistent low Gravity-root Foundation pair;
- Classic sine pair around played String pitch separated by Drift;
- old overtone/pluck transient DNA in a later M4 slice.

Do not restore old Study-98 behavior outside this narrow acoustic role.

## M4a source-derived sustain contract

Study 98 persistent Foundation:
- root = `scaleAt(frameDegree())`, not selected Gravity-point offset/octave;
- octave follows `foundationOctave`, default -1;
- Drift delta = current native audible Drift delta `d`;
- frequencies = root ± `0.55*d`;
- each gain = `0.016 * foundationLevel * stringLevel`;
- sine voices;
- dark low-pass target ~1050 Hz;
- fast pitch smoothing ~12 ms; gain smoothing ~70 ms.

Study 98 Classic sustain:
- centre = current Study-126 String pitch;
- frequencies = centre ± `0.5*d`;
- each gain = `0.020 * stringLevel`;
- sine voices;
- low-pass target ~1800 Hz;
- String glide controls pitch smoothing with a practical >=25 ms floor;
- gain smoothing ~70 ms.

M4a is dry only. Shared Cathedral/Space remains later work.

## M4a implementation rule

Build the DSP as a pure allocation-free component and prove it on host before writing audio in `app_audio()`.

Required first tests:
- exact source-derived voice targets;
- correct root/octave and Drift offsets;
- Foundation enable/disable;
- transport-gated smooth silence/restart;
- pitch glide without phase reset;
- bounded deterministic stereo Q15 output;
- no DC/filter runaway over long renders;
- normal and ASAN/UBSAN pass;
- no `libm` or target-unavailable 64-bit division dependency.

Do **not** rush live integration by inventing a Dual timbre. The live target gate starts only after a native Dual dry strategy has been documented.

## Exact next action

1. Add `apsis_string_dsp.[ch]` as a fixed-state, fixed-point 44.1 kHz renderer.
2. Add `tests/test_string_dsp.c` before connecting it to the FM-1 callback.
3. Implement target-safe pitch-to-phase conversion and deterministic sine/filter path.
4. Verify default Foundation + Classic targets and smoothing/continuity under host + sanitizers.
5. Define the dry Dual strategy explicitly.
6. Only then integrate String output into the generated FM-1 single translation unit and update emulator audio expectations from intentionally silent M3c to controlled nonzero M4 audio.

## Do not regress

- one shared Phase-3 scheduler; no module clocks;
- KEEP remains partial musical memory;
- CHAIN lane/chance/transpose/morph semantics remain unchanged;
- Mother structural hold/live mixer rules remain unchanged;
- Spiral/Bloom late binding remains unchanged;
- `stepOneResync` OFF by default;
- no shared Space/Cathedral during initial dry engines;
- do not call physical hardware verified without real device evidence.