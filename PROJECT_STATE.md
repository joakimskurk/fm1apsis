# PROJECT_STATE

Last updated: 2026-10-30

## Current milestone

**Phase 6 — shared Space / Cathedral: source/topology freeze complete; native memory/candidate prototyping active.**

Active branch: `p6-space-cathedral`  
Base: merged Phase 5 `d677a887cf6dc8552634bde5a71fec0221c299b5`  
Phase-6 contract: `docs/reference/P6_SPACE_CATHEDRAL_CONTRACT.md`

Phase 5 was final-gated on `17e7fd1247b0f5675261a7395b0558b5bbd6780d`:
- core #363 PASS
- staged #352 PASS including real JieLi target, emulator and exact M4f Bloom-bypass
- squash-merged through PR #15 as `d677a887cf6dc8552634bde5a71fec0221c299b5`

Phase 5 required no production scheduler/DSP/runtime change, so current firmware remains exact M4g.

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

## Current target baseline

Current production image remains exact M4g / merged Phase 5:
- image **100,944 / 581,564 B**
- RAM **87,336 / 98,304 B**
- ordinary static-RAM headroom **10,968 B**
- pool **30,720 / 344,064 B**
- pool headroom **313,344 B**
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
1. per-source sends -> shared room input;
2. trim 0.72;
3. room guard HP/LP/compressor;
4. four early reflection taps;
5. predelay -> 190 Hz HP -> long diffuse Cathedral -> damping LP -> wet return;
6. quiet filtered feedback echo feeds the same long-tail path;
7. wet return remains separate from dry.

Browser convolution IR is a perceptual target, not a literal FM-1 implementation requirement. Native must retain dense/dark/low-metallicity 4..12 s behaviour and source-send topology.

Documented Study-126 live-audio dispositions:
- effective live tail damping 2200..6800 Hz although UI exposes 2200..11000;
- effective live echo feedback 0..0.48 although UI/storage exposes 0..0.58.

## Phase 6 memory decision

Pinned Dinghy provides a separate **336 KiB `.pool` specifically for large zeroed buffers**, including delay-line use.

Therefore:
- large predelay/echo/reverb delay lines belong in `.pool`;
- small room/filter/control state stays in normal RAM;
- do not consume the final 10,968 B ordinary RAM with delay arrays.

Reference stereo int16 storage at 44.1 kHz:
- predelay max 140 ms: ~24.7 kB
- echo max 1.2 s: ~211.7 kB
- current pool headroom before Phase 6: 313.3 kB

This makes a compact algorithmic Cathedral feasible, but worst-case pool sizing must be measured before integration.

## Candidate gate now active

Prototype and compare, before `app_audio()` integration:
1. compact modulated FDN;
2. nested allpass/Schroeder hybrid.

Both must preserve the contract and report:
- pool bytes and ordinary state bytes;
- deterministic impulse hash;
- decay/energy behaviour at 4 / 8.4 / 12 s;
- stereo decorrelation;
- metallic/repeat proxy;
- damping behaviour;
- transient stability;
- cost proxy.

Then place the selected candidate's **exact delay buffers** in `.pool` in a silent target-validation build. The real target must fit and the old M4g/Phase-5 PCM/runtime must remain bit-identical while the room renderer is not called.

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

1. Open the Phase-6 draft PR from `p6-space-cathedral` after the topology-contract commit.
2. Add a deterministic host memory/impulse prototype for **both** room candidates.
3. Measure candidate pool usage and acoustic proxies under 4 / 8.4 / 12 s settings.
4. Select a candidate only from evidence, not preference.
5. Before audible integration, validate its exact `.pool` layout on real JieLi target while room rendering remains disabled.

## Do not regress

- one shared Phase-3 scheduler; no module-private clocks;
- one shared room with five source sends; no per-source reverbs;
- Study 126 topology/parameter roles remain authoritative;
- Study 98 remains narrow String acoustic authority only;
- `ApsisRuntime` remains 21,928 B unless explicitly re-approved;
- Phase-6 delay memory belongs in `.pool`, not ordinary BSS;
- inactive room code may not change the exact emulator cursor or dry PCM;
- physical FM-1 and real ISR timing remain unverified.
