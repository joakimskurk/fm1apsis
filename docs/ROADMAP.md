# APSIS → M-VAVE FM-1 executable roadmap

Status: **Phase 1 active**

Reference source: locked `apsis.html`, SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`.

The purpose of this roadmap is not to turn APSIS into a conventional FM-1 synth. The target is the closest practical 1:1 port of APSIS' musical behaviour, timing, state relationships, sound roles and visual language, with only the physical interaction adapted to the FM-1 panel.

## Non-negotiable priority order

When goals conflict, use this order:

1. musical behaviour and timing
2. interaction semantics
3. sound-role identity and routing
4. state and memory semantics
5. visual identity and feedback
6. exact browser control placement
7. implementation similarity

A phase may not be marked complete because the UI "looks right" while musical/state parity is missing.

## M0 — Source freeze and parity contract

**Status: complete.**

Artifacts:

- `docs/reference/SOURCE_LOCK.md`
- `docs/reference/BEHAVIOR_MAP.md`
- `docs/PORT_CONTRACT.md`
- `docs/PARITY_MATRIX.md`
- `docs/UI_MAPPING.md`
- `UPSTREAM.lock.json`

Gate:

- Browser source hash frozen.
- Identity-bearing systems enumerated.
- Browser defaults and central scheduler behaviour captured.
- Dinghy revision pinned by commit and tree.

## M1 — Reproducible native shell

**Phase 1. Current work.**

Goal: a silent APSIS firmware shell that boots in Dinghy host/emulator and target build without weakening updater/rescue behaviour.

Deliverables:

- unique APSIS application identity
- unique FM-1 package identity
- pinned Dinghy checkout verified by both commit and tree
- unchanged Dinghy host test before overlay
- unchanged Dinghy target build before overlay
- APSIS overlay applied only after baseline passes
- APSIS host test after overlay
- APSIS target package build
- package SHA-256 emitted
- target memory/resource report
- emulator smoke test
- no dynamic allocation in real-time code
- no screen/flash/blocking work in `app_audio`

Acceptance gate:

- `APSIS` appears as the application identity.
- package identity is `FM-1_972`.
- package stem is `apsis`.
- host/emulator boot succeeds.
- release package is deterministic from the pinned source and repository state.
- updater and USB rescue remain enabled.

## M2 — Pure musical/state core, no final DSP

Goal: encode APSIS behaviour as deterministic C that can be tested on a computer independently of the FM-1 audio ISR.

Implementation order:

1. versioned state and exact browser defaults
2. scale tables and aliases
3. Equal / Just / Pythagorean tuning data
4. Gravity scale indexing, rings, chord intervals, inversion and voicing helpers
5. String continuous position/ratio/pitch model
6. Drift beat-delta law and symmetric pair model
7. Clock pulse count, grouping, boundaries and step state
8. Spiral 36-point ordered route and derived sequence
9. deterministic attractor gates/energy from saved seed/state
10. Bloom harmony, voice count and voice-leading plan
11. KEEP partial snapshot semantics
12. CHAIN lane-scoped cells and duration rules
13. Mother pending structural-change model and horizon commit

Required tests:

- exact defaults regression
- scale/degree wrap tests
- Gravity chord/ring fixtures
- String split-to-step fixtures sampled from browser reference
- Drift fixtures
- Clock grouping fixtures
- Spiral order invariants
- deterministic attractor fixtures
- KEEP field-inclusion/exclusion tests
- CHAIN zero-duration protection
- Mother atomic-commit tests

Acceptance gate:

- No DSP is needed to run the entire deterministic model.
- Given the same state, pulse index and seed, host tests reproduce the same decisions every run.

## M3 — Sample-referenced transport and event engine

Goal: replace browser wall-clock/WebAudio scheduling with one deterministic 44.1 kHz timeline.

Requirements:

- one monotonic sample counter
- fixed-capacity event queues
- no heap allocation
- Clock pulse timing in sample units
- swing represented without cumulative drift
- event ordering preserves browser-visible state dependencies
- Spiral/Bloom support late-bound pitch while retaining original event sample time
- pending Gravity and Mother commits occur on the same structural boundaries as browser APSIS

Reference event order per Clock pulse:

1. String filter clock event
2. playhead / Mother pulse processing
3. main Clock percussion
4. Spiral attractor subdivisions/events
5. String rhythm
6. Bloom occurrence
7. snap / between / ghost Clock events
8. Chain pulse
9. counter advance

Acceptance gate:

- long-run host simulation has no transport drift.
- the same scripted interaction produces the same event log on repeated runs.

## M4 — Five dry sound sources

Goal: recreate the five APSIS source roles before shared Space.

Order:

1. STRING: body, wire, foundation, smooth pluck/sustain lifecycle
2. DRIFT: two beating voices around Gravity root
3. CLOCK: main/between/snap percussion roles
4. SPIRAL: glass/reed/halo/thread roles
5. BLOOM: choir/organ/glass/veil roles

Rules:

- fixed voice pools
- fade-before-steal, never audible hard-stop
- parameter smoothing where browser source smooths
- source-local filtering before dry/send split where APSIS does so
- no final shared reverb yet

Acceptance gate:

- every source can be isolated and compared dry.
- source identity is recognisable without Space masking differences.

## M5 — Live structural parity and late binding

Goal: make the instrument react like APSIS while running.

Includes:

- Gravity pending changes
- late-bound Spiral pitch
- late-bound Bloom voices
- String follow-Gravity behaviour
- String Motion frozen pitch context
- String Motion overdub/manual override semantics
- attractor strum and filter attractor
- Bloom core/petal/unfurl gesture state machine

Acceptance gate:

- scripted state-change tests match the browser-derived reference traces.

## M6 — Shared CATHEDRAL / SPACE

Goal: one shared spatial system, never separate generic reverbs per source.

Required topology semantics:

- five per-source sends into one room
- early energy + long diffuse tail
- damping control
- predelay
- quiet feedback echo into the tail path
- dry scene remains separate until final mix

FM-1 implementation direction:

- algorithmic room rather than generated browser convolution IR
- compact modulated FDN or sufficiently diffused allpass/Schroeder hybrid
- explicitly reject short, strongly periodic networks that produce comb/ring-mod character

Acceptance gate:

- long decay remains dense rather than metallic.
- source sends retain APSIS scene depth relationships.
- worst-case ISR remains comfortably inside the 256-frame callback budget.

## M7 — KEEP, CHAIN and durable storage

Goal: restore APSIS memory as musical structure rather than a generic preset system.

Requirements:

- KEEP stores only the source-defined partial snapshot
- 8 KEEP slots
- CHAIN cells retain slot/cycles/fine pulses/chance/transpose/morph
- lane scopes remain independent
- continuous morph only where the browser implements it unless later parity work proves otherwise
- start/return sync semantics retained
- durable storage uses APSIS-owned flash range, journal/two-slot strategy and CRC recovery
- Dinghy's 256-byte settings slot is not treated as sufficient for full APSIS memory

Acceptance gate:

- power-cycle/recovery tests cannot corrupt the last valid musical state.

## M8 — Native APSIS visual surfaces

Goal: redraw APSIS as APSIS on the 240×240 display rather than convert it to generic parameter menus.

Primary surfaces:

- STRING continuous surface
- DRIFT / TUNING field
- CLOCK orbit and group boundaries
- SPIRAL ordered route/probe
- BLOOM petals/core
- GRAVITY 8×3 ring structure
- CHAIN orbit/list state
- MOTHER source universe and pending horizon state

Rules:

- visual state must be driven by the real musical model
- redraw only changed regions where practical
- no display work from audio ISR
- generic text parameter pages are support views, not the identity of the instrument

Acceptance gate:

- a user familiar with APSIS can identify each surface without labels alone.

## M9 — Physical control parity

Goal: make the FM-1 panel feel like a purpose-built APSIS controller.

Proposed primary mapping, subject to interaction testing but not to musical simplification:

- HOME → MOTHER
- ENV → STRING
- LFO → DRIFT / TUNING
- SEQ → CLOCK
- ARP → SPIRAL
- FX → BLOOM
- GLO → GRAVITY
- EDIT → CHAIN
- SAVE → KEEP
- PLAY → transport
- REC → contextual record/performance
- SEL → modifier
- OCT-/OCT+ → contextual bank/register/layer

Keys are contextual APSIS surface controls, not permanently a chromatic keyboard.

Acceptance gate:

- every identity-bearing browser action has a direct or coherent FM-1-native equivalent.

## M10 — String Motion and advanced performance parity

Goal: finish the interaction-heavy behaviours most likely to expose hidden browser assumptions.

Includes:

- record arm/start/finish
- loop length/cycle handling
- both/left/right capture
- frozen context
- overdub/replacement semantics
- manual takeover
- continuous-pitch option
- attractor/manual interaction

Acceptance gate:

- long scripted performance traces are repeatable and click-free.

## M11 — Optimisation and stress validation

Required scenarios:

- all five sources active
- maximum practical polyphony
- long Space decay
- Spiral attractor + Bloom + String Motion simultaneously
- CHAIN transitions with Mother pending changes
- rapid panel interaction
- USB MIDI/audio active

Targets:

- no heap allocation during performance
- no audio ISR flash/screen/waits
- no buffer overruns
- no stuck notes
- no event-queue starvation
- initial engineering target: worst-case render <= 50% of callback period until measured hardware evidence permits a tighter budget

## M12 — Hardware validation and release

Hardware checklist:

- install from official/compatible firmware
- boot
- controls
- MIDI
- USB audio
- audio quality/noise
- sustained worst-case ISR timing
- save/load endurance behaviour
- updater path
- USB rescue
- return to official firmware

Release evidence:

- Dinghy commit/tree
- APSIS repo commit
- target memory report
- host test log
- emulator log
- hardware checklist
- `.fwsc` SHA-256

## Immediate work order from this commit

1. establish Phase 1 build/CI skeleton
2. add silent APSIS overlay and identity
3. add pure host-buildable core with exact defaults
4. run CI and fix target/host integration until green
5. begin M2 with scale/tuning/Gravity fixtures before any final synth DSP
