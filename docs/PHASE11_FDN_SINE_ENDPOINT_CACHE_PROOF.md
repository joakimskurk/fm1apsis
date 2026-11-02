# Phase 11Q — FDN sine endpoint-cache equivalence proof

Status: **complete, proof/projection only. Production Space remains exact P11O.**

## Authority

- proven technical head: `a0b0826c9f0951b780d84404cfdbe9543ba45b49`
- authoritative run: `41360291892` — PASS
- evidence artifact ID: `13104793891`
- evidence ZIP SHA-256: `7e7699b82e6540c64e099d4f8ea4d5a6d508e6a88d402a8c91c1d0aaa0cfcd05`
- repository production Space blobs remained exact P11O throughout the phase

P11Q is the proof counterpart to the P11P workload census. It does **not** put the candidate into production. The candidate exists only in the proof harness and in an ephemeral CI workspace target patch.

## Candidate

Per FDN line, cache the two exact signed-16-bit quarter-wave endpoints currently obtained through `space_sine_point()`:

- `lfo_sine_a_q15`
- `lfo_sine_b_q15`

Initialize both endpoints from the line's reset phase. In the render loop:

1. save the old LFO phase;
2. advance `lfo_phase` exactly as P11O does;
3. compare the old and new top byte;
4. refresh the two cached endpoints only if that top byte changed;
5. preserve the existing 8-bit fractional interpolation expression exactly.

No persistent index is needed. Source-level persistent cache cost is exactly **4 B per line / 32 B total**.

The ephemeral production candidate removes the now-dead `space_sine_q15()` wrapper but retains `space_sine_point()` for reset initialization and rare top-byte transitions.

## Equivalence proof

Optimized and ASAN/UBSAN runs both PASS.

Helper/domain coverage:

- complete top-16 sine input partition: **65,536** cases
- all old-index → new-index transitions × fractional values {0, 1, 127, 255}: **262,144** cases
- explicit `255 → 0` wrap coverage
- cache reset initialization checked against exact `space_sine_point()` values

Complete FDN coverage compares stereo output, every pre-existing FDN state field and the entire delay storage after every block:

- cold: **256 blocks / 16,384 frames**, **88** hot-path endpoint refreshes after reset initialization
- warm continuation: **1,024 blocks / 65,536 frames**, **367** refreshes
- randomized/reset/phase-randomized continuation: **4,096 blocks**
- total complete-FDN proof: **5,376 blocks / 164,767 frames**

All comparisons are bit-identical to frozen P11O authority.

The cold refresh count is eight lower than P11P's measurement-model count because P11Q initializes one endpoint pair per line during reset; those eight initializations therefore do not occur in the render hot path.

## Historical behavior gate

After applying the candidate ephemerally, the complete frozen historical host + ASAN/UBSAN suite passes through the established P11F runtime-size adaptation.

Measured host layout:

- `sizeof(ApsisSpaceFdnLine)`: production 24 B → candidate **28 B**
- `sizeof(ApsisSpaceFdn)`: production 232 B → candidate **264 B**
- exact persistent host delta: **+32 B**

No other production state is added.

## Real JieLi target projection

The exact P11O baseline was regenerated first:

- image **140,440 B**
- RAM **87,492 B**
- POOL **319,176 B**
- package bytes **610,110 B**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- `apsis_space_fdn_render`: **1,114 B / 374** static instruction lines
- `space_sine_point`: **48 B / 14** static instruction lines

Ephemeral candidate target:

- image **140,472 B** (**+32 B**)
- RAM **87,524 B** (**+32 B**)
- RAM headroom **10,780 B**
- POOL **319,176 B** (**0 B**)
- POOL headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- `apsis_space_fdn_render`: **1,098 B / 366** static instruction lines
- render delta: **−16 B / −8** static instruction lines
- `space_sine_point`: unchanged **48 B / 14** static instruction lines

The target ABI therefore confirms the exact 32-B state cost. The compiler also produces a smaller render hot path despite the added rare-transition branch/state loads.

## Workload interpretation

P11P measured **655,360** natural sine calls with **654,897** endpoint-pair reuses and only **463** pair refreshes. That means the candidate moves the two `space_sine_point()` evaluations out of more than 99.9% of those calls while retaining every interpolation and quadrant/wrap semantic.

P11Q proves the implementation shape is bit-identical and has a small, bounded target cost:

- +32 B RAM
- +32 B image
- 0 B POOL
- smaller FDN render code shape

This is sufficient evidence to authorize a separate P11R production-validation phase.

## P11R authorization

P11R may apply **only** the exact P11Q candidate:

- two signed-16-bit endpoint fields per `ApsisSpaceFdnLine`;
- exact reset initialization;
- top-byte transition refresh;
- unchanged fractional interpolation arithmetic;
- removal of the now-unused `space_sine_q15()` wrapper;
- no other Space/Cathedral/DSP/state changes.

Before promotion P11R must require:

- actual-production equivalence against frozen P11O authority;
- complete historical host + ASAN/UBSAN behavior;
- exact real JieLi resources matching the P11Q projection;
- exact idle PCM/runtime;
- exact active Motion lifecycle;
- narrow-diff verification and frozen package hashes.

## Physical-hardware boundary

P11P/P11Q show measured repeated work, exact equivalence and target compiler/resource effects. They do **not** measure physical FM-1 CPU cycles, ISR margin, runtime stack high-water or audible quality. Those claims remain Phase 12 hardware work.
