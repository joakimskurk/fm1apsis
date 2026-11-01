# Phase 11D — target memory ownership inventory

P11D is an instrumentation checkpoint. It does **not** optimise or resize firmware memory.

## Why this phase exists

P11C recovered 9,564 B of POOL without changing tested musical behavior. The current production target is now:
- image 140,408 / 581,564 B
- RAM 89,716 / 98,304 B
- POOL 319,176 / 344,064 B

RAM therefore has less absolute headroom than POOL. That does not by itself justify shrinking anything: the next step is to identify ownership from the real target ELF/linker output.

## Measurement path

P11D regenerates the exact frozen P11C production target with the pinned Dinghy/JieLi stack and captures:
- `objdump -h` section inventory;
- `objdump -t` symbol inventory;
- generated `app.ld`;
- the exact target resource/package report;
- generated build-file names/sizes.

`ci/p11_memory_inventory.py` then reports:
- largest ordinary `.data/.bss`-class symbols;
- special/dedicated RAM/POOL symbols and sections;
- section sizes/VMAs;
- stack-named ELF symbols and stack-related linker lines, if any;
- ordinary-RAM attribution versus Dinghy's aggregate `.data + .bss` metric.

## Hard ownership guard

The P11C Motion store must appear in the real ELF as:
- symbol `apsis_string_motion_store`;
- section `.apsis_motion_pool`;
- size exactly 9,644 B.

If that is not true, P11D fails rather than producing a misleading ownership report.

## Stack interpretation boundary

P11D may discover configured stack symbols, linker reservations, boundaries or sizes. Those are static facts only.

They are **not**:
- runtime stack high-water;
- physical FM-1 stack headroom;
- proof of ISR/callback timing margin.

A later phase may add runtime instrumentation when a trustworthy path exists. Emulator wall time must not be promoted to physical timing evidence.

## Acceptance rule

P11D is complete when:
1. the exact P11C target regenerates;
2. ELF section/symbol output is parseable;
3. Motion POOL ownership is independently confirmed;
4. ordinary and special RAM owners are ranked without double-counting exact symbol aliases;
5. stack findings are explicitly labelled static/configured only;
6. evidence is uploaded as raw and derived files.

Only then may a concrete RAM optimisation candidate be selected.
