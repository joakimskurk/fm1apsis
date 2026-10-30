# Phase 7 — KEEP / CHAIN persistent project contract

Status: **P7a host contract / journal implementation**

This document freezes the first APSIS project-storage format before target flash integration. It does not change the M3c scheduler, any source DSP, Space/Cathedral, or `ApsisRuntime`.

## Persisted musical/editor content

A project contains:

- all 8 KEEP snapshots;
- KEEP selected slot (`-1` or `0..7`), because selection affects the next capture destination;
- all 32 CHAIN cells up to the persisted chain count;
- CHAIN count and editor-selected cell (`-1` or within count);
- the 13 CHAIN settings from `ApsisState.chain`, because sync/default/scope settings change how the same cells behave.

CHAIN transport/runtime state is **not** persistent: `playing`, `armed`, `pos`, `pulse`, `phrase_pulse`, `master_pulses`, `hold_tail` and `last_applied` are reset on load. Loading a project therefore cannot resume halfway through a phrase or import stale scheduler state.

## Wire format v1

Native C structs are never written to flash. The format is explicit little-endian bytes, so compiler padding/alignment cannot silently change a project.

- magic: `APS7` (`0x37535041` as little-endian `uint32_t`)
- format version: `1`
- header: 24 bytes
- payload: 816 bytes
- complete record: 840 bytes
- CRC: standard CRC-32 / IEEE (`0xEDB88320`, check value `123456789 -> 0xCBF43926`)

Header fields:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | magic |
| 4 | 2 | format version |
| 6 | 2 | header size |
| 8 | 4 | payload size |
| 12 | 4 | generation |
| 16 | 4 | payload CRC-32 |
| 20 | 4 | header CRC-32 over bytes `0..19` |

Payload layout:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 1 | KEEP selected slot (`0xFF` = none) |
| 1 | 1 | CHAIN count (`0..32`) |
| 2 | 1 | CHAIN selected cell (`0xFF` = none) |
| 3 | 576 | 8 × 72-byte KEEP snapshots |
| 579 | 224 | 32 × 7-byte CHAIN cells |
| 803 | 13 | CHAIN settings |

Invalid/unused KEEP slots encode as canonical zeroes. CHAIN cells beyond `count` encode as canonical zeroes. Decode clamps enum/range fields and then resets transient CHAIN runtime state.

## Two-sector journal

Each journal slot owns one 4 KiB erase sector. Saves alternate between A and B.

Write order is mandatory:

1. choose the older/invalid sector;
2. erase that sector;
3. program payload bytes first at offset 24;
4. verify payload/readback on target;
5. program the 24-byte header **last**;
6. validate the complete record before considering the save committed.

A power cut before the final header is complete leaves that sector invalid. Boot scans both sectors, validates magic/version/sizes/header CRC/payload CRC, then selects the newest valid generation with wrap-safe sequence comparison. If the newest record is corrupt, the older valid record remains recoverable.

Desktop tests cover erased slots, payload-only interrupted writes, partial headers, complete commits, later corruption fallback and generation wraparound.

## FM-1 storage selection

The selected prototype sectors are:

- A: `0x000F4000`
- B: `0x000F5000`

They are deliberately outside Dinghy's one-sector settings store at `0x000FC000`.

Why these addresses:

- the pinned Dinghy flash driver maps the upper FM-1 data region into the plain XIP window, but the pinned `platform.c` currently narrows writes to OTA + settings only;
- newer Dinghy explicitly reserves FM-1 blob storage beginning at `0x000F4000`, below settings, confirming this region is intended for application-owned persistent data;
- Phase 7 target integration will backport only the narrow two-sector allow-list needed by APSIS rather than changing the pinned upstream baseline wholesale.

The APSIS record uses its own magic/version. Replacing APSIS with another firmware may overwrite firmware-owned data in the same general storage family; cross-firmware preservation is not promised by this prototype format.

## Audio-safety rule for target integration

The FM-1 NOR erase/program path disables interrupts while XIP is unavailable. Therefore no project flash operation may be launched from the audio callback, and no erase/program may run while APSIS expects audio callbacks to continue.

Target integration must use a deferred request from the main loop. A save is serviced only in a deliberate storage-safe window (transport stopped, audio renderer quiesced); payload/header are prepared in RAM before flash access. This keeps flash stalls out of the shared scheduler/audio path. Hardware timing remains unverified until a physical FM-1 is available.

## Frozen invariants

Phase 7 must not change:

- `sizeof(ApsisRuntime) = 21,928 B`;
- the one shared M3c event heap/scheduler;
- Phase-5 dry PCM reference;
- Phase-6 active/dry-only cursor and late/overflow gates;
- source/output rate 44.1 kHz;
- target Cathedral quarter-rate adaptation;
- `.apsis_space_pool` placement.
