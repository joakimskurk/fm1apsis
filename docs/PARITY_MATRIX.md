# APSIS FM-1 Parity Matrix

Status legend:

- `LOCK` — required final behaviour
- `PROPOSAL` — physical/UI implementation may change
- `TEST` — must have explicit host/device parity test
- `GAP` — known difference from browser source that must remain documented

## Global / state

| Area | Requirement | Status |
| --- | --- | --- |
| Defaults | Browser defaults reproduced exactly in native state | LOCK / TEST |
| State versioning | Native persistent state has explicit revision + migration | LOCK / TEST |
| Random seeds | String/Spiral attractor rerolls deterministic from stored seed | LOCK / TEST |
| Source identity | String, Drift, Clock, Spiral, Bloom remain separate audible roles | LOCK |
| Shared harmony | Gravity/Tuning changes propagate through shared harmony model | LOCK / TEST |
| Source sends | Five independent sends feed one shared Space return | LOCK / TEST |

## STRING

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Classic | One perceived String, BODY+WIRE layered at same pitch | LOCK / TEST |
| Dual | Independent BODY and WIRE positions | LOCK / TEST |
| Link | Physical movement mirrors while timbre/anchor/follow remain independent | LOCK / TEST |
| Anchors | root / third / fifth / seventh / ninth | LOCK / TEST |
| Follow Gravity | Per-side independent follow/hold behaviour | LOCK / TEST |
| Quantise | ON snaps according to current grid; OFF remains continuous | LOCK / TEST |
| Grid | scale / chromatic / chord | LOCK / TEST |
| Span | Same musical range law as source | LOCK / TEST |
| Glide | Smoothed continuous pitch movement | LOCK / TEST |
| Foundation | Separate low Gravity-root floor, octave + level | LOCK / TEST |
| Rhythm | off / bow-alternate / pizz / chord motion semantics | LOCK / TEST |
| Rhythm trigger | every N or trigger sequence | LOCK / TEST |
| Filter | shared String bus LPF with Q | LOCK / TEST |
| Filter sequence | pulse-triggered envelope behaviour | LOCK / TEST |
| Filter attractor | S&H rate/register/lock/gates/strum/slew | LOCK / TEST |
| Motion length | 1/2/4/8 Clock cycles | LOCK / TEST |
| Motion arm | starts on next Clock 1 | LOCK / TEST |
| Motion target | BODY/WIRE/BOTH where applicable | LOCK / TEST |
| Frozen context | recorded motion keeps its captured pitch context across later Gravity changes | LOCK / TEST |
| Manual override | live play can temporarily override motion playback | LOCK / TEST |
| Browser pointer | replaced by FM-1 keys/encoders while preserving String coordinate | PROPOSAL |

## DRIFT / TUNING

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Drift law | source nonlinear amount->Hz delta retained | LOCK / TEST |
| Pair | symmetric pair around Gravity root | LOCK / TEST |
| Depth | scales audible beat separation | LOCK / TEST |
| Width | stereo/spatial interpretation retained as closely as native DSP permits | LOCK / TEST |
| Tuning modes | Equal / Just / Pythagorean | LOCK / TEST |
| Strength | continuous interpolation amount | LOCK / TEST |

## CLOCK

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Length | 3..24 pulses | LOCK / TEST |
| BPM | 35..180 source range | LOCK / TEST |
| Division | 1/4, 1/8, 1/16 source semantics | LOCK / TEST |
| Swing | alternating `1-swing` / `1+swing` intervals | LOCK / TEST |
| Main | independent ON/OFF per Clock step | LOCK / TEST |
| Between | independent halfway events per Clock step | LOCK / TEST |
| Snap | independent main-grid snap events | LOCK / TEST |
| Group boundaries | separate from main trigger ON/OFF | LOCK / TEST |
| Step-1 resync | default OFF, optional reset of Spiral/Attractor subdivision phase | LOCK / TEST |
| Kick voices | Stone / Wood / Glass / Soft | LOCK / TEST |
| Ghost | source probability/placement behaviour | LOCK / TEST |
| FM-1 key edit | first N physical keys map to current Clock layer | PROPOSAL |

## SPIRAL

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Geometry | 36 fixed points / three-turn Archimedean model | LOCK |
| Route | ordered selected points, max 30 | LOCK / TEST |
| Route order | edits never silently convert route into unordered set | LOCK / TEST |
| Follow Gravity | route geometry stays fixed while musical interpretation shifts | LOCK / TEST |
| Register | low/mid/high source behaviour | LOCK / TEST |
| Voices | Thread / Glass / Reed / Halo spectral recipes | LOCK / TEST |
| Note length | source range and timing effect | LOCK / TEST |
| Attractor rate | 1x / 2x / 4x Clock | LOCK / TEST |
| Attractor length | 8 / 12 / 16 / 24 | LOCK / TEST |
| Lock | probability of preserving current cell value | LOCK / TEST |
| Density | reroll gate density behaviour | LOCK / TEST |
| Gate + energy | per attractor cell | LOCK / TEST |
| Seed | stored deterministic reroll evolution | LOCK / TEST |
| Late pitch binding | scheduled note time fixed while pitch can reflect committed Gravity close to note-on | LOCK / TEST |
| Probe | visual motion follows actual event route/timing | LOCK / TEST |
| FM-1 route entry | two-bank key mapping for 36 points | PROPOSAL |

## GRAVITY

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Degrees | 8 angular degrees | LOCK / TEST |
| Rings | 3 roles: Anchor / Colour / Air | LOCK / TEST |
| Key/scale | global tonic + mode | LOCK / TEST |
| Harmony depth | triad / 7th / 9th | LOCK / TEST |
| Inversion | source rules/range | LOCK / TEST |
| Scale lock | pitched material constrained when enabled | LOCK / TEST |
| Change timing | immediate / quantised | LOCK / TEST |
| Change grid | adapts to Clock length like source | LOCK / TEST |
| Pending state | queued Gravity distinct from committed Gravity | LOCK / TEST |
| Ring play | selected ring previews through String | LOCK / TEST |
| Per-point enabled | individual point enable | LOCK / TEST |
| Per-point offset | -2..+2 scale degrees | LOCK / TEST |
| Per-point octave | -1..+1 | LOCK / TEST |
| Per-point level/brightness/attack/release | retained | LOCK / TEST |
| FM-1 direct degrees | eight keys select degree, ring chosen physically | PROPOSAL |

## BLOOM

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Depth | follow Gravity / triad / 7th / 9th | LOCK / TEST |
| Occurrence | off or every 1/2/4/8 Clock cycles | LOCK / TEST |
| Rate | 1x / 2x / 4x inside eligible cycle | LOCK / TEST |
| Clock point | selectable trigger point | LOCK / TEST |
| Delay | post-Clock delay | LOCK / TEST |
| Harmonic spread | continuous openness target | LOCK / TEST |
| Register | low/mid/high | LOCK / TEST |
| Voice leading | common-tone preference + leap penalty + automatic inversion/octave placement | LOCK / TEST |
| Register wander | slow repeated-Bloom movement | LOCK / TEST |
| Voices | Veil / Choir / Organ / Glass | LOCK / TEST |
| Attack | retained | LOCK / TEST |
| Unfurl | first-to-last petal time, independent of harmonic spread | LOCK / TEST |
| Length | retained | LOCK / TEST |
| Width | retained | LOCK / TEST |
| Brightness | retained | LOCK / TEST |
| Tuning mode | follow/equal/just/pyth | LOCK / TEST |
| Tuning reference | chord root / global tonic | LOCK / TEST |
| Tuning amount | retained | LOCK / TEST |
| Petal | individual voice trigger | LOCK / TEST |
| Core | full chord trigger | LOCK / TEST |
| Grow gesture | progressive outward gesture fires petals by threshold | LOCK / TEST |
| Late binding | petal pitch can reflect Gravity committed near note time | LOCK / TEST |
| FM-1 grow | REC-held SELECT gesture candidate | PROPOSAL |

## KEEP

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Slots | exactly 8 musical memories | LOCK / TEST |
| String lane | saved String position/surface identity fields from source snapshot | LOCK / TEST |
| Drift lane | amount | LOCK / TEST |
| Clock lane | n/bounds/main/between/snap, not BPM/division/swing | LOCK / TEST |
| Spiral lane | selected route | LOCK / TEST |
| Tuning lane | mode | LOCK / TEST |
| Gravity lane | tonic/scale/degree/ring/depth/inversion | LOCK / TEST |
| Full preset | explicitly NOT the KEEP semantic | LOCK |

## CHAIN

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Max links | 32 | LOCK / TEST |
| Link slot | 0..7 | LOCK / TEST |
| Hold cycles | 0..32 whole Clock cycles with safe nonzero total | LOCK / TEST |
| Fine pulses | 0..n-1 | LOCK / TEST |
| Chance | 0..100% | LOCK / TEST |
| Transpose | -12..+12 st | LOCK / TEST |
| Morph | CUT / 1/4 / 1/2 / 1 cycle | LOCK / TEST |
| Start sync | Clock 1 / next pulse | LOCK / TEST |
| Return sync | Clock 1 / next pulse | LOCK / TEST |
| Lane scopes | key+scale / chord / String / Drift / Clock pattern / Spiral path / Tuning | LOCK / TEST |
| String morph | continuous when scoped + morph > 0 | LOCK / TEST |
| Drift morph | continuous when scoped + morph > 0 | LOCK / TEST |
| Other lanes | discrete structural application | LOCK / TEST |
| Orbit UI | curved/orbital representation preferred | PROPOSAL |

## MOTHER

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Sources | five levels: String/Drift/Clock/Spiral/Bloom | LOCK / TEST |
| Mutes | five independent source mutes | LOCK / TEST |
| Master | internal APSIS master retained | LOCK / TEST |
| Living core | visual responds to actual energy/activity | LOCK / TEST |
| `->1` arm | structural edits queue | LOCK / TEST |
| Mixer live while armed | source mix/mute/master not held by structural queue unless source explicitly says so | LOCK / TEST |
| Horizon | minimum pulse count before eligible next Clock 1 | LOCK / TEST |
| Atomic commit | pending structural changes commit together | LOCK / TEST |
| Auto disarm | returns live after commit | LOCK / TEST |

## SPACE / CATHEDRAL

| Behaviour | Requirement | Status |
| --- | --- | --- |
| Shared engine | one room return, not five inserts | LOCK |
| Per-source send | String/Drift/Clock/Spiral/Bloom independent | LOCK / TEST |
| Predelay | source range/role retained | LOCK / TEST |
| Decay | long source range retained perceptually | LOCK / TEST |
| Damping | low-pass character retained | LOCK / TEST |
| HP protection | low-frequency guard before/within tail | LOCK / TEST |
| Early reflections | audible early component | LOCK / TEST |
| Quiet echo | diffuse filtered feedback path | LOCK / TEST |
| Convolution | may be replaced by algorithmic network on FM-1 | GAP allowed |
| Metallic short-delay substitute | explicitly not acceptable | LOCK |

## FM-1 platform

| Requirement | Status |
| --- | --- |
| Pinned upstream commit/tree | LOCK / TEST |
| Unchanged baseline host + target builds first | LOCK / TEST |
| Unique FM-1 identity | LOCK / TEST |
| Updater/rescue retained | LOCK / TEST |
| No allocation/flash/LCD/wait in audio callback | LOCK / TEST |
| 44.1 kHz / 256-frame baseline assumption verified at build pin | LOCK / TEST |
| RAM/pool limits measured every target build | LOCK / TEST |
| Audio callback worst-case timed on device | LOCK / TEST |
| Exact release package SHA-256 recorded | LOCK / TEST |

## Release criterion

A release candidate cannot be called a 1:1 APSIS port while any `LOCK` row is knowingly missing unless that row is explicitly documented as an unavoidable hardware `GAP` with a native equivalent and evidence.