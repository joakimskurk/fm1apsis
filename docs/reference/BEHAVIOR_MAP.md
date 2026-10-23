# APSIS Browser Behaviour Map

Reference: `SOURCE_LOCK.md` SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`.

This is the first extraction map for the native FM-1 implementation.

## Default state

### STRING

- Dual by default, linked
- left/body split `0.47`, right/wire split `0.53`
- left anchor `root`, right anchor `fifth`
- both sides follow Gravity
- left level `0.72`, right level `0.68`
- resonance `0.66`
- brightness `0.54`
- quantise ON
- harmony grid `scale`
- span `1`
- foundation ON, level `0.52`, octave `-1`
- rhythm OFF
- filter mode OFF
- filter cutoff `6200 Hz`, Q `0.28`, depth `0.78`, decay `0.34 s`, slew `55 ms`
- String attractor: rate `1`, length `8`, lock `0.86`, strum `0.62`, strum level `0.78`, seed `317`
- Motion Loop length `1` Clock cycle, playback OFF
- glide `55 ms`
- send `0.72`, source level `0.80`

### DRIFT

- amount `0.42`
- depth `0.72`
- width `0.64`
- send `0.20`
- level `0.82`

The audible pair is symmetric around the Gravity root. The frequency separation law is:

```text
deltaHz = 0.05 + 3.95 * amount^1.65
```

then scaled by Drift depth/source state before creating the two carriers.

### CLOCK

- `12` pulses
- boundaries `3, 6, 9`
- all 12 MAIN pulses ON
- BETWEEN empty
- SNAP empty
- `72 BPM`
- note division `1/8`
- swing `0`
- step-1 resync OFF
- kick voice `stone`
- kick pitch `1.0`
- between level `0.66`
- snap level `0.62`
- snap tone `4200 Hz`
- send `0.06`, source level `0.78`

### SPIRAL

- empty selected melody route
- note length `0.82`
- register MID
- voice `thread`
- follows Gravity
- trigger source is attractor
- attractor rate `2x Clock`
- attractor length `16`
- lock `0.86`
- density `0.58`
- seed `911`
- stored gate and energy arrays
- send `0.62`, source level `0.78`

### BLOOM

- every `2` Clock cycles
- rate `1x`
- Clock point `0`
- post-Clock delay `60 ms`
- voice `veil`
- depth follows Gravity
- register MID
- harmonic spread `0.48`
- length `1.9 s`
- attack `18 ms`
- unfurl `0.42 s`
- stereo width `0.54`
- brightness `0.72`
- tuning follows global Tuning
- tuning reference `chord root`
- tuning amount `1.0`
- send `0.58`, source level `0.58`

### TUNING

- mode `equal`
- strength `1.0`

### GRAVITY

- tonic C (`0`)
- Dorian
- degree `0`
- voicing/ring index `1`
- harmony depth `4` voices = 7th
- inversion `0`
- scale lock ON
- change timing quantised to Clock
- change grid `2`
- ring play ON
- ring level `0.72`

### SPACE

- mix `0.48`
- decay `8.4 s`
- predelay `72 ms`
- damping `6200 Hz`
- echo time `362 ms`
- echo feedback `0.18`

### MOTHER

- structural sync OFF
- horizon `8` pulses
- all five source mix values `1.0`
- no source muted
- no pending structural changes

### CHAIN

- empty
- max 32 links
- default new link hold `4` whole Clock cycles
- start sync `Clock 1`
- return sync `Clock 1`
- default morph `CUT`
- default lane scope only `KEY + SCALE` enabled

## Timing backbone

The browser scheduler looks ahead and schedules from one shared Clock.

Nominal pulse interval:

```text
base = 60 / BPM * division
```

where division is `1`, `0.5`, or `0.25` for quarter/eighth/sixteenth behaviour.

Swing alternates:

```text
interval = base * (absoluteStep even ? 1-swing : 1+swing)
```

Per Clock step, ordering is conceptually:

1. String filter clock event
2. playhead / Mother pulse scheduling
3. MAIN Clock percussion if enabled
4. Spiral attractor subdivisions and Spiral musical events
5. String rhythm event if enabled
6. Bloom occurrence test
7. SNAP / BETWEEN / ghost Clock events
8. Chain pulse processing
9. advance absolute and cycle counters

The native engine should preserve the audible ordering where simultaneous events interact with Gravity/Chain/Mother state.

## STRING model

### Continuous coordinate

STRING stores physical positions rather than only final MIDI notes. Position is converted to a ratio around the `1:1` centre. Ratio marks include musically meaningful simple ratios.

Classic mode uses the right/single String coordinate and layers BODY + WIRE at the same pitch.

Dual mode retains two independent coordinates, with optional mirrored physical movement when linked.

### Motion Loop

Motion capture is clock-structured and has separate runtime state from the stored motion points. Recording can own BODY, WIRE or BOTH. In Dual free mode, recording one side preserves the other side's existing loop content.

A captured pitch context is retained so later Gravity changes do not rewrite the recorded melody.

### Filter attractor

Attractor cells carry a continuous random value plus a gate pattern. Lock determines whether a cell is replaced when visited. The same cell can drive filter value and optional strum excitation.

## GRAVITY model

Geometry is always:

- 8 angular degree positions
- 3 concentric roles/rings

The three rings map to different harmonic offsets/voicing concepts rather than merely octave copies.

A Gravity change can be immediate or pending. In quantised mode it is committed on a calculated Clock division boundary. The pending candidate is separate state until commit.

Every one of the 24 points can override:

- enabled
- scale offset
- octave
- level
- brightness
- attack
- release

## SPIRAL model

The visual pitch field is fixed at 36 points across three turns. The selected melody is an ordered list, max 30 points.

When `follow Gravity` is ON, the stored/drawn route remains in place while the interpreted pitches move with the harmonic frame.

The attractor controls when notes happen. It is separate from the selected melodic route.

At 2x or 4x rate multiple attractor cells can occur inside one Clock pulse.

Pitch resolution is deliberately late enough that a pending Gravity change may affect a note without moving its scheduled note-on time.

## BLOOM model

BLOOM has two different notions that must remain separate:

- `unfurl`: temporal distance from first petal to last
- `harmonic spread`: desired openness of voicing

Automatic Bloom chooses voicing/inversion/octave placement using voice-leading logic rather than random inversion. Common tones are favoured and large leaps penalised. Register can wander gradually over repeated Blooms.

A full Bloom creates 3–5 voice events depending on harmony depth. Each petal can also be triggered individually.

The grow gesture progressively triggers petals as radial openness crosses per-petal thresholds.

## KEEP model

A KEEP snapshot is intentionally partial. Current source snapshot contains:

- STRING position/surface identity subset
- DRIFT amount
- CLOCK structure: n, boundaries, MAIN, BETWEEN, SNAP
- SPIRAL selected route
- TUNING mode
- GRAVITY tonic, scale, degree, ring/voicing, harmony depth, inversion

It does not capture the whole sound engine.

## CHAIN model

Each link points to one KEEP slot and adds timing/performance metadata:

- whole Clock cycles
- fine pulses
- chance
- transpose
- transition duration

Lane scope determines which parts of the KEEP snapshot apply.

Continuous transition is only applied to String position and Drift amount in the current source. Other scoped lanes change discretely.

Loop start/return can quantise to Clock 1 or the next pulse.

## MOTHER model

MOTHER has two separate jobs:

1. five-source live mixer + master;
2. structural synchronisation.

When `->1` is armed, ordinary structural parameter changes are queued. Source mixing/muting and master remain live. The pending set commits atomically after at least the configured horizon and then on Clock step 1.

After commit, structural sync disarms.

## Audio topology

Each of the five audible source roles has:

```text
voice(s) -> source bus -> dry sum
                    \-> per-source send -> shared room input
```

STRING additionally has a bus filter and safety chain before both dry and wet feeds.

Shared wet path in browser source:

```text
room input
 -> trim
 -> HP/LP guard
 -> compressor guard
 -> early reflection taps -> master
 -> predelay -> HP -> cathedral convolver -> damping LP -> wet -> master
 -> filtered feedback echo -> long-tail path
```

Then dry + wet feed master protection/output.

The native port can change DSP implementation but not this role/routing model.

## Visual surfaces that must survive the port

- STRING: horizontal vibrating field, ratio centre/marks, one or two handles, Motion/Attractor visual layers
- DRIFT: two beating centres + Tuning stations
- CLOCK: three concentric trigger rings + grouping arcs
- SPIRAL: 36-point spiral + selected route + outer attractor ring + travelling probe
- GRAVITY: 8x3 concentric orbit with pending point feedback
- BLOOM: interval-derived flower + petals/core/stem/leaf
- CHAIN: memory orbit / phrase progression
- MOTHER: five source arcs around living central core

The 240x240 implementation can simplify glow rendering but not replace these with unrelated generic widgets.