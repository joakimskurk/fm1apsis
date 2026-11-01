# P9G MOTHER / SPACE physical-control contract

Date: 2026-10-31
Status: **P9G1 MOTHER and P9G2 SPACE host contracts frozen; P9H target binding remains.**
Base: P9F `426f9fdc34bca0e3d9943aaece84cfc1a1404645`.

## Authority

Study 126 remains behavioural authority. Native `ApsisMotherState` + `ApsisMotherPending` remain the only MOTHER state model. Native `ApsisSpaceState` remains the only shared room model. P8 already renders the real five-source MOTHER universe and the one shared Cathedral; P9 must not replace either with a generic mixer or per-source reverbs.

## P9G1 live source mixer

The browser MOTHER arcs are live controls with range `0..1.25`; keyboard editing uses `0.03` increments. FM-1 maps the same five sources directly:

- K1 -> String
- K2 -> Drift
- K3 -> Clock
- K4 -> Spiral
- SELECT -> Bloom

Each detent is `0.03` in source units, represented in the existing native Q15 convention. Values clamp at `0` and `1.25`. These edits are **MOTHER-domain live controls** and are never added to the structural `->1` pending queue.

## Source mute

Hold SEL and press the source surface button: ENV -> String, LFO -> Drift, SEQ -> Clock, ARP -> Spiral, FX -> Bloom. Mute remains the browser's source mute: native source `sound` is toggled and `mother.mute_mask` mirrors it. If several source-button edges arrive in one frame, the source order above wins deterministically. Mutes remain live while `->1` is armed.

## Structural sync

REC on MOTHER toggles `->1` through the existing `apsis_mother_set_sync()` path. Disarming clears queued structural edits exactly as the source does.

The source horizon set is retained exactly: `1, 2, 4, 8, 16, 32` Clock steps. OCT+ advances through that set and keeps the browser's `32 -> 1` wrap; OCT- is the physical inverse (`1 -> 32`). A horizon change while edits are pending re-arms `pending_start_abs` from the current shared transport absolute step through `apsis_mother_set_horizon()`.

Mixer and source mute never enter the pending queue. Structural module edits still do, and P9H remains responsible for routing all P9 structural parameter intents through the existing MOTHER hold/apply mechanism.

## P9G2 SPACE page model

SPACE remains a secondary control surface for **one shared Cathedral**, entered by the already-frozen SEL+FX global gesture. It is not a sixth instrument and does not own an audio clock.

Two FM-1 control pages are UI-only state and are never serialized:

1. `ROOM`
2. `ECHO/SEND`

ALGORITHM encoder motion cycles between the two pages. This is an FM-1 adapter only; it does not alter APSIS musical state. On `ECHO/SEND`, SELECT encoder motion chooses one of the same five source buses in order String, Drift, Clock, Spiral, Bloom. The source cursor is also UI-only.

## ROOM controls

The locked browser SYSTEM bounds and increments are retained exactly:

- K1 CATHEDRAL RETURN: `0..0.72`, step `0.01`;
- K2 DECAY: `4.0..12.0 s`, step `0.1 s`;
- K3 PRE-DELAY: `15..140 ms`, step `1 ms`;
- K4 DAMPING: `2200..11000 Hz`, step `100 Hz`.

Values are emitted as native intents: Q15 for return, milliseconds for decay/predelay, hertz for damping. P9G2 never mutates `ApsisState`.

## ECHO / SEND controls

The locked browser bounds and increments are retained exactly:

- K1 ECHO TIME: `80..1200 ms`, step `1 ms`;
- K2 ECHO FEEDBACK: `0..0.58`, step `0.01`;
- K3 selected source CATHEDRAL SEND: `0..1.00`, step `0.01`;
- K4 intentionally unassigned in P9G2 rather than inventing a non-source parameter.

Source sends address the existing `string.send_q15`, `drift.send_q15`, `clock.send_q15`, `spiral.send_q15`, and `bloom.send_q15` fields. TUNING/Temper is not silently added to the five-source MOTHER/SPACE universe.

## MOTHER hold rule for SPACE

Unlike MOTHER mixer/mutes, browser SPACE and per-source SEND sliders use the normal `applyParam()` path. Therefore every P9G2 edit is **structural** for `->1`: P9H must either apply it live when MOTHER sync is inactive, or queue/update the corresponding native MOTHER pending key when sync is active. The host layer deliberately emits intents only so it cannot bypass this rule.

## APSIS master / physical MASTER

Study 126 has an internal MOTHER output control, while the current native persisted `ApsisState` does not store a separate browser `settings.output` member. P9G does not invent a second persisted master field. The physical FM-1 MASTER remains platform output gain in P9H. Any later separate APSIS master requires an explicit state-version/persistence migration.

## P9H

P9H binds the frozen P9A-G host contracts to generated FM-1 input, MOTHER-aware structural edit application, manual String/Bloom sounding primitives, KEEP/CHAIN persistence dirtying, LEDs and native P8 surfaces. Idle target audio/runtime must remain byte/telemetry frozen.

Physical FM-1 feel remains unverified until real hardware exists.
