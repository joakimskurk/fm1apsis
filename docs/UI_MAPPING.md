# APSIS FM-1 UI Mapping

Status: first physical-interface proposal. This document maps APSIS semantics to the FM-1 without turning APSIS into a generic synth/groovebox.

## 1. Hardware we actually have

FM-1 provides:

- 27 piano keys
- 14 buttons: OCT-, OCT+, FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC
- 7 encoders: SELECT, ALGORITHM, K1, K2, K3, K4, PRESETS
- button/key LEDs
- 240×240 colour display
- MASTER analogue knob

The mapping below treats those labels as physical controls, not as semantic restrictions. For example, `LFO` may open DRIFT because DRIFT is APSIS's beating/modulation field.

## 2. Global navigation

Primary surfaces should be one-button reachable where possible.

Recommended baseline:

| FM-1 control | APSIS meaning |
| --- | --- |
| HOME | MOTHER |
| ENV | STRING |
| LFO | DRIFT / TUNING |
| SEQ | CLOCK |
| ARP | SPIRAL |
| FX | BLOOM |
| GLO | GRAVITY |
| EDIT | CHAIN |
| SAVE | KEEP / memory |
| PLAY | transport start/stop |
| REC | contextual performance/record action |
| SEL | contextual layer/select modifier |
| OCT-/OCT+ | contextual register/bank/layer navigation |

This uses existing legends only as mnemonic hints. The screen always shows the APSIS name prominently enough that the instrument does not read like the factory firmware.

### Encoder convention

- `SELECT`: current object / point / layer selection
- `ALGORITHM`: high-level mode or sub-surface selection
- `K1..K4`: four visible live parameters
- `PRESETS`: KEEP slot / Chain position / contextual memory navigator

Avoid deep menu hierarchies. A playable surface should remain playable while its secondary parameters are visible.

## 3. Piano keys are an APSIS control surface

The keys are not automatically a chromatic keyboard on every page.

They should become direct physical access to the current APSIS geometry. This is one of the strongest opportunities to make the FM-1 port feel native without changing APSIS semantics.

Key LEDs mirror active points/gates/steps wherever possible.

## 4. GRAVITY

### Display

Render all three concentric rings and eight angular points on 240×240. Use minimal text: tonic/scale, ring name and pending `NEXT` indicator.

### Keys

Use eight adjacent white/black physical key positions as the eight Gravity degrees, preserving the browser's existing eight-point keyboard concept. Exact physical key indices should be chosen during host UI testing for hand comfort.

- tap degree key: queue/select degree on current ring
- `OCT- / OCT+`: cycle Anchor / Colour / Air ring
- `SEL + degree key`: edit/select that exact point without necessarily sounding it

### Encoders

First live page:

- K1 tonic
- K2 scale/mode
- K3 harmony depth
- K4 inversion

`ALGORITHM` cycles secondary Gravity pages:

1. Harmony
2. Change timing/grid
3. Selected-point parameters

The visual orbit remains visible behind/around the values.

### Quantised changes

When a Gravity change is pending, LED/screen feedback must distinguish selected-now from queued-next. Do not silently commit early.

## 5. STRING

### Display

Draw the actual horizontal String with 1:1 centre, ratio marks, current handle(s), Motion loop halo and attractor field. Classic and Dual must look materially different.

### Keys

The 27 keys become a broad playable String map rather than ordinary fixed MIDI notes:

- in Classic: all keys address the single String across its configured pitch span/grid
- in Dual: default keys address WIRE; `SEL` held addresses BODY
- key press sets String position and plucks
- held key + `SELECT` can fine-slide from that point

This extends the browser's existing A–L keyboard mapping while preserving the same String position -> pitch function.

### Encoders

Default String performance page:

- SELECT: continuous String position / fine slide
- ALGORITHM: Classic / Dual or subpage
- K1 resonance
- K2 brightness
- K3 glide
- K4 span

In Dual, `SEL` switches K1/K2 quick focus between BODY and WIRE level/anchor as needed.

### REC / Motion Loop

`REC` arms String Motion for the next Clock 1, matching the browser behaviour. While recording, physical String movement is captured.

- `REC`: arm/cancel
- `PLAY` remains global transport
- `SEL + REC`: toggle Motion loop playback when a loop exists
- `OCT-/OCT+`: octave/register bank for key-based String playing

For Dual/free mode, `ALGORITHM` or SELECT press cycles record target BODY / WIRE / BOTH. The current target must be obvious on screen before recording starts.

### Attractor editing

A secondary String page maps attractor cells to the first N physical keys, LED on=gate. PRESETS/SELECT moves the window only if needed; max length is 16, so one window is sufficient.

## 6. DRIFT / TUNING

### Display

Preserve the paired-source beating visual and the three Tuning stations below it.

### Keys

Three designated low keys select PYTH / JUST / EQUAL, with LEDs. The rest may remain non-destructive audition controls later, but no conventional keyboard behaviour is required here.

### Encoders

- SELECT: Drift amount
- K1 Drift depth
- K2 Drift width
- K3 Tuning strength
- K4 reserved for contextual quick value/readout or future parity field

Tuning mode stays discrete and immediately visible.

## 7. CLOCK

### Display

Draw three concentric rings:

- outer MAIN
- middle BETWEEN
- inner SNAP

Show grouping arcs outside. Current playhead pulses around the orbit.

### Keys

The first `n` physical keys map directly to Clock positions for n <= 24. This is an unusually good hardware fit because APSIS Clock max is 24 and FM-1 has 27 keys.

Layer selection:

- ALGORITHM = MAIN / BETWEEN / SNAP / GROUP
- key LEDs show state for selected layer
- tap key toggles selected layer at that Clock position
- in GROUP mode, tap a nonzero position to toggle a grouping boundary

The remaining 3 keys can be left unused or assigned only after parity is stable; do not fill them with unrelated features.

### Encoders

- SELECT: pulses n (3..24)
- K1 BPM
- K2 note value
- K3 swing
- K4 contextual voice/tone control

Secondary page exposes kick voice, pitch, between level, snap level/tone and ghost amount.

`REC` may serve tap tempo on this surface if it does not conflict with a required APSIS gesture; this should be tested rather than assumed.

## 8. SPIRAL

### Display

Render the real 36-point three-turn spiral, selected route, trigger ring and moving probe. This page is the clearest place where replacing the APSIS visual with a step list is forbidden.

### Entering the 36 points with 27 keys

Use two pitch banks:

- base bank: points 0..26 on the 27 keys
- `OCT+`: upper bank maps keys to points 12..35, giving overlap for continuity

The screen highlights which spiral point each physical key currently addresses. Alternative bank splits may be tested, but the underlying point IDs remain 0..35.

### Route editing

- tap key: append/toggle selected point according to a clearly defined route-edit rule
- SELECT: move route cursor / selected node
- K1: move selected point along Spiral without changing route order
- `SEL + key`: insert at route cursor
- `EDIT + key` or a deliberate contextual gesture: delete route point

Because route order is identity-bearing, ordinary key toggling must not accidentally reorder the route.

### Attractor

`ALGORITHM` switches between ROUTE and ATTRACTOR views.

Attractor view:

- first 8/12/16/24 keys = attractor gates
- LEDs show gates and current cell
- K1 rate
- K2 lock
- K3 density
- K4 note length or energy edit
- REC = reroll attractor, matching the existing creative gesture pattern used elsewhere in the FM-1 projects

### Voice

A secondary subpage exposes Thread / Glass / Reed / Halo and register/follow Gravity.

## 9. BLOOM

### Display

Draw the flower: petals positioned from actual chord intervals, central core, stem/leaf and unfurl state. Individual currently sounding petals glow.

### Keys

Map chord voices/petals to the lowest available keys:

- first 3–5 designated keys trigger individual petals
- a central/designed key gesture triggers full core chord

The remaining keyboard should not simply become a generic poly synth on this page.

### Progressive grow gesture

We need a physical equivalent to dragging outward from the core. Proposed native mapping:

- hold `REC` and turn SELECT from 0→1: progressively opens Bloom and fires petals at the same thresholds as browser grow
- release REC: gesture ends

This is intentionally a gesture, not a parameter edit. It should feel performative.

Leaf/unfurl edit:

- K1 unfurl
- K2 harmonic spread
- K3 length
- K4 brightness

Secondary page:

- cycles
- rate
- Clock point
- post-Clock delay
- register
- width
- attack
- voice
- tuning options

## 10. KEEP

`SAVE` opens the eight KEEP memories rather than a generic full-preset browser.

### Direct access

Use eight physical keys for slots 1–8, with LEDs for occupied/current state.

- tap occupied slot: recall
- `SAVE + slot`: capture current musical snapshot to slot
- deliberate erase chord, e.g. `EDIT + SAVE + slot`: erase, with visual confirmation

PRESETS encoder scrolls slots and can recall/capture through encoder press only if the hardware press semantics prove reliable; do not depend on it for the primary workflow.

## 11. CHAIN

### Display

Render a small orbit or curved chain of up to 32 memory nodes. Selected node is enlarged; current play node and total phrase progress are distinct.

### Keys

Eight slot keys insert KEEP 1–8 as a new Chain link at/after the cursor.

- SELECT: choose link
- PRESETS: choose memory slot for selected link
- K1 cycles
- K2 fine pulses
- K3 chance
- K4 transpose
- ALGORITHM: morph value / lane-scope subpages

Context actions:

- REC: duplicate selected link
- EDIT + REC: remove selected link
- PLAY: global transport
- `SEL + PLAY`: Chain play/stop/arm as needed after interaction testing

Lane-scope page exposes KEY+SCALE, CHORD, STRING, DRIFT, CLOCK PATTERN, SPIRAL PATH, TUNING as toggles, preferably mapped to seven illuminated physical buttons/keys.

## 12. MOTHER

### Display

MOTHER is HOME and should feel like the instrument's centre, not a settings page.

Render five arcs around a living central core. Arc position/length shows source level; mute is visible. The master core breathes with output activity. `->1` pending state appears structurally, not as a popup.

### Physical source mixing

FM-1 has four K knobs but APSIS has five sources. Avoid hiding the fifth behind a generic mixer page.

Recommended mapping:

- K1 String
- K2 Drift
- K3 Clock
- K4 Spiral
- SELECT Bloom

This asymmetry is acceptable if the visual geometry makes Bloom's SELECT control obvious. `ALGORITHM` may rotate which source is assigned to SELECT if user testing proves the one-off mapping confusing.

Source mute: hold SEL and press the corresponding physical surface button (ENV/LFO/SEQ/ARP/FX).

### Structural sync

- REC on MOTHER toggles `->1` arm
- OCT-/OCT+ changes horizon
- queued change count and next boundary are shown on screen

Mixer and MASTER remain live while structural edits are queued, matching the source contract.

The physical MASTER knob remains platform output level after APSIS's internal master unless testing proves double-master control confusing. APSIS's own master should stay in state because it is part of MOTHER; the relationship must be documented and gain-staged deliberately.

## 13. SPACE

SPACE is a secondary MOTHER or FX edit page, not a separate instrument source.

Four K knobs can map naturally to:

- Mix
- Decay
- Predelay
- Damping

Second page:

- Echo time
- Echo feedback
- source send selection/amount

Per-source send can be edited by selecting the source with SELECT/PRESETS while K1 controls send, or through the corresponding surface button + K knob. The final mapping should prioritise live usefulness over symmetry.

## 14. Screen rendering rules

- black background
- no factory-FM1 visual chrome
- no dense boxed parameter UI on primary surfaces
- use APSIS geometry as the main affordance
- large text only for temporary value confirmation
- values fade back to the instrument surface
- redraw only dirty areas / bounded animation strips
- LED state mirrors screen semantics instead of adding a second unrelated state language

The goal is that a user seeing the FM-1 from across the room recognises APSIS, not a reskinned firmware.

## 15. Mapping status labels

Every mapping in this document is one of:

- **semantic lock** — underlying APSIS behaviour may not change
- **physical proposal** — button/encoder choice may change after host testing

All musical models described here are semantic locks. Exact button assignments are physical proposals until validated.