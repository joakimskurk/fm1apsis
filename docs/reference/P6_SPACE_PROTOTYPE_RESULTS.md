# Phase 6 — Space / Cathedral prototype results

Status: **candidate comparison and silent real-target memory validation complete; compact modulated FDN selected**

Prototype source: `tools/p6_space_prototype.c`  
Candidate comparison head: `bfeaed6dd1ba15c7369e71afeed3f40d28a11bbf`  
Silent target-memory gate head: `11ce731a740d11262d988c352a79774bbea3103a`

This comparison is a deterministic host proxy, not hardware-listening evidence. The Study-126 reference renderer uses a fixed LCG in place of browser `Math.random()` so the source IR statistics can be compared repeatably.

## Memory model

Frozen dry Dinghy pool before Phase 6: **30,720 / 344,064 B**.

Worst-case front-end storage model:
- shared stereo input history covering max 140 ms predelay / early taps: **24,696 B**
- max 1.2 s stereo echo: **211,680 B**

Candidate tail storage:
- 8-line modulated FDN: **38,340 B**
- nested allpass/Schroeder hybrid: **50,900 B**

Modelled total POOL-region use:
- FDN: **305,436 B**, headroom **38,628 B**
- Schroeder: **317,996 B**, headroom **26,068 B**

Both fit the 344,064 B hardware region in the host model, but the FDN retains **12,560 B more margin**.

## Deterministic comparison

### 4.0 s decay

| renderer | peak | RMS | L/R corr | HF proxy | metallicity | end dB | hash |
|---|---:|---:|---:|---:|---:|---:|---|
| Study-126 proxy | 0.638442 | 0.01888299 | +0.0330 | 0.2526 | 0.0401 | -63.05 | `ba35441a` |
| FDN | 0.008499 | 0.00018328 | +0.0349 | 0.2858 | 0.1087 | -62.63 | `ecbca731` |
| Schroeder | 0.009402 | 0.00026743 | +0.0301 | 1.0546 | 0.6886 | -45.72 | `34d8a97c` |

### 8.4 s decay

| renderer | peak | RMS | L/R corr | HF proxy | metallicity | end dB | hash |
|---|---:|---:|---:|---:|---:|---:|---|
| Study-126 proxy | 0.638832 | 0.02080075 | +0.0030 | 0.2325 | 0.0243 | -63.13 | `1df1df0f` |
| FDN | 0.008499 | 0.00016675 | +0.0220 | 0.2405 | 0.1123 | -63.83 | `f1f7e1b5` |
| Schroeder | 0.009444 | 0.00039520 | +0.3361 | 0.5231 | 0.7287 | -25.68 | `2f85b094` |

### 12.0 s decay

| renderer | peak | RMS | L/R corr | HF proxy | metallicity | end dB | hash |
|---|---:|---:|---:|---:|---:|---:|---|
| Study-126 proxy | 0.638940 | 0.02132973 | -0.0010 | 0.2277 | 0.0382 | -63.53 | `65ae55fd` |
| FDN | 0.008499 | 0.00015727 | +0.0190 | 0.2210 | 0.1154 | -64.38 | `90d4d8eb` |
| Schroeder | 0.009455 | 0.00061053 | +0.6341 | 0.2884 | 0.7149 | -21.40 | `7f2ef54e` |

The absolute peak/RMS scale is not used to select the topology; output gain is a later calibration problem. The comparison focuses on decay shape, HF density, stereo decorrelation, periodicity and memory.

## CI acceptance gates

The FDN passed all frozen prototype assertions in both normal and sanitizer builds:
- modelled pool total below 344,064 B;
- deterministic render hashes exact;
- end-of-decay proxy within 3 dB of Study-126 reference at 4 / 8.4 / 12 s;
- `abs(L/R correlation) < 0.10`;
- HF proxy within 0.06 of Study-126 reference;
- metallicity proxy below 0.20;
- metallicity below 35 % of the Schroeder candidate.

CI conclusion:
`PASS: P6 deterministic Space candidates fit pool; FDN tracks reference proxies and beats Schroeder metallicity`

## Selection

**Selected candidate: compact 8-line modulated FDN.**

Rationale:
1. Its decay endpoint tracks the deterministic Study-126 reference within about 1 dB across all three tested settings.
2. Its HF proxy follows the reference closely at default and long decay.
3. Stereo correlation remains near zero across all three settings.
4. Its metallicity proxy stays around 0.11, versus roughly 0.69–0.73 for the Schroeder candidate.
5. It needs 38,340 B tail storage versus 50,900 B for the Schroeder hybrid.
6. Under the conservative max-predelay + max-echo model it retains ~38.6 kB POOL headroom, ~12.6 kB more than the Schroeder candidate.

The Schroeder candidate is rejected for this port iteration because its long-decay energy and periodicity diverge strongly from the Study-126 target even though it fits memory.

## Selected FDN prototype geometry

Base delay lengths in samples at 44.1 kHz:
`1871, 1999, 2137, 2281, 2441, 2593, 2767, 2953`

Each line reserves 16 additional samples for modulation/interpolation.

Deterministic modulation rates in Hz:
`0.071, 0.083, 0.097, 0.109, 0.127, 0.139, 0.157, 0.181`

Prototype modulation depth: approximately ±9 samples.  
Feedback mixing: normalized 8x8 Hadamard.  
Output: decorrelated signed sums with small crossmix.  
Feedback damping: one-pole per FDN line.

These values are selected as the **native topology/memory baseline**, not a hardware-listened final tuning. Physical FM-1 remains unavailable.

## Silent real-target memory validation

### What initially failed

Two intermediate target layouts fit memory but shifted the frozen emulator cursor from 123,648 to 123,392:
- first with a runtime retention function;
- then with pure link-time `KEEP(*(.pool*))` and no runtime function.

Pinned Dinghy source explained the result: `fm1_cstart()` clears `[_pool_start, _pool_end)` at boot. Putting the additional 274,716 B inside ordinary `.pool` therefore added enough startup work to lose one 256-frame callback.

The cursor requirement was not weakened.

### Accepted storage layout

The accepted CI target layout places Space storage in a dedicated NOLOAD `.apsis_space_pool` section immediately before Dinghy's ordinary `.pool`, inside the same POOL memory region.

Consequences:
- `_pool_end` stays after both sections, so build reporting and linker capacity enforcement count the full Space footprint;
- `_pool_start` begins after Space, so Dinghy clears only its original ordinary pool;
- no Space code executes in the silent gate;
- production Space must self-initialise/validity-track this NOLOAD memory before reading it.

Gate head `11ce731a740d11262d988c352a79774bbea3103a`:
- APSIS core #375 PASS;
- APSIS staged validation #364 PASS;
- image **100,944 B**;
- ordinary RAM **87,336 / 98,304 B**;
- POOL-region use **305,436 / 344,064 B**;
- pool headroom **38,628 B**;
- probe package `5db84c9e09c76d1434abd3e01e55abd4e5e85afa4b39124869fe87d3bb02ffed`.

Room renderer remained disabled, and emulator output stayed exact frozen dry behaviour:
- PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`;
- queue 28/512;
- overflow 0;
- late flags 0;
- cursor **123,648**;
- late trace `811c9dc5`.

CI conclusions:
- `PASS: P6 selected-FDN pool storage fits real target pool=305436/344064 ram=87336/98304`
- `PASS: silent P6 pool probe preserves frozen M4g/Phase-5 PCM/runtime`

## Exact next gate

Before any audible target integration:
1. implement the selected FDN as a pure fixed-point host DSP with external storage;
2. make all NOLOAD reads validity-aware so unwritten history returns silence without a giant memset;
3. add the source-defined room trim/guard/early/predelay/tail-damping/echo/wet topology in deterministic host slices;
4. freeze impulse/decay/hash references under normal + ASAN/UBSAN;
5. only then integrate the five post-Mother source sends into one shared room input;
6. add a room-disabled target bypass gate that returns exactly to the frozen dry PCM/runtime.
