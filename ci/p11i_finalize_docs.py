#!/usr/bin/env python3
from pathlib import Path

roadmap = Path("ROADMAP.md")
s = roadmap.read_text()
old_status = "Status: **[~] active on `p11-optimization`. P11A–P11H are complete; P11I is the next evidence-backed optimization candidate.**"
new_status = "Status: **[~] active on `p11-optimization`. P11A–P11I are complete; any further optimization requires a new measured/equivalence-backed candidate.**"
if old_status not in s:
    raise SystemExit("ROADMAP status anchor mismatch")
s = s.replace(old_status, new_status, 1)

old_p11i = """### P11I — stereo-pair Cathedral delay-read optimization

Status: **[ ] next. Equivalence proof first; production patch only after the proof passes.**

- [ ] build current-single-read vs candidate stereo-pair-read equivalence harness
- [ ] cover invalid/empty/partial/full fill, ring wrap, integer/fractional delays, both channels and deterministic randomized storage
- [ ] run normal + ASAN/UBSAN equivalence
- [ ] if and only if equivalent, replace six stereo tap locations per room frame from 12 duplicated single-channel address calculations to 6 shared pair calculations
- [ ] run existing `test_space_room`, Space DSP and full historical regressions
- [ ] compare real JieLi target disassembly/code shape against P11G/P11H baseline
- [ ] require exact idle PCM/runtime and exact active Motion lifecycle
- [ ] freeze new target image/RAM/POOL/package values before promotion
"""
new_p11i = """### P11I — stereo-pair Cathedral delay-read optimization

Status: **[x] complete, exact-head validated and frozen.**

- [x] pre-patch equivalence proof: **150,412** cases, optimized + ASAN/UBSAN
- [x] complete historical host/sanitizer behavior green before and after patch
- [x] six stereo locations/roomframe now share address/fraction calculation instead of 12 duplicated single-channel calculations
- [x] production pair helper remains bit-identical to frozen single-read authority
- [x] target room code shape **2,270 → 2,260 B** and **718 → 707** static instruction lines
- [x] production image **140,408 → 140,392 B**; RAM **87,476 B** unchanged; POOL **319,176 B** unchanged
- [x] exact idle PCM/runtime retained
- [x] exact active Motion lifecycle retained; active image **137,672 → 137,656 B**
- [x] authoritative validation run `41344186184` on technical head `ab6960ad3acf78dbc9f366d2a5e9358b42011cca`
- [x] evidence artifact ID `13097594559`; ZIP SHA-256 `2a9b2d6f2adc8a5fd4a4f39dbe88cbd9b7c6bd2122b80ce83178a07a5a94e716`
- [x] historical P11I apply/validation workflows manual-only, read-only/pinned as appropriate

Frozen P11I production target:
- image **140,392 / 581,564 B**
- RAM **87,476 / 98,304 B**; headroom **10,828 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`

P11I proves a smaller target code-shape proxy, **not** physical FM-1 CPU cycles or ISR margin.
"""
if old_p11i not in s:
    raise SystemExit("ROADMAP P11I anchor mismatch")
s = s.replace(old_p11i, new_p11i, 1)

old_next = """## Exact next action

Create **one** P11I staging branch from the frozen P11H head. First prove the stereo-pair delay reader bit-identical to the existing single-channel reader over exhaustive boundary classes and deterministic randomized cases. Do not modify production `apsis_space_room.c` until that equivalence gate is green."""
new_next = """## Exact next action

Promote the frozen P11I state to `p11-optimization`, then return to measurement before selecting P11J. Do not claim physical CPU/ISR improvement from the P11I code-shape proxy; the next production optimization requires a new concrete hotspot and a falsifiable equivalence/regression gate."""
if old_next not in s:
    raise SystemExit("ROADMAP exact-next anchor mismatch")
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path("PROJECT_STATE.md")
s = state.read_text()
old_milestone = "**Phase 11 — FM-1 optimisation is active. P11A–P11H are complete. P11I is next: prove a stereo-pair Cathedral delay reader bit-identical to the existing single-channel reader, then and only then test it as a production optimization.**"
new_milestone = "**Phase 11 — FM-1 optimisation is active. P11A–P11I are complete. P11I is exact-head validated and frozen; further production optimization requires a new measured/equivalence-backed candidate.**"
if old_milestone not in s:
    raise SystemExit("PROJECT_STATE milestone anchor mismatch")
s = s.replace(old_milestone, new_milestone, 1)

old_baseline = """## Current frozen production baseline — P11F onward

P11G and P11H are measurement-only, so the accepted production binary remains P11F exactly:

- product `FM-1_972`
- image **140,408 / 581,564 B**
- ordinary RAM **87,476 / 98,304 B**
- RAM headroom **10,828 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `e0d0b826f0c2d5b8f1af781a9259b5d78dce9062939661ac7acad17bfefb78e5`
"""
new_baseline = """## Current frozen production baseline — P11I

P11G and P11H were measurement-only. P11I is the current accepted production target after the equivalence-backed stereo-pair delay-read optimization:

- product `FM-1_972`
- image **140,392 / 581,564 B**
- ordinary RAM **87,476 / 98,304 B**
- RAM headroom **10,828 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`
"""
if old_baseline not in s:
    raise SystemExit("PROJECT_STATE baseline anchor mismatch")
s = s.replace(old_baseline, new_baseline, 1)

marker = "## Exact next phase — P11I stereo-pair delay read\n"
end = "\n## Do not regress\n"
if marker not in s or end not in s:
    raise SystemExit("PROJECT_STATE P11I section anchors missing")
before, rest = s.split(marker, 1)
_, after = rest.split(end, 1)
p11i = """## P11I — stereo-pair Cathedral delay read complete

Pre-patch authority:
- proof run `41343196262`
- proof head `32bb42c52f666af12065737e14a8846d3d8f01ab`
- **150,412** bit-identical cases, optimized + ASAN/UBSAN
- complete historical host suite green
- production room source still frozen at pre-patch blob `2d7c34e32b48b904623d740a70cf657fee0801a7`

Production/validation authority:
- production pair-read commit `4629406bb6e417b0bcb544e6a770c0860613e7cd`
- exact technical head `ab6960ad3acf78dbc9f366d2a5e9358b42011cca`
- authoritative run `41344186184`, all five gates PASS
- artifact ID `13097594559`
- artifact ZIP SHA-256 `2a9b2d6f2adc8a5fd4a4f39dbe88cbd9b7c6bd2122b80ce83178a07a5a94e716`

Measured result:
- six stereo delay locations/roomframe share validity/wrap/index/fraction work instead of twelve duplicate single-channel address calculations
- target room code shape **2,270 → 2,260 B** and **718 → 707** static disassembly instruction lines
- production image **140,408 → 140,392 B**
- ordinary RAM remains **87,476 B**
- POOL remains **319,176 B**
- production package SHA-256 `868e1dd01fbe0eecb4c13c09dd46854491f3795dd268c9dedb3e16c519313a2c`
- active-Motion image **137,672 → 137,656 B** with RAM **87,524 B** and POOL **319,176 B** unchanged
- active package SHA-256 `86c44161d61b17697bebb2af7c3222449a0de1d20b74acc3cbaf2a8635cb7a75`

Behavior remains exact:
- idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`
- active Motion lifecycle remains seen `ff`, final `14`, 149/149 points, cursor 481,024, queue 30/512, overflow 0, late flags 0

P11I proves a reproducible target code-shape reduction, not physical FM-1 cycle/ISR improvement. The historical apply/validation workflows are frozen manual-only and pinned to the proven technical head; the apply verifier is read-only.

## Exact next phase

Return to measurement before P11J. A new production optimization requires a concrete measured hotspot plus a falsifiable equivalence/regression gate. Physical ISR timing, stack high-water, LCD/audio contention and listening quality remain deferred to hardware validation.
"""
state.write_text(before + p11i + end + after)
print("PASS: P11I frozen state blocks updated")
