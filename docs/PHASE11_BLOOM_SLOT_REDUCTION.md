# Phase 11F — conservative Bloom slot reduction

P11F is the falsification phase selected by P11E.

## Candidate

Change only:

`APSIS_RUNTIME_BLOOM_SLOTS: 64 -> 48`

The constant sizes both the M3c Bloom late-binding slots and the corresponding Bloom audio snapshot array.

P11E proved:
- 44 simultaneous Bloom scheduler slots are actually reached by the source-derived extreme trace;
- 44 is the exact legal upper bound under the locked transport/Bloom timing contract;
- 45 simultaneous slots are structurally impossible;
- host layout projects 56 B per runtime slot + 96 B per audio snapshot.

48 therefore retains four slots above the proven maximum while removing 16 unreachable entries from both arrays.

## First complete green P11F run

Run `41323228057` on technical head `a4b26b3a83ca879980af43f2d61c0cf4c10b4d0f` passed all six P11F gates:
1. full historical host behavior + ASAN/UBSAN;
2. reduced-capacity source-derived stress and Bloom audio bridge;
3. real JieLi production target with measured RAM gain;
4. exact frozen inactive emulator PCM/runtime;
5. reduced-capacity active Motion target;
6. exact frozen active Motion lifecycle.

The documentation/exact-gate head must rerun the same matrix with the measured target values locked byte-for-byte before P11F is promoted.

## Historical-test handling

The historical host suite contains three structural assertions that intentionally freeze the pre-P11F `sizeof(ApsisRuntime) == 21928u` baseline in addition to behavioral assertions:
- `tests/test_bloom_audio_live.c`;
- `tests/test_string_attractor_runtime.c`;
- `tests/test_string_rhythm_observer.c`.

P11F is specifically intended to reduce that structure, so `ci/p11f_prepare.sh` changes only those exact size assertions to the P11F host size **21,032 B** in the ephemeral CI workspace, runs the complete existing host/sanitizer suite, then restores every checked-in historical test byte-for-byte.

The P11E historical workflow remains pinned to the pre-P11F technical head, so the old 21,928-B structural baseline remains independently reproducible.

## Frozen host/stress behavior under 48 slots

The reduction does not move the existing behavioral traces:
- canonical M3c: queue 352/512, Spiral slots 59/64, Bloom slots **44/48**, random `78771ff5`, late `0e6fc422`;
- M3c extreme: queue 357/512, Spiral slots 59/64, Bloom slots **44/48**;
- M4c String Attractor extreme: queue 415/512, pending 60/64, random `b7a9a395`, late `e777a771`;
- exact Bloom upper bound remains `proven_max=44 current_cap=48`;
- Bloom occurrence snapshot/bind bridge passes normally and under ASAN/UBSAN.

`sizeof(ApsisRuntime)` falls from 21,928 B to **21,032 B** on the host layout.

## Measured real JieLi production result

The first complete green target run measured:
- image **140,408 / 581,564 B**;
- RAM **87,476 / 98,304 B**;
- RAM headroom **10,828 B**;
- POOL **319,176 / 344,064 B**;
- POOL headroom **24,888 B**;
- package bytes **610,110**;
- package SHA-256 `e0d0b826f0c2d5b8f1af781a9259b5d78dce9062939661ac7acad17bfefb78e5`.

Compared with the frozen P11C production target, P11F saves **2,240 B of real target RAM** while image and POOL remain unchanged.

P11E's host-layout projection was 2,432 B. The real JieLi target saves 2,240 B, demonstrating why the target build — not the host `sizeof` projection — remains authoritative for resource accounting.

## Frozen inactive regression

The reduced-capacity production target remains exactly:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`;
- cursor 123,392;
- queue high-water 28/512;
- queue overflow 0;
- late flags 0;
- late trace `811c9dc5`;
- peak 856; RMS 144/144; stereo delta 126.

## Measured active Motion target

The active telemetry build measures:
- image **137,672 / 581,564 B**;
- RAM **87,524 / 98,304 B**;
- RAM headroom **10,780 B**;
- POOL **319,176 / 344,064 B**;
- package bytes **610,110**;
- package SHA-256 `6dc05756b22dfbf7a825b1877207209a2cd667f8f0223fa56f5603a933ffe86c`.

This is also exactly **2,240 B less RAM** than the P11C active telemetry target.

The active Motion lifecycle remains exactly the frozen reference:
- seen flags `ff`; final flags `14`;
- left/right points 149/149;
- record start 220,672; finish 441,088; cycle 220,416 samples;
- manual override start 450,560; resume 466,176;
- cursor 481,024;
- queue high-water 30/512;
- overflow 0; late flags 0;
- WAV frames 521,984; peak 1,171; RMS 173.

## Acceptance result

At the strongest level currently possible without physical FM-1 hardware, the `64 -> 48` Bloom-slot reduction is accepted:
- the exact legal scheduler maximum is 44;
- 48 retains four slots of structural reserve;
- host and sanitizer behavior remains frozen;
- the Bloom audio snapshot/bind path remains valid;
- real JieLi RAM decreases by 2,240 B;
- frozen idle audio/runtime is bit-identical;
- active Motion behavior is identical.

P11F does **not** prove physical FM-1 ISR timing, stack high-water, LCD contention, NOR timing or DAC quality.

## Final freeze rule

Before promotion to `p11-optimization`, rerun the documentation/exact-gate head with production and active image/RAM/POOL/package hashes asserted exactly. After that exact-head run is green, freeze the workflow manual-only to that proven SHA and update the project handoff/roadmap.
