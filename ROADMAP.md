# ROADMAP

Short operational roadmap. Detailed parity-first phase descriptions remain in `docs/PLAN.md`; frozen implementation detail remains in `PROJECT_STATE.md` and phase-specific documents.

Status legend: `[x]` complete, `[~]` active, `[ ]` not started.

## Phase 0 — reference freeze
- [x] Study 126 behavioural authority locked
- [x] Study 98 narrow String acoustic authority locked
- [x] behaviour map / parity matrix / port contract

## Phase 1 — pinned FM-1 skeleton
- [x] pinned Dinghy baseline and `FM-1_972` identity
- [x] host / real-target / pinned-emulator validation pipeline

## Phase 2 — pure musical/state core
- [x] defaults/state, tuning, Gravity, String, Drift, Clock, Spiral, Bloom, KEEP, CHAIN, MOTHER

## Phase 3 — deterministic transport/runtime
- [x] shared M3c sample-referenced scheduler/runtime
- [x] structural/runtime consumers and late binding
- [x] long deterministic traces and 512-event shared heap

P11F explicitly re-approved the host runtime layout at 21,032 B with Bloom late slots 44/48 under the frozen stress trace; no module-private clocks.

## Phase 4 — five dry source engines
- [x] String persistent / finite Rhythm / Attractor
- [x] Drift
- [x] Clock
- [x] Spiral
- [x] Bloom

## Phase 5 — live structural/sounding integration
- [x] structural late binding
- [x] quantized Gravity sounding asymmetry
- [x] MOTHER `->1` sounding commit semantics
- [x] CHAIN discrete/continuous sounding semantics
- [x] step-one free-run/resync
- [x] merged as `d677a887cf6dc8552634bde5a71fec0221c299b5`

## Phase 6 — shared Space / Cathedral
- [x] one shared room + five source sends
- [x] compact 8-line fixed-point FDN
- [x] quarter-rate 11.025-kHz target room while source/output remain 44.1 kHz
- [x] dedicated pre-`.pool` NOLOAD room storage
- [x] exact dry/audio regression and guarded merge
- [x] merged as `cd321e9f7999d0568d280d89e7e3574c3da66054`

## Phase 7 — persistent KEEP / CHAIN storage
- [x] 8 KEEP slots + 32 CHAIN cells/settings
- [x] versioned little-endian CRC journal over two NOR sectors
- [x] corruption/torn-write/wraparound tests
- [x] pre-audio boot read + deferred audio-safe writes
- [x] merged as `e9000b2b2f74d4fafe96df87f030cc55916a8285`

## Phase 8 — native 240×240 visuals
- [x] ten deterministic APSIS surfaces
- [x] allocation-free renderer using existing Dinghy canvas
- [x] HAL-gated incremental LCD work outside `app_audio()`
- [x] exact audio regression
- [x] merged as `4d71d712b34a684fb8f74f6b98ed675a9d5e1457`

## Phase 9 — physical interaction parity
- [x] pinned FM-1 panel/control/gesture contract
- [x] all ten surfaces navigable from existing panel vocabulary
- [x] direct String/Bloom performance via finite mailboxes/adapters
- [x] existing shared runtime/state/persistence authorities reused
- [x] exact target/emulator regression and final matrix
- [x] merged as `bbfab322557b6760425503268b550b789f796c23` via PR #19

Frozen Phase-9 target: image 132,264 B; RAM 89,492 B; POOL 309,532 B.

## Phase 10 — String Motion and advanced interactive parity

Status: **[x] complete and squash-merged to `main` as `378d2023df2afe59475be53c09cc001788586514` via PR #20.**

- [x] Clock-quantized 1/2/4/8-cycle String Motion capture
- [x] Classic/Dual ownership, free-Dual overdub and frozen pitch context
- [x] manual override/resume on shared Clock timeline
- [x] String Attractor editor over existing M4c authority
- [x] Spiral Attractor editor over existing M4f authority
- [x] selected Gravity-point editor over existing 24-point grid
- [x] no private scheduler, musical RNG or duplicate musical state
- [x] final exact-head matrix 22/22 PASS on `ce5857a52c6326011e5af7cd7f73819b420550fc`

Frozen Phase-10 production baseline:
- image **140,152 / 581,564 B**
- RAM **89,684 / 98,304 B**
- POOL **328,740 / 344,064 B**
- package SHA-256 `5a82ea63c9fac432d3a4ca98119defcfc1c51e04ad59317e8633e072e868b9ce`
- idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0

Phase 10 is JieLi-target/pinned-emulator verified, **not physical-FM-1 verified**.

## Phase 11 — FM-1 optimisation

Status: **[~] active on `p11-optimization`. P11A–P11U are complete; P11R remains the accepted production baseline. P11U identifies cached sine delta as the next proof-only candidate; P11V is authorized for equivalence + ephemeral target projection only.**

### P11A — baseline inventory
- [x] exact final Phase-10 image/RAM/POOL baseline
- [x] host `sizeof(ApsisRuntime) = 21,928 B`
- [x] original MotionStore 19,208 B accounts for the complete P9→P10 POOL increase
- [x] explicit measured-vs-unmeasured boundary
- [x] green run `41307570193` on `5e9eb2348ec3bd075eecfff2cad63fcca90a32a8`

### P11B — Motion storage falsification proof
- [x] one frozen pitch context per lane reproduces every captured point pitch exactly
- [x] free-Dual independent lane contexts/overdub proved
- [x] int16 pitch compression rejected as lossy
- [x] projected MotionStore 19,208 → 9,644 B with 1,200 points/lane and 32-Hz bound unchanged
- [x] ASAN/UBSAN green run `41308160497` on `0b9c197fc922eb76b37a5de3ac6f64a3ab3e511c`

### P11C — compact lossless Motion store
- [x] point storage 8 B → 4 B; one 20-B frozen context/lane
- [x] MotionStore **19,208 → 9,644 B**
- [x] endpoint pitch cache avoids reconstructing quantized/tuned pitch every callback
- [x] host reference equivalence + ASAN/UBSAN
- [x] exact real JieLi production target
- [x] exact frozen inactive PCM/runtime
- [x] exact active Motion lifecycle
- [x] exact-head green run `41318100472` on `7ce6993065186aa66c89bc723ba3edfa95448596`
- [x] complete production + idle + active evidence artifact

Frozen P11C production target:
- image **140,408 / 581,564 B**
- RAM **89,716 / 98,304 B**; headroom **8,588 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package SHA-256 `e10c01525a808a4dea18d6b3f8777307bf4acf180c2cf958bebc4e1546b0d0be`
- POOL improvement vs Phase 10: **−9,564 B**

### P11D — real-target memory ownership inventory
- [x] exact P11C target regenerated with pinned JieLi stack
- [x] real `objdump -h/-t` and generated linker script captured
- [x] MotionStore independently confirmed as 9,644 B in `.apsis_motion_pool`
- [x] ordinary RAM span reconciled exactly: 12-B `.data` + 89,700-B `.bss` + 4-B alignment = 89,716 B
- [x] unique sized-symbol intervals cover 89,658 B; remaining 58 B is not treated as free RAM
- [x] largest ordinary RAM owners ranked
- [x] configured user/system stack ranges identified and explicitly separated from runtime high-water
- [x] green run `41320007779` on `2e1912f5d0ff8987382bd7af434fc322679bf762`

Largest ordinary target-RAM owners at the P11C/P11D baseline:
- `bloom_dsp` 22,864 B
- `apsis_runtime` 21,128 B on target ABI
- `string_transient_pool` 7,188 B
- `bloom_audio_runtime` 6,424 B
- `spiral_audio_runtime` 4,792 B

Static configured stack ranges:
- user stack 24,320 B + 256-B guard
- system stack 7,936 B + 256-B guard

These are **configured ranges only**, outside the 96-KiB ordinary RAM metric and after the POOL region. No runtime stack-high-water claim is made.

### P11E — bounded capacity/high-water inventory

Status: **[x] complete and frozen. Measurement only; no capacity changed.**

- [x] shared queue observed max 415/512; no global mathematical upper bound, so no shrink authorized
- [x] Spiral late slots 59/64; veto shrink
- [x] String transient pool saturates 80/80 logical and 112/112 physical under legal burst; veto shrink
- [x] String Attractor pending reaches 60/64; veto shrink
- [x] Spiral DSP reaches 30/32 legs; veto shrink
- [x] Bloom DSP reaches 125/136 petals; retain current stress margin
- [x] exact Bloom scheduler proof: **44 simultaneous slots reachable and 45 impossible**
- [x] P11F candidate selected: Bloom late/snapshot capacity only
- [x] green inventory run `41322747611`; historical gate frozen to its proven head

P11E established a strict rule for later work: high-water alone does not authorize a reduction. A structural upper-bound proof or equally strong evidence is required.

### P11F — conservative Bloom slot reduction

Status: **[x] complete, exact-head validated, frozen and promoted to `p11-optimization`.**

- [x] `APSIS_RUNTIME_BLOOM_SLOTS` **64 → 48**
- [x] exact legal maximum remains 44, leaving four slots of structural reserve
- [x] host `sizeof(ApsisRuntime)` **21,928 → 21,032 B**
- [x] canonical/extreme Bloom high-water remains **44/48** with no late-slot overflow
- [x] Bloom audio snapshot/bind bridge passes normal and ASAN/UBSAN
- [x] full historical host + sanitizer behavior remains green
- [x] exact frozen idle PCM/runtime remains bit-identical
- [x] exact active Motion lifecycle remains identical
- [x] real JieLi production RAM **89,716 → 87,476 B** (**−2,240 B**)
- [x] production RAM headroom **10,828 B**
- [x] active Motion target RAM **87,524 B**, also **−2,240 B** from its P11C baseline
- [x] POOL remains **319,176 B**; no POOL capacity changed
- [x] authoritative final run `41324331937` on technical head `eb651e768a049a1b4a1e88397a9021fccc69fd30`
- [x] historical P11F workflow is manual-only and pinned to that proven technical head

Frozen P11F production target:
- image **140,408 / 581,564 B**
- RAM **87,476 / 98,304 B**; headroom **10,828 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package SHA-256 `e0d0b826f0c2d5b8f1af781a9259b5d78dce9062939661ac7acad17bfefb78e5`

### P11G — target/source workload proxy inventory

Status: **[x] complete, frozen and promoted. Measurement only; no production firmware change.**

- [x] exact P11F production target regenerated
- [x] generated `app_audio()` inventory: 219 source lines, 51 lexical call-sites, 34 unique call names
- [x] no native visual/LCD/input polling work inside `app_audio()`
- [x] native display quantified as 30 DMA-gated 8-row strips / 115,200 B RGB565 payload per full redraw
- [x] real JieLi ELF static code-shape inventory captured
- [x] Cathedral and transport call-frequency hypotheses separated from static code size
- [x] green technical run `41333497987` on `dbe0b540e602ea5ce8d93f4b119951736f5549bc`
- [x] evidence artifact SHA-256 `4365212ac4ad5668840d150f44be12933c9dcbef2faa575fbfdc261f750c6b5e`
- [x] frozen/promoted head `70470ddda31450b6aa168da643c8244184e72c16`

P11G is a static/source workload proxy, **not** a physical CPU/ISR timing measurement.

### P11H — Cathedral dynamic workload census

Status: **[x] complete, exact-head validated, frozen and promoted. Measurement only; production target remains byte-identical to P11F.**

- [x] test-only ephemeral counters; no instrumentation enters production firmware
- [x] 256 callbacks × 256 output frames = 65,536 output frames / 16,384 quarter-rate room frames
- [x] measured **196,608** single-channel stereo-delay reads
- [x] measured **131,072** room biquad evaluations
- [x] measured **294,912** room Q15 multiplies
- [x] measured **131,072** FDN line reads + **131,072** FDN line writes
- [x] measured **655,360** FDN Q15 multiplies
- [x] fastest legal transport over the same span: **18 scheduler calls / 218 events**
- [x] room-frame/scheduler-call frequency ratio = **910:1** over the comparison scenario
- [x] exact P11F target/package hash retained; exact idle and active Motion regressions retained
- [x] authoritative run `41341918347` on technical head `ca20dd8608ae685349d483f68d72ce4022b90402`
- [x] evidence artifact SHA-256 `27c9fabb358331d5b34444ea6233e4df25c58572cc9bd0f42e16159ff3d57fb7`
- [x] frozen/promoted head `4b69145e5bf6d27a394725fdd923876795680a5a`

P11H identifies the room delay-read path as a narrow, repeated optimization candidate. It still does **not** measure physical FM-1 cycles or ISR margin.

### P11I — stereo-pair Cathedral delay-read optimization

Status: **[x] complete, exact-head validated and frozen.**

- [x] pre-patch equivalence proof: **150,412** cases, optimized + ASAN/UBSAN
- [x] complete historical host/sanitizer behavior green before and after patch
- [x] six stereo locations/roomframe now share address/fraction calculation instead of 12 duplicated single-channel calculations
- [x] production pair helper remains bit-identical to frozen single-read authority
- [x] target room code shape **2,270 → 2,260 B** and **718 → 707** static instruction lines
- [x] production image **140,408 → 140,392 B**; RAM **87,476 B** unchanged; POOL **319,176 B** unchanged
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,672 → 137,656 B**
- [x] authoritative validation run `41344186184` on technical head `ab6960ad3acf78dbc9f366d2a5e9358b42011cca`
- [x] evidence artifact ID `13097594559`; ZIP SHA-256 `2a9b2d6f2adc8a5fd4a4f39dbe88cbd9b7c6bd2122b80ce83178a07a5a94e716`
- [x] historical P11I apply/validation workflows manual-only, read-only/pinned as appropriate

Frozen P11I production target:
- image **140,392 / 581,564 B**
- RAM **87,476 / 98,304 B**; headroom **10,828 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`

P11I proves a smaller target code-shape proxy, **not** physical FM-1 CPU cycles or ISR margin.

### P11J — FDN coefficient recomputation census

Status: **[x] complete, frozen measurement phase. Production Space sources remain exact P11I.**

- [x] 256 FDN blocks × 8 feedback gains = **2,048** feedback-gain evaluations per scenario
- [x] current cost per scenario includes **2,048 32-bit divisions + 6,144 wide fixed-point multiplies** in coefficient setup
- [x] steady room: decay changes **0**; theoretical recompute-on-change calls **8**; **2,040/2,048 (99.6%) redundant**
- [x] one legal 4s→12s smoothed sweep: decay changes **192**; cached calls **1,544**; **504/2,048 (24.6%) redundant**
- [x] adversarial legal extreme churn: decay changes **255**; cached calls **2,048**; **0** redundant
- [x] optimized + ASAN/UBSAN census PASS
- [x] P11I production Space blobs unchanged
- [x] authoritative run `41350487323` on technical head `17030232e5f21f95a81b834e831d9021683d81bf`
- [x] historical census workflow manual-only and pinned to that technical head

Decision: authorize P11K **proof only** for a cache of the eight feedback gains keyed solely by the rounded `decay_ms`. Worst-case churn has zero computation savings and must remain exact.

### P11K — feedback-gain cache equivalence proof

Status: **[x] complete, exact proof + ephemeral real-target projection frozen. Production FDN source/layout remain P11I.**

- [x] cached vs always-recomputed complete FDN output/state/storage bit-identical
- [x] reset/partial fill, steady, smoothed sweep, adversarial churn, all **8,001** legal decay keys and 4,096 random/reset blocks covered
- [x] optimized + ASAN/UBSAN PASS; host external cache representation **20 B**
- [x] steady: **8** gain evaluations vs 2,048 baseline; sweep: **1,544**; adversarial: **2,048**
- [x] proof run `41350818792` on head `ffb436dd87247af23c37e7e533cfd87fc2d161d0`
- [x] ephemeral target projection run `41351123630` on head `72b48c499ce7619fd3a077edac664ea2c65c4ca5`
- [x] projected target delta: image **+48 B**, RAM **+16 B**, POOL **0 B**
- [x] projected target RAM **87,492 B**; headroom **10,812 B**
- [x] FDN-render code shape **1,082 → 1,118 B**, **353 → 379** static instruction lines
- [x] evidence artifact ID `13101765292`; ZIP SHA-256 `a5cf89df0e49c8d79c5118db8f444f74a65808e6ca0f820e43bfda4d40ff0ef3`
- [x] repo production FDN source/header blobs remained unchanged throughout P11K
- [x] proof/projection workflows manual-only and pinned to their proven heads

Decision: authorize P11L production validation. The cache intentionally spends 16 B target RAM / 48 B image to remove 2,040 divisions + 6,120 wide multiplies in steady census and 504 divisions + 1,512 wide multiplies in the measured one-way sweep. Worst-case adversarial churn has zero coefficient-computation savings.

### P11L — production feedback-gain cache validation

Status: **[x] complete, exact-head validated and frozen.**

- [x] production FDN state stores only the eight exact gains + rounded decay key/validity
- [x] reset invalidates cache; render recomputes only when rounded decay changes
- [x] `space_feedback_gain()` arithmetic and all other FDN math/topology remain exact
- [x] actual production render is bit-identical to frozen always-recomputed authority across reset/partial fill, steady, smoothed sweep, adversarial churn, all **8,001** legal decay keys and 4,096 random/reset blocks
- [x] optimized + ASAN/UBSAN production equivalence PASS
- [x] complete historical host + sanitizer behavior PASS
- [x] actual real-target build exactly matches P11K projection
- [x] production image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**
- [x] production package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,704 B**, RAM **87,540 B**, POOL **319,176 B**
- [x] active package SHA-256 `54616e07ad5b579d33ec07732b5ec3daa824d71950ec8844805631251245374a`
- [x] authoritative run `41351791632` on production head `4a08705815fffcbf9407193a21334764d8710d51`
- [x] evidence artifact ID `13101743282`; ZIP SHA-256 `819f932f701d2e46cf13b26a641d68130785f84cc026eec39347563903f24d72`
- [x] validation workflow manual-only and pinned to the proven production head

Frozen P11L production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `131aeaffac35f1443abdaa4b1ce944c8d08b311e22bc5709b1acb071fd2e86e3`

Accepted dynamic workload tradeoff from P11J–P11L: steady census eliminates 2,040 coefficient divisions + 6,120 wide multiplies; the measured 4s→12s sweep eliminates 504 divisions + 1,512 wide multiplies; adversarial every-block decay churn has zero coefficient-computation savings. Physical CPU/ISR improvement remains unverified.

### P11M — FDN inner-loop branch census

Status: **[x] complete, frozen measurement phase. Production Space sources remain exact P11L.**

- [x] cold census: **16,384 frames / 131,072 line reads**
- [x] warm census: **65,536 frames / 524,288 line reads** after all eight FDN lines reach full capacity
- [x] modulation range measured **−2,304 .. +2,303 Q8**
- [x] minimum-delay clamp opportunities: **0** cold and warm; topology proves the branch unreachable under the production caller
- [x] upper ring-wrap condition: **0** cold and warm; topology proves one negative-wrap correction is sufficient
- [x] cold startup not-ready returns: **19,047 / 131,072 (14.53%)** — startup semantics must remain
- [x] cold reads already at full capacity: **111,902 / 131,072 (85.37%)**
- [x] warm full-capacity reads: **524,288 / 524,288 (100%)**; readiness failure impossible once full
- [x] warm negative-wrap condition: **521,026 / 524,288 (99.38%)** — negative wrap must remain
- [x] warm integer reads: **4,438 / 524,288 (0.85%)**; interpolated reads **519,850 (99.15%)** — interpolation must remain
- [x] warm `i1` ring-end wraps: **224 (0.043%)** — rare but real
- [x] optimized + ASAN/UBSAN census PASS
- [x] production P11L Space blobs unchanged
- [x] authoritative run `41355338593` on technical head `04b807617ffec084e3721fa2a166ac6ad9e76d59`
- [x] census workflow manual-only and pinned to the proven technical head

Decision: authorize P11N **proof only** for a production-caller-specialized `space_line_read()` candidate: retain exact partial-line startup readiness, fast-path full lines without `needed`, remove only the structurally unreachable minimum-delay clamp and upper-wrap correction, and leave modulation/interpolation/negative-wrap/`i1` semantics untouched. Defensive helper-guard removal requires a separate call-path proof.

### P11N — FDN line-read invariant simplification proof

Status: **[x] complete, exact proof + ephemeral real-target projection frozen. Production Space source remains P11L.**

- [x] current P11L helper vs candidate bit-identical across all eight line topologies and every integer modulation value **−2,304 .. +2,303 Q8**
- [x] readiness/ring boundary matrix + **500,000** deterministic randomized valid states
- [x] total helper comparisons **905,504**
- [x] complete-FDN authority/candidate comparison **3,328 blocks / 210,677 frames**
- [x] cold startup, warm continuation, randomized frame/decay/damping, resets and randomized LFO phases covered
- [x] exact partial-fill zero-return, negative wrap, fractional interpolation and `i1` wrap preserved
- [x] optimized + ASAN/UBSAN PASS
- [x] authoritative proof run `41355949491` on combined head `a13bce4ce821b68d8daab66ec1432ba3b5bf9a99`
- [x] ephemeral target projection run `41355949486` on the same head
- [x] P11L baseline regenerated exactly before candidate application
- [x] target image **140,440 B**, RAM **87,492 B**, POOL **319,176 B** — all unchanged
- [x] candidate package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- [x] inlined FDN-render code shape **1,118 → 1,114 B**, **379 → 374** static instruction lines
- [x] evidence artifact ID `13103071866`; ZIP SHA-256 `6d39137a817dd8a2d842790c40b4f51f6d0c0ef12ce7b33c56733f6aaa84711b`
- [x] repo production Space blobs remained exact P11L throughout P11N
- [x] proof/projection workflows manual-only and pinned to the combined proven head

Decision: authorize P11O production validation. The candidate has zero target-resource cost, a small strict code-shape reduction, and removes repeated full-line readiness work identified by P11M. Physical CPU/ISR improvement remains unverified.

### P11O — production FDN line-read validation

Status: **[x] complete, exact-head validated and frozen.**

- [x] production `space_line_read()` uses only the P11N-proved specialization
- [x] defensive helper guard, exact partial-line readiness, negative wrap, interpolation and `i1` wrap retained
- [x] actual production helper/full-FDN bit-identical to frozen P11L authority across **905,504** helper cases and **3,328 blocks / 210,677 frames**
- [x] optimized + ASAN/UBSAN production equivalence PASS
- [x] complete historical host + sanitizer behavior PASS
- [x] actual real-target build exactly matches P11N projection
- [x] production image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**
- [x] production package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- [x] inlined FDN-render code shape **1,114 B / 374** static instruction lines
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,704 B**, RAM **87,540 B**, POOL **319,176 B**
- [x] active package SHA-256 `adb581ff61ab1dc48c7bd08f7ca83bfa33f786fb635c31f36813f40d964b54ed`
- [x] authoritative validation run `41356749785` on production head `6560118c1cf7dfa6e40d1e33e86167d4f0b6ea5b`
- [x] evidence artifact ID `13104130300`; ZIP SHA-256 `0d747a515218da33e6bd9d61ae027aea64ce0a18033f6c4af64d32b27ccd0d9d`
- [x] validation workflow manual-only and pinned to the proven production head

Frozen P11O production target:
- image **140,440 / 581,564 B**
- RAM **87,492 / 98,304 B**; headroom **10,812 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`

P11O proves exact behavior and a strict inlined code-shape reduction with no image/RAM/POOL cost. Physical CPU/ISR improvement remains unverified.

### P11P — FDN LFO/sine workload census

Status: **[x] complete, frozen measurement phase. Production Space remains exact P11O.**

- [x] cold: **131,072** sine calls / **262,144** endpoint evaluations; only **96** endpoint-pair refreshes in cache model
- [x] warm: **524,288** sine calls / **1,048,576** endpoint evaluations; only **367** endpoint-pair refreshes
- [x] combined: **655,360** calls; endpoint pair unchanged for **654,897 (99.929%)** calls; **1,309,794** point evaluations avoidable by the measured cache model
- [x] all **256** fractional values occur; interpolation therefore remains mandatory
- [x] exhaustive top-16 domain: **65,536** cases, all 256 indices + 256 fractions, **1,024** real quadrant-boundary crossings, **0** equal adjacent endpoint pairs
- [x] optimized + ASAN/UBSAN census PASS
- [x] complete historical host + sanitizer preparation PASS via frozen P11F adaptation
- [x] exact P11O target/package regenerated unchanged
- [x] target `apsis_space_fdn_render` **1,114 B / 374** static instruction lines
- [x] `space_sine_q15()` fully inlined; `space_sine_point()` remains a **48 B / 14-line** target helper
- [x] authoritative run `41359406941` on technical head `2afb819dd865385f7547811b6422e7a281c80b7e`
- [x] evidence artifact ID `13104249141`; ZIP SHA-256 `d1a5d5898d00150c0f757f11fa01683d705a131bb35387d614741c49fa3b3517`
- [x] historical census workflow manual-only and pinned to the proven technical head

Decision: authorize P11Q **proof only** for two exact signed-16-bit sine endpoints per FDN line, initialized from reset phase and refreshed only when the phase top byte changes. Preserve the current interpolation expression exactly. Source-level cache cost is 32 B before target ABI confirmation. Production remains P11O until proof and target projection pass.

### P11Q — FDN sine endpoint-cache equivalence proof

Status: **[x] complete, exact proof + ephemeral real-target projection frozen. Production Space remains P11O.**

- [x] complete top-16 sine domain **65,536** cases bit-identical
- [x] all old→new top-byte transitions × fractions {0,1,127,255}: **262,144** cases, including `255 → 0`
- [x] complete-FDN comparison **5,376 blocks / 164,767 frames** with cold, warm, randomized trajectories, resets and randomized phases
- [x] cold hot-path endpoint refreshes **88** after exact reset initialization; warm **367** over 65,536 frames
- [x] optimized + ASAN/UBSAN PASS
- [x] full historical host + sanitizer behavior PASS with ephemeral candidate
- [x] host `ApsisSpaceFdnLine` **24 → 28 B**; `ApsisSpaceFdn` **232 → 264 B** (**+32 B**)
- [x] exact P11O target regenerated before projection
- [x] candidate target image **140,472 B** (**+32 B**), RAM **87,524 B** (**+32 B**), POOL **319,176 B** (**0 B**)
- [x] candidate package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- [x] inlined FDN render **1,114 → 1,098 B**, **374 → 366** static instruction lines
- [x] `space_sine_point` remains **48 B / 14** static instruction lines; dead `space_sine_q15` wrapper removed in candidate
- [x] authoritative run `41360291892` on technical head `a0b0826c9f0951b780d84404cfdbe9543ba45b49`
- [x] evidence artifact ID `13104793891`; ZIP SHA-256 `7e7699b82e6540c64e099d4f8ea4d5a6d508e6a88d402a8c91c1d0aaa0cfcd05`
- [x] repo production Space blobs remained exact P11O throughout P11Q
- [x] proof/projection workflow manual-only and pinned to the proven technical head

Decision: authorize P11R production validation. The candidate spends exactly 32 B RAM / 32 B image, no POOL, while P11P measured endpoint-pair reuse on 99.929% of natural sine calls and P11Q produces a smaller target render hot path. Physical CPU/ISR improvement remains unverified.

### P11R — production FDN sine endpoint-cache validation

Status: **[x] complete, exact-head validated and frozen.**

- [x] production stores exactly two signed-16-bit sine endpoints per FDN line; reset initializes them and top-byte transitions refresh them
- [x] reset phase/increment, interpolation, quadrant/wrap and all other FDN arithmetic/topology remain exact
- [x] actual production bit-identical to frozen P11O authority across **65,536** interpolation cases, **262,144** transition cases and **5,376 blocks / 164,675 frames**
- [x] optimized + ASAN/UBSAN production equivalence PASS
- [x] complete historical host + sanitizer behavior PASS
- [x] actual real-target build exactly matches P11Q projection
- [x] production image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**
- [x] production package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- [x] inlined FDN render **1,098 B / 366** static instruction lines
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,736 B**, RAM **87,572 B**, POOL **319,176 B**
- [x] active package SHA-256 `4e55174f0fd68f0752eba3ca331c397dddfbbc573591815c3f049b856ecdbccb`
- [x] authoritative validation run `41361026431` on production head `fe8f9dc44a43ce427dab234934573b96108fe842`
- [x] evidence artifact ID `13105652509`; ZIP SHA-256 `40a6658c752c67cea3bf14afa99b9428d23e72681004bdb02eb799cca7961849`
- [x] validation workflow manual-only and pinned to the proven production head

Frozen P11R production target:
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`

P11R accepts the measured P11P/P11Q tradeoff: +32 B image and +32 B RAM for endpoint-pair reuse on 99.929% of the measured natural sine calls and a smaller target render code shape. Physical CPU/ISR improvement remains unverified.

### P11S — FDN Q15 multiply operand-range census

Status: **[x] complete, frozen measurement phase. Production Space remains exact P11R.**

- [x] all **40 `space_mul_q15()` calls / room frame** classified into six dynamic families
- [x] alpha **10/frame**, output **2/frame**, cross **4/frame**, Hadamard normalization **8/frame**, excitation **8/frame**, feedback **8/frame**
- [x] cold **16,384 frames / 655,360 calls** and warm **65,536 frames / 2,621,440 calls** measured
- [x] legal damping/decay sweep **13,536 frames / 541,440 calls** measured
- [x] deterministic randomized/reset trajectory **4,096 blocks / 197,319 frames / 7,892,760 calls** measured
- [x] optimized + ASAN/UBSAN census PASS
- [x] structural bounds include the frozen ±16,384 rounding term rather than relying on observed high-water
- [x] structurally signed-32-safe: alpha, output, cross, excitation = **24/40 calls per frame**
- [x] structurally unsafe for signed-32: Hadamard normalization + feedback = **16/40 calls per frame**
- [x] safe structural max products: alpha **1,003,391,074**, output **859,019,672**, cross **1,900,354,932**, excitation **425,184,592**
- [x] rejected structural max products: Hadamard normalization **3,036,845,560**, feedback **2,992,665,298**
- [x] exact P11R target regenerated unchanged: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,098 B / 366** static instruction lines
- [x] production package SHA-256 remains `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- [x] authoritative run `41383031271` on technical head `6ddfa625b371d9a93db3d0a5a635b48b7eb08046`
- [x] evidence artifact ID `13114240999`; ZIP SHA-256 `2365e3fbcc4578479d438f42085562e77d16417dd9a8443bc419ba194c96bddc`
- [x] committed production Space blobs remained exact P11R throughout
- [x] historical census workflow manual-only and pinned to the proven technical head

Decision: authorize P11T **proof only** for a selective 32-bit exact Q15 helper at alpha/output/cross/excitation sites only. Hadamard normalization and feedback must stay on the wide helper. Preserve exact rounding/right-shift semantics; production remains P11R until proof, historical behavior and ephemeral target projection all pass.

### P11T — selective FDN Q15 narrow-helper proof

Status: **[x] complete, exact proof + ephemeral target projection frozen; candidate rejected for production. Production remains P11R.**

- [x] narrow helper restricted to the P11S-safe alpha/output/cross/excitation families = **24/40 dynamic calls per room frame**
- [x] Hadamard normalization + feedback remain on the frozen wide helper = **16/40 calls per frame**
- [x] helper boundary matrix: alpha **35**, output **7**, cross **14**, excitation **14** cases
- [x] **1,000,000** deterministic randomized helper comparisons, all bit-identical
- [x] complete-FDN comparison **6,311 blocks / 340,682 frames** across cold, warm, legal sweep, randomized/reset and adversarial full-scale trajectories
- [x] complete stereo output, state and delay storage bit-identical after every block
- [x] optimized + ASAN/UBSAN proof PASS
- [x] complete historical host + sanitizer behavior PASS with ephemeral candidate
- [x] exact P11R target regenerated before projection: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,098 B / 366** static instruction lines
- [x] candidate target image **140,552 B** (**+80 B**), RAM **87,524 B** (**0 B**), POOL **319,176 B** (**0 B**)
- [x] candidate FDN render **1,166 B / 410** static instruction lines = **+68 B / +44 lines**
- [x] candidate package SHA-256 `e466515629a7be86f49de2db5bc9eb81d887025c32fafc235b2834e4a57df15c`
- [x] authoritative run `41383730919` on technical head `a1907d5389de36ba094d86e2a74a6af729327651`
- [x] evidence artifact ID `13114376798`; ZIP SHA-256 `cfbf81487f4d5c7ffd6c6093baed7ae8a0f04a779bb8a43d82e73ff786169ed4`
- [x] committed production Space remained exact P11R throughout

Decision: **reject production validation for the P11T candidate.** The candidate is exact, but every changed static target proxy moves in the wrong direction: +80 B image, +68 B render size, +44 render instruction lines, with no RAM/POOL saving. Physical cycle timing is unavailable, so speculative 32-bit execution-speed benefit cannot override the measured target regression. Do not create a production-validation phase for this candidate.

### P11U — post-P11R residual FDN workload/code-shape inventory

Status: **[x] complete, frozen measurement phase. Production Space remains exact P11R.**

- [x] residual fixed work classified: **8** line reads, **8** line writes, **8** sine interpolations, **8** sine endpoint-delta subtractions, **16** fixed-sign output projections, **24** Hadamard add/subs, **8** clamps and **40** Q15 multiplies per room frame
- [x] cold **16,384 frames / 131,072 sine calls**: endpoint transitions **88**, reuses **130,984**
- [x] warm **65,536 frames / 524,288 sine calls**: endpoint transitions **367**, reuses **523,921**
- [x] combined **655,360** current `sine_b - sine_a` evaluations vs **455** dynamic endpoint transitions
- [x] including eight reset initializations, an A+delta representation would need only **463** delta evaluations over the measured window
- [x] current render performs **654,905** delta subtractions that are unnecessary under that cache model
- [x] exhaustive 256-index delta domain **−804 .. +804**, max abs **804**, 128 positive / 128 negative / 0 zero; signed-16 representation exact
- [x] exact P11R target regenerated: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, package `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- [x] FDN render remains **1,098 B / 366** static instruction lines
- [x] target out-of-line helpers: `space_mul_q15` **88 B / 27**, `space_sine_point` **48 B / 14**; line-read/Hadamard/clamp/damping-alpha/feedback-gain are inlined
- [x] render disassembly contains **14** call instructions: 12 static `space_mul_q15` references + 2 `space_sine_point` references
- [x] optimized + ASAN/UBSAN measurement PASS
- [x] authoritative run `41384451346` on technical head `28386fe6e26d6aadb2102469bceac662f310e599`
- [x] evidence artifact ID `13115075736`; ZIP SHA-256 `02b8f2d74fd932721e89524f1b2bad174a452ae7ccc189dd3193a853f69a88c3`
- [x] committed production Space remained exact P11R throughout
- [x] historical inventory workflow manual-only and pinned to the proven technical head

Decision: authorize P11V **proof only** for replacing each line's cached sine `A+B` representation with `A+delta`, where `delta=B-A`. Delta is exhaustively bounded to ±804 and fits the same signed-16 field width, so expected state size remains unchanged. P11V must prove complete interpolation/FDN equivalence and perform an ephemeral real-target projection before any production authorization.

### P11V — cached sine A+delta equivalence proof

Status: **[x] complete, exact proof + real-target projection frozen. Production remains P11R.**

- [x] **65,536** top-16 interpolation cases bit-identical
- [x] **65,536** index-transition cases; 65,280 refreshes / 256 reuses including `255 → 0`
- [x] complete FDN **6,311 blocks / 340,003 frames** across cold/warm/legal/random/adversarial trajectories
- [x] stereo output, existing FDN state, feedback-cache state and full delay storage bit-identical after every block
- [x] optimized + ASAN/UBSAN PASS
- [x] complete historical host + sanitizer behavior PASS with ephemeral candidate
- [x] state unchanged: `ApsisSpaceFdnLine` **28 B**, `ApsisSpaceFdn` **264 B**
- [x] exact P11R target regenerated first: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,098 B / 366** lines
- [x] ephemeral candidate target: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B** — **0 B resource delta**
- [x] candidate render **1,094 B / 364** static instruction lines = **−4 B / −2 lines**
- [x] candidate projection package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- [x] authoritative run `41391510585` on technical head `e7b91e768eb44a07a2149cb4588da65cab653bfa`
- [x] evidence artifact ID `13116934949`; ZIP SHA-256 `3a884a48d8fc14ad6147b09c9f902819c90a085d4dc89e05f9cfedb6a87bf93f`
- [x] committed production Space remained exact P11R throughout

Decision: **authorize P11W production validation.** Apply only the proved A+B → A+delta representation change; require actual-production equivalence, full historical behavior, exact JieLi target, idle PCM/runtime and active Motion before promotion.

### P11W — production FDN sine A+delta cache validation

Status: **[ ] next.**

- [ ] replace only cached `B` with exact signed `delta=B-A`; preserve two-int16 state width
- [ ] reset/refresh store exact A and delta; interpolation remains `A + ((delta*frac)>>8)`
- [ ] actual-production equivalence against frozen P11R A+B authority, optimized + ASAN/UBSAN
- [ ] complete historical host + sanitizer behavior
- [ ] exact real JieLi target must match P11V projection: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,094 B / 364** lines
- [ ] exact idle PCM/runtime and active Motion lifecycle
- [ ] freeze final package hashes before promotion

### Later Phase-11 work
- [ ] further Cathedral/FDN optimization only from measured, equivalence-backed evidence
- [ ] UI redraw/DMA optimization only if later measurement justifies it
- [ ] combined control/UI/audio stress gates as permanent regression coverage where useful
- [ ] Cathedral listening/tuning on physical hardware when available

Phase-11 invariant: optimise only from measured evidence; preserve Phase-10/P11F behavior and all frozen regression gates unless a change is explicitly re-approved.

## Phase 12 — hardware validation/release
- [ ] physical FM-1 install
- [ ] rescue/back-to-official
- [ ] controls/LEDs/display
- [ ] worst-case audio stress
- [ ] storage power interruption
- [ ] runtime stack high-water / real ISR margin and hardware timing
- [ ] DAC/listening quality
- [ ] release SHA-256

Physical hardware is not currently available, so no Phase 12 item is verified.

## Exact next action

Promote frozen P11V proof/projection state to `p11-optimization`, then create one P11W production-validation branch. Apply only the proved zero-growth A+B → A+delta cache representation and require complete actual-production equivalence, historical, real-target, idle and active-Motion gates before promotion.
