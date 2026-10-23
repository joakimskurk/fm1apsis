# APSIS FM-1 Staged Plan

This plan is deliberately parity-first. No phase may silently simplify APSIS to make the next phase easier.

## Phase 0 — freeze the reference

Goal: turn the current browser APSIS into a testable behavioural specification before native coding.

Deliverables:

- exact defaults table for every state field;
- exact parameter ranges and enums;
- event/timing map;
- audio signal-flow map;
- KEEP/CHAIN lane contract;
- Mother `->1` queue/commit contract;
- String Motion contract;
- late-binding contract for Gravity-sensitive Spiral/Bloom events;
- deterministic random/seed rules where present;
- baseline screenshots for every primary surface;
- representative reference audio renders from browser APSIS.

Exit gate: every identity-bearing behaviour in `PORT_CONTRACT.md` has a test or explicit reference note.

## Phase 1 — establish pinned FM-1 build skeleton

Goal: make `fm1apsis` build and run as an unchanged, auditable platform before APSIS DSP exists.

Steps:

1. pin exact Dinghy commit/tree in `UPSTREAM.lock.json`;
2. reproduce unchanged Dinghy host build;
3. reproduce unchanged Dinghy target build;
4. create APSIS overlay/app identity;
5. add CI checks for upstream identity, RAM/pool and target package;
6. add emulator smoke test;
7. add host screen/WAV harness.

Use the proven SkurkyMello build discipline rather than copying an opaque generated source tree.

Exit gate: empty/silent APSIS shell boots in host and emulator with updater/rescue path intact.

## Phase 2 — pure musical core, no DSP

Goal: port APSIS state and music logic as C that can be unit-tested on desktop.

Implement in order:

1. defaults + versioned state;
2. scales and tuning tables;
3. Gravity point model and chord/voicing helpers;
4. String position/ratio/pitch mapping;
5. Drift delta model;
6. Clock structure/grouping;
7. Spiral 36-point route and derived sequence;
8. deterministic String/Spiral attractor state;
9. Bloom harmony and voice-leading planner;
10. KEEP snapshot/apply semantics;
11. CHAIN cells/lane scope/morph targets;
12. Mother pending queue/horizon.

No sound required yet.

Exit gate: host parity tests match browser vectors for state transitions and timing-independent musical calculations.

## Phase 3 — deterministic transport

Goal: one native timebase replaces browser scheduler/timers without changing musical timing.

Implement:

- 35–180 BPM source range;
- quarter/eighth/sixteenth source division semantics;
- 3–24 Clock positions;
- alternating swing law;
- step/cycle absolute counter;
- main/between/snap event positions;
- String rhythm/filter event timing;
- Spiral 1x/2x/4x subdivision timing;
- Bloom cycle/rate/Clock-point/delay timing;
- quantised Gravity boundary calculation;
- Chain cell and phrase boundaries;
- Mother horizon + next-1 commit;
- fixed-capacity event queue.

Exit gate: long deterministic timing trace matches browser event ordering and expected boundaries with no main-loop timing dependency.

## Phase 4 — first sound: five dry sources

Goal: hear all five APSIS source identities without Space.

Implement individually and keep golden WAVs.

### 4A String

- foundation
- Classic BODY+WIRE
- Dual BODY/WIRE
- glide
- pluck/strum transients
- resonance/brightness character
- String bus filter

### 4B Drift

- persistent symmetric beating pair
- correct nonlinear delta range
- tuning interaction

### 4C Clock

- Stone/Wood/Glass/Soft profiles
- main, between and snap voices
- ghost behaviour

### 4D Spiral

- Thread/Glass/Reed/Halo partial recipes
- note length/register
- attractor-driven note events

### 4E Bloom

- Veil/Choir/Organ/Glass recipes
- individual petals
- full Bloom
- voice leading
- width/attack/length/brightness

Exit gate: each source is recognisably APSIS in isolation and passes deterministic host renders.

## Phase 5 — late binding + live structural behaviour

Goal: implement the interactions most likely to be accidentally simplified.

Required:

- Gravity immediate vs quantised changes;
- pending Gravity visual/state distinction;
- Spiral symbolic event -> late pitch resolution;
- Bloom voicing-plan start + late petal pitch resolution;
- step-1 resync OFF by default;
- Mother `->1` queues structural edits while mixer/master remain live;
- Chain changes use the same event/boundary model.

Exit gate: scripted tests change Gravity immediately before scheduled Spiral/Bloom events and verify the expected committed pitch state.

## Phase 6 — shared Space / Cathedral

Goal: reproduce the role of the browser shared room without exceeding FM-1 memory/CPU.

Start from topology, not from a generic reverb preset:

- per-source sends;
- room input trim/guard;
- early taps;
- predelay;
- long diffuse tail;
- HP/LP damping;
- quiet filtered feedback echo;
- wet return separate from dry.

Prototype two native candidates on host, preferably:

1. compact modulated FDN;
2. nested allpass/Schroeder hybrid.

Compare density, metallic periodicity, decay shape, high-frequency damping, transient response and CPU/pool cost.

Do not adopt a short periodic delay network that makes the Cathedral sound like ring modulation/comb filtering.

Exit gate: selected algorithm meets perceptual reference targets and device resource budget in worst-case five-source playback.

## Phase 7 — KEEP, CHAIN and persistent storage

Goal: long-form APSIS composition works before visual polish.

Implement:

- 8 KEEP slots;
- exact musical-memory snapshot scope;
- 32 Chain links;
- cycles/fine pulses/chance/transpose/morph;
- lane scope;
- start/return sync;
- continuous String/Drift morph where defined;
- versioned CRC-protected project storage;
- two-slot/journal recovery;
- save deferral until safe/silent condition.

Exit gate: reboot roundtrip, repeated save, corrupted newest record fallback and full Chain playback all pass on host/emulator.

## Phase 8 — native APSIS visual surfaces

Goal: the FM-1 now visibly becomes APSIS.

Implement primary screens in this order:

1. MOTHER
2. GRAVITY
3. STRING
4. CLOCK
5. SPIRAL
6. BLOOM
7. DRIFT/TUNING
8. CHAIN
9. KEEP overlay
10. SPACE secondary page

Rules:

- primary surface is geometry, not a parameter spreadsheet;
- sparse black field and luminous APSIS palette;
- bounded animation rate;
- dirty regions / small canvas strips;
- activity linked to real engine state;
- temporary values fade back into the surface.

Exit gate: screenshot review confirms each surface reads as APSIS before labels are read.

## Phase 9 — physical control parity

Goal: make the same instrument playable through FM-1 controls.

Implement and test `UI_MAPPING.md` surface by surface.

Priority:

1. Clock key-grid mapping;
2. Gravity 8-point direct play;
3. String key-map + continuous encoder slide;
4. Spiral route entry/edit;
5. Bloom petal/core/grow gesture;
6. KEEP capture/recall;
7. Chain editing;
8. Mother mixing/structural sync;
9. secondary parameter pages.

Exact button choices may change; semantic actions may not.

Exit gate: every browser performance gesture has a documented native equivalent and no primary musical action requires menu-diving.

## Phase 10 — String Motion and advanced interactive parity

Goal: complete the interaction-heavy systems after the basic physical mappings are stable.

Implement:

- arm to next Clock 1;
- 1/2/4/8 cycle length;
- Classic and Dual capture;
- BODY/WIRE/BOTH record targets;
- overdub preservation of the unowned side;
- frozen pitch context;
- manual override during playback;
- String attractor edit/reroll;
- Spiral attractor gate/energy editing;
- selected Gravity point editor.

Exit gate: motion survives Gravity changes exactly according to its frozen-context rules.

## Phase 11 — FM-1 optimisation

Only now optimise algorithms.

Measure:

- audio ISR worst case;
- RAM `.data/.bss`;
- pool usage;
- stack headroom;
- UI redraw cost;
- USB/MIDI load interaction;
- event queue high-water marks;
- max concurrent String/Spiral/Bloom voices;
- Cathedral buffers.

Optimisation order:

1. remove redundant work;
2. cache shared pitch/filter coefficients;
3. use lookup tables;
4. reduce visual redraw rate;
5. tune voice pools/stealing;
6. simplify DSP only with A/B parity evidence;
7. change numeric representation only when measurement supports it.

Exit gate: stable margin under worst-case audio + UI + USB load with no characteristic system removed.

## Phase 12 — validation/release

Mandatory checks:

- pinned upstream verified;
- host parity suite passes;
- emulator screen/input/audio smoke passes;
- exact release package SHA-256 recorded;
- unique FM-1 identity;
- real-device install from official and custom firmware;
- OCT- rescue;
- return to official firmware;
- USB MIDI/audio as supported by baseline;
- all 27 keys, 14 buttons, 7 encoders, LEDs and MASTER tested;
- worst-case ISR timing measured on hardware;
- all five sources + 4x Spiral + Bloom + String Motion + Cathedral stress test;
- KEEP/Chain save/reboot/recovery;
- power interruption during storage write;
- install another firmware from APSIS.

No public release before these pass.

## First implementation milestone

The first meaningful APSIS build should **not** try to contain everything.

Target milestone `M1`: host/emulator native shell with exact defaults + harmony + Clock + Gravity + String pitch model, real APSIS screen primitives, no final DSP yet.

That gives us the architectural centre of the instrument before expensive audio work starts.