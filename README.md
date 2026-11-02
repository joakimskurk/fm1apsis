# APSIS for the M-VAVE FM-1

A native FM-1 firmware of **APSIS**, the browser instrument: one shared musical state seen through STRING,
DRIFT/TUNING, CLOCK, SPIRAL, BLOOM, GRAVITY, CHAIN, KEEP and MOTHER, five sounding sources into one shared
Space/Cathedral. It is APSIS on FM-1 hardware, not a generic synth with APSIS names.

Built on [Dinghy](https://github.com/zvenson/dinghy) 0.4 (GPL-3.0): the platform (updates, USB rescue, MIDI,
panel, screen, flash) is Dinghy's, kept close to upstream; APSIS is `apsis/` plus one app file.

## Start here

1. `PROJECT_STATE.md`: where the project is and the next action
2. `ROADMAP.md`: what comes after that
3. `TESTING.md`: what is verified, and how
4. `DECISIONS.md`: decisions that would be expensive to rediscover
5. `docs/ARCHITECTURE.md`: how the firmware is put together
6. `docs/reference/`: the behaviour map and the frozen musical contracts; `docs/history/`: the old phase records

GitHub is the source of truth; a new contributor (or chat) continues from the repository alone.

## Layout

```text
apsis/              APSIS: state, runtime/scheduler, source DSP, room, controls, visuals, project journal
firmware/src/app.c  the app: the audio interrupt, the panel, the surfaces, saving (on Dinghy's platform.c)
firmware/           Dinghy 0.4 (hal/, loader/, src/ platform files); see PLATFORM.md for what differs
tools/ build.sh     Dinghy's build (JieLi toolchain; Docker on macOS) -> build/apsis.fwsc
tests/apsis/        APSIS's host tests            tests/sound.sh   the whole firmware in host mode, sound pinned
tests/emu.sh        fm1-emu under FM1_STRICT: the audio interrupt's load      tests/run_tests.sh   Dinghy's tests
```

## Build and test

```sh
make test        # APSIS's tests, clang, plain + ASan/UBSan (seconds, native on the Mac)
make linux       # gcc, clang MSan and the firmware's pinned sound in host mode (Docker)
make fw          # build/apsis.fwsc: JieLi toolchain in ~/.jieli (tools/get_toolchain.sh), AC79 SDK in ~/fw-AC79_AIoT_SDK
```

CI (`.github/workflows/ci.yml`) runs all of it, builds the package and measures it on the emulator; a tag `vX.Y`
publishes `apsis-X.Y.fwsc` with SHA-256 and a build attestation.

## Verification boundary

No physical FM-1 has run this firmware yet. Host tests, real JieLi builds and Joakim Eriksson's emulator
(including its FM1_STRICT cycle model) are evidence, not hardware verification: real DAC sound, real interrupt
margin, physical controls and real power-cut behaviour remain unverified until an FM-1 runs it.
