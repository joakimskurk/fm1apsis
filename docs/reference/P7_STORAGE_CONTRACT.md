# Phase 7 — KEEP / CHAIN persistent project contract

Status: **final technical contract; PR #17 merge pending**

This document freezes APSIS project persistence. It does not change the M3c scheduler, any source DSP, Space/Cathedral, or `ApsisRuntime`.

## Persisted musical/editor content

A project contains:
- all 8 KEEP snapshots;
- KEEP selected slot (`-1` or `0..7`);
- all 32 CHAIN cells up to the persisted chain count;
- CHAIN count and editor-selected cell (`-1` or within count);
- the 13 CHAIN behavioural/default/scope settings from `ApsisState.chain`.

CHAIN transport/runtime state is **not** persistent: `playing`, `armed`, `pos`, `pulse`, `phrase_pulse`, `master_pulses`, `hold_tail` and `last_applied` are reset on load. Shared scheduler state and DSP histories are also not project data.

## Wire format v1

Native C structs are never written to flash. The format is explicit little-endian bytes, so compiler padding/alignment cannot silently change a project.

- magic: `APS7` (`0x37535041` little-endian)
- format version: `1`
- header: 24 bytes
- payload: 816 bytes
- complete record: 840 bytes
- CRC: standard CRC-32 / IEEE (`0xEDB88320`, check `123456789 -> 0xCBF43926`)

Header:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | magic |
| 4 | 2 | format version |
| 6 | 2 | header size |
| 8 | 4 | payload size |
| 12 | 4 | generation |
| 16 | 4 | payload CRC-32 |
| 20 | 4 | header CRC-32 over bytes `0..19` |

Payload:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 1 | KEEP selected slot (`0xFF` = none) |
| 1 | 1 | CHAIN count (`0..32`) |
| 2 | 1 | CHAIN selected cell (`0xFF` = none) |
| 3 | 576 | 8 × 72-byte KEEP snapshots |
| 579 | 224 | 32 × 7-byte CHAIN cells |
| 803 | 13 | CHAIN settings |

Invalid/unused KEEP slots encode as canonical zeroes. CHAIN cells beyond `count` encode as canonical zeroes. Decode clamps enum/range fields and resets transient CHAIN runtime state.

## Two-sector journal

Each journal slot owns one 4 KiB erase sector:
- A `0x000F4000`
- B `0x000F5000`

Dinghy's one-sector settings store at `0x000FC000` is deliberately not used.

Save protocol:
1. choose the inactive/older sector;
2. build the next-generation record completely in RAM;
3. erase the target sector;
4. program payload bytes first at offset 24;
5. read payload back and verify against the prepared header/CRC;
6. program the 24-byte header **last**;
7. reread and validate the complete record before advancing active generation.

A power cut before the commit header is complete leaves that new sector invalid. Boot validates magic/version/sizes/header CRC/payload CRC on both sectors and selects the newest valid generation with wrap-safe sequence comparison. If the newest record is torn/corrupt, the older valid record remains recoverable.

Desktop tests cover erased slots, payload-only interruption, partial header, complete commit, later corruption fallback and generation wraparound. Fake-NOR integration tests additionally exercise alternating saves, reboot/load, failed header program and corrupt readback.

## FM-1 storage selection

The pinned Dinghy platform originally allows OTA + the tiny settings sector. Phase 7 backports only a narrow project range `[0x000F4000, 0x000F6000)` into `FL_RANGE_OK`. Newer Dinghy uses this same upper-FM-1 data family for application blob storage, supporting the choice without wholesale upgrading the pinned baseline.

The APSIS project record owns its own magic/version. Cross-firmware preservation is not promised if another firmware intentionally reuses these sectors.

## Boot safety

FM-1 flash reads can perturb the first active DMA block if performed after ALNK0 starts. This was measured during P7: only frames 1804..2047 differed, 202 frames total, by at most 2 Q15 steps, while the stream after frame 2048 was already bit-identical and runtime telemetry remained exact.

That regression was not accepted. Final P7 boot is split:
1. `app_pre_audio_init()` resets project storage and reads/validates the newest record from NOR **before** `fm1_audio_init()`;
2. ALNK0/audio DMA starts normally;
3. `app_main()` installs APSIS defaults and resets KEEP/CHAIN;
4. `apsis_project_fm1_apply_prefetched()` decodes only the already-buffered RAM record.

The full P6 audible and Phase-5 dry hashes are restored exactly.

## Save/audio safety

Erase/program masks interrupts while XIP is unavailable. Therefore:
- the input gesture only sets `save_pending`;
- no flash erase/program is reachable from `app_audio()`;
- service happens only in `app_main()` while transport is stopped;
- `runtime_ready` is cleared and FM-1 audio/DMA is stopped before NOR mutation;
- the audio buffer is cleared and the existing audio engine restarted afterward;
- a failed operation consumes the request instead of hammering flash each main-loop iteration.

The current prototype gesture is SAVE while already on KEEP. Final physical interaction mapping belongs to Phase 9.

## Verification boundary

Verified now:
- desktop format/journal tests;
- fake-NOR save/reboot/failure recovery;
- ASAN/UBSAN;
- real JieLi target compilation/link/resource accounting;
- empty-flash pinned-emulator boot preserving frozen P6/Phase-5 PCM and runtime telemetry.

Not verified without a physical FM-1:
- real NOR timing;
- physical power interruption at arbitrary program/erase points;
- real DAC/UI behaviour around save/restart;
- actual ISR headroom.

## Frozen invariants

Phase 7 does not change:
- `sizeof(ApsisRuntime) = 21,928 B`;
- the one shared M3c event heap/scheduler;
- Phase-5 dry PCM reference;
- Phase-6 active/dry-only cursor and late/overflow gates;
- source/output rate 44.1 kHz;
- target Cathedral quarter-rate adaptation;
- `.apsis_space_pool` placement.