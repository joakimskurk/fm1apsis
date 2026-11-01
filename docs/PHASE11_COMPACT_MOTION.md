# Phase 11C — compact lossless String Motion storage

P11C is the first implemented Phase-11 optimisation. It is deliberately narrow: recover POOL headroom without changing String Motion capture capacity, timing, pitch precision or playback behavior.

## Starting baseline

Frozen Phase-10 production target:
- image 140,152 B
- RAM 89,684 B
- POOL 328,740 B
- POOL headroom 15,324 B
- MotionStore 19,208 B

P11A proved that the complete Phase-9 → Phase-10 POOL increase was exactly MotionStore. P11B then proved that per-point absolute pitch was redundant because one immutable pitch context per recorded lane can reproduce every captured point pitch exactly from `split_q15`.

## Storage change

Before P11C each point stored:
- `phase_q16` — 16 bit
- `split_q15` — 16 bit
- `pitch_milli` — signed 32 bit

P11C stores per point only:
- `phase_q16` — 16 bit
- `split_q15` — 16 bit

Each lane stores one frozen 20-B `ApsisStringMotionPitchContext`.

Measured host layout:
- point: 8 B → 4 B
- lane: 4,822 B
- two-lane store: 19,208 B → **9,644 B**
- exact structural saving: **9,564 B**
- source-locked capacity remains 1,200 points/lane
- capture bound remains 32 Hz

This is not lossy compression. Playback still produces signed 32-bit millisemintone pitch values.

## Playback cost control

Removing per-point pitch must not move expensive harmony/tuning reconstruction into every audio callback.

P11C therefore:
- performs one lane segment search for both split and pitch interpolation;
- caches the two derived endpoint pitches for the current segment per lane;
- recomputes endpoint pitch only when the sampled segment changes;
- invalidates only the lane owned by a new recording/overdub.

The previous implementation did separate linear segment searches for split and pitch. P11C combines those searches, offsetting the small cache/runtime bookkeeping cost.

This is still only a structural/code-path argument. It is **not** a claim about physical FM-1 ISR cycles; true ISR margin remains a hardware/instrumentation task.

## Host proof

P11C passes:
- the existing String Motion source contract;
- a dedicated reference playback model across hold and continuous pitch modes;
- repeated same-segment calls to exercise the endpoint cache;
- free-Dual right-only overdub with the left lane/context preserved bit-for-bit;
- cache invalidation on owned lane only;
- later live Gravity/Tuning edits with both recorded lane contexts remaining frozen;
- ASAN/UBSAN;
- the unchanged persistent Motion DSP override contract.

## Real JieLi production result

First complete green run: `41308798678` on `8c31d63d04d48efeed9bcb3c28c0ec8286e00e0c`.

Production target:
- product `FM-1_972`
- image **140,408 / 581,564 B**
- RAM **89,716 / 98,304 B**
- POOL **319,176 / 344,064 B**
- RAM headroom **8,588 B**
- POOL headroom **24,888 B**
- package bytes 610,110
- package SHA-256 `e10c01525a808a4dea18d6b3f8777307bf4acf180c2cf958bebc4e1546b0d0be`

Measured delta vs Phase 10:
- POOL **−9,564 B**
- POOL headroom **+9,564 B**
- image **+256 B**
- RAM **+32 B**

The real target POOL saving equals the P11B structural prediction exactly.

## Frozen inactive regression

Still exact:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue high-water 28/512
- queue overflow 0
- late flags 0
- late trace `811c9dc5`
- peak 856
- RMS 144/144
- stereo delta 126

## Active Motion regression

Telemetry target:
- image 137,672 B
- RAM 89,764 B
- POOL 319,176 B
- package SHA-256 `d6e2d264d13d82c9cf93bce96f1d3b6666c6429cd5894fcf92db884b412bcb0e`

Pinned emulator lifecycle remains exactly:
- seen flags `ff`
- final flags `14`
- 149 points left / 149 points right
- record start 220,672
- record finish 441,088
- cycle 220,416 samples
- manual override start 450,560
- manual resume 466,176
- runtime cursor 481,024
- queue high-water 30/512
- overflow 0
- late flags 0
- WAV peak 1,171
- WAV RMS 173

## Verification boundary

P11C proves a lossless storage optimisation under:
- deterministic host/reference tests;
- ASAN/UBSAN;
- existing historical regressions;
- real JieLi target build;
- exact inactive pinned-emulator regression;
- active pinned-emulator Motion lifecycle.

It does **not** prove:
- physical FM-1 ISR cycle margin;
- physical stack high-water;
- physical LCD/NOR timing;
- DAC/listening quality.

## Evidence-retention tightening

The first green run executed every gate successfully, but its second `prepare_dinghy.sh` pass resets `work/` and `build/`, which meant the final uploaded artifact kept active evidence while earlier production/idle JSON files were removed.

The P11C workflow is therefore tightened to keep cross-pass evidence under `/tmp/p11c-evidence` and to assert exact production/active image, RAM, POOL, package size and package hashes. This changes CI evidence retention only, not firmware.

## Next

P11D is a real-target memory ownership inventory. The next optimisation target should be chosen only after linker/ELF evidence identifies the largest RAM owners and clarifies which stack reservation/bounds are statically knowable. Configured stack is not the same thing as measured stack high-water.
