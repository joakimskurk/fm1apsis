# APSIS FM-1 Architecture

Status: planning baseline. Implementation intentionally deferred until the reference/parity map is frozen.

## 1. Principle

Do not port the browser DOM/WebAudio graph directly. Port the **musical model** into deterministic native subsystems, then map those subsystems to FM-1 audio, storage and UI.

The engine is one shared state graph. Modules are not independent mini-apps.

## 2. Proposed source layout

```text
apsis/
  apsis_state.[ch]       versioned state, defaults, migration
  apsis_transport.[ch]   Clock timebase, swing, event queue, boundaries
  apsis_harmony.[ch]     scales, Gravity, tuning, quantisation, voicing
  apsis_string.[ch]      String, motion, attractor, DSP
  apsis_drift.[ch]       beating field DSP
  apsis_clock.[ch]       pulse/group state and percussion voices
  apsis_spiral.[ch]      36-point route, attractor, late-bound events
  apsis_bloom.[ch]       voicing engine, unfurl, late-bound voices
  apsis_chain.[ch]       KEEP snapshots, links, lane recall/morph
  apsis_mother.[ch]      source mix/mutes and ->1 structural commit
  apsis_space.[ch]       shared Cathedral / echo return
  apsis_audio.[ch]       fixed-pool renderer and source buses
  apsis_storage.[ch]     journaled persistence
  apsis_ui.[ch]          physical input mapping / UI state
  apsis_draw.[ch]        240x240 APSIS-native surfaces
```

Keep state/logic separable from the FM-1 platform so host tests exercise the same musical engine.

## 3. State domains

Separate four classes:

1. **Musical scene** — String positions/mode, Drift, Clock structure, Spiral route/attractor, Bloom, Tuning, Gravity.
2. **Global sound/performance** — timbres, levels, filter/glide, BPM/division/swing, Space, master.
3. **KEEP memory** — eight compact musical snapshots matching the browser memory-lane contract.
4. **CHAIN** — up to 32 links with slot, whole Clock cycles, fine pulses, chance, transpose and morph.

Do not make KEEP a full-engine preset dump.

## 4. Transport and event model

Use one sample-referenced transport and fixed-capacity event queues.

Required events include:

- Clock main pulse
- between pulse
- snap
- String rhythm
- String filter sequence / attractor sample
- String attractor strum
- Spiral attractor tick
- Spiral note-on
- Bloom occurrence
- Bloom petal note-on
- quantised Gravity commit
- Chain cell boundary
- Mother structural commit

The main loop may schedule structural events, but it must not be responsible for exact note-on timing.

### Swing

Preserve the source law: alternating pulse intervals using `1-swing` / `1+swing` around the nominal interval.

### Late binding

Spiral and Bloom intentionally resolve Gravity-sensitive pitch close to note-on. Native flow:

1. schedule event time + symbolic musical identity;
2. keep pitch unresolved;
3. resolve against committed Gravity/Tuning shortly before event;
4. render at the original sample time.

This is identity-bearing behaviour, not an implementation accident.

## 5. Harmony engine

Shared by String, Drift, Spiral and Bloom.

Required concepts:

- tonic 0..11
- source scale IDs: major, natural minor, harmonic minor, melodic minor, dorian, phrygian, lydian, mixolydian
- scale lock
- Gravity degree 0..7
- three Gravity rings: Anchor / Colour / Air
- triad / 7th / 9th harmony depth
- inversion
- per-point enabled, scale offset, octave, level, brightness, attack, release
- Equal / Just / Pythagorean tuning
- tuning strength

Keep pitch symbolic as long as possible, converting to oscillator increment at the audio boundary.

## 6. STRING

Preserve:

- Classic single perceived String
- Dual BODY / WIRE
- linked/free movement
- independent anchors: root / third / fifth / seventh / ninth
- independent Gravity-follow
- quantise on/off
- grid: scale / chromatic / chord
- span and glide
- foundation voice
- clocked String modes
- filter sequence / attractor and attractor strum
- Motion Loop with 1/2/4/8 Clock-cycle lengths and target semantics

DSP should use persistent oscillators for sustained layers and a bounded transient pool for plucks/strums. Preserve BODY as the darker harmonic/bowed layer, WIRE as the brighter/plucked layer and FOUNDATION as the low root floor. Classic layers BODY + WIRE at one pitch.

STRING's bus filter affects both dry and wet feeds and must be smoothed.

## 7. DRIFT / TUNING

DRIFT is two continuously running sine-like voices symmetrically offset around the Gravity root. The offset follows the source nonlinear Drift amount law and depth. It is not vibrato and should not become a generic LFO effect.

TUNING changes shared pitch interpretation rather than acting as a conventional separate voice.

## 8. CLOCK

CLOCK supports 3..24 pulses and three timing layers:

1. MAIN — event on the Clock point;
2. BETWEEN — event halfway after that point;
3. SNAP — independent transient on the main grid.

Grouping boundaries are structural/accent metadata independent of MAIN on/off.

Retain the four kick profiles and snap/noise transient character with reusable fixed DSP state.

## 9. SPIRAL

Always 36 fixed pitch points in a three-turn Archimedean spiral.

State includes:

- ordered route, max 30 points
- register
- follow Gravity
- Thread / Glass / Reed / Halo voice
- note length
- attractor length 8/12/16/24
- rate 1/2/4
- lock, density, seed
- per-cell gate and energy
- optional step-1 resync inherited from Clock

The route is ordered. It must never collapse into a mere set of active notes or a rectangular step sequencer.

## 10. BLOOM

BLOOM is a harmonic instrument, not a chord macro.

Preserve:

- follow Gravity / triad / 7th / 9th depth
- 1x / 2x / 4x occurrences inside eligible cycles
- eligible cycle spacing 1/2/4/8 or off
- selected Clock point + post-Clock delay
- register and harmonic spread
- automatic voice-leading with common-tone retention and leap penalty
- slow register wandering
- Veil / Choir / Organ / Glass voice
- attack, unfurl, length, width, brightness
- tuning mode/reference/amount
- individual petal, core and progressive grow/unfurl gestures

Choose a voicing plan at Bloom start, but keep final petal pitch late-bound where the source does.

## 11. MOTHER

MOTHER owns:

- five source gains
- five source mutes
- master output
- structural `->1` pending queue
- minimum horizon

With `->1` armed, eligible structural changes queue while mixer/master stay live. Pending changes commit together only after the horizon and a Clock step-1 boundary.

This is not ordinary preset quantisation.

## 12. CHAIN / KEEP

KEEP stores musical memory lanes rather than the whole project. CHAIN recalls selected lanes from eight KEEP memories.

Each link supports:

- memory slot
- whole Clock cycles
- fine pulses
- chance
- transpose
- transition: cut / quarter / half / one cycle

Continuous morph applies only to lanes that are continuous in the source, especially String position and Drift amount. Discrete lanes commit structurally.

## 13. SPACE / CATHEDRAL

Browser APSIS uses one shared room fed by per-source sends. Preserve that topology.

Native FM-1 target can replace convolution with an algorithmic room if needed, but should retain the perceptual stages:

```text
source sends
  -> room guard / bandwidth protection
  -> early reflections
  -> predelay
  -> high-pass
  -> diffuse long tail
  -> damping
  -> wet return

room guard
  -> quiet filtered feedback echo
  -> long-tail path
```

The initial native candidate is a low-cost modulated FDN / nested allpass network rather than an IR. Do not accept it until decay, predelay, damping, early-energy and density tests are defined.

## 14. Audio buses

Five source buses:

- String
- Drift
- Clock
- Spiral
- Bloom

Each has:

- source level / Mother gain
- mute
- dry feed
- independent send to shared Space

Then:

```text
five dry buses -> dry sum ----\
                              -> master protection -> master
shared Space wet -----------/
```

Avoid WebAudio-style dynamic node creation on device. Use fixed voice pools and fixed routing.

## 15. Resource strategy

Use Dinghy host/device architecture and measure continuously.

- audio callback: 44.1 kHz / 256 frames on the pinned Dinghy baseline
- no allocation, flash, LCD or waiting in audio
- fixed voice counts
- persistent delay/reverb buffers in pool memory
- tables/constants in flash where appropriate
- UI dirty-region rendering only
- main-loop visual rate independent from audio timing

Initial safe target: worst-case APSIS render <= roughly half the callback period before adding UI/USB load. This is an engineering target, not a platform guarantee; device measurement decides.

## 16. Suggested fixed pools

Start conservatively and profile:

- String sustained voices: 5–6 oscillators including foundation
- String transient partial voices: bounded shared bank
- Drift: 2 persistent voices
- Clock: 4–6 transient slots
- Spiral: enough for worst-case overlap at 4x rate
- Bloom: 5 musical voices × up to 3 partial oscillators each, implemented as one fixed Bloom pool rather than heap objects
- one reusable noise table
- one shared Space buffer/network

Voice stealing must fade before teardown. Never hard-stop a waveform above silence.

## 17. Storage

Dinghy's simple settings slot is only 256 bytes, so APSIS needs its own flash range for project state while keeping platform settings separate.

Use a versioned, CRC-protected two-slot or journaled format with power-loss recovery. Never erase/write while audio is active unless the final platform strategy proves a safe asynchronous path.

Store:

- engine/global state
- eight KEEP memories
- Chain
- model revision

No samples are required by the current APSIS engine.

## 18. Host parity harness

Before a device UI exists, build host tests for:

- exact defaults/state roundtrip
- scale/tuning and Gravity vectors
- String position -> pitch vectors
- Drift delta vectors
- Clock intervals + swing
- Spiral route and attractor deterministic seeds
- Gravity commit boundaries
- Bloom voicing decisions
- KEEP capture/recall scope
- Chain duration/chance/morph transitions
- Mother `->1` horizon
- representative audio renders for all five sources + Space

These become golden tests and survive later optimisation.
