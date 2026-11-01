# Phase 10E — String Attractor editor contract

Status: host-first contract for the Phase-10 advanced-interaction layer.

## Authority and invariants

The existing M4c String Attractor remains the musical/runtime authority. Phase 10E does **not** add a scheduler, attractor DSP, cell store, musical RNG, or shadow copy of the attractor.

Existing `ApsisStringState` fields remain authoritative:

- `filter_attractor_length` — active cell count, runtime-normalized to 4..16;
- `filter_attractor_gates` — one gate bit per cell;
- `filter_attractor_seed` — bounded seed used by the existing deterministic M4c cell regeneration;
- all existing rate/lock/strum/target/filter fields remain untouched unless a later source-backed editor contract explicitly exposes them.

Existing `apsis_string_attractor_sync()` remains responsible for regenerating cell values/cursor when seed or length changes. A gate edit must not regenerate cells.

## Explicit edit context

Phase-9 STRING performance remains the default and is not remapped.

- `ENV` enters normal STRING performance from another surface.
- while already on STRING, `SEL + ENV` enters the STRING ATTRACTOR editor.
- plain `ENV` while on STRING returns from ATTRACTOR to normal STRING performance.
- leaving the STRING surface clears the editor context.
- `SEL + ENV` on MOTHER retains the frozen Phase-9 MOTHER source-mute gesture; it never opens the editor directly from MOTHER.

This two-stage entry is deliberate: P10E must not steal the existing contextual `SEL + source` MOTHER interaction just to make a secondary editor page convenient.

The editor context is UI/control-session state only. It is not persisted musical state and must never be consulted by M3c scheduling or DSP.

While the editor is active, ordinary Phase-9 STRING performance dispatch and Phase-10 Motion gestures are suppressed for that input frame. This prevents gate keys from plucking/moving the String and prevents editor `REC` from arming Motion.

## Gate editing

The first N physical note keys map directly to attractor cells `0..N-1`, where N is the runtime-normalized attractor length (4..16).

- note-key press toggles the corresponding bit in `filter_attractor_gates`;
- multiple simultaneous key edges toggle all addressed cells;
- note keys outside N are ignored musically but remain consumed by the editor so they cannot leak into normal STRING performance;
- bits above N already present in `filter_attractor_gates` are preserved, so shortening and later re-extending the attractor does not destructively erase hidden gates.

The existing P8 STRING renderer already reads `filter_attractor_gates`; target integration invalidates only that existing incremental surface after an editor state change. No new framebuffer or renderer is introduced.

## Reroll

`REC` in the explicit ATTRACTOR editor rerolls the attractor seed. It does **not** arm String Motion.

The seed domain is `1..9999`, matching the existing M4c normalization. Reroll is implemented as a deterministic permutation step over that domain rather than consuming the shared M3c musical RNG or introducing mutable private RNG state:

```text
normalized = clamp(existing seed to 1..9999, with 0 -> 317)
next       = ((normalized - 1 + 7919) mod 9999) + 1
```

The step is non-zero, so one reroll always changes the seed. M4c then regenerates cell values through its already-frozen deterministic LCG when `apsis_string_attractor_sync()` observes the changed seed.

This preserves reproducible CI, avoids perturbing M3c random draw order, and keeps the reroll result entirely represented by existing musical state.

## Host gates before target integration

P10E host tests must prove:

1. editor context can only be entered from STRING, plain ENV exits it, and leaving STRING clears it;
2. MOTHER `SEL+ENV` remains outside the editor contract;
3. inactive editor is a strict no-op;
4. direct gate toggling is bounded to normalized length 4..16 and preserves hidden bits;
5. simultaneous gate presses toggle every addressed cell exactly once;
6. editor frames suppress ordinary STRING dispatch and Motion interpretation;
7. reroll changes only the bounded existing seed and introduces no RNG/session state;
8. `apsis_string_attractor_sync()` regenerates cells after reroll and is idempotent when seed/length stay unchanged;
9. gate-only edits do not regenerate cells or reset the M4c cursor;
10. normal and ASAN/UBSAN builds pass before generated FM-1 target binding begins.

## Target integration boundary

After the host checkpoint is green, target integration may add only the finite control-session bit and routing needed to select/apply this editor. It must retain:

- frozen Phase-9 MOTHER `SEL+source` semantics;
- frozen Phase-9 STRING performance outside ATTRACTOR context;
- P10D3 Motion ownership and mailbox behavior outside ATTRACTOR context;
- shared M3c runtime size/timeline;
- exact frozen Phase-9 idle PCM/runtime regression;
- no new POOL allocation for the editor.
