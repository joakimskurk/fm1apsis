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

That run established the measured target values later frozen byte-for-byte in the exact-head gate.

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

The complete green target run measured:
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

## Final exact-head acceptance

Authoritative final run **`41324331937`** on technical head **`eb651e768a049a1b4a1e88397a9021fccc69fd30`** passed the complete six-gate matrix with all measured production and active target values asserted exactly.

That run therefore proves, at the strongest level currently available without physical FM-1 hardware:
- the exact legal scheduler maximum remains 44 while the implementation retains 48 slots;
- all historical host tests and ASAN/UBSAN behavior remain green after the intentional layout reduction;
- Bloom snapshot/bind behavior remains valid;
- production JieLi RAM is exactly **87,476 B**, a **2,240 B** reduction;
- active Motion JieLi RAM is exactly **87,524 B**, also a **2,240 B** reduction;
- POOL remains exactly **319,176 B** in both target builds;
- frozen idle PCM/runtime remains bit-identical;
- the complete active Motion lifecycle remains identical.

The evidence artifact from the authoritative run is `p11f-bloom-slot-reduction-evidence` (artifact ID `13091170310`, ZIP digest `sha256:fabf6d34d0497f00c4617ca945200634e1d7f28993ffd799f1eb66734cc02700`).

## Acceptance result

The `64 -> 48` Bloom-slot reduction is accepted. Four slots remain above the mathematically proven legal maximum, so P11F deliberately stops at 48 rather than attempting a zero-margin 44-slot implementation.

P11F does **not** prove physical FM-1 ISR timing, stack high-water, LCD contention, NOR timing or DAC quality.

## Historical freeze

`.github/workflows/p11-bloom-slot-reduction.yml` is retained as a manual-only historical gate and always checks out the proven technical head `eb651e768a049a1b4a1e88397a9021fccc69fd30`. Later Phase 11 work must not reinterpret the P11F acceptance baseline by changing the workflow and the implementation together.
