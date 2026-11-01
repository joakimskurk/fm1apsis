# Phase 11F — conservative Bloom slot reduction

P11F is the falsification phase selected by P11E.

## Candidate

Change only:

`APSIS_RUNTIME_BLOOM_SLOTS: 64 -> 48`

The constant sizes both the M3c Bloom late-binding slots and the corresponding Bloom audio snapshot array.

P11E proved:
- 44 simultaneous Bloom scheduler slots are actually reached by the source-derived extreme trace;
- 44 is the exact legal upper bound under the locked transport/Bloom timing contract;
- 45 simultaneous slots are structurally impossible;
- host layout projects 56 B per runtime slot + 96 B per audio snapshot.

48 therefore retains four slots above the proven maximum while removing 16 unreachable entries from both arrays.

## Historical-test handling

The old M4c host test intentionally freezes `sizeof(ApsisRuntime) == 21928` in addition to its behavioural assertions. P11F is specifically intended to reduce that size, so `ci/p11f_prepare.sh` changes only that one expected-size assertion to the projected P11F host size (21,032 B) in the ephemeral CI workspace, runs the complete existing host/sanitizer suite, then restores the checked-in historical test byte-for-byte.

The P11E historical workflow remains pinned to the pre-P11F technical head, so the old 21,928-B structural baseline remains independently reproducible.

## Veto gates

The 48-slot candidate is rejected if any of these fail:
1. full historical host behavior and sanitizer suite;
2. M3c extreme must remain queue 357/512, Spiral slots 59/64 and Bloom high-water 44/48 with no late-slot overflow;
3. M4c extreme must remain queue 415/512 and pending 60/64;
4. the exact P11E Bloom upper-bound proof must report `proven_max=44 current_cap=48`;
5. Bloom audio occurrence snapshot/bind bridge must pass normally and under ASAN/UBSAN;
6. real JieLi production RAM must be lower than the P11C baseline 89,716 B while POOL remains exactly 319,176 B;
7. frozen idle PCM/runtime must remain exact;
8. active Motion target RAM must also decrease and its pinned emulator lifecycle must remain exact.

The first green target run measures the real RAM delta. Only after that measurement may exact P11F target bytes/hashes be frozen.

## Non-claims

P11F does not measure or prove physical FM-1 ISR timing, stack high-water, LCD contention, NOR timing or DAC quality.
