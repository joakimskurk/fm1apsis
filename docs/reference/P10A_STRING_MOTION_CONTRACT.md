# Phase 10A — String Motion contract

Authority: Study 126 (`apsis_study_126_string_pulse_isolation.html`) for final behavior, with the earlier Study 41/42 intent retained only where Study 126 still preserves it.

## Source-locked behavior

- Arm does not begin recording immediately. Recording begins only on the next zero-based Clock step `0` (visible step 1).
- Loop length is exactly 1, 2, 4 or 8 complete Clock cycles.
- Recording closes on the later step 1 after the selected number of full cycles and playback starts phase-synchronously from that boundary.
- Raw capture is throttled to 1/32 second except for forced Clock-boundary captures.
- Study 126 allows 1800 raw points and simplifies the completed loop to at most 1200 points.
- Recorded points own both physical String position and absolute pitch. Pitch context is frozen at record start, so later Gravity/tuning movement does not rewrite the recorded melody.
- Classic records RIGHT (the one perceived String controller).
- Dual + LINK records BOTH.
- Dual + FREE may record LEFT, RIGHT or BOTH. Recording one side is an overdub: the opposite side's existing motion is preserved.
- While recording, manual String movement is the position owner. Gravity commits may change harmony underneath but must not move the recorded physical String position.
- During playback, a manual String gesture temporarily overrides motion. Releasing the gesture returns to the loop at its current shared-Clock phase; playback is not restarted.
- Pulse Audit modes other than FULL suppress clock-derived String Motion playback.
- Quantized recordings hold recorded pitch between points. Unquantized recordings use continuous geometric pitch interpolation.

## Native representation

`ApsisRuntime` remains frozen at 21,928 B. Phase 10A uses a separate `ApsisStringMotionRuntime`.

Browser Study 126 stores shared tuples `[phase,leftSplit,rightSplit,leftHz,rightHz]`. Native stores LEFT and RIGHT as independent tracks. This preserves the audible curves while making Dual+FREE overdub cheaper: replacing LEFT does not require cloning RIGHT.

Each side track stores:
- phase: Q15
- split: Q15
- pitch: unsigned 24-bit centi-Hz

Centi-Hz gives 0.01-Hz resolution, below Study 126's 0.08-Hz simplification threshold, while retaining >167-kHz representable range. Two 1800-point raw tracks plus runtime metadata stay below 26 KiB, leaving the target integration feasible inside the current POOL budget without enlarging frozen `ApsisRuntime`.

The playback API deliberately returns absolute-pitch bracketing values plus interpolation fraction rather than hiding pitch interpolation in UI state. The later target/audio adapter must use those frozen absolute pitches; it must not recompute them from current Gravity.

## Phase 10A host gates

Tests must prove:
- next-step-1 arm/start and whole-cycle finish;
- Classic / Dual+LINK / Dual+FREE ownership resolution;
- frozen record-start pitch context across Gravity changes;
- position ownership restore during recording;
- one-side overdub leaves the opposite track byte-semantically unchanged;
- 1/32-second ordinary capture throttle;
- manual override and release-to-current-phase behavior;
- Pulse Audit suppression;
- continuous-vs-held pitch mode captured from quantize state;
- runtime storage remains below 26 KiB.

## Not yet claimed

Phase 10A is host contract work only. It does not yet claim:
- generated FM-1 target integration;
- physical REC/PLAY gesture ergonomics;
- active-performance ISR margin;
- physical display/LED feedback;
- persistence of motion-loop data.

Those remain later Phase-10 integration decisions. Frozen Phase-9 idle PCM/runtime remains the target gate when integration begins.
