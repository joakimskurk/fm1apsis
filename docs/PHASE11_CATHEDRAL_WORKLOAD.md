# Phase 11H — Cathedral dynamic workload census

P11H is a measurement-only phase. It changes no production firmware behavior, memory capacity, DSP topology or target binary.

## Authority

- technical head: `ca20dd8608ae685349d483f68d72ce4022b90402`
- green workflow run: `41341918347`
- evidence artifact: `p11h-cathedral-workload-evidence`
- artifact SHA-256: `27c9fabb358331d5b34444ea6233e4df25c58572cc9bd0f42e16159ff3d57fb7`

All five P11H gates passed on the exact technical head:
1. complete frozen P11F host behavior + ASAN/UBSAN;
2. deterministic test-only Cathedral/transport dynamic census;
3. real JieLi production target byte-identical to P11F;
4. exact frozen idle emulator PCM/runtime;
5. exact frozen active String Motion lifecycle.

## Measurement method

P11H does **not** insert counters into production firmware.

`ci/p11h_instrument_sources.py` creates ephemeral `/tmp` copies of:
- `apsis_space_target.c`;
- `apsis_space_room.c`;
- `apsis_space_dsp.c`;
- `apsis_transport.c`.

Only those host-test copies include `tests/p11h_probe.h` and deterministic operation counters. The actual source tree is left unchanged. The subsequent real JieLi target build is required to reproduce the exact accepted P11F package hash, proving the measurement instrumentation is absent from production.

The census scenario renders **256 callbacks × 256 output frames = 65,536 output frames**. The target Cathedral adaptation is quarter-rate, so this produces exactly **16,384 room frames**.

For comparison, the same 65,536-sample span is run through the fastest legal Clock configuration: 180 BPM, 1/4 division. Its pulse interval is 3,675 samples, yielding exactly **18 calls** to `apsis_transport_schedule_next()` over that span.

## Exact dynamic Cathedral census

Measured over 65,536 output frames / 16,384 room frames:

- target render calls: **256**
- room render calls: **256**
- room frame iterations: **16,384**
- parameter smoothing steps: **16,384**
- stereo delay single-channel reads: **196,608**
- room biquad evaluations: **131,072**
- room Q15 multiply helper calls: **294,912**
- stereo delay writes: **32,768**
- FDN render calls: **256**
- FDN frames: **16,384**
- FDN line reads: **131,072**
- FDN line writes: **131,072**
- FDN Q15 multiply helper calls: **655,360**

Per room frame this is exactly:

- **12** single-channel delay reads;
- **8** room biquad evaluations;
- **18** room Q15 multiplies;
- **2** stereo delay writes;
- **8** FDN line reads;
- **8** FDN line writes;
- **40** FDN Q15 multiplies.

The transport comparison over the same 65,536 output frames measured:

- `apsis_transport_schedule_next()` calls: **18**
- transport events added: **218**
- room frames per fastest-legal transport scheduling call: **910** (integer ratio).

This does not mean one room frame costs more CPU than one scheduler call. It proves only the **dynamic frequency** difference that the P11G static inventory could not show.

## Frozen production identity

The P11H technical head regenerates the exact accepted P11F production target:

- product `FM-1_972`
- image **140,408 / 581,564 B**
- ordinary RAM **87,476 / 98,304 B**
- RAM headroom **10,828 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `e0d0b826f0c2d5b8f1af781a9259b5d78dce9062939661ac7acad17bfefb78e5`

Therefore P11H adds **zero production image/RAM/POOL/package change**.

## Frozen behavioral identity

Idle emulator remains exactly:

- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392
- queue high-water 28/512
- overflow 0
- late flags 0
- late trace `811c9dc5`
- peak 856; RMS 144/144; stereo delta 126.

Active Motion remains exactly:

- seen flags `ff`; final flags `14`
- points 149 left / 149 right
- record start 220,672; finish 441,088; cycle 220,416
- manual override start 450,560; resume 466,176
- cursor 481,024
- queue high-water 30/512
- overflow 0; late flags 0
- WAV frames 521,984; peak 1,171; RMS 173.

## P11H decision

P11H confirms that Cathedral is a repeated audio-hotpath candidate worth optimizing before the much less frequently invoked transport scheduling path.

The strongest narrow candidate is the room delay reader. `space_room_stereo_read()` currently computes the same validity, wrap, frame-index and interpolation fraction separately for left and right channels. Every room frame performs:

- 4 early-reflection taps × 2 channels = 8 calls;
- predelay × 2 channels = 2 calls;
- echo delay × 2 channels = 2 calls;
- total **12 single-channel calls per room frame**.

A stereo-pair reader can potentially perform the common address/fraction calculation once per stereo tap while retaining the exact same per-channel interpolation arithmetic. That would reduce six stereo tap locations per room frame from 12 common address calculations to 6 without changing delay lengths, coefficients, state, topology or sample format.

This is an optimization hypothesis, not yet an accepted production change.

## Exact next phase — P11I

P11I should test the stereo-pair delay-read hypothesis with this order:

1. build a reference equivalence harness that compares the current single-channel reader against a candidate stereo-pair reader across zero/partial/full fill, wrap boundaries, integer and fractional delays, both channels and deterministic randomized storage contents;
2. run the equivalence harness normally and under ASAN/UBSAN;
3. only after exact equivalence is proved, apply the minimal `apsis_space_room.c` production patch;
4. run all existing Space/room and full historical regressions;
5. build the real JieLi target and compare `apsis_space_room_render` / related target disassembly against the P11G static baseline;
6. require exact idle PCM/runtime and active Motion lifecycle;
7. record the new exact target image/RAM/POOL/package values before any promotion.

P11I must not claim physical FM-1 CPU/ISR improvement. Target disassembly/code-shape changes remain a proxy until hardware exists.
