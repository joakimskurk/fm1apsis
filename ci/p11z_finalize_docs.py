#!/usr/bin/env python3
from pathlib import Path

roadmap = Path('ROADMAP.md')
s = roadmap.read_text()
old_status = 'Status: **[~] active on `p11-optimization`. P11A–P11Y are complete. P11W remains the accepted production baseline; P11Y is a proved-but-rejected target regression. P11Z is the final pre-hardware Phase-11 closure/readiness matrix.**'
new_status = 'Status: **[x] software/target/emulator closure complete on `p11-optimization`. P11W remains the accepted production baseline. P11Z passed on the frozen closure head; guarded Phase-11 merge to `main` is next, followed by Phase 12 physical-hardware validation.**'
if old_status not in s:
    raise SystemExit('ROADMAP Phase-11 status anchor mismatch')
s = s.replace(old_status, new_status, 1)

old_p11z = '''### P11Z — Phase-11 closure/readiness matrix

Status: **[ ] next. Measurement/regression only; no new production optimization.**

- [ ] regenerate exact P11W production target/package and current ownership/resource inventory
- [ ] rerun permanent high-water/stress authorities for scheduler, Bloom, Spiral, String transient/Attractor and Motion
- [ ] run complete historical host + sanitizer matrix on the accepted P11W source
- [ ] run exact idle pinned-emulator PCM/runtime gate
- [ ] run exact active Motion lifecycle gate
- [ ] capture final pre-hardware image/RAM/POOL/package hashes and explicit unmeasured hardware boundary
- [ ] freeze one Phase-11 closure head and hand Phase 12 only the genuinely physical checks

P11Z is intended to close Phase 11, not discover another speculative micro-optimization. New optimization work after P11Z requires new evidence or physical timing data.
'''
new_p11z = '''### P11Z — Phase-11 closure/readiness matrix

Status: **[x] complete, exact software/real-target/pinned-emulator closure frozen. No production optimization added.**

- [x] exact accepted P11W production source authority regenerated
- [x] complete historical host + sanitizer behavior PASS
- [x] bounded-capacity/high-water authorities revalidated: queue 415/512, Spiral 59/64, Bloom 44/48 with proven legal max 44, String transient 80/80 logical + 112/112 physical, String Attractor pending 60/64, Clock 23/32, Spiral legs 30/32, Bloom petals 125/136
- [x] exact P11W real JieLi target regenerated: image **140,472 B**, RAM **87,524 B**, POOL **319,176 B**
- [x] production package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- [x] exact idle PCM/runtime retained: PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`, cursor 123,392, queue 28/512, overflow 0, late 0
- [x] exact active Motion target/lifecycle retained: 149/149 points, record 220,672→441,088, cycle 220,416, override 450,560, resume 466,176, cursor 481,024, queue 30/512, overflow 0, late 0
- [x] closure branch contains no production mutation; P11W remains production authority
- [x] authoritative closure run `41399427006` on technical head `fb1789477ada900d54ed319c9ee25645c6d3d055`
- [x] evidence artifact ID `13120614128`; ZIP SHA-256 `de7eb00947513f9d527fb116a20b06defa6bf677f44d3e0dff8fb08f13f39668`
- [x] physical ISR/callback margin, runtime stack high-water, LCD/audio contention, real NOR timing/power interruption, DAC/listening quality and install/rescue remain explicitly deferred to Phase 12

P11Z closes software/target/emulator Phase 11. New optimization work after this checkpoint requires new measured evidence or physical timing data; it is not part of the closure path.
'''
if old_p11z not in s:
    raise SystemExit('ROADMAP P11Z block anchor mismatch')
s = s.replace(old_p11z, new_p11z, 1)

old_next = '''## Exact next action

Promote frozen P11Y proof/rejection state to `p11-optimization`, then create one P11Z closure branch. Do not change production DSP. Run the final Phase-11 pre-hardware readiness matrix against accepted P11W and freeze a single closure head that cleanly separates verified software/target/emulator evidence from Phase-12 physical-hardware work.'''
new_next = '''## Exact next action

Freeze the documented P11Z closure head, rerun the closure matrix on that exact documentation head, then open the guarded Phase-11 PR from `p11-optimization` to `main`. Merge only after the required exact-head workflows are green. After merge, Phase 12 begins with physical FM-1 install/rescue, real ISR/stack/LCD/NOR stress and DAC/listening validation; no further speculative optimization is part of the pre-hardware closure.'''
if old_next not in s:
    raise SystemExit('ROADMAP next-action anchor mismatch')
roadmap.write_text(s.replace(old_next, new_next, 1))

state = Path('PROJECT_STATE.md')
s = state.read_text()
old_milestone = '**Phase 11 — FM-1 optimisation is active. P11A–P11Y are complete. P11W remains the accepted production baseline. P11Y proved fixed add/sub output mixing exact but rejected it on real-target regression; P11Z is the final pre-hardware closure/readiness matrix.**'
new_milestone = '**Phase 11 — software/real-target/pinned-emulator closure is complete on `p11-optimization`. P11W is the final accepted production baseline and P11Z passed the final pre-hardware readiness matrix. Guarded merge to `main` is next; Phase 12 remains physical-FM-1 validation only.**'
if old_milestone not in s:
    raise SystemExit('PROJECT_STATE milestone anchor mismatch')
s = s.replace(old_milestone, new_milestone, 1)

old_tail = '''## Exact next phase — P11Z Phase-11 closure/readiness matrix

No production optimization is authorized. Regenerate and stress the accepted P11W stack as one final software/real-target/pinned-emulator checkpoint: complete host/sanitizer behavior, bounded-capacity/high-water authorities, exact production target/package, exact idle PCM/runtime and exact active Motion lifecycle. Freeze one closure head and move all remaining physical ISR/stack/LCD/NOR/DAC/listening checks to Phase 12.
'''
new_tail = '''## P11Z — Phase-11 closure/readiness matrix complete

Authority:
- technical closure head `fb1789477ada900d54ed319c9ee25645c6d3d055`
- run `41399427006` — all closure gates PASS
- evidence artifact ID `13120614128`
- evidence ZIP SHA-256 `de7eb00947513f9d527fb116a20b06defa6bf677f44d3e0dff8fb08f13f39668`
- closure adds no production optimization; accepted production remains exact P11W

Final accepted production target:
- product `FM-1_972`
- image **140,472 / 581,564 B**
- RAM **87,524 / 98,304 B**; headroom **10,780 B**
- POOL **319,176 / 344,064 B**; headroom **24,888 B**
- package bytes **610,110 B**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined FDN render **1,094 B / 364** static instruction lines

Final bounded-capacity closure:
- shared queue **415/512** observed high-water
- Spiral late slots **59/64**
- Bloom late slots **44/48**, exact legal maximum **44**
- String transient **80/80 logical, 112/112 physical**, steals 480, drops 0
- String Attractor pending **60/64**
- Clock pool **23/32**
- Spiral legs **30/32**
- Bloom petals **125/136**

Final emulator closure:
- idle PCM `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- idle cursor 123,392; queue 28/512; overflow 0; late flags 0; trace `811c9dc5`
- active Motion target image/RAM/POOL **137,736 / 87,572 / 319,176 B**
- active Motion seen `ff`, final `14`, points 149/149
- record 220,672→441,088; cycle 220,416 samples
- manual override 450,560; resume 466,176; cursor 481,024
- active queue 30/512; overflow 0; late flags 0; WAV peak 1,171; RMS 173

Phase-12-only unverified boundary:
- physical `app_audio()` ISR/callback cycle margin under worst-case combined load
- physical runtime stack high-water and guard margin
- LCD DMA/audio bus contention and physical control/LED/display behavior
- real NOR erase/program timing and power-interruption behavior
- DAC/listening quality and Cathedral perceptual tuning
- real FM-1 install/rescue/update path

No physical-hardware claim is made by P11Z.

## Exact next phase — guarded Phase-11 merge, then Phase 12 hardware validation

Rerun P11Z on the exact documentation head, open the guarded `p11-optimization` → `main` PR and merge only when the required exact-head workflows are green. After merge, do not reopen speculative Phase-11 micro-optimization without new measured evidence. Phase 12 is the physical FM-1 install/rescue, ISR/stack/LCD/NOR stress and DAC/listening phase.
'''
if old_tail not in s:
    raise SystemExit('PROJECT_STATE P11Z tail anchor mismatch')
state.write_text(s.replace(old_tail, new_tail, 1))

checkpoint = Path('docs/PHASE11_FINAL_CHECKPOINT.md')
if checkpoint.exists():
    raise SystemExit('checkpoint already exists')
checkpoint.write_text('''# Phase 11 final checkpoint — P11Z\n\nDate: 2026-11-02\n\nStatus: **software / real JieLi target / pinned-emulator closure PASS.** Physical FM-1 validation remains Phase 12.\n\n## Authority\n\n- Phase-10 merge baseline: `378d2023df2afe59475be53c09cc001788586514`\n- accepted Phase-11 production authority: **P11W**\n- P11Z technical closure head: `fb1789477ada900d54ed319c9ee25645c6d3d055`\n- P11Z run: `41399427006`\n- evidence artifact ID: `13120614128`\n- evidence ZIP SHA-256: `de7eb00947513f9d527fb116a20b06defa6bf677f44d3e0dff8fb08f13f39668`\n\nP11Z is closure-only. It introduces no production DSP/state/runtime optimization.\n\n## Final accepted production target\n\n- product `FM-1_972`\n- image **140,472 / 581,564 B**\n- ordinary RAM **87,524 / 98,304 B** — **10,780 B headroom**\n- POOL **319,176 / 344,064 B** — **24,888 B headroom**\n- package bytes **610,110**\n- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`\n- inlined FDN render **1,094 B / 364** static instruction lines\n\nRelative to the Phase-10 merge baseline, final Phase 11 uses **9,564 B less POOL** and **2,160 B less ordinary RAM**. Image size is **320 B larger**; Phase 11 was evidence-led memory/workload optimization, not a blanket size-minimization pass.\n\n## Accepted production changes during Phase 11\n\n- lossless compact String Motion store: **19,208 → 9,644 B**\n- Bloom late-slot capacity **64 → 48** after exact legal bound 44 was proved\n- Cathedral stereo-pair delay reads share address/fraction work\n- FDN feedback gains cache on rounded decay changes\n- FDN line-read specialization removes proved-unreachable/redundant work while preserving startup/ring/interpolation behavior\n- FDN sine endpoint cache stores exact signed-16 endpoint pairs\n- endpoint representation changed from A+B to exact A+delta with zero state growth\n\nRejected candidates remain rejected: selective 32-bit Q15 narrowing regressed target shape (P11T), and fixed add/sub output mixing regressed target image/render shape (P11Y).\n\n## Bounded-capacity closure\n\n- shared event queue high-water **415/512**\n- Spiral late slots **59/64**\n- Bloom late slots **44/48**, exact legal maximum **44**\n- String transient **80/80 logical, 112/112 physical**, 480 steals, 0 drops\n- String Attractor pending **60/64**\n- Clock DSP pool **23/32**\n- Spiral DSP legs **30/32**\n- Bloom DSP petals **125/136**\n\nNo additional pool/capacity shrink is authorized from these observations.\n\n## Frozen emulator closure\n\nIdle:\n- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`\n- cursor **123,392**\n- queue **28/512**\n- overflow **0**\n- late flags **0**\n- late trace `811c9dc5`\n\nActive Motion:\n- target image/RAM/POOL **137,736 / 87,572 / 319,176 B**\n- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`\n- seen `ff`, final `14`\n- points **149 / 149**\n- record start **220,672**, finish **441,088**, cycle **220,416** samples\n- manual override **450,560**, resume **466,176**\n- cursor **481,024**\n- queue **30/512**, overflow **0**, late flags **0**\n- WAV frames **521,984**, peak **1,171**, RMS **173**\n\n## Phase 12 boundary — physical FM-1 only\n\nP11Z does **not** claim any of the following:\n\n- physical `app_audio()` ISR/callback cycle margin under worst-case combined load\n- physical runtime stack high-water and guard margin\n- LCD DMA/audio bus contention or physical control/LED/display behavior\n- real NOR erase/program timing or power-interruption behavior\n- DAC/listening quality or Cathedral perceptual tuning\n- real FM-1 install/rescue/update behavior\n\nThese are the remaining release-readiness tasks. New optimization after this checkpoint requires new measured evidence or physical timing data.\n''')

print('PASS: Phase 11 closure documentation prepared')
