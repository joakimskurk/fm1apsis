# P9F KEEP / CHAIN physical-control contract

Date: 2026-10-31
Status: **host contract frozen; generated target/LED/persistence-save binding remains P9H.**
Base: P9E2 `93fc70d36747efa55391d454758aa8eb5d49551f`.

## Authority

Study 126 is behavioural authority. Native `ApsisKeepBank` (8 snapshots) and `ApsisChainSequence` (32 cells) remain the only musical memory models. P9F adds no shadow project state.

## KEEP

The first eight physical note keys address KEEP slots 1..8. Source semantics are preserved:

- tap a filled slot -> select and recall it;
- tap an empty slot -> select it without recalling;
- SAVE while on KEEP -> capture to the selected slot; if no valid selection, use the first empty slot; if all are full, use slot 8;
- recall uses native `apsis_keep_apply(..., options=NULL)` and therefore recalls musical memory, not engine/global settings.

Study 126 ERASE removes the selected KEEP slot and removes every CHAIN cell referencing that memory. FM-1 maps this contextual ERASE to **REC while on KEEP**. This deliberately replaces the older `UI_MAPPING.md` suggestion `EDIT+SAVE+slot`: SAVE already owns KEEP navigation/capture and EDIT owns global CHAIN navigation, so the three-control chord would create an avoidable routing collision. Erase still operates on the selected slot exactly as the source does.

Erasing the final memory reference also leaves the native CHAIN sequence in a valid stopped/empty state. This is a defensive native-index normalization around the source's array filtering, not a new musical behavior.

## CHAIN

On CHAIN, the first eight note keys refer to KEEP 1..8:

- tap a filled KEEP key -> append a native default Chain cell to the tail and select it;
- empty KEEP slots do nothing;
- a full 32-cell Chain refuses append.

Editing the selected cell:

- SELECT -> select Chain cell cyclically;
- PRESETS -> change the selected cell's KEEP memory, skipping empty KEEP slots;
- K1 -> HOLD CYCLES `0..32`, step 1;
- K2 -> HOLD FINE `0..ClockN-1`, step 1;
- K3 -> CHANCE `0..1`, source step `0.05`;
- K4 -> TRANSPOSE `-12..+12`, step 1 semitone;
- ALGORITHM -> TRANSITION `{CUT, 1/4, 1/2, 1 cycle}` using native morph-quarter values `{0,1,2,4}`.

The browser source rule after cell edits is retained: `cycles=0` and `fine=0` is not allowed; that state becomes **fine=1 pulse**. This is applied before native normalization so physical editing does not silently become one whole cycle.

Context actions:

- REC -> duplicate selected cell immediately after itself, max 32;
- EDIT held + REC edge -> delete exactly the selected cell;
- SEL held + PLAY edge -> Chain START/STOP intent. P9A already suppresses the global transport toggle when SEL is held, so this does not create a second transport action.

Deleting the active cell reports `apply_current` to P9H when Chain is playing; P9H must then use the existing Chain apply/runtime path rather than inventing an immediate UI-only recall.

## Persistence / MOTHER

KEEP and CHAIN structural edits mutate the native bank/sequence just as the browser source does. They are project-memory edits, not MOTHER `->1` parameter edits. P9H must mark project storage dirty/deferred and let Phase-7 persistence save outside audio; it must never write flash from the control callback/audio path.

KEEP recall and Chain application continue to use native KEEP/CHAIN semantics and lane scopes. No private scheduler or clock is introduced.

## Verification boundary

Host tests cover KEEP select/recall/capture/erase + Chain reference cleanup, valid-slot append, Chain cursor selection, filled-slot PRESETS navigation, source bounds/steps for cell parameters, the zero-cycle/zero-fine rule, duplicate/delete, active-delete handoff, and SEL+PLAY transport intent. The suite runs normally and under ASAN/UBSAN. Physical FM-1 ergonomics remain unverified.
