# CHANGELOG

Human-readable milestone history. Git remains the detailed source history.

## 2026-10-31 - Phase 8 native visuals merged

PR #18 merged to `main` as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457` after final documentation/CI head `856c4e35ae1f041da8ccdc555f4c82b54f43a04a` passed core #460, staged #449, P6-final #54, P7-storage #41, P7-target #34, P8-visuals #44 and P8-target #19.

Completed:
- source-derived allocation-free 240×240 visual raster and canonical RGB565 palette
- ten deterministic native surfaces: MOTHER, GRAVITY, STRING, DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, KEEP, CHAIN and SPACE/CATHEDRAL
- KEEP/CHAIN visuals read the existing persistent/runtime structures directly; no UI shadow musical state
- one shared Space/Cathedral view with five source sends
- frozen per-surface visual hashes plus full-frame/strip-equivalence tests
- normal and ASAN/UBSAN visual gates
- target renderer reuses Dinghy canvas with no second framebuffer and no POOL growth
- 8-row LCD updates gated by HAL-level DMA completion and kept outside `app_audio()`
- all ten surfaces target-addressable; final physical SPACE mapping intentionally deferred to Phase 9
- real JieLi target build within image/RAM/POOL budgets
- pinned emulator preserves exact P6/P7 audible PCM and integrated runtime telemetry

Frozen technical candidate:
- source `87575aa829f3fab6af43092b440d585745eecc25`
- image 118,040 / 581,564 B
- RAM 88,644 / 98,304 B
- POOL 309,532 / 344,064 B
- package `2f79a9dada63b950bbe77f9c3f6987f1c72c78cc39f8262e7590f06dcce3d743`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

A first native redraw changed only early audio despite exact runtime telemetry. Frame-level comparison showed differences limited to frames 1795..4351. The baseline was not relaxed: native takeover is now deferred until runtime sample 8192, restoring the complete frozen PCM hash.

The Phase-8 visual contract is deterministic and source-derived; screenshot/pixel-identical browser parity and real FM-1 panel appearance are not claimed.

Next: Phase 9 physical interaction parity.

## 2026-10-30 - Phase 7 persistent KEEP/CHAIN storage merged

PR #17 merged to `main` as `e9000b2b2f74d4fafe96df87f030cc55916a8285` after final documentation/CI head `2fd054d90a49ff8e52544c092937dadc8e4097d9` passed core #436, staged #425, P6-final #32, P7-storage #19 and P7-target #12.

Completed:
- explicit little-endian v1 project format for 8 KEEP slots, 32 CHAIN cells and behavioural CHAIN settings
- 24-byte CRC header + 816-byte payload; native struct padding is never persisted
- dedicated 4 KiB project sectors at `0x000F4000` and `0x000F5000`, outside Dinghy settings
- alternating generation journal with payload-first/header-last commit semantics
- torn-write/corruption/wraparound desktop tests
- fake FM-1 NOR save/reboot/failure/recovery tests and ASAN/UBSAN gates
- narrow pinned-Dinghy flash allow-list extension only for the two APSIS sectors
- boot NOR prefetch moved before audio/DMA startup; later project apply is RAM-only
- deferred main-loop save with stopped transport and quiesced audio; no flash write from `app_audio()`
- real JieLi target build within budgets
- exact P6 audible PCM and exact Phase-5 dry PCM preserved

Frozen technical candidate:
- firmware source `e290813f5e67affc7404dcb9fc7af63d2c671a8c`
- image 110,848 / 581,564 B
- RAM 88,628 / 98,304 B
- POOL 309,532 / 344,064 B
- package `9c51d1ab833b1f4aa1d95ebbd0e31246fab0b19a456de3f28aa5761b7734f7cc`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- dry-only PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

A boot-time NOR read initially perturbed 202 frames of the first active audio block by at most two Q15 steps. The baseline was not relaxed: prefetch was moved before `fm1_audio_init()`, restoring full bit-exact PCM.

## 2026-10-30 - Phase 6 shared Space/Cathedral merged

PR #16 merged to `main` as `cd321e9f7999d0568d280d89e7e3574c3da66054` after the final guarded head `cfdfff673204d4fa36208508e343c6297a37ff37` passed core #424, staged #413 and P6-final #22.

Completed one shared room, compact modulated 8-line FDN, dedicated pre-`.pool` NOLOAD storage, target source/send integration, quarter-rate 11.025 kHz Cathedral adaptation, exact Phase-5 dry regression and audible-room emulator gate.

Frozen Phase-6 target: image 107,352 B; RAM 87,780 B; POOL 309,532 B; audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`; dry-only PCM exact Phase 5 `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`.

## 2026-10-30 - Phase 5 sounding integration merged

Phase 5 verified existing M3c + Phase-4 DSP structural behaviour end-to-end and merged as `d677a887cf6dc8552634bde5a71fec0221c299b5` without changing production scheduler/DSP/runtime.

## 2026-10-30 - M4g Bloom merged

M4g completed source-correct occurrence planning, M3c occurrence/petal-bind reuse, deterministic finite Bloom DSP, 136-petal evidence-driven pool and exact M4f bypass. Frozen M4g target: image 100,944 B, RAM 87,336 B, pool 30,720 B, package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`.

## 2026-10-30 - M4f Spiral merged

PR #12 merged as `734229f4d23626a176a8366de82aab2c58c48cc9`.

## 2026-10-30 - M4e Clock merged

PR #11 merged as `db9d0b3ecbc7931be343cec7717103059d3c5160`.

## 2026-10-30 - M4d Drift merged

PR #10 merged as `d7e39e2481c33628f402bed51d761b8be8304bb3`.

## 2026-10-29 - M4c String Attractor merged

PR #9 merged as `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`.

## 2026-10-29 - M4b finite String + Rhythm merged

PR #8 merged as `c96c00debf9a67948f1e874e36b090999dacadb9`.

## 2026-10-29 - M4a persistent String merged

PR #7 merged as `2ea217f09c143d30385f136e84e2b29d582987a0`.

## 2026-10-29 - Phase 3 completed

M3a `fe2911c43fd69e63839220482e50bb10c7a2e4c2`, M3b `a61a40dc83c9b52129f7d3f920da26a9a61837d9`, M3c `ec0dc0b350793572f19040efc938f00121128dc6`.

## 2026-10-23 - Phase 2 completed

M2c `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361` completed pure state semantics.

## 2026-10-23 - Phase 1 completed

Established pinned Dinghy baseline, APSIS identity, host build, real JieLi target build, staged CI and FM-1 emulator smoke.

## 2026-10-23 - Project reference freeze

Locked Study 126 behavioural authority, port contract, architecture direction, parity matrix and source behaviour maps.
