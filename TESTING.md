# TESTING

This file records what has actually been verified. Do not promote a result to a higher verification level without evidence.

## Verification levels

1. implemented
2. desktop build/test
3. sanitizer test
4. real FM-1 target build
5. emulator smoke
6. physical FM-1 hardware

## Baseline

Pinned Dinghy:
- commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`
- tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`

APSIS identity:
- product `FM-1_972`

CI target budgets:
- app image: 581,564 B
- RAM `.data + .bss`: 98,304 B
- pool: 344,064 B
- sample rate: 44.1 kHz
- callback baseline: 256 frames

Physical FM-1 is not currently available. Nothing below is promoted to physical-hardware verification.

## Phase 1

Status: **verified through emulator**.

Verified pinned source/upstream identity, unchanged Dinghy host/target, APSIS overlay/package identity and emulator boot/input smoke.

## Phase 2 / M2a + M2b + M2c

Status: **verified through emulator and merged**.

Desktop source-derived state suites and ASAN/UBSAN pass.

Final M2c target:
- image 56,700 B
- RAM 14,104 B
- pool 30,720 B
- package SHA-256 `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`
- emulator WAV peak 0

## Phase 3 / M3a

Merged to `main` as `fe2911c43fd69e63839220482e50bb10c7a2e4c2` via PR #4.

Status: **verified through emulator and merged**.

Final evidence:
- core #60: PASS
- staged #50: PASS
- image 56,680 B
- RAM 14,104 B
- pool 30,720 B
- package SHA-256 `6f2b4549eef194672d4a340e8b7cb33fe1c04befe564ad7da39d3275b8f96e94`
- emulator WAV peak 0

## Phase 3 / M3b

Merged to `main` as `a61a40dc83c9b52129f7d3f920da26a9a61837d9` via PR #5.

Status: **verified through emulator and merged**.

Final evidence:
- core #81 implementation head: PASS incl. ASAN/UBSAN
- staged #71 implementation head: PASS
- final documentation-head core #84 / staged #74: PASS
- image 66,428 B
- RAM 22,552 B
- pool 30,720 B
- host runtime sizeof 8,456 B
- host queue 14/256
- emulator live queue 7/256, overflow 0, cursor 123,648
- package SHA-256 `16ff1f412649bfb392ddc7fbfabd03bb405d0a86fca96a0d37d0b60b4f19c819`
- emulator WAV peak 0

## Phase 3 / M3c

Merged to `main` as `ec0dc0b350793572f19040efc938f00121128dc6` via PR #6.
Final pre-merge head: `4fa7d25920523b9e5d8591ae6185e1b5f910c083`.

Status: **verified through emulator and merged**.

Normative references:
- `docs/reference/LATE_BINDING_CONTRACT.md`
- `docs/reference/M3C_REFERENCE_TRACE.md`

Verified source behavior includes:
- Spiral occurrence identity frozen but final sequence/pitch/register/tuning re-read at native bind lead 80 ms
- Bloom occurrence at target−45 ms with frozen plan/register/random/unfurl geometry and per-petal target-aware bind at `max(occurrence, target−50 ms)`
- narrow-browser 110/70 ms leads retained as source facts
- normal Gravity target−18 ms, Mother target−26 ms, Chain discrete target−24 ms, Chain morph at target
- Stop/restart cancellation semantics
- Bloom unfurl represented as 0..3600 ms, default 420 ms
- 32-bit `spiral_step` for long-run parity

Capacity/determinism evidence:
- original 256-entry M3 queue failed legal M3c worst-case stress and was raised to 512
- canonical 30 s: runtime 21,928 B, queue 352/512, Spiral slots 59/64, Bloom slots 44/64, random `78771ff5`, late `0e6fc422`
- 60 s max-swing extreme: queue 357/512, Spiral 59/64, Bloom 44/64
- overflow/starvation/late-slot overflow all 0
- normal and ASAN/UBSAN agree

Final exact-head gates:
- APSIS core #123: PASS incl. ASAN/UBSAN
- staged validation #113: PASS through real JieLi target and emulator

Final target:
- image 71,608 B / 581,564 B
- RAM 35,272 B / 98,304 B
- pool 30,720 B / 344,064 B
- package SHA-256 `854f3fd315f1d489017cf1c9dfca2d8b59107281ba9deefd27d0067518fcf491`

Final emulator:
- queue 28/512
- overflow 0
- late flags 0
- cursor 123,648
- late trace `811c9dc5`
- WAV peak 0, mandatory because M3c is intentionally silent

## Phase 4 / M4a — persistent dry String core

Merged to `main` as `2ea217f09c143d30385f136e84e2b29d582987a0` via PR #7.
Final pre-merge documentation head: `afe81ae3984797bcb3b99153c96b493dfb8575ec`.

Status: **verified through real target + emulator and merged**.

Normative references:
- `docs/reference/STRING_TIMBRE_REFERENCE.md`
- `docs/reference/STRING_DSP_CONTRACT.md`
- `docs/reference/M4A_REFERENCE_AUDIO.md`

Verified implementation:
- 2 Study-98 Foundation voices
- 2 Study-98 Classic sine/Drift voices or Study-126 Dual BODY/WIRE
- fixed four persistent oscillator slots
- allocation-free fixed-point/Q15 renderer, no `libm`
- transport-gated sustain
- smooth pitch/gain and timbre swaps without phase reset/click
- M3c remains the only scheduler

Frozen host references:
- Classic peak 1477 Q15, hash `b20610c1`
- Dual L/R peak 823/787 Q15, hash `10d23cf4`

Final gates:
- APSIS core #141: PASS incl. ASAN/UBSAN
- staged validation #130: PASS through real JieLi target and emulator

Final target:
- image 75,700 B / 581,564 B
- RAM 35,464 B / 98,304 B
- pool 30,720 B / 344,064 B
- package SHA-256 `eb11271d26bb56a60d1883aae5eedb1c76300596d0803fceed3a69ae9a5aec0d`

Final emulator:
- channels 2
- frames 124,928
- peak 317 Q15
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- raw PCM SHA-256 `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

## Phase 4 / M4b — finite String transients + shared Rhythm audio

Branch: `m4b-string-transients`.
PR: #8.
Base: M4a merge `2ea217f09c143d30385f136e84e2b29d582987a0`.
Validated implementation/test head: `0749ac86e6162fab190f343730ebc9b133c13e47`.

Status: **implementation complete and verified through real target + emulator; merge pending final documentation-head gate**.

Normative reference:
- `docs/reference/M4B_STRING_TRANSIENT_CONTRACT.md`

### M4b1 engine verification

Verified source-derived finite transient behaviour:
- Study-98 normal harmonic stack 1..4
- Study-98 pizz harmonic stack 1..3
- fixed-point brightness-dependent attenuation
- sample-referenced 3 ms partial staggering
- current Study-126 16 ms String onset safety override
- triangle low-pass / sine bypass
- folded Gravity-root shadow
- source pan topology
- no broadband String noise
- click-safe 14 ms / 105 ms stealing behavior
- no allocation and no `libm`

Pool architecture:
- 80 logical voice ceiling
- 32 physical fade-tail reserve
- 112 total physical slots
- pool is separate from `ApsisStringDsp`; frozen M4a sustain struct remains 196 B

Host references, normal + ASAN/UBSAN:
- M4b1 transient peak 1309, hash `97142d3e`
- legal 30 ms burst: logical 80 / physical 112, steals 480, drops 0
- BODY peak 947, hash `c1168570`, L/R energy `4176243 / 2564882`
- Classic-composite stress: logical 80 / physical 108, steals 410, drops 0
- idle M4a hashes remain exactly Classic `b20610c1`, Dual `10d23cf4`

### M4b2 shared Rhythm verification

Verified integration:
- consumes existing `APSIS_EVENT_STRING_RHYTHM`; no new scheduler or String-private event queue
- event target remains Clock pulse +26 ms
- EVERY/SEQUENCE selection remains transport-owned
- planned BOW/PIZZ/MOTION mode is frozen into event `sub_index`
- pitch/surface/side levels/brightness and Motion Gravity harmony are late-read at audio bind
- Classic uses Study-126 composite BODY/WIRE topology
- Dual alternates left/BODY and right/WIRE by pulse parity with source BOW/PIZZ/MOTION level/motion rules
- passive observer lives outside `ApsisRuntime`, preserving runtime size 21,928 B
- canonical/extreme M3c queue, random and late trace references remain unchanged

Full-chain callback-boundary test:
- first Rhythm source target = sample 1147
- no event/audio through sample 1024
- callback 1024..1279 consumes target 1147 but does not render it early
- first legal audible callback begins sample 1280
- handoff is 133 samples late, less than one 256-frame callback
- test freezes BOW in event, then changes live Rhythm mode to OFF before consumption; planned BOW still binds while sound state is late-read
- queue overflow, late-slot overflow, scheduler starvation and transient drops remain 0

Exact implementation-head gates:
- APSIS core #170: PASS including normal + ASAN/UBSAN and `test_string_rhythm_live`
- staged validation #159: PASS through all five gates, real JieLi target + FM-1 emulator

Target #159:
- image 81,752 B / 581,564 B
- RAM 42,648 B / 98,304 B
- pool 30,720 B / 344,064 B
- image headroom 499,812 B
- RAM headroom 55,656 B
- pool headroom 313,344 B
- package bytes 610,110 B
- package SHA-256 `8abc090ffa51efe93b21b4f59f2a40a9ab70a6983a2c5720633baba26fa7163b`

Emulator #159:
- default Rhythm remains OFF, so M4b finite voices are idle in the smoke run
- M4a output remains bit-identical: raw PCM SHA-256 `10e361083954aee154eb8aa4c77868dacde090988cf4080b5b42727103c34f64`
- peak 317 Q15
- RMS L/R 101/100
- stereo delta 63
- mean L/R -2/-2
- M3c queue 28/512, overflow 0, late flags 0, cursor 123,648, late trace `811c9dc5`

Interpretation:
- target build + default emulator prove the complete live M4b path is linked/bootable and regression-transparent while Rhythm is OFF
- audible finite/Rhythm behavior is proven by deterministic host + sanitizer integration tests
- this does not prove physical FM-1 sound, real ISR margin, or hardware controls
- sample-exact event-to-audio timing is not claimed; Phase 5 owns that architecture change

Before merge, gate the exact final documentation head with both workflows. Do not make a new checkbox-only commit after those gates merely to record the result; record final gate SHAs/runs in PR metadata and merge with an expected-head guard.

## Explicitly deferred after M4b

- String Attractor excitation and its late pitch/side re-read policy
- dynamic String bus filter modulation
- shared Space / Cathedral
- physical FM-1 install/audio/control validation
- real ISR worst-case timing

## Release-level validation not yet performed

- physical FM-1 install
- OCT- rescue
- return to official firmware
- USB/MIDI interaction on device
- all keys/buttons/encoders/LEDs/MASTER
- real ISR worst-case timing
- five-source + 4x Spiral + Bloom + String Motion + Cathedral stress with real DSP
- persistent KEEP/CHAIN reboot/recovery
- power interruption during flash write

Do not call any of these verified until actual hardware evidence exists.
