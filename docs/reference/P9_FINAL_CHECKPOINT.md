# Phase 9 Final Checkpoint — Physical Interaction Parity

Date: 2026-11-01
Technical firmware head: `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`
PR: #19 (`p9-controls` -> `main`)
Status at this checkpoint: technical implementation complete; documentation/merge validation follows on a docs-only head.

## Scope completed

Phase 9 maps the existing APSIS state/runtime/audio model onto the pinned FM-1 panel without introducing a second musical model, private module clocks, or direct UI ownership of DSP state.

Physical vocabulary frozen from pinned Dinghy:
- 14 buttons
- 7 detent/relative encoders
- 27 note keys
- MASTER remains platform output gain
- no encoder-push input exists and none is assumed

Surface navigation:
- HOME -> MOTHER
- ENV -> STRING
- LFO -> DRIFT/TUNING
- SEQ -> CLOCK
- ARP -> SPIRAL
- FX -> BLOOM
- GLO -> GRAVITY
- EDIT -> CHAIN
- SAVE -> KEEP
- SEL+FX -> the one shared SPACE/CATHEDRAL

HOME remains absolute. PLAY without SEL uses the existing shared runtime transport. SEL+PLAY is contextual CHAIN transport and cannot accidentally toggle the global transport.

## Source/state interaction

The control layer reads/writes the existing APSIS structures and APIs only:
- CLOCK edits existing masks/group boundaries.
- GRAVITY candidates go through `apsis_runtime_request_gravity()`.
- STRING maps the 27 keys as three overlapping 9-key Study-126 source-keyboard banks, not as a generic chromatic keyboard.
- SPIRAL edits the ordered raw 0..35 point route by route index; duplicates remain legal and identity-bearing.
- BLOOM preserves separate petal/core/grow semantics.
- KEEP/CHAIN use the existing 8-slot bank and 32-cell sequence.
- MOTHER `->1` uses the existing pending/commit mechanism.
- SPACE edits the one shared Cathedral state.
- DRIFT/TUNING remain part of the shared state graph.

Relative detent encoders edit from the current parameter value. There is no absolute knob position to cross, so no hidden soft-takeover/pickup state is required and untouched parameters do not jump on a surface change.

## Main-loop / audio ownership

Control decoding remains main-loop work. Structural edits use existing runtime/state APIs and P7 persistence remains deferred/coalesced outside active audio.

Direct performance audio crosses a narrow main -> audio handoff:
- manual STRING uses a one-byte mailbox and the existing finite String transient pool;
- manual BLOOM uses a packed 16-bit mailbox; main-loop read/modify/write is protected by a short `fm1_irq_off()` / `fm1_irq_on()` section;
- the audio callback is the sole owner of String/Bloom finite DSP mutation.

No manual gesture fabricates a M3c transport event.

## Manual STRING freeze

P9C maps FM-1 keys to Study-126 `keyboardStringPress()`, so the P9-only manual acoustic adapter uses the browser keyboard pluck level **0.080**.

The adapter reuses existing BODY/WIRE finite-transient primitives:
- Classic: BODY + WIRE at one perceived pitch, including the source 4-ms WIRE offset;
- Dual: selected BODY/LEFT or WIRE/RIGHT side only;
- 30-ms / 1323-sample manual retrigger debounce;
- mute/level are rechecked at audio consumption.

Historical M4b/M4f code remains byte-for-byte isolated. The Phase-9 generator hard-checks the frozen M4b blobs so manual String work cannot silently move the historical M4f bypass gate.

## Manual BLOOM freeze

P9-only manual Bloom audio reuses `ApsisBloomAudioBinding`, `ApsisBloomDsp`, the existing voicing engine and the existing late-binding resolver.

Source distinctions are retained:
- direct petal level scale: **1.04**;
- grow petal level scale: **0.78 + 0.055*i**;
- grow thresholds: one voice = 0.22; otherwise `0.14 + i/(n-1)*0.72`;
- direct petal snapshots `bloomDisplayHarmony()` semantics immediately, including a future quantized Gravity candidate;
- core chooses/advances one voice-leading plan from display harmony, then later petals retain their absolute UNFURL targets and resolve Gravity/tuning near their normal native bind lead.

Manual core uses a finite P9-only queue (4 concurrent core snapshots / 20 pending petals). This queue owns only finite gesture target times; it is **not** a musical clock or scheduler. Core voicing draws use the one shared M3c RNG; there is no private UI RNG.

## Persistence and visuals

Phase 7 project storage remains versioned/journaled/CRC protected. Phase-9 edits mark project state dirty through the existing deferred main-loop save service; flash erase/program remains unreachable from `app_audio()`.

Phase 8 native rendering remains unchanged: same ten surfaces, same post-sample-8192 native takeover, same 8-row DMA-gated LCD path, and no extra framebuffer/POOL allocation.

## Final technical target evidence

Technical head `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- product `FM-1_972`
- image **132,264 / 581,564 B**
- RAM **89,492 / 98,304 B**
- RAM headroom **8,812 B**
- POOL **309,532 / 344,064 B**
- POOL headroom **34,532 B**
- package bytes **610,110 B**
- package SHA-256 `5b0ec5dc136542ae9d3a96510ba8c05a587726e1c44b877d9a4f90b56d448018`

Pinned emulator, idle control scenario:
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93` — exact P6/P7/P8
- peak 856
- RMS 144/144
- stereo delta 126
- cursor **123,392**
- queue 28/512
- overflow 0
- late flags 0
- late trace `811c9dc5`
- dynamic HOME -> MOTHER PASS

Historical regressions remain frozen:
- M4/M5 cursor 123,648
- M4f Bloom-bypass image **95,804 B** and its historical PCM/cursor contract
- Phase-5 dry PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

## Technical-head CI matrix

On `02a0cfe55ffd4fb6ee3e346d5a49e037f2b6ad7e`:
- APSIS core #503 — PASS
- APSIS staged validation #492 — PASS
- APSIS Phase 6 final checkpoint #95 — PASS
- APSIS Phase 7 storage #82 — PASS
- APSIS Phase 7 target persistence #75 — PASS
- APSIS Phase 8 visuals #85 — PASS
- APSIS Phase 8 target visuals #60 — PASS
- APSIS Phase 9 controls #82 — PASS
- APSIS Phase 9 H4b Bloom performance #6 — PASS
- APSIS Phase 9 target controls #40 — PASS

The final documentation head is intentionally validated after this technical checkpoint is written. Its run numbers belong in PR/merge evidence rather than rewriting this file and creating a validation loop.

## Verification boundary

Phase 9 is verified through implementation, host tests, ASAN/UBSAN, real JieLi target build and pinned emulator regression. A physical FM-1 is still unavailable.

Therefore this checkpoint does **not** claim:
- real control feel/ergonomics or switch/encoder behavior;
- physical LED/display behavior;
- real ISR-cycle headroom under active manual performance;
- actual DAC/listening quality;
- real NOR power-cut robustness.

Those remain Phase 11/12 hardware work.

## Next

Phase 10 owns advanced interactive parity only:
- Clock-quantized String Motion capture;
- Classic/Dual ownership and frozen pitch context;
- overdub/manual override;
- String/Spiral attractor editors;
- Gravity point editor.

Do not fold Phase-10 editors back into the frozen Phase-9 mapping layer without explicit evidence and tests.
