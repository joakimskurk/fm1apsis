# Phase 11G — workload proxy inventory

P11G is a measurement-only phase. It changes no production firmware behavior or capacity.

## Authority

- technical head: `dbe0b540e602ea5ce8d93f4b119951736f5549bc`
- green workflow run: `41333497987`
- evidence artifact: `p11g-workload-inventory-evidence`
- artifact SHA-256: `4365212ac4ad5668840d150f44be12933c9dcbef2faa575fbfdc261f750c6b5e`

All five P11G gates passed on the exact technical head: frozen P11F host/sanitizer behavior, exact P11F production target, deterministic target/source workload inventory, exact idle emulator behavior, and the frozen active Motion control/audio probe.

## Frozen production baseline

P11G regenerates the accepted P11F target exactly:

- product `FM-1_972`
- image **140,408 / 581,564 B**
- ordinary RAM **87,476 / 98,304 B**
- RAM headroom **10,828 B**
- POOL **319,176 / 344,064 B**
- POOL headroom **24,888 B**
- package bytes **610,110**
- package SHA-256 `e0d0b826f0c2d5b8f1af781a9259b5d78dce9062939661ac7acad17bfefb78e5`

Idle PCM/runtime and active Motion lifecycle remain bit-for-bit frozen.

## Audio callback inventory

The generated production `app_audio()` contains:

- **219 source lines**
- **51 lexical call-sites**
- **34 unique call names**

The callback contains no native visual renderer, main-loop control dispatcher, LCD/canvas operation or input poll. Performance mailboxes are consumed on the audio side as intended.

Selected direct audio-side calls include the five dry engines, shared runtime consumers, performance-mailbox consumers and the single shared Space/Cathedral target. The shared Space path begins one block, reuses one source buffer, commits each source and renders one shared room.

These are source-level call counts, not dynamic CPU-cycle measurements.

## Static target-code proxy

The real JieLi production ELF contains **246 disassembled APSIS functions** and **228 APSIS text symbols** in the inventory parser.

Selected target function sizes / static disassembly instruction-line counts:

- `apsis_transport_schedule_next`: 2,064 B / 715
- `apsis_space_room_render`: 2,164 B / 675
- `apsis_space_fdn_render`: 1,082 B / 353
- `apsis_spiral_dsp_render`: 948 B / 294
- `apsis_string_dsp_render`: 898 B / 277
- `apsis_bloom_dsp_render`: 888 B / 276
- `apsis_string_transient_render`: 856 B / 266
- `apsis_clock_dsp_render`: 654 B / 210
- `apsis_drift_dsp_render`: 554 B / 171
- `apsis_space_target_render`: 454 B / 145
- `apsis_runtime_process`: 422 B / 91

Static size/instruction counts do **not** include loop trip counts, active voice counts, memory/cache waits or hardware scheduling. They therefore rank code shape, not physical CPU cost.

## Display workload proxy

The accepted native visual path remains incremental and outside `app_audio()`:

- display: 240 × 240 RGB565
- native strip height: 8 rows
- full redraw: **30 strips**
- maximum pixel payload per strip: **3,840 B**
- full redraw pixel payload: **115,200 B**
- protocol/command overhead is not included
- every strip is gated by `apsis_lcd_dma_ready()` / `fm1_lcd_done()` before the next `cv_blit()`
- native visual takeover remains at runtime sample **8,192**

This quantifies transfer work but does not measure LCD/audio bus contention on physical hardware.

## Dynamic interpretation from existing code

P11G intentionally avoids turning static instruction count into a fake timing result.

Two large functions illustrate why:

1. `apsis_transport_schedule_next()` is statically large, but `apsis_runtime_process()` calls it only while the next transport pulse falls inside the current callback window. Its cost is pulse/lookahead driven, not per audio sample.
2. `apsis_space_target_render()` is called once for each audio callback. The accepted target converts the 44.1-kHz callback to the quarter-rate Cathedral domain: `room_frames = ceil(frames / 4)`, then calls `apsis_space_room_render()` once. For a 256-frame callback this is **64 room frames**. Every room frame performs repeated filtering, early-reflection reads, predelay/echo work, smoothing and then participates in the FDN block render.

Therefore static function size alone does not justify optimizing the transport scheduler first. The Space/Cathedral path has the stronger evidence-backed case for the next dynamic workload measurement because it is a bounded repeated audio-hotpath.

## P11G decision

P11G is accepted as a workload baseline, not as a timing proof.

It proves:

- target/source workload structure can be regenerated deterministically;
- UI/display work remains outside `app_audio()`;
- one full native redraw is exactly 30 DMA-gated 8-row strips / 115,200 B pixel payload;
- the target ELF provides a stable static code-shape baseline;
- static code size must be paired with dynamic operation frequency before selecting a CPU optimization.

It does **not** prove:

- physical FM-1 ISR headroom;
- CPU cycles or wall-time per callback;
- runtime stack high-water;
- cache/memory wait-state cost;
- LCD/audio bus contention;
- NOR timing;
- DAC or listening quality.

## Exact next phase — P11H

P11H is measurement-only and should instrument/bound **Cathedral dynamic work per audio block** without changing the audible algorithm.

Primary measurements:

1. output frames and resulting quarter-rate room frames per callback;
2. `apsis_space_room_render()` frame iterations;
3. early-reflection tap reads;
4. predelay and echo delay reads/writes;
5. biquad evaluations;
6. FDN frames / line operations where they can be counted without perturbing production behavior;
7. comparison with transport scheduling calls/events over the same deterministic scenario;
8. exact idle PCM/runtime and active Motion regressions after any test-only instrumentation.

P11H must use deterministic counters or a host/reference harness. GitHub-runner wall time must not be a correctness gate, and no physical ISR-margin claim may be made without hardware.
