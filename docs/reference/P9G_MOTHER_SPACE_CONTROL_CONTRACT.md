# P9G MOTHER / SPACE physical-control contract

Date: 2026-10-31
Status: **P9G1 MOTHER host contract frozen; P9G2 SPACE and P9H target binding remain.**
Base: P9F `426f9fdc34bca0e3d9943aaece84cfc1a1404645`.

## Authority

Study 126 remains behavioural authority. Native `ApsisMotherState` + `ApsisMotherPending` remain the only MOTHER state model. P8 already renders the real five-source MOTHER universe and pending horizon; P9 must not replace it with a generic mixer page.

## P9G1 live source mixer

The browser MOTHER arcs are live controls with range `0..1.25`; keyboard editing uses `0.03` increments. FM-1 maps the same five sources directly:

- K1 -> String
- K2 -> Drift
- K3 -> Clock
- K4 -> Spiral
- SELECT -> Bloom

Each detent is `0.03` in source units, represented in the existing native Q15 convention. Values clamp at `0` and `1.25`. These edits are **MOTHER-domain live controls** and are never added to the structural `->1` pending queue.

## Source mute

Hold SEL and press the source surface button:

- ENV -> String
- LFO -> Drift
- SEQ -> Clock
- ARP -> Spiral
- FX -> Bloom

Mute remains the browser's source mute: native source `sound` is toggled and `mother.mute_mask` mirrors it. If several source-button edges arrive in one frame, the source order above wins deterministically. Mutes remain live while `->1` is armed.

## Structural sync

REC on MOTHER toggles `->1` through the existing `apsis_mother_set_sync()` path. Disarming clears queued structural edits exactly as the source does.

The source horizon set is retained exactly: `1, 2, 4, 8, 16, 32` Clock steps. OCT+ advances through that set and keeps the browser's `32 -> 1` wrap; OCT- is the physical inverse (`1 -> 32`). A horizon change while edits are pending re-arms `pending_start_abs` from the current shared transport absolute step through `apsis_mother_set_horizon()`.

Mixer and source mute never enter the pending queue. Structural module edits still do, and P9H remains responsible for routing all P9C/P9E and later structural parameter intents through the existing MOTHER hold/apply mechanism.

## APSIS master / physical MASTER

Study 126 has an internal MOTHER output control, while the current native persisted `ApsisState` does not store a separate browser `settings.output` member. P9G1 therefore does not invent a second persisted master field. The physical FM-1 MASTER remains platform output gain in P9H. Any later decision to add a separate APSIS internal master requires an explicit state-version/persistence migration rather than silently changing the project record.

## P9G2 / P9H

P9G2 freezes SPACE room/echo/send physical controls. P9H then binds MOTHER + SPACE to generated target input/LED/render flow and proves frozen audio/runtime remains unchanged at idle.

Physical FM-1 feel remains unverified until real hardware exists.
