# Phase 10A — String Motion Contract

Status: host-first contract. No FM-1 target binding in this checkpoint.

Authority: Study 126 (`012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`). Phase 9 remains frozen beneath this layer.

## Source behavior frozen here

- Motion length is 1, 2, 4 or 8 complete Clock cycles.
- RECORD may be armed at any time, but capture begins only on the next visible Clock step 1 (`step == 0`).
- Recording ends only on a later step 1 after the selected whole-cycle count.
- Motion phase is Clock-pulse referenced, so tempo changes do not destroy loop phase.
- Classic always records the single perceived String through the RIGHT/WIRE coordinate.
- Dual + LINK records BOTH physical coordinates.
- Dual + FREE can record BODY, WIRE or BOTH independently.
- Recording one Dual/Free side is an overdub: the untouched side keeps its existing loop.
- Recording freezes the tonal context. Each captured point contains physical split plus absolute pitch; later Gravity/Tuning changes do not transpose the recorded melody.
- Quantized recording stores stepped pitch behavior. Unquantized recording stores continuous pitch behavior.
- During recording, the recorder owns the String positions: Gravity/scene changes may update harmony but must restore the owned physical String coordinates.
- Manual String movement during playback is a temporary override. Releasing the gesture resumes the current Clock-derived loop phase rather than restarting the loop.
- Source continuous capture is throttled to at most 32 Hz between forced Clock captures.
- Source final simplified motion is bounded to 1200 points.

## Native representation

`ApsisRuntime` is not changed.

Phase 10A uses an external `ApsisStringMotionStore` with two independent lanes. Each point is exactly 8 bytes:

- `phase_q16` — normalized loop phase
- `split_q15` — physical String coordinate
- `pitch_milli` — frozen absolute log-pitch in milli-semitones

There are at most 1200 points per lane. The two-lane store is about 19.2 KiB and is intended for the FM-1 POOL in target integration, not ordinary runtime RAM.

The independent-lane representation is a native storage adaptation, not a claim that the browser stores separate arrays. It preserves the observable Study-126 overdub law more directly: a WIRE overdub never rewrites BODY storage and vice versa.

Frozen pitch uses a compact `ApsisStringMotionPitchContext`, containing only the String/Gravity/Tuning fields required to resolve recording-time pitch. It does not copy the complete APSIS musical state.

Pitch is stored in log space. Study 126 interpolates continuous recorded pitch geometrically in Hz; linear interpolation of milli-semitones is mathematically the equivalent operation and avoids float/libm in the motion runtime.

## Bounded-capture adaptation

Study 126 may temporarily collect more than 1200 points before simplifying to 1200. Native Phase 10A allocates only the final 1200-point ceiling. If a lane reaches that ceiling during an unusually dense recording, the newest endpoint replaces the current final point rather than allocating beyond the fixed store. This bound is explicit and must be stress-reviewed before Phase 10 target freeze.

## Host gates

`tests/test_string_motion.c` must prove:

- 8-byte point representation and bounded store size
- Classic / Dual LINK / Dual FREE target resolution
- arm-anywhere but start/end only on step 1
- 1/2/4/8 cycle normalization
- one-side overdub leaves the opposite lane byte-identical
- frozen pitch context survives later Gravity/Tuning edits
- recording ownership restores String positions after external harmonic changes
- non-forced capture is limited to 32 Hz
- quantized pitch is held between points
- continuous pitch interpolates in log-pitch
- manual override suppresses playback only while active and resumes current phase

Normal and ASAN/UBSAN builds are required before any target integration.

## Explicitly deferred

- FM-1 button/encoder gesture binding for Motion
- POOL linker placement
- sustained String DSP pitch override from recorded `pitch_milli`
- active hardware ISR/load validation
- final dense-gesture simplification/stress evidence
- persistence decisions for Motion data

Those belong to later Phase-10 checkpoints and may not change the frozen Phase-9 idle audio/runtime baseline without explicit evidence.
