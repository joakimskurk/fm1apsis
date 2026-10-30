# CHANGELOG

Human-readable milestone history. Git remains the detailed source history.

## 2026-10-30 - Phase 7 persistent KEEP/CHAIN storage final checkpoint

PR #17 reached a technically complete persistence candidate at `e290813f5e67affc7404dcb9fc7af63d2c671a8c` and is awaiting the final documentation-head regression pass + guarded merge.

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
- image 110,848 / 581,564 B
- RAM 88,628 / 98,304 B
- POOL 309,532 / 344,064 B
- package `9c51d1ab833b1f4aa1d95ebbd0e31246fab0b19a456de3f28aa5761b7734f7cc`
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- dry-only PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

A boot-time NOR read initially perturbed 202 frames of the first active audio block by at most two Q15 steps. The baseline was not relaxed: prefetch was moved before `fm1_audio_init()`, restoring full bit-exact PCM.

Next after guarded merge: Phase 8 native 240×240 visual surfaces.

## 2026-10-30 - Phase 6 shared Space/Cathedral merged

PR #16 merged to `main` as `cd321e9f7999d0568d280d89e7e3574c3da66054` after the final guarded head `cfdfff673204d4fa36208508e343c6297a37ff37` passed core #424, staged #413 and P6-final #22.

Completed:
- one shared room with frozen String/Drift/Clock/Spiral/Bloom sends
- selected compact modulated 8-line FDN
- fixed-point full host/reference room with deterministic + ASAN/UBSAN gates
- dedicated pre-`.pool` NOLOAD history/echo/FDN storage
- source split / post-Mother send accumulation without changing dry PCM
- audible room call from target `app_audio()`
- generated FM-1 room retimed to 11.025 kHz internal rate while sources/output remain 44.1 kHz
- target build within image/RAM/POOL budgets
- audible emulator gate and exact-dry P6 gate
- explicit P6 boot-to-stop cursor baseline 123,392 with queue 28/512, no overflow/late flags and unchanged late trace
- multi-variant target report fixed to use the latest append-only build summary

Frozen Phase-6 target:
- image 107,352 / 581,564 B
- RAM 87,780 / 98,304 B
- POOL 309,532 / 344,064 B
- audible PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- dry-only PCM exact Phase 5 `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

Next phase: persistent KEEP / CHAIN storage.

## 2026-10-30 - Phase 5 sounding integration merged

Phase 5 verified existing M3c + Phase-4 DSP structural behaviour end-to-end and merged as `d677a887cf6dc8552634bde5a71fec0221c299b5` without changing production scheduler/DSP/runtime.

## 2026-10-30 - M4g Bloom merged

M4g completed source-correct occurrence planning, M3c occurrence/petal-bind reuse, deterministic finite Bloom DSP, 136-petal evidence-driven pool and exact M4f bypass. Frozen M4g target: image 100,944 B, RAM 87,336 B, pool 30,720 B, package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`.

## 2026-10-30 - M4f Spiral merged

PR #12 merged as `734229f4d23626a176a8366de82aab2c58c48cc9`. Frozen worst stress: 30/32 legs, 90 oscillators, 0 steals/drops.

## 2026-10-30 - M4e Clock merged

PR #11 merged as `db9d0b3ecbc7931be343cec7717103059d3c5160` with deterministic STONE/GLASS/SNAP references and shared-timeline consumption.

## 2026-10-30 - M4d Drift merged

PR #10 merged as `d7e39e2481c33628f402bed51d761b8be8304bb3` with frozen Drift reference 488 / `0aec6bc2`.

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