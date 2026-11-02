# Margin plan: APSIS on the FM-1 with room to spare

Written 2026-11-02, from CI's FM1_STRICT run on `main` 4a54af2 (20 s scripted performance, 3364 blocks).
Budget: 256 frames at 44.1 kHz = 5805 us per audio block, on core 0.

## Where we really are

| part (app.c `block_part`) | in the worst block | its own worst |
| --- | ---: | ---: |
| 0 updates + Clock triggers | 174 | 370 |
| 1 String drone | 650 | 703 |
| 2 Drift | 344 | 407 |
| 3 Clock | 77 | 274 |
| 4 String notes | 1781 | 1781 |
| 5 runtime + Spiral + Bloom | 17 | 1058 |
| 6 triggers (strum, rhythm, pluck, Bloom manual) | 0 | 355 |
| 7 room + Motion | 1004 | 1065 |
| **total** | **4051 (69.8 %)** | **6013 (104 %)** |

What this says:

- **The margin is luck.** The worst block was 69.8 %, but each part's own worst adds up to 104 %. A block in
  which they coincide (a dense strum while Bloom grows and the room is busy) drops out. The 20 s script did not
  happen to stack them.
- **The String notes are unbounded in practice.** The pool allows 80 partial oscillators. The script reached 24 at
  1781 us, about 74 us each. 80 would take about 5.9 ms, which is the whole block on its own.
- **Control work runs in the audio interrupt.** Planning (Bloom grow and voicing, Spiral, the attractor) and note
  setup cause the spikes in parts 0, 5 and 6. They are not sound; they only need to be ready in time.
- **The room** (FDN + early room, quarter rate) has the most cache misses in the profile and costs about 1 ms
  steadily.
- **The mix** (`apsis_space_target_commit_source`, 5.6 % of all CPU) makes an extra pass over each source's buffer.
- **The model is not the chip.** FM1_STRICT is a cycle and cache model, and nobody has compared it with a real FM-1
  yet. USB and the key scan interrupt the audio on top of this.

## The stress run (step A, ca04914)

The APSIS_STRESS build turns every source to its densest setting and plays plucks at the 35 ms guard and Bloom
petals with grow. All of these settings can be reached from the panel.

| part | own worst (us) |
| --- | ---: |
| planning | 17809 |
| Spiral + Bloom sound (59 petals) | 15457 |
| String notes (80 logical / 112 physical partials) | 10137 |
| Clock | 1415 |
| room + Motion | 1120 |
| triggers | 811 |
| String drone | 702 |
| updates + Clock triggers | 637 |
| Drift | 400 |

The average was 302 % and the worst block 483 %. Only 970 of about 3445 blocks were rendered, so the audio
collapses.

**Correction (82e2096, the probes now say when each worst happens).** The 17.8 ms "planning" was the **first
blocks after boot**: the tables fill and the first plans are made once (22.7 ms in the stress build, 3.3 ms in the
normal one). These blocks are now measured apart. Once it is playing, the stress build's own worsts are Spiral +
Bloom sound 15.3 ms, String notes 10.2 ms, planning 2.1 ms, Clock 1.4 ms, room 1.1 ms; the bound is 560 %.

That gives the order of the work:

1. **B, the voice budget, comes first.** It must cover String notes, Spiral and Bloom. A profile of the stress
   build puts 34 % of all CPU in the String's partials and about 22 % in Spiral, which has a biquad per voice.
2. **Boot:** warm the tables and the first plan before the audio starts, so the first blocks don't click.
3. **C** is smaller than it looked (planning 0.8 ms normal, 2.1 ms stress) but still belongs in the main loop.
4. **The room (D)** is a small part of the worst case; it matters for the steady average.


Margin means a **bound by construction**: no input, however dense, can make the audio interrupt exceed its
budget. It does not mean a good number from one script.

On core 0 under FM1_STRICT:

- **the bound** (the sum of each part's hard worst case) is **50 % or less** (2900 us)
- the measured worst in the stress script is 40 % or less
- the average is 30 % or less

That leaves about 2x against model error and the other interrupts. It also leaves core 1 free.

Target per part, in us:

| part | now (own worst) | target | how |
| --- | ---: | ---: | --- |
| String notes | 1781 (unbounded) | 600 hard | B |
| String drone | 703 | 300 | E |
| Drift | 407 | 300 | E |
| Clock | 274 | 274 | (as is) |
| Spiral + Bloom sound | in part 5 | 400 hard | B (the same voice budget) |
| control work in the interrupt (0, 5, 6) | 1783 | 300 | C |
| room on core 0 | 1065 | 100 (core 1) / 600 (fallback) | D |
| mix | in each part | 150 | E |
| **bound** | **6013** | **about 2400 (41 %)** | |

## The steps, in order

Each step is a branch with its own CI result. A step changes the sound only where it says so. Such a change is
checked by listening against the browser and then re-pinned in `tests/sound.sha256`, with the reason in the
commit.

### A. Measure the truth (first, small)

- `tests/scripts/apsis_stress.txt`: every source on, Turing strum at the densest register, 16th-note rhythm, plucks
  as fast as the guard allows, Bloom grow, Motion, CHAIN, knob sweeps on the room.
- emu.sh reports the **bound** (the sum of the parts' own worsts) next to the measured worst, and warns when it is
  over the budget. It becomes a gate (50 %) once B to D are done; until then a real drop-out still fails CI.
- The stress run is a diagnostic build (`DINGHY_DEFS=-DAPSIS_STRESS`, `apsis_stress.c`) with `STRESS=1 tests/emu.sh`.
  CI reports it but does not fail on it yet.
- Part 5 splits into planning and Spiral/Bloom sound, so each is measured on its own.
- A **load-meter build** (diagnostic, not production): avg, worst and blocks over 80 %/100 % on the screen, so
  someone with an FM-1 can take one photo. This is the only step that touches the chip question.

### B. A voice budget for all notes (the biggest redesign)

The String notes, Spiral hits and Bloom petals are all short additive notes. They share one **fixed oscillator
budget**, chosen so the hard worst case fits the 600 + 400 us above (about 16 + 8 partials at today's per-partial
cost, more after the cost work).

- When the budget is full, the allocator drops the **least audible** partial: root-shadow partials first, then a
  note's highest partials, then the quietest and oldest. It never drops a note's fundamental while it can still be
  heard. Study 98 itself lets notes decay below hearing; we stop rendering a partial once it is below -60 dB.
- **Same-pitch lanes:** most String notes play the current tension pitch. Notes on the same pitch and partial can
  share one oscillator, with their envelopes summed, so the cost grows with distinct pitches, not with notes.
  This changes the sound: a new note continues the running phase instead of starting at 0, which changes how
  overlapping notes reinforce each other. It is done only if it passes a listening test against Study 98 (an A/B
  of the stress script rendered both ways).
- Partial cost: one oscillator form for all partials (the triangle + lowpass of partials 1 and 2 as a precomputed
  band-limited wavetable), a block-rate envelope, and no per-sample branches.

### C. Control work out of the audio interrupt

- Planning (Bloom grow and voicing, Spiral routes, the attractor register, rhythm observation, the pitch math of a
  note's setup) moves to the main loop. It runs one or two blocks ahead and fills a lock-free queue of
  **sample-stamped** events. The interrupt only consumes events that are ready-made.
- Timing stays sample-exact, because the events carry their target sample; only the planning moves earlier.
- If the main loop is late (the screen or a save), the interrupt still plays what is queued. Events planned too
  late are counted in a probe, never computed in the interrupt.
- This is the same rule Dinghy's RULES.md gives: the interrupt renders, the loop thinks.

### D. The second core: two jobs a block (redesigned 2026-11-02, from the measured parts)

The way the FM-1 firmwares already do it (Melodee, X0X, SLOOP, fm1-dock; fm1-dock's `fm1_cpu1.h`, GPL-3.0 like
ours, with its attribution chain):

- Core 1 waits in RAM (`.c1_text`).
- Inside the audio interrupt, core 0 hands it one job (a function and an argument), does its own share at the
  same time, and waits for the job before it goes on.
- There is no added latency: both halves belong to the same block.
- If core 1 does not answer within 8 ms, it is held for good and core 0 does the job itself. That is the
  one-core fallback for the units whose core 1 is flaky (fm1-emu `--flaky-core1` tests it).

The plan in two phases, with the stress build's worsts after the voice budget (us):

| phase | core 0 | core 1 |
| --- | --- | --- |
| A, from the block's start | updates + Clock triggers 487, planning 955, triggers 581 = 2023 | String drone 513, Drift 424, the room on the previous block's sends 1128 = 2065 |
| B, once the notes are set up | String notes 1652 + half of the rest | Spiral + Bloom 1659, Clock 1181, shared out to balance |
| per block | A 2065 + B ~2250 = **~4.3 ms (74 %)** | |

The room moves one block late, which adds 5.8 ms of pre-delay to a 72 ms pre-delay and is inaudible; it is
checked by listening. Everything else stays sample-exact.

To take over from fm1-dock:
- `fm1_cpu1.h` and its entry (`fm1_cpu1.S`)
- the `.c1_text` section and core 1's stacks in the linker script
- core 1's stop and the PC-limit guard (`fm1_sys.h`, `fm1_guard.h`)

PLATFORM.md lists them as differences from Dinghy.

### E. The steady costs

- **The mix:** each source renders straight into the dry and send buses with its gains, so the
  `commit_source` pass goes away. This keeps the sound bit-identical.
- **The String drone** (4 oscillators at 703 us, about 0.7 us each per sample, which is too much): sine pairs
  from the shared table, and the timbre and lowpass coefficients at block rate.
- **Drift:** the same treatment.

### F. Confirm on a chip

- The load-meter build from A goes to someone with an FM-1 (Joakim Eriksson or the FM-1 community), together with
  the stress script as a short playing guide.
- Their numbers are compared with FM1_STRICT for the same build, and the gap is written into TESTING.md. If the
  chip is slower than the model, the 2x margin is there to absorb it, and we know by how much.

## What does not change

APSIS's sound and behaviour (the browser is the authority), the Study 98 String, the five sources, SPACE, Motion,
Bloom and CHAIN. Only how and where the work is done changes. Where a step would change the sound (B's lanes, D's
latency, the fallback room), it says so and is checked by listening.
