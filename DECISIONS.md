# DECISIONS

Only durable decisions that would be expensive or confusing to rediscover belong here.

## 2026-10-23 - APSIS identity is parity-first

**Decision:** The FM-1 port preserves APSIS as a distinct instrument. Port musical/state/timing relationships as close to 1:1 as practical; adapt only interaction where FM-1 differs from browser mouse/touch.

## 2026-10-23 - Source reference is frozen

**Decision:** Study 126 / `apsis.html` SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21` is behavioural/state/timing/safety authority. Study 98 is only the narrow String acoustic reference.

## 2026-10-23 - Dinghy baseline and product identity are pinned

**Decision:** Use Dinghy `d44df7db09b98ccad2f1233b8f83f5f09c04829e`, tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6`, product `FM-1_972` until explicitly revalidated.

## 2026-10-23 - State -> transport -> sound

**Decision:** Musical/state semantics and deterministic shared transport precede final DSP. Sound consumes APSIS semantics; it does not define them.

## 2026-10-23 - One shared state graph and one shared transport

**Decision:** STRING/DRIFT/CLOCK/SPIRAL/BLOOM/GRAVITY/KEEP/CHAIN/MOTHER are views of one state graph. Clock-derived musical time uses one sample-referenced fixed-capacity timeline; modules do not get private clocks.

## 2026-10-23 - Transport time representation

**Decision:** Long-running time is 64-bit whole samples plus Q32 fractional sample, not one 64-bit Q32 absolute value.

## 2026-10-23 - Fixed-capacity audio-side structures

**Decision:** Audio scheduling/voice management uses allocation-free fixed-capacity structures. Capacities are evidence-driven by legal worst-case stress. M3c raised the shared heap to 512 only after measured starvation.

## 2026-10-23 - Browser timer jitter is not a feature

**Decision:** Preserve musical boundary semantics and late-binding intent, not incidental JavaScript timer jitter.

## 2026-10-23 - Spiral/Bloom late binding is mandatory

**Decision:** Event time remains fixed while final musical pitch resolves against source-visible state at the source-defined late observation point. Spiral does not project ordinary pending Gravity; Bloom may under the frozen target-aware rule. Mother-held future state is not projected.

## 2026-10-23 - KEEP / CHAIN / MOTHER semantics are source-defined

**Decision:** KEEP stores only source-defined partial musical memory. CHAIN remains the 32-cell lane system. MOTHER `->1` may hold structural edits while mixer/mute/output remain live.

## 2026-10-23 - Step-one resync defaults OFF

**Decision:** `stepOneResync` remains OFF by default. When enabled, resync precedes Clock-linked String/Spiral events at Clock 1.

## 2026-10-23 - Shared Space/Cathedral topology

**Decision:** Final room is one shared Space/Cathedral return with per-source sends. Do not introduce unrelated per-source reverbs.

## 2026-10-23 - Project persistence uses dedicated journal storage

**Decision:** Full project storage uses versioned CRC-protected journal/two-slot persistence with safe write deferral. Dinghy's tiny settings slot is not the project store.

## 2026-10-23 - Verification language must be literal

**Decision:** Distinguish implemented, desktop-tested, sanitizer-tested, target-built, emulator-tested and physical-hardware-tested.

## 2026-10-29 - Study 98 is a narrow String acoustic authority

**Decision:** Study 126 remains String behaviour/state/timing/Gravity/routing/safety authority. Study 98 LF SHA `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8` is only the acoustic/timbre reference for String features that existed there.

## 2026-10-29 - M3c deterministic identity is locked

**Decision:** `sizeof(ApsisRuntime)=21,928 B`, 512-event heap, seed `0x31415926`, random trace `78771ff5`, late trace `0e6fc422`, canonical queue 352/512; extreme max-swing reaches 357/512.

## 2026-10-30 - Drift persistent pair is a documented source-wiring repair

**Decision:** Native DRIFT is the paired beating field around the Gravity root. Preserve source delta/depth/level and historical stereo-width law rather than making DRIFT silent.

## 2026-10-30 - Clock consumes the shared timeline

**Decision:** Clock audio passively consumes existing Clock events. No second Clock scheduler/queue or Ghost random draw.

## 2026-10-30 - Preserve exact integer DSP math on JieLi via target ABI shim

**Decision:** Where pi32v2 omits DI-mode division helpers, provide target-only `__divdi3` / `__udivdi3` rather than weakening frozen host math.

## 2026-10-30 - M4f Spiral reuses the frozen M3c lifecycle

**Decision:** Spiral audio never schedules another occurrence or bind. M4f snapshots only source-defined acoustic data keyed to the existing M3c token/slot. `ApsisRuntime` does not grow.

## 2026-10-30 - Inactive dry engines preserve exact emulator callback budget

**Decision:** The historical M4-series pinned three-second emulator advances to **123,648 samples exactly**. An inactive dry engine may not consume a callback unless a later scenario is explicitly rebaselined with evidence.

## 2026-10-30 - M4g Bloom reuses M3c causality

**Decision:** Bloom creates no second lifecycle or RNG stream. Existing occurrence and petal-bind state remains authoritative.

## 2026-10-30 - Bloom activation preserves exact M4f idle path

**Decision:** M4g remains on the exact M4f callback path until an already-planned Bloom occurrence is due.

## 2026-10-30 - Static RAM is a hard constraint

**Decision:** Large delay/state storage may not be added to ordinary BSS without measured evidence. Memory savings may not silently trade away parity.

## 2026-10-30 - Phase 5 is verification-only when architecture is already correct

**Decision:** Do not change production scheduler/DSP/runtime merely to manufacture a phase diff. P5a-P5f prove existing source-defined behaviour end-to-end.

## 2026-10-30 - Sounding identity stays in the layer that owns it

**Decision:** Tests read scheduler/attractor identity from existing M3c token/slot state rather than duplicating structural fields in acoustic snapshots.

## 2026-10-30 - Phase 6 selects the compact 8-line modulated FDN

**Decision:** Use the deterministic 8-line modulated FDN as native Cathedral tail topology. It tracks the reference proxy with much lower metallicity than the tested Schroeder candidate and lower tail storage.

## 2026-10-30 - Space owns a dedicated pre-.pool NOLOAD region

**Decision:** Long-lived Space delay memory lives in `.apsis_space_pool` immediately before Dinghy's ordinary `.pool`, inside the same hardware POOL region.

**Why:** Dinghy clears `[_pool_start,_pool_end)` at boot. Keeping the large arrays outside ordinary `.pool` avoids a startup callback shift while linker accounting still includes their footprint.

## 2026-10-30 - Space delay memory is self-initialising

**Decision:** Production Space does not rely on CRT zeroing or giant startup memset. Delay histories use fill/valid state so unread NOLOAD bytes are never interpreted as audio.

## 2026-10-30 - Generated FM-1 Cathedral uses a quarter-rate internal room clock

**Decision:** Source engines, scheduler and final audio remain 44.1 kHz, while the generated FM-1 Space/Cathedral renderer runs at **11.025 kHz**. The target bridge performs 4:1 send averaging and wet hold; target-only generation retimes delay lengths, milliseconds, filter coefficients, decay law and modulation increments so their real-time roles remain equivalent.

## 2026-10-30 - Phase-6 integrated scenario has an explicit 123,392 boot-to-stop baseline

**Decision:** Keep the historical M4/M5 cursor baseline at **123,648** for those gates. The integrated P6 scenario is separately frozen at **123,392** for both audible and room-return-disabled P6 builds.

## 2026-10-30 - Multi-variant target reports use the latest build summary

**Decision:** `inspect_target_build.py` reads the last resource summary from the append-only target build log.

## 2026-10-30 - Phase-7 project sectors and commit protocol are frozen

**Decision:** APSIS owns two 4 KiB project sectors at `0x000F4000` and `0x000F5000`. Project records use explicit little-endian v1 encoding, CRC-protected header/payload, wrap-safe generations and payload-first/header-last commit. Dinghy's `0x000FC000` settings sector remains separate.

## 2026-10-30 - Project NOR is never touched by active audio

**Decision:** Boot project NOR prefetch happens before `fm1_audio_init()` / ALNK0 DMA. Runtime project saves are deferred to `app_main()`, require stopped transport and explicitly quiesce audio before erase/program. No project flash mutation may occur from `app_audio()`.

**Why:** A post-audio-start boot read measurably perturbed 202 frames in the first active block by up to two Q15 steps even though later PCM and runtime were exact. The project rejected a new baseline and restored bit-exact P6 behaviour by moving the read before audio startup.

## 2026-10-31 - Phase-8 visuals are read-only views of existing APSIS state

**Decision:** Native visuals consume `ApsisState` / `ApsisRuntime` directly. KEEP and CHAIN may additionally observe the existing `ApsisKeepBank` and `ApsisChainSequence`. The UI layer does not own a duplicate musical model.

**Why:** Visual adaptation must not create a second source of truth or alter scheduler/audio semantics.

## 2026-10-31 - Phase-8 target redraw reuses Dinghy canvas and is incremental

**Decision:** Reuse the existing `cv_px` canvas scratch. Do not allocate a second 240×240 framebuffer. Native full-screen changes are emitted as 8-row strips, and a strip may start only when the previous LCD DMA has completed via a HAL-level readiness helper.

**Why:** Large back-to-back LCD blits can block in `lcd_sync()` and perturb the otherwise frozen audio timeline. The final design keeps POOL at 309,532 B and register access inside HAL.

## 2026-10-31 - Native visual takeover is deferred past frozen audio bootstrap

**Decision:** Preserve the exact Phase-7 legacy boot draw until the shared runtime cursor reaches sample **8192**, then enable native incremental rendering.

**Why:** Even non-blocking strip rendering during the initial bootstrap changed only early PCM frames 1795..4351 while later audio and all runtime telemetry were exact. We rejected a new PCM baseline; deferred native takeover restores the full P6/P7 hash exactly.

## 2026-10-31 - Phase 8 defines SPACE as a surface, Phase 9 defines its physical access

**Decision:** SPACE/CATHEDRAL is one of the ten native target-addressable surfaces, but Phase 8 does not assign it an FM-1 button gesture. Final physical routing belongs to Phase 9.

**Why:** Surface identity and rendering are visual architecture; control ergonomics must be frozen separately so the visual phase does not silently choose interaction policy.
