# Phase 10F — Spiral Attractor editor contract

Status: host-first staging contract. Target binding is forbidden until the host and sanitizer gates are green.

## Authority and invariants

The existing M3c/M4f Spiral Attractor remains the musical and acoustic authority. Phase 10F does not add a scheduler, event queue, attractor store, musical RNG, renderer or DSP path.

Existing `ApsisSpiralState` fields remain authoritative:

- `attractor_rate` — legal source rates 1x / 2x / 4x;
- `attractor_length` — legal source lengths 8 / 12 / 16 / 24;
- `attractor_lock_q15`;
- `attractor_density_q15`;
- `attractor_seed`;
- `attractor_gates`;
- `attractor_energy_q15[24]`.

The frozen `apsis_spiral_reset_attractor(..., 1)` function remains the sole reroll/regeneration authority. REC must call it directly. P10F must not consume the shared M3c runtime RNG or duplicate its deterministic LCG.

The existing route remains an ordered list of raw Spiral point ids. Attractor editing must never append, insert, move, delete or reorder route points.

## ROUTE / ATTRACTOR context

On the SPIRAL surface, relative `ALGORITHM` detents toggle between ordinary Phase-9 ROUTE editing and the Phase-10 ATTRACTOR editor. An odd number of detents toggles once; an even number leaves the current context unchanged.

Leaving SPIRAL clears ATTRACTOR context. The editor context and selected attractor cell are UI/control-session state only and are not persisted musical state.

While ATTRACTOR context is active, the input frame is consumed before the frozen P9 Spiral route dispatcher. This is mandatory because the same physical controls have different meanings:

- note keys must toggle attractor gates, not append/insert route points;
- K1 must edit attractor rate, not move a selected route point;
- REC must reroll the attractor, not delete a route point.

Global navigation/transport remain outside this intercept and continue to work normally.

## Direct gate and energy editing

The first N physical note keys map directly to attractor cells `0..N-1`, where N is the normalized existing attractor length (8/12/16/24).

A note-key edge:

1. toggles that cell's bit in `attractor_gates`;
2. makes that cell the editor's selected energy cell.

Multiple simultaneous note edges toggle every addressed cell exactly once. The lowest addressed cell becomes the deterministic selected cell. Keys outside N are consumed but do not mutate state. Manual gate edits preserve bits outside the active length.

K4 edits `attractor_energy_q15[selectedCell]` in 0.01 detents, bounded by the frozen M4f acoustic range 0.12..1.00. Selecting a gate cell and moving K4 in the same frame edits that newly selected cell.

## Other attractor controls

- K1: attractor rate, discrete and saturating across 1x / 2x / 4x;
- K2: lock, 0.01 detents, bounded 0..1;
- K3: density, 0.01 detents, bounded to the existing regeneration range 0.15..0.90;
- K4: selected-cell energy as above;
- REC: reroll through `apsis_spiral_reset_attractor(..., 1)`.

The editor deliberately does not invent a physical mapping for attractor length. P10F respects the existing 8/12/16/24 state and exposes the corresponding number of direct keys. A length mapping may only be added later from an approved source-backed interaction contract.

Processing order for a single physical frame is deterministic:

1. rate / lock / density edits;
2. REC reroll using the possibly updated density;
3. direct gate toggles and cell selection;
4. selected-cell K4 energy edit.

Thus a rare REC+key frame toggles the requested gate on the newly rerolled pattern rather than being overwritten by reroll.

## Reroll semantics

REC delegates to the frozen Spiral source implementation. It may change seed, active gates and all generated energy values exactly as that authority defines. It must not mutate route identity or introduce session RNG state.

For the locked source vector `seed=911`, density=.58 and length=24, the existing authority produces seed 4825 and active gate mask `0x00FDA5FD`; P10F tests retain that vector rather than reimplementing it.

## Host gates before target integration

P10F host tests must prove:

1. ALGORITHM toggles ATTRACTOR context only on SPIRAL and leaving SPIRAL clears it;
2. inactive editor is a strict no-op;
3. first N keys toggle all addressed gate cells, preserve hidden bits and deterministically select an energy cell;
4. 1x/2x/4x rate stepping and lock/density/energy bounds;
5. gate-only edits do not alter seed or energy;
6. REC uses the frozen deterministic reset result and leaves route identity unchanged;
7. REC+key ordering applies the key toggle after regeneration;
8. active editor frames explicitly request suppression of P9 route dispatch;
9. normal and ASAN/UBSAN builds pass;
10. the editor source contains no runtime scheduler/event/RNG ownership.

## Target integration boundary

After the host checkpoint is green, generated FM-1 integration may add only finite editor-session state, routing and existing-P8 redraw invalidation. It must preserve:

- Phase-9 ROUTE editing whenever ATTRACTOR context is inactive;
- existing M3c/M4f scheduler, late binding, acoustic planning and DSP;
- existing 24-cell outer-orbit Spiral visualization and runtime probe;
- frozen Phase-9 idle PCM/runtime regression;
- P10D3 String Motion and P10E String Attractor behavior;
- no new POOL allocation.