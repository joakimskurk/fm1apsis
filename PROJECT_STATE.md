# PROJECT_STATE

Last updated: 2026-11-01

## Current milestone

**Phase 11 — FM-1 optimisation is active. P11A–P11I are complete. P11I is exact-head validated and frozen; further production optimization requires a new measured/equivalence-backed candidate.**

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

## Current frozen production baseline — P11I

P11G and P11H were measurement-only. P11I is the current accepted production target after the equivalence-backed stereo-pair delay-read optimization:

- product `FM-1_972`
- image **140,392 / 581,564 B**
- ordinary RAM **87,476 / 98,304 B**
- RAM headroom **10,828 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`

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
