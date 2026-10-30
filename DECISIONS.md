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

## 2026-10-30 - Phase 6 starts with topology + memory prototypes

**Decision:** Freeze the Space/Cathedral contract and compare compact modulated FDN and nested allpass/Schroeder candidates before target integration.

## 2026-10-30 - Phase 6 selects the compact 8-line modulated FDN

**Decision:** Use the deterministic 8-line modulated FDN as native Cathedral tail topology. It tracks the reference proxy with much lower metallicity than the tested Schroeder candidate and lower tail storage.

## 2026-10-30 - Space owns a dedicated pre-.pool NOLOAD region

**Decision:** Long-lived Space delay memory lives in `.apsis_space_pool` immediately before Dinghy's ordinary `.pool`, inside the same hardware POOL region.

**Why:** Dinghy clears `[_pool_start,_pool_end)` at boot. Keeping the large arrays outside ordinary `.pool` avoids a startup callback shift while linker accounting still includes their footprint.

## 2026-10-30 - Space delay memory is self-initialising

**Decision:** Production Space does not rely on CRT zeroing or giant startup memset. Delay histories use fill/valid state so unread NOLOAD bytes are never interpreted as audio.

## 2026-10-30 - Generated FM-1 Cathedral uses a quarter-rate internal room clock

**Decision:** Source engines, scheduler and final audio remain 44.1 kHz, while the generated FM-1 Space/Cathedral renderer runs at **11.025 kHz**. The target bridge performs 4:1 send averaging and wet hold; target-only generation retimes delay lengths, milliseconds, filter coefficients, decay law and modulation increments so their real-time roles remain equivalent.

**Why:** A full-rate room passed memory/link tests but consumed too much emulated target execution budget. Quarter-rate processing restores the complete pinned emulator scenario while preserving the selected shared-room topology and perceptual role. The high-precision host/reference room remains untouched at 44.1 kHz.

## 2026-10-30 - Phase-6 integrated scenario has an explicit 123,392 boot-to-stop baseline

**Decision:** Keep the historical M4/M5 cursor baseline at **123,648** for those gates. The integrated P6 scenario is separately frozen at **123,392** for both audible and room-return-disabled P6 builds.

**Why:** Both P6 variants have the same one-callback-lower boot-to-stop cursor while preserving exact dry PCM in the disabled-return variant, queue 28/512, overflow 0, late flags 0 and late trace `811c9dc5`. This is an explicit scenario rebaseline, not permission to relax M4/M5 regressions or alter the shared scheduler.

## 2026-10-30 - Multi-variant target reports use the latest build summary

**Decision:** `inspect_target_build.py` reads the last resource summary from the append-only target build log.

**Why:** staged workflows intentionally build several variants in one workspace. Reading the first summary misreported a correct 95,804 B M4f Bloom-bypass as the preceding 100,944 B M4g build. Package bytes were correct; only reporting was stale.

## 2026-10-30 - Phase-7 project sectors and commit protocol are frozen

**Decision:** APSIS owns two 4 KiB project sectors at `0x000F4000` and `0x000F5000`. Project records use explicit little-endian v1 encoding, CRC-protected header/payload, wrap-safe generations and payload-first/header-last commit. Dinghy's `0x000FC000` settings sector remains separate.

**Why:** Two validatable generations let a torn/corrupt new save fall back to the prior project, while explicit serialization prevents compiler layout from becoming the storage ABI.

## 2026-10-30 - Project NOR is never touched by active audio

**Decision:** Boot project NOR prefetch happens before `fm1_audio_init()` / ALNK0 DMA. Runtime project saves are deferred to `app_main()`, require stopped transport and explicitly quiesce audio before erase/program. No project flash mutation may occur from `app_audio()`.

**Why:** A post-audio-start boot read measurably perturbed 202 frames in the first active block by up to two Q15 steps even though later PCM and runtime were exact. The project rejected a new baseline and restored bit-exact P6 behaviour by moving the read before audio startup.