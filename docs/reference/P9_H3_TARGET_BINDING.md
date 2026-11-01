# Phase 9H3 — generated-target structural control binding

H3 is the first target checkpoint that binds the host-proven P9B–G dispatcher to the generated FM-1 firmware.

Scope:
- consume the existing FM-1 edge word without adding continuous encoder/note polling;
- bind CLOCK/GRAVITY/STRING-state/DRIFT/SPIRAL/BLOOM-parameters/KEEP/CHAIN/MOTHER/SPACE control state;
- route Gravity through `apsis_runtime_request_gravity()`;
- route queued structural edits through the existing MOTHER runtime commit callback;
- request only the existing Phase-7 deferred project save path for KEEP/CHAIN edits;
- keep direct String pluck and Bloom core/petal transient emission reserved for H4;
- preserve the frozen P8 audible PCM and cursor when controls are idle.

The generated target marks real note/encoder/release activity in the already-consumed Dinghy button-edge word. No extra idle scan loop is introduced. This marker is target-only and does not change the pinned upstream source on disk.

The CHAIN active-cell deletion path reuses the existing single-TU `runtime_apply_chain_cell()` implementation so the source-equivalent immediate apply keeps the frozen RNG and morph law rather than duplicating it in the UI adapter. This is a target bridge detail, not a second musical scheduler.
