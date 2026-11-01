# Phase 10G — selected Gravity-point editor contract

Status: host-first contract. Target binding is forbidden until host + sanitizer gates are green.

## Authority and invariants

Phase 10G edits the existing `ApsisGravityState`/`ApsisGravityPoint` grid directly. It does not add a scheduler, pending musical shadow, point store, RNG, DSP path or framebuffer.

The existing 24 points remain indexed exactly as the source renderer/runtime already expects:

```text
point_index = degree * APSIS_GRAVITY_RINGS + ring
```

with 8 degrees × 3 rings.

Existing point fields remain authoritative:

- `enabled`
- `offset` bounded -2..2
- `octave` bounded -1..1
- `level_q15` bounded 0.15..1.25
- `brightness_q15` bounded 0.15..0.78
- `attack_ms` bounded 12..220
- `release_ms` bounded 250..3000

The existing Gravity degree-change request/pending semantics remain untouched. Point editing is live grid/pointer editing; the MOTHER `->1` pending structure has no GravityPoint keys, so P10G must not invent a parallel pending model.

## Gravity subpages

While on GRAVITY, relative `ALGORITHM` detents cycle three UI/control pages:

0. HARMONY
1. TIMING / GRID
2. SELECTED POINT

Only page 2 is owned by P10G. Pages 0 and 1 leave the existing Phase-9 Gravity degree/ring interaction untouched. Leaving GRAVITY resets the UI-only page to HARMONY.

The page selector is control-session state only and is not musical/persisted state.

## Point selection without audition

On SELECTED POINT:

- plain degree keys retain the existing Phase-9 Gravity degree request;
- `SEL + degree key` selects `gravity.selected_point_degree` without requesting/auditioning that degree;
- if several direct degree keys arrive in one frame, the lowest degree is selected deterministically and all direct degree-key edges are consumed for that selection frame;
- the selected ring remains the existing `gravity.selected_ring`, still controlled by the Phase-9 OCT-/OCT+ ring gesture.

Target integration therefore must **not** suppress the entire Gravity dispatcher. It filters only the direct degree-note edges from the P9 dispatch frame when P10G reports `suppress_degree_request`.

## Selected-point controls

The FM-1 has six relative encoders besides ALGORITHM, enough to expose all six numeric point fields without a hidden parameter bank:

- K1: `offset`, integer detents, -2..2
- K2: `octave`, integer detents, -1..1
- K3: `level_q15`, 0.01 detents, 0.15..1.25
- K4: `brightness_q15`, 0.01 detents, 0.15..0.78
- SELECT encoder: `attack_ms`, 4 ms per detent, 12..220 ms
- PRESETS encoder: `release_ms`, 25 ms per detent, 250..3000 ms
- REC: toggle `enabled`

A `SEL + degree` selection and encoder edit in the same input frame edits the newly selected point.

The 4 ms / 25 ms hardware steps are control-resolution adaptations only; the source bounds and stored millisecond values remain authoritative.

## Existing visual authority

The P8 Gravity renderer already draws all 24 enabled states and highlights `selected_ring + selected_point_degree`. P10G therefore needs only existing-surface redraw invalidation after selection/point changes; it must not allocate another framebuffer or replace the Gravity visual.

## Host gates before target integration

Host tests must prove:

1. ALGORITHM wraps deterministically across the three pages and leaving GRAVITY resets to HARMONY;
2. non-POINT pages are strict no-ops for P10G point editing;
3. plain degree keys remain unsuppressed;
4. `SEL+degree` selects the exact degree, requests suppression of direct degree audition, and does not mutate point parameters;
5. point indexing is degree-major/ring-minor (`degree*3+ring`);
6. all six numeric controls use the existing fields and source bounds;
7. REC toggles only the selected point's enabled state;
8. same-frame selection + parameter edit targets the newly selected point;
9. no neighboring point is mutated;
10. normal + ASAN/UBSAN builds pass;
11. editor source owns no runtime scheduler/event/RNG/pending musical state.

## Target integration boundary

After host green, target binding may add only finite page/session state, filtered P9 dispatch input for `SEL+degree`, existing-P8 redraw invalidation and test telemetry. It must preserve:

- ordinary Phase-9 Gravity degree requests for plain keys;
- OCT ring selection;
- M3c Gravity quantization/pending semantics;
- P10D3 Motion, P10E String Attractor and P10F Spiral Attractor;
- frozen Phase-9 idle PCM/runtime;
- no new POOL allocation.
