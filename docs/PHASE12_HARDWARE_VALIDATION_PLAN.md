# Phase 12 — physical FM-1 hardware validation and release plan

Date started: 2026-11-02

Status: **prepared; blocked only on physical FM-1 availability.**

Branch: `p12-hardware-validation`

## Frozen authority entering Phase 12

Phase 12 starts from merged Phase 11 commit:

- `fc21e7e8e5c8c19752f23c5c5155dcf0db43a18c`
- product `FM-1_972`
- accepted production authority: Phase-11 P11W
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B** — 10,780 B headroom
- POOL **319,176 / 344,064 B** — 24,888 B headroom
- package bytes **610,110**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- idle PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`

The final pre-hardware closure is `docs/PHASE11_FINAL_CHECKPOINT.md`.

## Phase-12 rule

**Do not change production DSP, musical state, control mappings, persistence format or visual behavior merely because a physical test is inconvenient.**

Phase 12 is evidence-led. A production change is authorized only when a reproducible physical failure, measured timing/resource problem or clearly documented listening/ergonomic defect justifies reopening the relevant subsystem.

Diagnostic/instrumented builds must be kept separate from the release candidate. The release candidate remains the exact frozen P11W production source until a physical result requires a deliberate replacement.

## Gate order

1. **P12A — install and rescue**
2. **P12B — physical I/O smoke**
3. **P12C — functional musical parity**
4. **P12D — real-time ISR/callback timing**
5. **P12E — LCD/control/audio contention stress**
6. **P12F — runtime stack high-water**
7. **P12G — real NOR persistence and interruption**
8. **P12H — DAC/listening and perceptual tuning**
9. **P12I — release freeze**

Do not perform destructive/power-interruption storage testing before P12A rescue/back-to-official has been proven on the actual unit.

---

## P12A — install, boot and rescue

Goal: prove that the real device can safely enter APSIS and can be returned to a known-good state.

### Procedure

- record physical FM-1 unit identity/revision and any observable stock firmware version
- preserve the official/known-good recovery path before flashing APSIS
- build/regenerate the exact frozen P11W package from a clean pinned environment
- verify the generated package SHA-256 against the frozen authority before install
- install APSIS using the current FM-1/Dinghy-supported path
- cold boot, warm reboot and repeated power-cycle smoke test
- verify the device can be put back into the official/known-good firmware state
- reinstall APSIS once rescue is proven

### PASS

- exact expected package was installed
- APSIS boots repeatedly without recovery intervention
- the official/known-good rescue path is demonstrated on the same unit
- reinstall after rescue succeeds
- no boot-loop, unrecoverable display state or persistent corruption is observed

### Evidence to freeze

- unit/revision notes
- exact repo SHA and package SHA-256
- build/install command transcript or equivalent installer log
- rescue/back-to-official transcript
- boot/power-cycle count and result

---

## P12B — physical controls, LEDs, display and audio I/O smoke

Goal: verify the source-locked Phase-9/10 physical interaction contract against the actual panel.

### Controls

Exercise every physical input individually and in the frozen modifier combinations:

- all 14 buttons
- all 7 relative/detent encoders
- all 27 keys
- `SEL` modifier routes including shared SPACE/CATHEDRAL access
- MOTHER, STRING, DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, GRAVITY, CHAIN and KEEP navigation
- String Motion capture/ownership/overdub controls
- String Attractor, Spiral Attractor and Gravity selected-point editing

Confirm there is no assumed encoder-push behavior and that MASTER remains platform output gain.

### Display/LED

- all ten native 240×240 surfaces become reachable
- no persistent tearing, stale strips, corruption or lock-up during normal navigation
- LEDs/indicators correspond to the actual interaction state where the platform exposes them
- cold boot legacy draw → native APSIS takeover is visually sane

### Audio I/O

- left/right DAC output present
- no obvious DC-like fault, repeated click storm or stuck full-scale output
- MASTER behaves as expected

### PASS

Every physical control produces the intended frozen action exactly once per intended edge/detent, every required surface is reachable, display updates remain stable, and basic stereo audio output is healthy.

---

## P12C — functional musical parity on hardware

Goal: prove that the complete APSIS instrument behaves musically on-device, not merely in host/emulator tests.

Exercise, at minimum:

- MOTHER structural/live behavior
- GRAVITY point selection and normal degree triggering
- STRING Classic/Dual manual play, rhythm behavior, Attractor and Motion
- SPIRAL path editing, triggering and Attractor behavior
- BLOOM petal/grow/core performance and queued behavior
- CLOCK editing and transport interaction
- KEEP save/recall behavior
- CHAIN editing, playback and morph boundaries
- shared SPACE/CATHEDRAL across representative sources
- DRIFT/TUNING range and interaction

Use both sparse and dense states. Include repeated navigation away from and back to each engine while transport/audio is active.

### PASS

No physical-only ownership bug, stuck note/event, missing route, double-trigger, state corruption, impossible editor state or unexplained divergence from the frozen deterministic behavior is observed.

Any failure must be reduced to a repeatable physical scenario before source changes are authorized.

---

## P12D — real `app_audio()` ISR/callback timing

Goal: replace static/proxy confidence with real timing margin on the physical JieLi target.

### Measurement build

Create a **diagnostic-only** build once the hardware is available. Use an actual platform timing source available on the target; do not invent a software wall-clock approximation. Instrument only enough to capture callback duration/high-water and deadline violations.

Keep diagnostic instrumentation out of the release candidate.

### Scenarios

Measure at least:

1. idle frozen baseline
2. dense MOTHER/structural activity
3. heavy STRING transient load
4. active String Motion playback/overdub
5. heavy SPIRAL
6. heavy BLOOM
7. SPACE/CATHEDRAL active at demanding legal settings
8. combined dense musical load
9. combined dense load while aggressively using controls/display

For each scenario record:

- callback budget implied by the real sample-rate/block configuration
- maximum observed callback duration/cycles
- representative mean/percentile if feasible
- deadline miss/xrun/late indicator count
- test duration and exact musical/control state

### PASS

No callback deadline misses occur in the defined stress matrix and measured worst-case margin is positive and repeatable. If margin is uncomfortably narrow, the measured hotspot—not a speculative candidate—determines the next optimization investigation.

---

## P12E — LCD DMA / control / audio contention stress

Goal: prove that native 240×240 rendering and physical interaction coexist with uninterrupted audio on the real buses/IRQs.

Stress simultaneously:

- rapid surface changes
- continuous relative encoder movement
- dense key/button activity
- display regions that force ongoing strip updates
- active String/Bloom performance
- active SPACE/CATHEDRAL
- transport/CHAIN activity where relevant

### Observe

- audible glitches/clicks/dropouts
- callback timing outliers/deadline misses
- display tearing/corruption/stalls
- lost/duplicated controls
- persistent DMA busy state or UI starvation

### PASS

No repeatable audio deadline failure, display lock-up, control loss or bus-contention artifact occurs under the defined combined stress sequence.

---

## P12F — runtime stack high-water

Goal: verify real stack safety, which static RAM accounting does not prove.

Use a target-appropriate diagnostic method only after confirming the actual stack bounds/startup model. A guard/sentinel or platform-provided high-water mechanism is acceptable; a guessed memory range is not.

Exercise the full P12D/P12E worst-case matrix and record the minimum remaining stack margin.

### PASS

Stack remains within the valid region with a documented non-zero safety margin in every stress scenario, with no guard corruption.

If this gate cannot be measured safely with the available platform primitives, record it explicitly as an unresolved release blocker rather than claiming PASS from static RAM headroom.

---

## P12G — real NOR persistence and power interruption

Goal: validate the Phase-7 two-slot journal on real flash behavior.

Prerequisite: **P12A rescue proven.**

### Normal persistence

- save distinct KEEP/CHAIN project states
- cold reboot and verify newest committed generation
- repeat enough times to exercise alternating slots
- verify ordinary musical state remains intact around saves

### Interruption matrix

Use controlled interruption only against APSIS project-save activity, not bootloader/update flashing.

Test interruption around representative save phases where practical, then reboot and verify:

- previous committed record survives incomplete newer write
- corrupt/torn candidate is rejected
- newest valid generation is selected
- device remains bootable

### PASS

Real hardware reproduces the intended two-slot journal recovery semantics with no unrecoverable corruption and no flash operation observed from the audio callback path.

---

## P12H — DAC/listening quality and perceptual tuning

Goal: validate what deterministic hashes cannot: real sound quality, balance and musical usability.

Listen on a known monitoring chain at conservative output level first.

### Check

- idle noise/hum/whine
- startup/shutdown and navigation clicks
- left/right balance and obvious polarity/phase anomalies
- clipping or harsh overflow under dense legal states
- relative loudness across MOTHER/GRAVITY/STRING/SPIRAL/BLOOM
- transient behavior for manual String and Bloom
- SPACE/CATHEDRAL decay, width, damping, density and wet/dry usability
- Motion playback continuity and manual override/resume feel
- control ergonomics and editor discoverability on the real panel

### Tuning rule

Perceptual tuning may reopen parameters, but not topology/state architecture by default. Any proposed change must document:

- the physical listening defect
- the exact parameter/behavior changed
- before/after evidence
- regression coverage needed afterward

---

## P12I — release freeze

P12I begins only after P12A–H are either PASS or explicitly accepted as documented residual limitations.

### Final release checklist

- clean build from pinned sources/toolchain
- exact release commit frozen
- exact package SHA-256 frozen
- image/RAM/POOL budgets recorded
- physical unit/revision(s) tested recorded
- install instructions verified from scratch
- rescue/back-to-official instructions verified from scratch
- physical test matrix and evidence archived
- no diagnostic instrumentation present in production release
- final regression CI green on release commit
- version/tag/release notes created

### Release blocker rule

The following are hard blockers unless deliberately waived with written rationale:

- unrecoverable install/rescue failure
- repeatable audio callback deadline miss under legal use
- stack guard corruption/overflow
- repeatable persistence corruption that defeats valid-record recovery
- physical control path that cannot access or correctly operate a required feature
- persistent display/audio contention failure
- severe DAC fault, clipping bug or dangerous output behavior

---

## Physical evidence log format

For every Phase-12 run record:

- date/time
- unit/revision
- repo commit
- package SHA-256
- diagnostic or production build
- exact scenario/state
- duration/repetition count
- measured values
- PASS/FAIL
- links/names for logs, photos, videos or captures
- follow-up issue/commit if FAIL

Never replace a failed measurement with a new baseline until the cause is understood and the change is explicitly accepted.

## Current exact next action

Acquire/connect the physical FM-1, then execute **P12A install + rescue first** against the frozen Phase-11 production authority. Until hardware exists, Phase 12 should make no production-code changes and should not reopen rejected Phase-11 optimization candidates.
