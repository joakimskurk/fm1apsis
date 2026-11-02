# PROJECT_STATE

Last updated: 2026-11-02

## Current milestone

**Phase 11 — software/real-target/pinned-emulator closure is complete on `p11-optimization`. P11W is the final accepted production baseline and P11Z passed the final pre-hardware readiness matrix. Guarded merge to `main` is next; Phase 12 remains physical-FM-1 validation only.**

Physical FM-1 hardware remains unavailable. All current claims are host, real JieLi target-build, static-target or pinned-emulator claims. Physical ISR margin, runtime stack high-water, LCD/audio bus contention, NOR timing and DAC/listening quality remain unverified.

## Source/product authority

APSIS remains a parity-first native port, not a generic synth/groovebox.

Behaviour/state/timing/safety authority:
- Study 126 SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

String acoustic exception only:
- Study 98 LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`

Pinned FM-1 baseline:
- Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`
- product `FM-1_972`

Phase 10 merge authority:
- feature-complete technical head `7d8b072cbe5d17f44fd7117379af5d9284daa5d2`
- exact-head documentation/CI gate `ce5857a52c6326011e5af7cd7f73819b420550fc`
- 22/22 required workflows PASS
- squash merge `378d2023df2afe59475be53c09cc001788586514` via PR #20

## Current frozen production baseline — P11W

P11W is the current accepted production target after the zero-growth P11V-proved A+B → A+delta sine-cache representation layered over the P11R endpoint cache, P11O line-read specialization and P11L feedback-gain cache:

- product `FM-1_972`
- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B**
- RAM headroom **10,780 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined FDN render **1,094 B / 364** static instruction lines

Integrated frozen idle baseline:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor **123,392**
- queue high-water 28/512
- queue overflow 0
- late flags 0
- late trace `811c9dc5`
- peak 856
- RMS 144/144
- stereo delta 126

Historical M4/M5 pinned emulator baseline remains cursor 123,648 with queue 28/512, overflow 0, late flags 0 and trace `811c9dc5`.

Frozen Phase-5 dry PCM remains `6c3b8fff90f381da36ade0e610efb85769b04a020d17333308866e95055caa2b`.

## Frozen scheduler/runtime contract

M3c remains the only musical scheduler/runtime timeline. No module-private clocks or musical RNG authorities may be introduced.

Current P11F host invariants:
- `sizeof(ApsisRuntime) = 21,032 B`
- shared event heap 512 events
- canonical queue high-water 352/512
- extreme queue high-water 357/512
- Spiral late slots 59/64
- Bloom late slots **44/48**
- P11E proved 44 is the exact legal Bloom maximum and 45 is impossible under the frozen contract
- random trace `78771ff5`
- late trace `0e6fc422`

Historical pre-P11F host layout remains reproducible through pinned gates:
- `sizeof(ApsisRuntime) = 21,928 B`
- Bloom late slots 44/64

## Shared Space / Cathedral contract

- one shared room with five source sends
- source DSP and final output remain 44.1 kHz
- target Cathedral remains quarter-rate at 11.025 kHz
- compact 8-line fixed-point FDN
- long room storage remains dedicated pre-`.pool` NOLOAD memory
- no per-source reverb instances

Current special POOL ownership:
- Motion store 9,644 B in `.apsis_motion_pool`
- Space pool 278,812 B in `.apsis_space_pool`
- Dinghy canvas `cv_px` 30,720 B in `.pool`
- total POOL 319,176 B

P11H establishes Cathedral as a repeated audio-workload candidate, but not a physical timing result.

## Persistence contract

Persisted:
- all 8 KEEP snapshots + selected slot
- up to 32 CHAIN cells + count + editor-selected cell
- 13 CHAIN behavioural/default/scope settings

Wire format v1:
- magic `APS7`
- explicit little-endian encoding
- 24-byte header
- 816-byte payload
- 840-byte complete record
- CRC-32/IEEE for payload and header
- wrap-safe 32-bit generation

Journal:
- sector A `0x000F4000`
- sector B `0x000F5000`
- payload programmed/verified first; commit header last
- newest fully valid generation wins; corrupt/torn newest falls back to older valid generation

Audio safety:
- boot NOR prefetch before audio DMA startup
- later decode RAM-only
- save request is dirty intent only
- erase/program in main loop with transport stopped/audio quiesced
- no flash write/erase reachable from `app_audio()`

## Native display/control contract

Ten deterministic 240×240 surfaces remain authoritative:
- MOTHER `27373657`
- GRAVITY `55aff30b`
- STRING `00532f1d`
- DRIFT/TUNING `9ff4af95`
- CLOCK `903a5f21`
- SPIRAL `562a4043`
- BLOOM `7fcc0856`
- KEEP `df715375`
- CHAIN `709ed554`
- SPACE/CATHEDRAL `9605ed04`

Display invariants:
- reuse Dinghy `cv_px`; no second framebuffer
- 8-row native strips
- each next strip gated by `fm1_lcd_done()` / existing LCD HAL ownership
- display work remains outside `app_audio()`
- native takeover after runtime sample 8192
- P11G measured 30 strips / **115,200 B RGB565 pixel payload** per full native redraw

Panel vocabulary:
- 14 buttons
- 7 relative/detent encoders
- 27 note keys
- MASTER remains platform volume
- no encoder-push exists

Surface navigation:
- HOME → MOTHER
- ENV → STRING
- LFO → DRIFT/TUNING
- SEQ → CLOCK
- ARP → SPIRAL
- FX → BLOOM
- GLO → GRAVITY
- EDIT → CHAIN
- SAVE → KEEP
- SEL+FX → SPACE/CATHEDRAL

Direct performance crosses finite main→audio mailboxes/adapters only.

## Frozen String Motion / advanced interaction

String Motion:
- REC arms/cancels at next Clock step 1
- SEL+REC toggles playback when a loop exists
- 1/2/4/8-cycle capture
- 1,200 points/lane maximum
- 32-Hz capture bound
- Classic owns single/right lane
- linked Dual owns BOTH
- free Dual can own BODY/WIRE/BOTH
- free-Dual overdub preserves unowned lane
- frozen pitch context survives later Gravity/Tuning edits
- held String performance temporarily overrides playback and releases back at current phase

Current compact Motion storage:
- point 4 B: `phase_q16`, `split_q15`
- one 20-B frozen pitch context per lane
- lane 4,822 B
- store **9,644 B**
- runtime 88 B including bounded endpoint caches

Frozen active Motion lifecycle:
- seen `ff`; final `14`
- 149 points left / 149 right
- record start 220,672
- record finish 441,088
- cycle 220,416 samples
- manual override start 450,560
- resume 466,176
- cursor 481,024
- queue high-water 30/512
- overflow 0
- late flags 0
- WAV frames 521,984
- peak 1,171
- RMS 173

String Attractor, Spiral Attractor and selected Gravity-point editors continue to reuse their existing DSP/runtime/state authorities; no UI shadow musical state exists.

## Phase 11 method

Phase 11 is optimisation, not redesign.

Every accepted optimization requires:
1. measured baseline;
2. explicit target/hypothesis;
3. minimal patch;
4. before/after evidence on the same path;
5. relevant frozen musical/audio/control regressions green;
6. no baseline relaxation simply to make the patch pass.

Static target counts and emulator behavior are proxies. Never infer physical ISR margin, runtime stack high-water, LCD contention or listening quality from them.

## Phase 11 evidence authority

- P11A baseline run `41307570193` on `5e9eb2348ec3bd075eecfff2cad63fcca90a32a8`
- P11B Motion proof run `41308160497` on `0b9c197fc922eb76b37a5de3ac6f64a3ab3e511c`
- P11C exact-head run `41318100472` on `7ce6993065186aa66c89bc723ba3edfa95448596`
- P11D memory inventory run `41320007779` on `2e1912f5d0ff8987382bd7af434fc322679bf762`; artifact SHA-256 `81df7625e1f8421d2d469083276b47f0cb77fae99e86a3a25367d19969b68f06`
- P11E capacity inventory run `41322747611`; frozen to its proven technical head
- P11F exact-head run `41324331937` on `eb651e768a049a1b4a1e88397a9021fccc69fd30`; artifact SHA-256 `fabf6d34d0497f00c4617ca945200634e1d7f28993ffd799f1eb66734cc02700`
- P11G workload proxy run `41333497987` on `dbe0b540e602ea5ce8d93f4b119951736f5549bc`; artifact SHA-256 `4365212ac4ad5668840d150f44be12933c9dcbef2faa575fbfdc261f750c6b5e`
- P11H Cathedral census run `41341918347` on `ca20dd8608ae685349d483f68d72ce4022b90402`; artifact SHA-256 `27c9fabb358331d5b34444ea6233e4df25c58572cc9bd0f42e16159ff3d57fb7`
- P11H frozen/promoted head `4b69145e5bf6d27a394725fdd923876795680a5a`

## P11A–P11F accepted optimization results

P11A established the exact Phase-10 baseline.

P11B proved captured Motion pitch can be reconstructed losslessly from per-lane frozen context + stored split; int16 pitch compression was rejected as lossy.

P11C changed MotionStore **19,208 → 9,644 B**, improving POOL by **9,564 B** while preserving point count, capture bound, idle output and active Motion lifecycle.

P11D reconciled real target memory ownership. Its configured user/system stack ranges are **reservations, not runtime high-water measurements**; no stack shrink is authorized.

P11E measured bounded capacities. It vetoed shrinking the shared queue, Spiral slots, String transient pools, String Attractor pending slots and Spiral/Bloom DSP capacities. It uniquely proved the Bloom late-slot mathematical bound 44.

P11F changed only `APSIS_RUNTIME_BLOOM_SLOTS 64 → 48`, retained four slots above the proven maximum, reduced host runtime 21,928 → 21,032 B and reduced real target RAM by **2,240 B** with all frozen behavior intact.

## P11G — workload proxy inventory complete

Technical authority:
- run `41333497987`
- technical head `dbe0b540e602ea5ce8d93f4b119951736f5549bc`
- artifact SHA-256 `4365212ac4ad5668840d150f44be12933c9dcbef2faa575fbfdc261f750c6b5e`
- frozen/promoted head `70470ddda31450b6aa168da643c8244184e72c16`

Measured/source facts:
- generated `app_audio()` = 219 source lines, 51 lexical call-sites, 34 unique call names
- no visual renderer/main-loop dispatcher/LCD/canvas/input polling inside `app_audio()`
- native full redraw = 30 DMA-gated strips / 115,200 B pixel payload
- real JieLi ELF static inventory captured
- selected static code shape: `apsis_transport_schedule_next` 2,064 B / 715 disassembly lines; `apsis_space_room_render` 2,164 B / 675; `apsis_space_fdn_render` 1,082 B / 353

P11G showed why static size alone cannot select a CPU optimization: transport scheduling is pulse-driven while Cathedral work repeats every audio block.

## P11H — Cathedral dynamic workload census complete

Technical authority:
- run `41341918347`
- technical head `ca20dd8608ae685349d483f68d72ce4022b90402`
- artifact ID `13097912548`
- artifact SHA-256 `27c9fabb358331d5b34444ea6233e4df25c58572cc9bd0f42e16159ff3d57fb7`
- frozen/promoted head `4b69145e5bf6d27a394725fdd923876795680a5a`

Method:
- test-only instrumentation is generated into `/tmp`; production sources remain untouched
- production target is required to retain the exact P11F package hash
- census: 256 callbacks × 256 output frames = 65,536 output frames = 16,384 quarter-rate room frames

Exact measured dynamic counts:
- target render calls 256
- room render calls 256
- room frame iterations 16,384
- smoothing steps 16,384
- delay single-channel reads **196,608**
- room biquads **131,072**
- room Q15 multiplies **294,912**
- stereo delay writes 32,768
- FDN render calls 256
- FDN frames 16,384
- FDN line reads **131,072**
- FDN line writes **131,072**
- FDN Q15 multiplies **655,360**

Per room frame:
- 12 single-channel delay reads
- 8 room biquads
- 18 room Q15 multiplies
- 2 stereo delay writes
- 8 FDN reads
- 8 FDN writes
- 40 FDN Q15 multiplies

Fastest legal transport over the same 65,536 output frames:
- 180 BPM, 1/4 division → 3,675-sample pulse interval
- schedule calls **18**
- events added **218**
- integer room-frame/scheduler-call ratio **910**

Interpretation:
- this proves dynamic operation frequency, not CPU cycles
- Cathedral is a stronger repeated-workload optimization target than the statically large but pulse-driven scheduler
- exact idle and active Motion behavior remained frozen

## P11I — stereo-pair Cathedral delay read complete

Pre-patch authority:
- proof run `41343196262`
- proof head `32bb42c52f666af12065737e14a8846d3d8f01ab`
- **150,412** bit-identical cases, optimized + ASAN/UBSAN
- complete historical host suite green
- production room source still frozen at pre-patch blob `2d7c34e32b48b904623d740a70cf657fee0801a7`

Production/validation authority:
- production pair-read commit `4629406bb6e417b0bcb544e6a770c0860613e7cd`
- exact technical head `ab6960ad3acf78dbc9f366d2a5e9358b42011cca`
- authoritative run `41344186184`, all five gates PASS
- artifact ID `13097594559`
- artifact ZIP SHA-256 `2a9b2d6f2adc8a5fd4a4f39dbe88cbd9b7c6bd2122b80ce83178a07a5a94e716`

Measured result:
- six stereo delay locations/roomframe share validity/wrap/index/fraction work instead of twelve duplicate single-channel address calculations
- target room code shape **2,270 → 2,260 B** and **718 → 707** static disassembly instruction lines
- production image **140,408 → 140,392 B**
- ordinary RAM remains **87,476 B**
- POOL remains **319,176 B**
- production package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`
- active-Motion image **137,672 → 137,656 B** with RAM **87,524 B** and POOL **319,176 B** unchanged
- active package SHA-256 `86c44161d61b17697bebb2af7c3222449a0de1d20b74acc3cbaf2a8635cb7a75`

Behavior remains exact:
- idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`
- active Motion lifecycle remains seen `ff`, final `14`, 149/149 points, cursor 481,024, queue 30/512, overflow 0, late flags 0

P11I proves a reproducible target code-shape reduction, not physical FM-1 cycle/ISR improvement. The historical apply/validation workflows are frozen manual-only and pinned to the proven technical head; the apply verifier is read-only.

## Exact next phase

Return to measurement before P11J. A new production optimization requires a concrete measured hotspot plus a falsifiable equivalence/regression gate. Physical ISR timing, stack high-water, LCD/audio contention and listening quality remain deferred to hardware validation.

## P11J — FDN coefficient recomputation census complete

Authority:
- technical head `17030232e5f21f95a81b834e831d9021683d81bf`
- run `41350487323`
- optimized and ASAN/UBSAN census PASS
- production `apsis_space_dsp.c` blob `72210e122b68bff77c56805618b2703d363a25ef` unchanged
- production `apsis_space_room.c` P11I blob `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346` unchanged

Current FDN behavior recomputes eight feedback gains on every FDN render. Across 256 FDN blocks this is always 2,048 `space_feedback_gain()` evaluations = 2,048 32-bit divisions + 6,144 wide fixed-point multiplies before per-frame work.

Measured rounded-decay churn:
- steady: 0 changes / 1 unique value → recompute-on-change model needs 8 calls; 2,040 redundant current calls (99.6%)
- one legal 4s→12s smoothed transition: 192 changes / 193 unique values → 1,544 calls; 504 redundant current calls (24.6%)
- adversarial extreme alternation: 255 changes → 2,048 calls; zero savings

P11J therefore selects one narrow P11K candidate: cache exactly the eight feedback gains keyed by rounded `decay_ms`, invalidated on reset and recomputed only when that key changes. No other coefficient or DSP arithmetic is authorized to change.

## P11K — FDN feedback-gain cache proof complete

Equivalence authority:
- proof head `ffb436dd87247af23c37e7e533cfd87fc2d161d0`
- run `41350818792`
- optimized + ASAN/UBSAN PASS
- complete baseline/candidate stereo output, existing FDN state and full delay storage compared after every block
- reset/partial-fill, steady, exact room-style sweep, adversarial churn, all 8,001 legal decay keys and 4,096 deterministic random/reset blocks covered
- host external cache representation **20 B**

Measured proof counts:
- steady: 256 blocks / 16,384 frames → 1 recompute block / 8 gain evaluations
- 4s→12s smoothed sweep: 192 decay changes → 193 recompute blocks / 1,544 gain evaluations
- adversarial: 256 recompute blocks / 2,048 gain evaluations
- all legal keys: 16,002 blocks / 8,001 recomputes / 64,008 gain evaluations
- random/reset: 4,096 blocks / 92,927 frames / 3,840 recomputes / 30,720 gain evaluations

Ephemeral real-target projection authority:
- head `72b48c499ce7619fd3a077edac664ea2c65c4ca5`
- run `41351123630`
- artifact ID `13101765292`
- artifact ZIP SHA-256 `a5cf89df0e49c8d79c5118db8f444f74a65808e6ca0f820e43bfda4d40ff0ef3`
- exact P11I baseline regenerated before candidate patch
- candidate patched only in generated target worktree; repo production blobs remained exact

Projected target tradeoff:
- image **140,392 → 140,440 B** (**+48 B**)
- ordinary RAM **87,476 → 87,492 B** (**+16 B**); projected headroom **10,812 B**
- POOL **319,176 B** unchanged
- package bytes **610,110 B** unchanged
- projection package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`
- FDN-render code shape **1,082 → 1,118 B**, **353 → 379** static instruction lines

Workload payoff from P11J/P11K model:
- steady census: 2,040 fewer 32-bit divisions + 6,120 fewer wide multiplies
- measured one-way sweep: 504 fewer divisions + 1,512 fewer wide multiplies
- adversarial every-block decay churn: zero coefficient-computation savings, with only the cache-key decision added

P11K therefore authorizes P11L production validation. It does not claim physical FM-1 cycle or ISR improvement.

## P11L — production FDN feedback-gain cache complete

Production authority:
- exact production head `4a08705815fffcbf9407193a21334764d8710d51`
- authoritative run `41351791632`, all six gates PASS
- artifact ID `13101743282`
- artifact ZIP SHA-256 `819f932f701d2e46cf13b26a641d68130785f84cc026eec39347563903f24d72`

Production change is narrow:
- eight exact cached feedback gains + rounded decay key/validity in `ApsisSpaceFdn`
- cache invalidated on reset
- exact gains recomputed only when rounded `decay_ms` changes
- `space_feedback_gain()` arithmetic, FDN topology and Cathedral room source unchanged

Actual production equivalence:
- optimized + ASAN/UBSAN PASS
- reset/partial fill, 256-block steady, exact smoothed sweep, adversarial churn, all 8,001 legal decay keys and 4,096 deterministic random/reset blocks
- bit-identical stereo output, all pre-existing FDN state and full FDN delay storage after every block

Frozen production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`
- FDN render **1,118 B / 379** static instruction lines

Idle authority remains exact:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Active Motion authority remains exact:
- active image **137,704 B**
- RAM **87,540 B**; headroom **10,764 B**
- POOL **319,176 B**
- package SHA-256 `54616e07ad5b579d33ec07732b5ec3daa824d71950ec8844805631251245374a`
- seen `ff`, final `14`, 149/149 points, record 220,672→441,088, manual 450,560→466,176, cursor 481,024, queue 30/512, overflow 0, late flags 0, WAV peak 1,171, RMS 173

Accepted workload tradeoff from P11J/P11K:
- steady census: 2,040 fewer coefficient divisions + 6,120 fewer wide multiplies
- measured one-way sweep: 504 fewer divisions + 1,512 fewer wide multiplies
- adversarial every-block decay churn: zero coefficient-computation savings

P11L proves exact behavior and real-target resource cost. It still does not prove physical FM-1 CPU cycles or ISR margin.

## P11M — FDN inner-loop branch census complete

Authority:
- technical head `04b807617ffec084e3721fa2a166ac6ad9e76d59`
- run `41355338593`
- optimized + ASAN/UBSAN PASS
- production DSP source/header and Cathedral room blobs remain exact P11L authority
- instrumentation exists only in generated `/tmp` source

Cold 16,384-frame census:
- 131,072 sine/LFO calls and line reads
- modulation **−2,304 .. +2,303 Q8**
- minimum-delay-clamp opportunities **0**
- not-ready startup returns **19,047 (14.53%)**
- full-capacity calls **111,902 (85.37%)**
- ready reads **112,025**
- negative-wrap **111,261 (99.32% of ready)**
- upper-wrap **0**
- integer reads **348 (0.31% of ready)**
- interpolated reads **111,677 (99.69%)**
- `i1` wraps **44 (0.039%)**

Warm 65,536-frame census after full prefill:
- 524,288 sine/LFO calls and line reads
- all **524,288** reads at full line capacity
- not-ready **0**
- minimum-delay-clamp opportunities **0**
- negative-wrap **521,026 (99.38%)**
- upper-wrap **0**
- integer reads **4,438 (0.85%)**
- interpolated reads **519,850 (99.15%)**
- `i1` wraps **224 (0.043%)**

Structural P11M bounds:
- min base delay 1,871 samples and modulation bound −2,304..+2,303 Q8 make the minimum-delay clamp unreachable;
- capacity `base_delay+16` and ±9-sample modulation guarantee raw non-negative reads stay far below span and negative reads need at most one `+span`, so the upper-wrap correction is unreachable;
- once `fill_count==capacity`, `needed<=base_delay+9<capacity`, so readiness failure is permanently impossible while startup semantics remain required before full capacity.

Decision: P11N may prove only a narrow production-caller-specialized reader: partial lines retain the exact readiness calculation, full lines bypass it; unreachable minimum-delay and upper-wrap branches may be removed; negative-wrap, interpolation and `i1` wrap remain exact. Defensive helper-guard removal is not authorized without separate call-path proof.

## P11N — FDN line-read invariant proof complete

Proof/projection authority:
- combined head `a13bce4ce821b68d8daab66ec1432ba3b5bf9a99`
- authoritative proof re-run `41355949491` — PASS
- target-projection run `41355949486` — PASS
- optimized + ASAN/UBSAN proof PASS
- production P11L Space blobs remained unchanged

Helper proof:
- all eight FDN line topologies
- every integer production modulation value −2,304..+2,303 Q8
- readiness and ring boundary classes
- 500,000 deterministic randomized valid states
- **905,504** total helper comparisons, all bit-identical

Complete-FDN proof:
- cold startup 16,384 frames
- warm continuation 65,536 frames
- 2,048 deterministic randomized parameter/frame blocks with resets and randomized LFO phases
- **3,328 blocks / 210,677 frames** total
- bit-identical stereo output, FDN state, feedback-cache state and full delay storage after every block

Ephemeral real-target projection:
- exact P11L baseline regenerated first
- candidate patched only into generated target worktree
- image **140,440 B** unchanged
- RAM **87,492 B** unchanged; headroom **10,812 B**
- POOL **319,176 B** unchanged
- package bytes **610,110 B** unchanged
- candidate projection package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- inlined `apsis_space_fdn_render` **1,118 → 1,114 B**, **379 → 374** static instruction lines
- artifact ID `13103071866`
- artifact ZIP SHA-256 `6d39137a817dd8a2d842790c40b4f51f6d0c0ef12ce7b33c56733f6aaa84711b`

P11M provides the repeated-work rationale: 100% of warm measured line reads and 85.37% of the cold census are full-line, so the candidate skips readiness setup on the dominant path while preserving exact startup semantics. Minimum-delay and upper-wrap branches are structurally unreachable under the frozen caller.

Decision: P11O may apply only the proved candidate to production `space_line_read()`. No other DSP/state/header change is authorized.

## P11O — production FDN line-read validation complete

Production authority:
- validated production head `6560118c1cf7dfa6e40d1e33e86167d4f0b6ea5b`
- validation run `41356749785` — PASS
- workflow frozen manual-only at that production head
- evidence artifact ID `13104130300`
- evidence ZIP SHA-256 `0d747a515218da33e6bd9d61ae027aea64ce0a18033f6c4af64d32b27ccd0d9d`

Actual production equivalence:
- **905,504** helper comparisons against frozen P11L reader
- **3,328 blocks / 210,677 frames** complete-FDN comparison
- optimized + ASAN/UBSAN PASS
- complete historical host + sanitizer behavior PASS
- exact partial-fill zero-return, negative wrap, fractional interpolation and `i1` wrap retained

Frozen production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- inlined FDN render **1,114 B / 374** static instruction lines

Idle authority remains exact:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Active Motion authority remains exact:
- active image **137,704 B**
- RAM **87,540 B**; headroom **10,764 B**
- POOL **319,176 B**
- package SHA-256 `adb581ff61ab1dc48c7bd08f7ca83bfa33f786fb635c31f36813f40d964b54ed`
- seen `ff`, final `14`, 149/149 points, record 220,672→441,088, manual 450,560→466,176, cursor 481,024, queue 30/512, overflow 0, late flags 0, WAV peak 1,171, RMS 173

P11O therefore replaces P11L as the accepted production baseline. It proves exact behavior and target code shape, not physical FM-1 cycle or ISR improvement.

## P11P — FDN LFO/sine workload census complete

Measurement authority:
- technical head `2afb819dd865385f7547811b6422e7a281c80b7e`
- run `41359406941` — PASS
- artifact ID `13104249141`
- artifact ZIP SHA-256 `d1a5d5898d00150c0f757f11fa01683d705a131bb35387d614741c49fa3b3517`
- production `apsis_space_dsp.c/.h` and Cathedral room source remained exact P11O
- historical workflow manual-only and pinned to the proven technical head

Natural LFO trajectory:
- cold 16,384 frames: **131,072** sine calls / **262,144** point evaluations
- cold cache-model endpoint refreshes **96**; reuses **130,976**
- warm continuing 65,536 frames: **524,288** calls / **1,048,576** point evaluations
- warm refreshes **367**; reuses **523,921**
- combined **655,360** calls, **463** refreshes, **654,897** reuses
- combined avoidable point evaluations under the cache model: **1,309,794**
- endpoint pair unchanged for **99.929%** of natural calls

Exact-domain constraints:
- all 256 fractional values occur naturally
- exhaustive top-16 sweep covers all **65,536** index/fraction combinations
- `frac == 0`: **256**; `frac != 0`: **65,280**
- real endpoint-boundary crossings: **1,024**
- equal adjacent endpoint pairs: **0**
- therefore interpolation, quadrant and wrap semantics must remain exact

Frozen target authority:
- exact P11O image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- inlined `apsis_space_fdn_render` **1,114 B / 374** static instruction lines
- `space_sine_q15()` fully inlined on target
- `space_sine_point()` survives as **48 B / 14** static instruction lines
- complete historical host + sanitizer preparation also PASS in the target gate

Decision: authorize P11Q proof-only for a per-line pair of exact signed-16-bit sine endpoints. Initialize them from each reset phase; refresh them only on an 8-bit phase-index transition; preserve the existing interpolation expression exactly. The source representation is 4 B/line (**32 B total**) before target ABI confirmation. Production remains P11O.

## P11Q — FDN sine endpoint-cache proof complete

Proof authority:
- technical head `a0b0826c9f0951b780d84404cfdbe9543ba45b49`
- run `41360291892` — PASS
- artifact ID `13104793891`
- artifact ZIP SHA-256 `7e7699b82e6540c64e099d4f8ea4d5a6d508e6a88d402a8c91c1d0aaa0cfcd05`
- repository production Space remained exact P11O
- historical workflow manual-only and pinned to the proven technical head

Exact equivalence coverage:
- top-16 helper domain **65,536** cases
- index-transition matrix **262,144** cases including `255 → 0`
- complete FDN **5,376 blocks / 164,767 frames**
- cold **256 blocks / 16,384 frames**, **88** hot-path refreshes after reset initialization
- warm **1,024 blocks / 65,536 frames**, **367** refreshes
- randomized continuation **4,096 blocks** with resets and randomized LFO phases
- stereo output, every pre-existing FDN state field and full delay storage bit-identical
- optimized + ASAN/UBSAN PASS
- full historical host + sanitizer behavior PASS with the ephemeral candidate

State/layout cost:
- host `ApsisSpaceFdnLine` **24 → 28 B**
- host `ApsisSpaceFdn` **232 → 264 B**
- target ordinary RAM delta **+32 B**, confirming exactly 4 B per line

Ephemeral real-target projection:
- baseline P11O: image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**, package `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- candidate: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**
- candidate RAM headroom **10,780 B**; POOL headroom **24,888 B**
- package bytes **610,110 B**
- candidate package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- inlined FDN render **1,114 → 1,098 B**, **374 → 366** static instruction lines
- `space_sine_point` stays **48 B / 14** static instruction lines
- dead `space_sine_q15` wrapper is removed by the candidate

Combined with P11P, the candidate replaces repeated endpoint evaluation on more than 99.9% of the measured natural sine calls for a bounded 32-B RAM/image cost and a smaller render code shape. P11Q therefore authorizes P11R production validation. It does not claim measured physical CPU or ISR improvement.

## P11R — production FDN sine endpoint-cache validation complete

Production authority:
- validated production head `fe8f9dc44a43ce427dab234934573b96108fe842`
- validation run `41361026431` — PASS
- evidence artifact ID `13105652509`
- evidence ZIP SHA-256 `40a6658c752c67cea3bf14afa99b9428d23e72681004bdb02eb799cca7961849`
- historical validation workflow manual-only and pinned to the proven production head

Actual production equivalence:
- **65,536** complete top-16 interpolation authority cases
- **262,144** old→new top-byte transition cases including `255 → 0`
- **5,376 blocks / 164,675 frames** complete-FDN comparison with cold, warm, randomized/reset trajectories
- optimized + ASAN/UBSAN PASS
- complete historical host + sanitizer behavior PASS
- actual production state/layout `ApsisSpaceFdnLine` **28 B**, `ApsisSpaceFdn` **264 B**

Frozen production target:
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- inlined FDN render **1,098 B / 366** static instruction lines

Idle authority remains exact:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`

Active Motion authority remains exact:
- active image **137,736 B**
- RAM **87,572 B**; headroom **10,732 B**
- POOL **319,176 B**
- package SHA-256 `4e55174f0fd68f0752eba3ca331c397dddfbbc573591815c3f049b856ecdbccb`
- seen `ff`, final `14`, 149/149 points, record 220,672→441,088, manual 450,560→466,176, cursor 481,024, queue 30/512, overflow 0, late flags 0, WAV peak 1,171, RMS 173

P11R therefore replaces P11O as the accepted production baseline. It accepts +32 B image/RAM for the P11P-measured endpoint reuse and P11Q-proved smaller render code shape. It proves exact behavior and static target properties, not physical FM-1 cycle or ISR improvement.

## P11S — FDN Q15 multiply operand-range census complete

Measurement authority:
- technical head `6ddfa625b371d9a93db3d0a5a635b48b7eb08046`
- run `41383031271` — PASS
- evidence artifact ID `13114240999`
- evidence ZIP SHA-256 `2365e3fbcc4578479d438f42085562e77d16417dd9a8443bc419ba194c96bddc`
- optimized + ASAN/UBSAN PASS
- exact P11R real-target package regenerated unchanged
- committed production Space remained byte-for-byte P11R
- historical census workflow manual-only and pinned to the proven technical head

Dynamic workload classification:
- alpha: **10 calls/frame**
- output scaling: **2 calls/frame**
- stereo cross/self mix: **4 calls/frame**
- Hadamard normalization: **8 calls/frame**
- excitation: **8 calls/frame**
- feedback gain: **8 calls/frame**
- total: **40 calls/frame**

Structural width result, including the frozen ±16,384 rounding term:
- alpha: max absolute product **1,003,391,074** — signed-32 safe
- output: **859,019,672** — signed-32 safe
- cross: **1,900,354,932** — signed-32 safe
- excitation: **425,184,592** — signed-32 safe
- Hadamard normalization: **3,036,845,560** — signed-32 unsafe
- feedback: **2,992,665,298** — signed-32 unsafe

Therefore **24/40 dynamic calls per FDN room frame** are structurally eligible for a narrow-helper proof and **16/40 must remain wide**. The natural trajectories happened to keep all observed products inside signed 32-bit, but P11S explicitly rejects using that high-water fact to narrow Hadamard or feedback because their legal structural bounds exceed signed 32-bit.

Frozen production authority remains P11R: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, package `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`, FDN render **1,098 B / 366** static instruction lines.

## P11T — selective FDN Q15 narrow-helper proof complete; production candidate rejected

Proof authority:
- technical head `a1907d5389de36ba094d86e2a74a6af729327651`
- run `41383730919` — PASS
- evidence artifact ID `13114376798`
- evidence ZIP SHA-256 `cfbf81487f4d5c7ffd6c6093baed7ae8a0f04a779bb8a43d82e73ff786169ed4`
- optimized + ASAN/UBSAN PASS
- complete historical host + sanitizer behavior PASS
- committed production Space remained byte-for-byte P11R

Exact arithmetic proof:
- alpha/output/cross/excitation only: **24/40 dynamic calls per room frame**
- Hadamard normalization + feedback remain wide: **16/40**
- **1,000,000** randomized helper comparisons + **70** explicit family-boundary cases
- complete FDN **6,311 blocks / 340,682 frames** across cold, warm, legal, random/reset and adversarial trajectories
- stereo output, complete state and delay storage bit-identical throughout

Target projection:
- P11R baseline: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,098 B / 366** static instruction lines
- candidate: image **140,552 B** (**+80 B**), RAM **87,524 B** (**0 B**), POOL **319,176 B** (**0 B**)
- candidate render **1,166 B / 410** = **+68 B / +44 static instruction lines**
- candidate package SHA-256 `e466515629a7be86f49de2db5bc9eb81d887025c32fafc235b2834e4a57df15c`
- narrow helper fully inlined; original wide helper remains required by the 16 vetoed calls

Decision: **no production validation.** There is no physical timing evidence that could justify accepting a strictly worse static target shape, and there is no RAM/POOL benefit. P11R remains the production authority. The selective narrow-Q15 path is closed unless future physical timing evidence explicitly reopens it.

## P11U — post-P11R residual FDN workload/code-shape inventory complete

Measurement authority:
- technical head `28386fe6e26d6aadb2102469bceac662f310e599`
- run `41384451346` — PASS
- evidence artifact ID `13115075736`
- evidence ZIP SHA-256 `02b8f2d74fd932721e89524f1b2bad174a452ae7ccc189dd3193a853f69a88c3`
- optimized + ASAN/UBSAN PASS
- exact P11R real-target package regenerated
- committed production Space remained byte-for-byte P11R

Residual sine result:
- **655,360** current `sine_b - sine_a` evaluations in the cold+warm measurement
- only **455** dynamic endpoint transitions; **463** delta evaluations total if reset initialization is included
- **654,905** render-time delta subtractions are avoidable under an A+delta cache model
- exhaustive adjacent endpoint delta domain is **−804 .. +804**, exactly signed-16 representable
- A+delta therefore fits the same two 16-bit per-line cache fields as P11R's A+B state

Target residual shape:
- P11R render **1,098 B / 366** static instruction lines
- `space_mul_q15` **88 B / 27** and `space_sine_point` **48 B / 14** are the relevant out-of-line helpers
- line-read, Hadamard, clamp, damping-alpha and feedback-gain are inlined
- render has **14** call instructions: 12 static wide-multiply references and 2 sine-point references

Decision: authorize P11V proof-only for cached A+delta sine interpolation. This is a representation proof, not a production change. It must retain exact interpolation semantics and complete FDN behavior, preserve state size, pass sanitizer/historical gates, and produce a non-regressive ephemeral JieLi target projection before any production-validation authorization.

## Exact next phase — P11V cached sine A+delta equivalence proof

Production remains P11R. Replace cached endpoint B with cached signed-16 delta only in proof/ephemeral target workspaces, prove the complete phase and FDN domains, then measure exact target image/RAM/POOL/package and render code shape. Reject the candidate if target evidence does not improve or otherwise justify it.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.

## P11V — FDN sine A+delta cache proof complete

Authority:
- technical head `e7b91e768eb44a07a2149cb4588da65cab653bfa`
- run `41391510585` — PASS
- artifact ID `13116934949`
- artifact ZIP SHA-256 `3a884a48d8fc14ad6147b09c9f902819c90a085d4dc89e05f9cfedb6a87bf93f`
- committed production Space remained exact P11R throughout

Proof coverage:
- **65,536** complete top-16 interpolation cases
- **65,536** index transitions, 65,280 refreshes / 256 reuses including wrap
- cold 256 blocks / 16,384 frames
- warm 1,024 blocks / 65,536 frames
- legal sweep 423 blocks / 13,536 frames
- random 4,096 blocks / 211,779 frames
- adversarial 512 blocks / 32,768 frames
- total **6,311 blocks / 340,003 frames**
- output, existing FDN state, feedback-cache state and full delay storage bit-identical
- optimized + ASAN/UBSAN PASS
- complete historical host + sanitizer behavior PASS

State remains exactly **28 B/line / 264 B FDN**.

Real-target projection:
- exact P11R baseline first: **140,472 B image / 87,524 B RAM / 319,176 B POOL**, package `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- candidate: **140,472 / 87,524 / 319,176 B** — zero resource delta
- candidate package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- FDN render **1,098 → 1,094 B**, **366 → 364** static instruction lines
- `space_sine_point` remains **48 B / 14** lines

Decision: P11W may apply only the proved A+B → A+delta representation to production. No other FDN, room or state change is authorized. P11V proves equivalence/static target properties, not physical FM-1 cycle or ISR improvement.

## P11W — production FDN sine A+delta validation complete

Production authority:
- exact technical production head `ac39372d8181c7fb27c9c25b55e825a5db53e10c`
- authoritative run `41393330606` — PASS
- evidence artifact ID `13118836564`
- evidence ZIP SHA-256 `c866cf8b844b80e31d070c934f9c26dc7378f21fe7acbae36f9c1ac550bb1c05`
- validation workflow frozen manual-only and pinned to the proven production head

Production source authority:
- `apsis_space_dsp.c` blob `3bcd294dd9fb610df888ddd0517da7de3bf4c086`
- `apsis_space_dsp.h` blob `ec221e22aeb6670ec628cbb0cc7c7eea91038554`
- Cathedral room source remains `ee00267cbbaef04bb1eed963b7b5cb1c2cc46346`
- committed source regenerated byte-for-byte from frozen P11R authority plus the P11V candidate transform

State/algorithm:
- `ApsisSpaceFdnLine` remains **28 B**
- `ApsisSpaceFdn` remains **264 B**
- cached sine representation changes only from exact A+B to exact A+delta (`delta=B-A`)
- reset/refresh and interpolation retain the P11V-proved semantics
- no FDN topology, phase/increment, room, feedback or scheduler change

Frozen production target:
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- FDN render **1,094 B / 364** static instruction lines

Idle authority remains exact:
- PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`
- peak 856; RMS 144/144; stereo delta 126

Active Motion authority remains exact:
- active image **137,736 B**
- RAM **87,572 B**; headroom **10,732 B**
- POOL **319,176 B**
- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- seen `ff`, final `14`, 149/149 points, record 220,672→441,088, manual 450,560→466,176, cursor 481,024, queue 30/512, overflow 0, late flags 0, WAV peak 1,171, RMS 173

P11W therefore replaces P11R as the accepted production baseline. Relative to P11R it has zero image/RAM/POOL growth and reduces the inlined FDN render from 1,098 B / 366 lines to 1,094 B / 364 lines. This is exact static/proxy evidence, not physical FM-1 cycle timing.

## P11X — FDN output sign-mix census complete

Authority:
- technical head `a28fb0047550b729c15cc7e5bc3413a71a13819b`
- run `41396085585` — PASS
- evidence artifact ID `13119167571`
- evidence ZIP SHA-256 `5ab3d57b84c7fb941cabc46ae478144763d8cc6e2661c9919f8d996ffab51c66`
- production Space source/header remain exact P11W

Measured facts:
- left/right sign vectors are fixed ±1 only, each **5 positive / 3 negative**
- eight-term Q15 signed-sum absolute bound **262,136**, safely inside int32
- source performs **16 sign products/frame**
- cold 16,384 frames → **262,144** sign products
- warm 65,536 frames → **1,048,576** sign products
- optimized + ASAN/UBSAN PASS

Exact target authority:
- P11W package regenerated exactly: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, package `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- render remains **364** static instruction lines
- raw JieLi loop loads signed sign-vector bytes and executes `r1 *= r0` and `r0 *= r2` for each line
- therefore the compiler retains **two general sign multiplications/line = 16/frame**
- the generic opcode classifier is not semantic authority for JieLi assignment-style disassembly; raw disassembly is

P11X authorizes only a P11Y proof candidate: replace the frozen sign products with explicit add/sub using exactly the same sign pattern. No production change is authorized.

## P11Y — fixed FDN output add/sub proof complete; candidate rejected

Authority:
- technical head `ca46ac5a5d3db9b9f0e43adb061ba780fab39c9a`
- run `41397297203` — all gates PASS
- evidence artifact ID `13119718531`
- evidence ZIP SHA-256 `dde978fd3a521dac64c3c5cb1a66e417b39a3a70508be2569481e765c228f1ec`
- committed production Space remains exact P11W

Equivalence proof:
- **1,000,312** fixed-sign arithmetic cases exact
- complete FDN **6,311 blocks / 386,191 frames** exact
- cold/warm steady, legal sweep, adversarial extremes, random frames/parameters and resets covered
- stereo output, complete FDN state and full delay storage bit-identical after every block
- optimized + ASAN/UBSAN PASS

Exact real-target projection:
- P11W baseline: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,094 B / 364** lines, package `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- fixed add/sub candidate: image **140,520 B (+48 B)**, RAM **87,524 B**, POOL **319,176 B**
- candidate render **1,154 B / 385** = **+60 B / +21 lines**
- candidate package SHA-256 `01d9f3fbc225973b6f2dcb861a86c2ee62af49594a288ba43c7cd2fac4ba61d0`

Decision: reject production validation. The candidate is mathematically/behaviorally exact, but every changed static target proxy regresses and there is no RAM/POOL saving. Physical timing is unavailable, so the 16 removed semantic sign multiplies/frame do not justify the larger target. P11W remains production authority.

## P11Z — Phase-11 closure/readiness matrix complete

Authority:
- technical closure head `fb1789477ada900d54ed319c9ee25645c6d3d055`
- run `41399427006` — all closure gates PASS
- evidence artifact ID `13120614128`
- evidence ZIP SHA-256 `de7eb00947513f9d527fb116a20b06defa6bf677f44d3e0dff8fb08f13f39668`
- closure adds no production optimization; accepted production remains exact P11W

Final accepted production target:
- product `FM-1_972`
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined FDN render **1,094 B / 364** static instruction lines

Final bounded-capacity closure:
- shared queue **415/512** observed high-water
- Spiral late slots **59/64**
- Bloom late slots **44/48**, exact legal maximum **44**
- String transient **80/80 logical, 112/112 physical**, steals 480, drops 0
- String Attractor pending **60/64**
- Clock pool **23/32**
- Spiral legs **30/32**
- Bloom petals **125/136**

Final emulator closure:
- idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`
- active Motion target image/RAM/POOL **137,736 / 87,572 / 319,176 B**
- active Motion seen `ff`, final `14`, points 149/149
- record 220,672→441,088; cycle 220,416 samples
- manual override 450,560; resume 466,176; cursor 481,024
- active queue 30/512; overflow 0; late flags 0; WAV peak 1,171; RMS 173

Phase-12-only unverified boundary:
- physical `app_audio()` ISR/callback cycle margin under worst-case combined load
- physical runtime stack high-water and guard margin
- LCD DMA/audio bus contention and physical control/LED/display behavior
- real NOR erase/program timing and power-interruption behavior
- DAC/listening quality and Cathedral perceptual tuning
- real FM-1 install/rescue/update path

No physical-hardware claim is made by P11Z.

## Exact next phase — guarded Phase-11 merge, then Phase 12 hardware validation

Rerun P11Z on the exact documentation head, open the guarded `p11-optimization` → `main` PR and merge only when the required exact-head workflows are green. After merge, do not reopen speculative Phase-11 micro-optimization without new measured evidence. Phase 12 is the physical FM-1 install/rescue, ISR/stack/LCD/NOR stress and DAC/listening phase.

## Do not regress

- one shared M3c scheduler; no module-private clocks
- one shared Space/Cathedral with five source sends; no per-source reverbs
- source DSP/output 44.1 kHz; target Cathedral quarter-rate unless explicitly re-approved
- current host ApsisRuntime 21,032 B with Bloom late capacity 48
- long Space storage in dedicated pre-`.pool` NOLOAD memory
- integrated idle cursor 123,392 with zero late/overflow flags
- audible idle PCM exact; dry-only Phase-5 PCM exact
- persistence journaled/CRC-protected and never blocking active audio
- native visuals consume existing state; no UI shadow musical state
- native boot takeover after sample 8192 unless exact PCM is revalidated
- direct performance finite/mailboxed; no private scheduler/RNG
- Motion 1,200 points/lane, 32-Hz bound and 9,644-B store unless later evidence explicitly re-approves a change
- Attractor/Gravity editors retain existing DSP/runtime/state authorities
- no bounded pool shrinks without strong bound evidence
- physical FM-1 ISR timing, stack high-water, storage/display/control timing and listening quality remain unverified
