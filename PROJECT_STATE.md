# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 6 — shared Space / Cathedral: selected FDN + complete pure host room engine are green; target/audible integration is next.**

Active branch: `p6-space-cathedral`  
Draft PR: #16  
Base: merged Phase 5 `d677a887cf6dc8552634bde5a71fec0221c299b5`  
Phase-6 contract: `docs/reference/P6_SPACE_CATHEDRAL_CONTRACT.md`  
Candidate evidence: `docs/reference/P6_SPACE_PROTOTYPE_RESULTS.md`

Phase 5 was final-gated on `17e7fd1247b0f5675261a7395b0558b5bbd6780d` and squash-merged through PR #15 as `d677a887cf6dc8552634bde5a71fec0221c299b5`.

Physical FM-1 hardware is still unavailable. No real ISR-cycle-margin or hardware-listening claim is made.

## Product/source invariant

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behavioural/state/timing/safety authority:
- Study 126 · String Pulse Isolation
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic exception only:
- Study 98 · Performance Architecture
- LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

## Frozen shared runtime baseline

M3c remains the only musical scheduler/runtime timeline.

Host invariants:
- `sizeof(ApsisRuntime) = 21,928 B`
- heap 512 events
- canonical queue 352/512; extreme 357/512
- Spiral slots 59/64; Bloom 44/64
- random trace `78771ff5`
- late trace `0e6fc422`

Pinned emulator invariants:
- queue 28/512
- overflow 0
- late flags 0
- cursor **123,648 exactly**
- late trace `811c9dc5`

## Frozen dry target baseline

Merged Phase 5 remains exact M4g on the dry path:
- image **100,944 / 581,564 B**
- RAM **87,336 / 98,304 B**
- ordinary static-RAM headroom **10,968 B**
- ordinary Dinghy pool **30,720 / 344,064 B**
- package `b4e74e8bdd71b79e1a496576d29f537bd76b223127ca4161554bfffb502deeca`
- default PCM `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`

## Phase 6 source/topology freeze

Study 126 defines **one shared room** fed by five post-source/post-Mother sends:
- String 0.72
- Drift 0.20
- Clock 0.06
- Spiral 0.62
- Bloom 0.58

Shared Space defaults already match native state:
- wet return 0.48
- decay 8.4 s
- predelay 72 ms
- damping 6200 Hz
- echo 362 ms / 18 % feedback

Frozen topology:
1. each source passes through its Mother mix/mute before both dry and send;
2. five sends feed one shared room input;
3. trim 0.72;
4. room guard HP/LP/compressor;
5. four early reflection taps;
6. predelay -> 190 Hz HP -> long diffuse Cathedral -> damping LP -> wet return;
7. quiet filtered feedback echo feeds the same long-tail path;
8. wet return remains separate from dry.

Browser convolution IR is a perceptual target, not a literal FM-1 implementation requirement. Native must retain dense/dark/low-metallicity 4..12 s behaviour and source-send topology.

Documented Study-126 live-audio dispositions:
- effective live tail damping 2200..6800 Hz although UI exposes 2200..11000;
- effective live echo feedback 0..0.48 although UI/storage exposes 0..0.58.

## Selected Phase-6 room algorithm

The deterministic candidate comparison is complete.

**Selected:** compact 8-line modulated FDN.

Selected geometry:
- delays: `1871, 1999, 2137, 2281, 2441, 2593, 2767, 2953` samples;
- +16 samples reserve per line;
- modulation rates `.071, .083, .097, .109, .127, .139, .157, .181 Hz`;
- approximately ±9 samples modulation;
- normalized 8x8 Hadamard feedback mix;
- one-pole damping per line;
- decorrelated signed stereo output with small crossmix.

Host prototype evidence at 4 / 8.4 / 12 s:
- FDN decay endpoint remains within the frozen reference tolerance;
- stereo correlation remains near zero;
- HF proxy tracks the Study-126 proxy closely;
- metallicity stays around 0.11;
- rejected Schroeder candidate measures roughly 0.69..0.73 metallicity;
- FDN tail storage 38,340 B versus 50,900 B for Schroeder.

## Real target memory gate — complete

Conservative exact selected storage classes at 44.1 kHz:
- shared stereo 140 ms input history: **24,696 B**;
- stereo 1.2 s echo: **211,680 B**;
- 8-line FDN tail: **38,340 B**;
- existing Dinghy pool: **30,720 B**;
- total POOL-region footprint: **305,436 / 344,064 B**;
- remaining pool headroom: **38,628 B**.

Exact validated gate head: `11ce731a740d11262d988c352a79774bbea3103a`.

Evidence:
- core #375 PASS;
- staged #364 PASS;
- real target image **100,944 B**;
- ordinary RAM **87,336 B**;
- pool **305,436 B**;
- probe package `5db84c9e09c76d1434abd3e01e55abd4e5e85afa4b39124869fe87d3bb02ffed`;
- emulator PCM remains exact frozen dry `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`;
- queue 28/512, overflow 0, late flags 0, cursor **123,648**, late trace `811c9dc5`.

### Dinghy pool-startup discovery

Pinned Dinghy `fm1_cstart()` clears the complete range `[_pool_start, _pool_end)` at boot. Placing the new 274,716 B Space storage inside ordinary `.pool` therefore delayed startup by one 256-frame callback even when no Space code ran.

The accepted layout places Space storage in a dedicated NOLOAD `.apsis_space_pool` section **immediately before** Dinghy's ordinary `.pool` in the POOL memory region:
- linker capacity/reporting still accounts the full 305,436 B footprint because `_pool_end` remains after both sections;
- `_pool_start` begins after Space storage, so Dinghy clears only its original pool;
- no Phase-6 runtime call executes while the room is disabled;
- dry PCM/runtime/cursor remain bit-identical.

Production Space explicitly tracks validity/fill instead of requiring a giant boot-time memset, so stale NOLOAD memory is never read before it has been overwritten by the room.

## Pure fixed-point room engine — complete host gate

Current validated Phase-6 head: `014c6f843d6e8e7a15ac35e9bbc372e460756e9c`.

Core #398: **PASS**, including normal + ASAN/UBSAN for the selected FDN and the full shared-room front end.

Implemented in `apsis_space_dsp.[ch]` + `apsis_space_room.[ch]` with caller-owned external delay storage:
- selected 8-line fixed-point FDN;
- room trim 0.72;
- guard 170 Hz HP + smoothed 3.8..5.6 kHz LP + compressor safety transfer;
- four source-defined early reflections at 21 / 43 / 71 / 109 ms;
- predelay;
- 190 Hz tail HP;
- FDN decay/damping control;
- 4.2 kHz filtered feedback echo;
- independent early path + wet return;
- self-initialising history/echo/FDN validity through fill counters, with **no storage clear on bind**.

Frozen host references on the current head:
- selected FDN default: peak **217**, hash `a190ca86`, late `2646000`;
- FDN controls: decay4 `1455300`, decay12 `3507773`, dark `7d4c13e3`, bright `7e1a4d9e`;
- full room default: peak **47**, hash `1350a6da`, late `1278900`, first non-zero sample **927**;
- room no-echo hash `97669507`;
- room dark hash `5ff1f59a`.

Staged #387 on the same head is also **PASS** through the existing dry target/emulator + silent selected-storage gate. The room renderer is deliberately **not yet called from `app_audio()`**, so this staged result does not claim audible Space on target yet.

## Completed merged baselines

- M3a `fe2911c43fd69e63839220482e50bb10c7a2e4c2`
- M3b `a61a40dc83c9b52129f7d3f920da26a9a61837d9`
- M3c `ec0dc0b350793572f19040efc938f00121128dc6`
- M4a `2ea217f09c143d30385f136e84e2b29d582987a0`
- M4b `c96c00debf9a67948f1e874e36b090999dacadb9`
- M4c `6fcdd8ff5330a9b3ed4ffaa93f3e940e41b7c5cc`
- M4d `d7e39e2481c33628f402bed51d761b8be8304bb3`
- M4e `db9d0b3ecbc7931be343cec7717103059d3c5160`
- M4f `734229f4d23626a176a8366de82aab2c58c48cc9`
- M4g `713a1e9ac746a1a4d19ba825a0e6765b337ba410`
- Phase 5 sounding integration `d677a887cf6dc8552634bde5a71fec0221c299b5`

## Exact next action

1. Split the existing dry render path into five post-source/post-Mother source contributions without changing their summed dry PCM.
2. Apply the frozen String/Drift/Clock/Spiral/Bloom Space-send gains to one shared stereo room-input accumulator.
3. Bind the already-proven dedicated NOLOAD Space storage and call `apsis_space_room_render()` from `app_audio()` only after the dry source contributions are available.
4. Add an explicit room-disabled bypass gate requiring exact frozen Phase-5/M4g dry PCM/runtime/cursor.
5. Add an audible-room target/emulator scenario and verify target image/RAM/pool, callback cursor and deterministic room output before considering Phase 6 merge-ready.

## Do not regress

- one shared Phase-3 scheduler; no module-private clocks;
- one shared room with five source sends; no per-source reverbs;
- Study 126 topology/parameter roles remain authoritative;
- Study 98 remains narrow String acoustic authority only;
- `ApsisRuntime` remains 21,928 B unless explicitly re-approved;
- Phase-6 long delay memory belongs in the dedicated POOL-region Space section, not ordinary BSS;
- inactive room code may not change the exact emulator cursor or dry PCM;
- physical FM-1 and real ISR timing remain unverified.
