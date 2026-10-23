# APSIS FM-1 Port Contract

Status: planning baseline.

This document defines what the project means by a **1:1 APSIS port**.

## 1. Priority order

When two goals conflict, use this order:

1. musical behaviour and timing;
2. interaction semantics;
3. sound-role identity and routing;
4. state / memory semantics;
5. visual identity and feedback;
6. exact browser control placement;
7. implementation similarity.

The FM-1 implementation does not need to resemble the JavaScript internally. It does need to produce the same musical consequences.

## 2. Things that are NOT allowed to collapse into generic substitutes

The following are identity-bearing and must remain first-class systems:

- STRING as a continuous ratio / pitch surface with Classic and Dual BODY/WIRE behaviour;
- independent BODY/WIRE anchors, Gravity-follow states, levels and linked/free movement;
- String Motion Loop, including frozen pitch context and overdub target semantics;
- String filter sequence / attractor behaviour and attractor strum;
- DRIFT as the paired beating field around the Gravity root, not a generic chorus/LFO macro;
- TUNING as Equal / Just / Pythagorean pitch interpretation with strength;
- CLOCK as a variable-length 3–24 pulse orbit with main, between and snap layers plus grouping boundaries;
- SPIRAL as an ordered route across 36 pitch points, not a rectangular step sequencer;
- Spiral attractor gates / energy / rate / register / lock / density and free-running phase behaviour;
- GRAVITY as 8 angular degrees × 3 voicing rings with quantised/immediate change timing and per-point editing;
- BLOOM as a voiced, late-bound, voice-led harmonic instrument with individual petals, core trigger and unfurl gesture;
- KEEP as musical-memory capture, not a full-engine preset dump;
- CHAIN as an ordered memory orbit with lane scope, duration, fine pulses, chance, transpose and continuous morph where defined;
- MOTHER as five-source mix + master plus structural `→1` hold/commit behaviour;
- one shared CATHEDRAL / SPACE return with per-source sends.

If any of these is temporarily simplified during development it must be marked as an explicit parity gap, not silently accepted as the final design.

## 3. Browser gestures versus FM-1 gestures

Exact pointer mechanics are not sacred. Their **meaning** is.

Examples:

- dragging along STRING may become keys + encoder movement, but it must still address the same continuous String coordinate and quantisation logic;
- drawing SPIRAL may become ordered key entry and encoder editing, but the route remains an ordered list of the same 36 pitch points;
- dragging outward from BLOOM's core may become a physical grow gesture using an encoder/key combination, but petals must still fire progressively according to an open/unfurl gesture rather than becoming a single static chord button;
- tapping GRAVITY points may become dedicated key mappings, but the same 8×3 point model and pending quantised commit must remain;
- CLOCK ring taps may become key LEDs and layer selection, but main / between / snap remain separate rings with the same timing positions.

## 4. Timing parity

Timing is part of the instrument.

The port must preserve, as closely as the FM-1 scheduler permits:

- BPM range and note-value semantics;
- alternating swing interval law;
- Clock cycle position and grouping boundaries;
- optional step-1 Spiral / Attractor resync, default OFF;
- Spiral attractor subdivisions at 1× / 2× / 4× Clock rate;
- String clock triggers and filter events;
- Bloom cycle occurrence, intra-cycle rate, Clock point and post-Clock delay;
- Gravity immediate versus quantised change timing;
- late-binding of Gravity-sensitive Spiral and Bloom pitches close to note-on;
- Chain start/return sync, cell duration, fine pulses, chance and morph duration;
- Mother structural `→1` horizon and commit boundary.

The native engine should use deterministic fixed-capacity event queues referenced to a single transport timebase. Main-loop timing must not be responsible for note-on accuracy.

## 5. Memory parity

KEEP deliberately stores musical memory rather than every sound/system parameter.

The parity target is the current memory-lane model:

- STRING tension / position and related surface identity fields;
- DRIFT amount;
- CLOCK pulse structure, not global BPM/division/swing;
- SPIRAL selected route;
- TUNING mode;
- GRAVITY key/scale and chord state.

CHAIN controls which lanes are recalled. Global engine settings such as sound voice choices, source levels, Space, String glide and Clock tempo remain live/global unless the reference source explicitly changes this contract.

Eight KEEP slots and up to 32 Chain links are required.

## 6. Audio-routing parity

Each audible source gets its own dry bus and send level:

- String
- Drift
- Clock
- Spiral
- Bloom

All sends feed one shared Space/Cathedral engine. Dry and wet remain separate before the master stage.

The FM-1 reverb implementation may be algorithmic instead of convolution-based because of memory/CPU constraints, but it must preserve the control topology and perceptual role:

- shared room input guard / bandwidth protection;
- early reflections;
- predelay;
- high-passed long tail;
- decay;
- damping;
- shared wet level;
- quiet diffuse feedback echo;
- source-specific send amounts.

Changing the algorithm is acceptable. Changing APSIS into five unrelated insert reverbs is not.

## 7. Visual parity

The native screen must preserve APSIS's visual grammar:

- black field;
- sparse luminous geometry;
- source-specific colours;
- circles, orbits, nodes and traces rather than conventional mixer/step-grid chrome;
- activity shown as movement / glow / pulse;
- minimal text on playable surfaces;
- parameter lists only as secondary edit views.

The FM-1 cannot render browser blur/compositing literally. Use a small intensity palette, concentric sprites, short trails, dithering and selective redraws instead of replacing the visual language.

## 8. Development rule inherited from SkurkyMello

Parity changes require evidence.

Before replacing or simplifying a source behaviour, record:

1. exact source behaviour;
2. reason the literal implementation is unsuitable on FM-1;
3. proposed native equivalent;
4. audible / behavioural test vector;
5. remaining difference.

Optimisation starts after parity tests exist, not before.
