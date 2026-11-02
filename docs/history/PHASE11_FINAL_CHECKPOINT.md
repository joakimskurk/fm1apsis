# Phase 11 final checkpoint — P11Z

Date: 2026-11-02

Status: **software / real JieLi target / pinned-emulator closure PASS.** Physical FM-1 validation remains Phase 12.

## Authority

- Phase-10 merge baseline: `378d2023df2afe59475be53c09cc001788586514`
- accepted Phase-11 production authority: **P11W**
- P11Z technical closure head: `fb1789477ada900d54ed319c9ee25645c6d3d055`
- P11Z run: `41399427006`
- evidence artifact ID: `13120614128`
- evidence ZIP SHA-256: `de7eb00947513f9d527fb116a20b06defa6bf677f44d3e0dff8fb08f13f39668`

P11Z is closure-only. It introduces no production DSP/state/runtime optimization.

## Final accepted production target

- product `FM-1_972`
- image **140,472 / 581,564 B**
- ordinary RAM **87,524 / 98,304 B** — **10,780 B headroom**
- POOL **319,176 / 344,064 B** — **24,888 B headroom**
- package bytes **610,110**
- package SHA-256 `f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168`
- inlined FDN render **1,094 B / 364** static instruction lines

Relative to the Phase-10 merge baseline, final Phase 11 uses **9,564 B less POOL** and **2,160 B less ordinary RAM**. Image size is **320 B larger**; Phase 11 was evidence-led memory/workload optimization, not a blanket size-minimization pass.

## Accepted production changes during Phase 11

- lossless compact String Motion store: **19,208 → 9,644 B**
- Bloom late-slot capacity **64 → 48** after exact legal bound 44 was proved
- Cathedral stereo-pair delay reads share address/fraction work
- FDN feedback gains cache on rounded decay changes
- FDN line-read specialization removes proved-unreachable/redundant work while preserving startup/ring/interpolation behavior
- FDN sine endpoint cache stores exact signed-16 endpoint pairs
- endpoint representation changed from A+B to exact A+delta with zero state growth

Rejected candidates remain rejected: selective 32-bit Q15 narrowing regressed target shape (P11T), and fixed add/sub output mixing regressed target image/render shape (P11Y).

## Bounded-capacity closure

- shared event queue high-water **415/512**
- Spiral late slots **59/64**
- Bloom late slots **44/48**, exact legal maximum **44**
- String transient **80/80 logical, 112/112 physical**, 480 steals, 0 drops
- String Attractor pending **60/64**
- Clock DSP pool **23/32**
- Spiral DSP legs **30/32**
- Bloom DSP petals **125/136**

No additional pool/capacity shrink is authorized from these observations.

## Frozen emulator closure

Idle:
- PCM SHA-256 `c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93`
- cursor **123,392**
- queue **28/512**
- overflow **0**
- late flags **0**
- late trace `811c9dc5`

Active Motion:
- target image/RAM/POOL **137,736 / 87,572 / 319,176 B**
- package SHA-256 `4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63`
- seen `ff`, final `14`
- points **149 / 149**
- record start **220,672**, finish **441,088**, cycle **220,416** samples
- manual override **450,560**, resume **466,176**
- cursor **481,024**
- queue **30/512**, overflow **0**, late flags **0**
- WAV frames **521,984**, peak **1,171**, RMS **173**

## Phase 12 boundary — physical FM-1 only

P11Z does **not** claim any of the following:

- physical `app_audio()` ISR/callback cycle margin under worst-case combined load
- physical runtime stack high-water and guard margin
- LCD DMA/audio bus contention or physical control/LED/display behavior
- real NOR erase/program timing or power-interruption behavior
- DAC/listening quality or Cathedral perceptual tuning
- real FM-1 install/rescue/update behavior

These are the remaining release-readiness tasks. New optimization after this checkpoint requires new measured evidence or physical timing data.
