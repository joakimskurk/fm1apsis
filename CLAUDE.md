# Working on APSIS for the FM-1

- Read PROJECT_STATE.md first (where we are, next action), then ROADMAP.md and TESTING.md. GitHub is the truth.
- The browser APSIS (Study 126, `/Users/jhf/Documents/Apsis/index.html`) is the authority for sound, behaviour and
  feel; docs/reference holds the contracts. APSIS stays APSIS, not a generic synth.
- The user has no physical FM-1. Builds are tested in Joakim Eriksson's fm1-emu. Hand every build meant for testing
  over as the `.fwsc` file itself, with its commit and SHA-256 (`make fw` locally is byte-identical to CI's).
- After a change: `make test`; before a push also `make linux` (gcc, MSan, the pinned sound). A sound change is
  deliberate: `make sound-update` and say why in the commit.
- The audio interrupt (`render_block`, app.c): no flash, no screen, no unbounded loops, no 64-bit division; never two
  RNG draws in one argument list (C leaves the order open). It must fit: CI's emulator job measures it (FM1_STRICT).
- Dinghy's files (`firmware/hal`, `loader`, platform `src/`, `tools/`) stay as upstream; PLATFORM.md lists the few
  differences. Change them only when the task needs it, and say so in the commit.
- Work on a branch, small commits. Ask before merging to main or tagging a release.
