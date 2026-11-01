#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11O are complete; P11P is the next measurement-only phase for the remaining high-frequency FDN LFO/sine path.**'
new_status = 'Status: **[~] active on `p11-optimization`. P11A–P11P are complete; P11Q is authorized as a proof-only phase for the measured FDN sine-endpoint cache candidate.**'
if old_status not in s: raise SystemExit('ROADMAP status anchor mismatch')
s = s.replace(old_status, new_status, 1)
old = '''### P11P — FDN LFO/sine workload census

Status: **[ ] next. Measurement only; no production patch authorized.**

- [ ] quantify `space_sine_q15()` / quarter-wave lookup work on cold and fully warm FDN paths
- [ ] classify interpolation and quadrant/lookup behavior before selecting any candidate
- [ ] capture real-target static code shape for the sine/LFO path and surrounding inlined render
- [ ] separate measured repeated work from structural invariants
- [ ] authorize proof work only if a narrow exact candidate is justified

### Later Phase-11 work
'''
new = '''### P11P — FDN LFO/sine workload census

Status: **[x] complete, frozen measurement phase. Production Space remains exact P11O.**

- [x] cold: **131,072** sine calls / **262,144** endpoint evaluations; only **96** endpoint-pair refreshes in cache model
- [x] warm: **524,288** sine calls / **1,048,576** endpoint evaluations; only **367** endpoint-pair refreshes
- [x] combined: **655,360** calls; endpoint pair unchanged for **654,897 (99.929%)** calls; **1,309,794** point evaluations avoidable by the measured cache model
- [x] all **256** fractional values occur; interpolation therefore remains mandatory
- [x] exhaustive top-16 domain: **65,536** cases, all 256 indices + 256 fractions, **1,024** real quadrant-boundary crossings, **0** equal adjacent endpoint pairs
- [x] optimized + ASAN/UBSAN census PASS
- [x] complete historical host + sanitizer preparation PASS via frozen P11F adaptation
- [x] exact P11O target/package regenerated unchanged
- [x] target `apsis_space_fdn_render` **1,114 B / 374** static instruction lines
- [x] `space_sine_q15()` fully inlined; `space_sine_point()` remains a **48 B / 14-line** target helper
- [x] authoritative run `41359406941` on technical head `2afb819dd865385f7547811b6422e7a281c80b7e`
- [x] evidence artifact ID `13104249141`; ZIP SHA-256 `d1a5d5898d00150c0f757f11fa01683d705a131bb35387d614741c49fa3b3517`
- [x] historical census workflow manual-only and pinned to the proven technical head

Decision: authorize P11Q **proof only** for two exact signed-16-bit sine endpoints per FDN line, initialized from reset phase and refreshed only when the phase top byte changes. Preserve the current interpolation expression exactly. Source-level cache cost is 32 B before target ABI confirmation. Production remains P11O until proof and target projection pass.

### P11Q — FDN sine endpoint-cache equivalence proof

Status: **[ ] next. No production patch until proof and projection pass.**

- [ ] exhaustive 65,536-case top-16 sine equivalence
- [ ] all 256 top-byte transitions including `255 → 0` wrap and reset initialization
- [ ] complete-FDN output/state/storage equivalence across cold, warm, randomized trajectories and resets
- [ ] optimized + ASAN/UBSAN proof
- [ ] exact host and target state/layout cost
- [ ] ephemeral real-target image/RAM/POOL/package and inlined-render projection
- [ ] only after all gates pass, decide whether a separate production-validation phase is justified

### Later Phase-11 work
'''
if old not in s: raise SystemExit('ROADMAP P11P anchor mismatch')
s = s.replace(old, new, 1)
old_next = '''## Exact next action

Promote frozen P11O production state to `p11-optimization`, then create one P11P measurement branch. Measure the remaining high-frequency FDN LFO/sine path without changing production DSP; only a narrow evidence-backed candidate may advance to a later proof phase.'''
new_next = '''## Exact next action

Promote frozen P11P measurement state to `p11-optimization`, then create one P11Q proof branch. Prove the per-line FDN sine-endpoint cache against frozen P11O authority and project it ephemerally on the real JieLi target; do not modify production Space until all equivalence and target-cost gates pass.'''
if old_next not in s: raise SystemExit('ROADMAP next-action anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11O are complete. P11O is the current accepted production baseline after the invariant-simplified FDN line reader; P11P is the next measurement-only phase.**'
new_m = '**Phase 11 — FM-1 optimisation is active. P11A–P11P are complete. P11O remains the accepted production baseline; P11P measured the remaining FDN LFO/sine endpoint redundancy and authorizes P11Q proof-only.**'
if old_m not in s: raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_m, new_m, 1)
old_next = '''## Exact next phase — P11P FDN LFO/sine workload census

Measure the remaining high-frequency FDN LFO/sine path without changing production DSP. Quantify sine lookup/interpolation behavior and target code shape, then authorize a later proof phase only if the evidence identifies a narrow exact candidate.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
new_next = '''## P11P — FDN LFO/sine workload census complete

Measurement authority:
- technical head `2afb819dd865385f7547811b6422e7a281c80b7e`
- run `41359406941` — PASS
- artifact ID `13104249141`
- artifact ZIP SHA-256 `d1a5d5898d00150c0f757f11fa01683d705a131bb35387d614741c49fa3b3517`
- production `apsis_space_dsp.c/.h` and Cathedral room source remained exact P11O
- historical workflow manual-only and pinned to the proven technical head

Natural LFO trajectory:
- cold 16,384 frames: **131,072** sine calls / **262,144** point evaluations
- cold cache-model endpoint refreshes **96**; reuses **130,976**
- warm continuing 65,536 frames: **524,288** calls / **1,048,576** point evaluations
- warm refreshes **367**; reuses **523,921**
- combined **655,360** calls, **463** refreshes, **654,897** reuses
- combined avoidable point evaluations under the cache model: **1,309,794**
- endpoint pair unchanged for **99.929%** of natural calls

Exact-domain constraints:
- all 256 fractional values occur naturally
- exhaustive top-16 sweep covers all **65,536** index/fraction combinations
- `frac == 0`: **256**; `frac != 0`: **65,280**
- real endpoint-boundary crossings: **1,024**
- equal adjacent endpoint pairs: **0**
- therefore interpolation, quadrant and wrap semantics must remain exact

Frozen target authority:
- exact P11O image **140,440 B**, RAM **87,492 B**, POOL **319,176 B**
- package SHA-256 `0a3c1ecc8adf99a625979e1a5369c3cad1f4a87264a227a7feefba3388da98a2`
- inlined `apsis_space_fdn_render` **1,114 B / 374** static instruction lines
- `space_sine_q15()` fully inlined on target
- `space_sine_point()` survives as **48 B / 14** static instruction lines
- complete historical host + sanitizer preparation also PASS in the target gate

Decision: authorize P11Q proof-only for a per-line pair of exact signed-16-bit sine endpoints. Initialize them from each reset phase; refresh them only on an 8-bit phase-index transition; preserve the existing interpolation expression exactly. The source representation is 4 B/line (**32 B total**) before target ABI confirmation. Production remains P11O.

## Exact next phase — P11Q FDN sine endpoint-cache equivalence proof

Before any production source modification:
1. prove exhaustive top-16 helper equivalence and every top-byte transition including `255 → 0`;
2. prove reset initialization and complete-FDN output/state/storage equivalence across cold, warm, randomized trajectories and resets;
3. run optimized + ASAN/UBSAN;
4. measure exact host/target state-size cost;
5. project the candidate ephemerally on the real JieLi target for image/RAM/POOL/package and inlined render code shape;
6. authorize a separate production-validation phase only if every gate passes and the target tradeoff remains justified.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
if old_next not in s: raise SystemExit('PROJECT_STATE P11P anchor mismatch')
state.write_text(s.replace(old_next, new_next, 1))
print('PASS: P11P roadmap/project state updated')
