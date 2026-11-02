# Phase 11E — bounded capacity / high-water inventory

P11E is a measurement checkpoint. It changes **no firmware capacity**.

Authoritative green run: `41322747611` on technical head `a294e6da89d82df1bd77f147c70c3ae032d34232`.

## Why this phase exists

P11D identified the largest static RAM owners, but static size alone does not prove that a bounded runtime structure is oversized. P11E therefore reuses the source-derived worst-case stress gates and the counters already built into the engines before selecting any capacity-reduction candidate.

## Frozen capacity evidence

| Structure | High-water / proven max | Capacity | Reserve | Decision |
|---|---:|---:|---:|---|
| shared M3c/M4c event queue | 415 | 512 | 97 | unknown reserve; no shrink |
| M3c Spiral late slots | 59 | 64 | 5 | minimal; veto shrink |
| M3c Bloom late slots | **44 proven max** | 64 | **20** | candidate for P11F falsification |
| String transient logical | 80 | 80 | 0 | saturated; veto shrink |
| String transient physical | 112 | 112 | 0 | saturated; veto shrink |
| String Attractor pending | 60 | 64 | 4 | minimal; veto shrink |
| Clock finite pool | 23 | 32 | 9 | measured reserve; low-priority owner |
| Spiral DSP legs | 30 | 32 | 2 | one complete hit reserve; veto shrink |
| Bloom DSP petals | 125 | 136 | 11 | retain existing stress margin |

Additional frozen stress facts:
- String legal 30-ms burst: 480 steals, 0 drops, physical high-water 112/112;
- String BODY/composite stress: logical 80, physical 108, 410 steals, 0 drops;
- Spiral DSP: 384 hits over 8 s, logical high-water 15, leg high-water 30, oscillator high-water 90, 0 steals/drops;
- Bloom DSP: 186 triggers, petal high-water 125/136, oscillator high-water 375, 0 drops, frozen hash `5881bb36`;
- M3c extreme: queue 357/512, Spiral slots 59/64, Bloom slots 44/64;
- M4c String-Attractor extreme: queue 415/512, pending 60/64.

## Exact Bloom scheduler upper bound

P11E does more than observe a 44-slot high-water. It proves 44 is the exact legal maximum under the locked transport/Bloom contract.

Source bounds used by the proof:
- maximum Clock rate: 180 BPM, division 1/4;
- maximum Bloom density: at most one Bloom occurrence per transport pulse;
- maximum Bloom unfurl: 3600 ms;
- Bloom occurrence lead: 45 ms;
- native Bloom bind lead: 50 ms.

The final petal therefore keeps a scheduler slot alive for at most:

`3600 ms - (50 ms - 45 ms) = 3595 ms`

At 44.1 kHz this is exactly **158,540 samples** on the native integer timing path.

`tests/test_p11_bloom_slot_bound.c` exhausts every representable legal swing value at the fastest pulse rate and both starting parities. It proves:
- shortest possible span of 43 consecutive pulse intervals: **156,739 samples**;
- shortest possible span of 44 consecutive pulse intervals: **161,700 samples**.

Thus 43 intervals can fit inside one maximum slot lifetime, allowing 44 simultaneous occurrences, but 44 intervals cannot fit. **45 simultaneous Bloom slots are impossible.**

The existing M3c extreme trace independently reaches 44/64, so the bound is tight rather than theoretical slack.

## Why the other large pools are vetoed

### String transient pool

The legal 30-ms performance burst reaches both ceilings: 80/80 logical and 112/112 physical, with stealing already active. Reducing either capacity would change a legal source-derived behavior envelope.

### Spiral

The DSP stress reaches 30/32 legs, where two legs equal one complete Spiral hit. M3c also reaches 59/64 late-binding slots. Both reserves are intentionally small and are not P11 candidates.

### String Attractor and shared queue

String Attractor reaches 60/64 pending entries. The shared event queue reaches 415/512, but P11E has no mathematical global maximum proof for that queue. Observed reserve alone is not authority to shrink it.

### Bloom DSP petal pool

The 136-petal pool was already sized around a 125-petal source-derived worst case with an intentional multi-petal margin. P11E does not reopen that prior accepted capacity decision.

## P11F candidate

The only high-value static-RAM candidate with a new exact structural reserve is:

`APSIS_RUNTIME_BLOOM_SLOTS`

This constant sizes both:
- `ApsisRuntime.bloom_slots[]`; and
- `ApsisBloomAudioRuntime.snapshot[]`.

Host layout reports:
- `sizeof(ApsisRuntimeBloomSlot) = 56 B`;
- `sizeof(ApsisBloomAudioSnapshot) = 96 B`.

A conservative `64 -> 48` falsification patch therefore projects a host-layout reduction of `16 * (56 + 96) = 2,432 B`, while still retaining four entries above the proven absolute maximum of 44.

**This is only a projection.** P11F must measure the actual JieLi `.data/.bss` delta. P11E does not claim target bytes saved.

## P11F acceptance boundary

A Bloom-slot reduction is acceptable only if all of the following hold on an isolated staging branch:
1. `APSIS_RUNTIME_BLOOM_SLOTS = 48` survives the exact P11E upper-bound and source-derived stress gates;
2. no Bloom audio-runtime capture/bind miss or overflow is introduced;
3. the real JieLi production target measures a positive RAM reduction from the frozen P11C baseline;
4. frozen idle PCM/runtime remains exact;
5. active Motion lifecycle remains exact;
6. all relevant historical host/target regressions remain green;
7. no physical ISR, stack, LCD or storage claim is inferred from host/emulator evidence.

If any gate fails, 64 remains authoritative.
