#!/usr/bin/env python3
from pathlib import Path


def replace_one(path, old, new):
    p = Path(path)
    s = p.read_text()
    n = s.count(old)
    if n != 1:
        raise SystemExit(f'FAIL: {path}: anchor count {n} != 1 for {old[:100]!r}')
    p.write_text(s.replace(old, new, 1))

replace_one(
    'ROADMAP.md',
    'Status: **[~] active on `p11-optimization`. P11A–P11R are complete; P11R is the accepted production baseline and P11S is the next measurement-only phase.**',
    'Status: **[~] active on `p11-optimization`. P11A–P11S are complete; P11R remains the accepted production baseline and P11T is authorized as proof-only for the structurally safe Q15 multiply sites.**')

old = '''### P11S — FDN Q15 multiply operand-range census

Status: **[ ] next. Measurement only; no production patch authorized.**

- [ ] instrument the unchanged P11R FDN path only in tests and classify every `space_mul_q15()` call-site family
- [ ] measure signed operand/product extrema over cold, fully warm, legal parameter sweeps and deterministic randomized/reset trajectories
- [ ] include structural/adversarial boundary cases so observed high-water alone cannot justify a narrower multiply
- [ ] determine exactly which call-site families, if any, provably fit signed 32-bit product plus the existing ±16,384 rounding term
- [ ] preserve the current rounding/right-shift semantics as the authority; no arithmetic rewrite in P11S
- [ ] capture current real-target FDN code shape/package unchanged from P11R
- [ ] authorize a later proof phase only for a call-site set backed by both measured and structural bounds

P11H measured **40 FDN Q15 multiplies per room frame / 655,360 over 16,384 room frames**. P11S asks whether the current 64-bit helper is necessary at each of those call-site families; it does not assume that a 32-bit replacement is safe or faster.
'''
new = '''### P11S — FDN Q15 multiply operand-range census

Status: **[x] complete, frozen measurement phase. Production Space remains exact P11R.**

- [x] all **40 `space_mul_q15()` calls / room frame** classified into six dynamic families
- [x] alpha **10/frame**, output **2/frame**, cross **4/frame**, Hadamard normalization **8/frame**, excitation **8/frame**, feedback **8/frame**
- [x] cold **16,384 frames / 655,360 calls** and warm **65,536 frames / 2,621,440 calls** measured
- [x] legal damping/decay sweep **13,536 frames / 541,440 calls** measured
- [x] deterministic randomized/reset trajectory **4,096 blocks / 197,319 frames / 7,892,760 calls** measured
- [x] optimized + ASAN/UBSAN census PASS
- [x] structural bounds include the frozen ±16,384 rounding term rather than relying on observed high-water
- [x] structurally signed-32-safe: alpha, output, cross, excitation = **24/40 calls per frame**
- [x] structurally unsafe for signed-32: Hadamard normalization + feedback = **16/40 calls per frame**
- [x] safe structural max products: alpha **1,003,391,074**, output **859,019,672**, cross **1,900,354,932**, excitation **425,184,592**
- [x] rejected structural max products: Hadamard normalization **3,036,845,560**, feedback **2,992,665,298**
- [x] exact P11R target regenerated unchanged: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, render **1,098 B / 366** static instruction lines
- [x] production package SHA-256 remains `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`
- [x] authoritative run `41383031271` on technical head `6ddfa625b371d9a93db3d0a5a635b48b7eb08046`
- [x] evidence artifact ID `13114240999`; ZIP SHA-256 `2365e3fbcc4578479d438f42085562e77d16417dd9a8443bc419ba194c96bddc`
- [x] committed production Space blobs remained exact P11R throughout
- [x] historical census workflow manual-only and pinned to the proven technical head

Decision: authorize P11T **proof only** for a selective 32-bit exact Q15 helper at alpha/output/cross/excitation sites only. Hadamard normalization and feedback must stay on the wide helper. Preserve exact rounding/right-shift semantics; production remains P11R until proof, historical behavior and ephemeral target projection all pass.
'''
replace_one('ROADMAP.md', old, new)

replace_one(
    'ROADMAP.md',
    'Promote frozen P11R production state to `p11-optimization`, then create one P11S measurement branch. Measure the remaining high-frequency FDN Q15 multiply operand/product ranges without changing production DSP; only structurally bounded call-site families may advance to a later proof phase.',
    'Promote frozen P11S measurement state to `p11-optimization`, then create one P11T proof branch. Prove a selective signed-32 Q15 helper only for alpha/output/cross/excitation (24/40 calls per FDN room frame), keep Hadamard normalization + feedback on the frozen wide helper, and use an ephemeral real-target projection before any production authorization.')

replace_one(
    'PROJECT_STATE.md',
    '**Phase 11 — FM-1 optimisation is active. P11A–P11R are complete. P11R is the accepted production baseline after the measured, proved and production-validated FDN sine-endpoint cache; P11S is the next measurement-only phase.**',
    '**Phase 11 — FM-1 optimisation is active. P11A–P11S are complete. P11R remains the accepted production baseline; P11S structurally classified the FDN Q15 multiply sites and authorizes P11T proof-only for the 24/40 signed-32-safe calls.**')

old = '''## Exact next phase — P11S FDN Q15 multiply operand-range census

Return to measurement. P11H measured 40 FDN `space_mul_q15()` evaluations per room frame; the unchanged helper currently forms a signed 64-bit product. P11S must classify every call-site family, measure and structurally bound operand/product ranges, and preserve exact rounding semantics. Production remains P11R. Only a call-site set proven safe for a narrower exact representation may advance to a later proof phase.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
new = '''## P11S — FDN Q15 multiply operand-range census complete

Measurement authority:
- technical head `6ddfa625b371d9a93db3d0a5a635b48b7eb08046`
- run `41383031271` — PASS
- evidence artifact ID `13114240999`
- evidence ZIP SHA-256 `2365e3fbcc4578479d438f42085562e77d16417dd9a8443bc419ba194c96bddc`
- optimized + ASAN/UBSAN PASS
- exact P11R real-target package regenerated unchanged
- committed production Space remained byte-for-byte P11R
- historical census workflow manual-only and pinned to the proven technical head

Dynamic workload classification:
- alpha: **10 calls/frame**
- output scaling: **2 calls/frame**
- stereo cross/self mix: **4 calls/frame**
- Hadamard normalization: **8 calls/frame**
- excitation: **8 calls/frame**
- feedback gain: **8 calls/frame**
- total: **40 calls/frame**

Structural width result, including the frozen ±16,384 rounding term:
- alpha: max absolute product **1,003,391,074** — signed-32 safe
- output: **859,019,672** — signed-32 safe
- cross: **1,900,354,932** — signed-32 safe
- excitation: **425,184,592** — signed-32 safe
- Hadamard normalization: **3,036,845,560** — signed-32 unsafe
- feedback: **2,992,665,298** — signed-32 unsafe

Therefore **24/40 dynamic calls per FDN room frame** are structurally eligible for a narrow-helper proof and **16/40 must remain wide**. The natural trajectories happened to keep all observed products inside signed 32-bit, but P11S explicitly rejects using that high-water fact to narrow Hadamard or feedback because their legal structural bounds exceed signed 32-bit.

Frozen production authority remains P11R: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**, package `bc1ff9cc52a5969a70bfeef3ae90b199b8aae6dfe25e54c771a3af46adbdc2f2`, FDN render **1,098 B / 366** static instruction lines.

## Exact next phase — P11T selective Q15 narrow-helper proof

Production remains P11R. P11T may introduce a candidate only in proof/ephemeral target workspaces. The candidate may use signed-32 product arithmetic exclusively at alpha/output/cross/excitation call sites and must leave Hadamard normalization + feedback on the frozen wide helper. Prove exact helper arithmetic at structural boundaries and randomized values, complete-FDN stereo/state/storage equivalence across cold/warm/legal/random/reset trajectories, optimized + ASAN/UBSAN, full historical behavior, and exact real-target resource/code-shape delta before considering production validation.

Physical FM-1 CPU cycles, ISR margin, stack high-water, LCD/audio contention and listening quality remain unverified.
'''
replace_one('PROJECT_STATE.md', old, new)

print('PASS: P11S roadmap/state freeze prepared; P11T proof-only next')
