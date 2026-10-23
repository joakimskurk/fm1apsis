# DECISIONS

Only durable decisions that would be expensive or confusing to rediscover belong here.

## 2026-10-23 - APSIS identity is parity-first

**Decision:** The FM-1 port must preserve APSIS as a distinct instrument. Port the musical/state/timing relationships as close to 1:1 as practical, while adapting only the physical interaction to FM-1.

**Why:** Turning it into a conventional synth/groovebox would lose the instrument's identity-bearing systems.

## 2026-10-23 - Source reference is frozen

**Decision:** `apsis.html` with SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21` is the behavioural source reference for this port.

**Why:** Native implementation decisions need a stable reference rather than memory or later accidental browser changes.

## 2026-10-23 - Dinghy baseline is pinned

**Decision:** Use Dinghy commit `d44df7db09b98ccad2f1233b8f83f5f09c04829e`, tree `8cefd253b92b544109a315fd2fb9ffcea7c429b6` until an explicit revalidation changes it.

**Why:** This is the proven baseline already used in the FM-1 work and keeps host/target/emulator results auditable.

## 2026-10-23 - Unique FM-1 product identity

**Decision:** APSIS uses `FM-1_972`.

**Why:** Avoid package/product collisions with Dinghy and other custom FM-1 firmware.

## 2026-10-23 - Phase order is state -> transport -> sound

**Decision:** Do not start final DSP before the pure musical/state core and deterministic shared transport are established.

**Why:** Sound code must consume APSIS semantics, not accidentally define them.

## 2026-10-23 - One shared native state graph

**Decision:** APSIS modules share one state graph. Do not implement STRING/DRIFT/CLOCK/SPIRAL/BLOOM/GRAVITY/KEEP/CHAIN/MOTHER as unrelated mini-apps.

**Why:** Cross-system relationships are central to APSIS.

## 2026-10-23 - One shared transport

**Decision:** Use one sample-referenced, fixed-capacity event timeline for Clock-derived events. Do not give modules independent clocks.

**Why:** This prevents drift and preserves the source ordering/quantization relationships.

## 2026-10-23 - Transport time representation

**Decision:** Store long-running transport time as 64-bit whole samples plus Q32 fractional sample, not one 64-bit Q32 absolute value.

**Why:** It keeps fractional precision without introducing a practical ~27-hour wrap limit at 44.1 kHz.

## 2026-10-23 - Fixed-capacity event queue

**Decision:** M3 transport uses an allocation-free 256-entry min-heap queue.

**Why:** Audio-side scheduling must avoid dynamic allocation and remain deterministic on FM-1.

**Status:** Capacity is provisional until later high-water stress tests prove margin.

## 2026-10-23 - Browser timer jitter is not a feature

**Decision:** Preserve musical boundary semantics and late-binding intent, not incidental JavaScript `setTimeout` jitter.

**Why:** The browser uses lookahead and lead timers to work around main-thread/WebAudio constraints. Native sample scheduling can implement the same intent more directly.

## 2026-10-23 - Late binding is mandatory

**Decision:** Spiral and Bloom events keep their event time, but final pitch is resolved against the latest committed Gravity/Tuning state shortly before note-on.

**Why:** This interaction is identity-bearing and explicitly present in the browser architecture.

## 2026-10-23 - KEEP is not a preset

**Decision:** KEEP stores only the source-defined partial musical memory: String tension/anchors, Drift amount, Clock geometry, Spiral route, Tuning mode and Gravity harmony.

**Why:** Full sound-state recall would change the composition model.

## 2026-10-23 - CHAIN preserves lane semantics

**Decision:** CHAIN remains a 32-cell musical-memory lane system with chance, transpose, morph, lane scopes and start/loop sync. Continuous morph is limited to source-defined String position and Drift amount.

**Why:** It must not become a generic scene/preset sequencer.

## 2026-10-23 - MOTHER structural hold keeps mixer live

**Decision:** `->1` may hold structural edits, but `mother.*` mixer/mute controls and output/master stay live. Pending items preserve first insertion order; repeat param updates replace value in place; a repeated toggle cancels; pending Gravity commits after ordered items.

**Why:** This matches browser behaviour and preserves MOTHER as a performance surface.

## 2026-10-23 - Step-one resync default remains off

**Decision:** `stepOneResync` is OFF by default. If enabled, resync is processed before Clock-linked String/Spiral events at Clock 1.

**Why:** Default source behaviour is free-running, while explicit resync must reset phase before downstream events observe it.

## 2026-10-23 - Shared SPACE/Cathedral topology

**Decision:** The final room is one shared Space/Cathedral return with per-source sends. Prototype a compact modulated FDN and a nested allpass/Schroeder alternative before choosing.

**Why:** The browser uses one shared room, and prior FM-1 work showed that short periodic delay networks become metallic/comb-like.

## 2026-10-23 - Storage will not use the tiny settings slot for the full project

**Decision:** Persistent APSIS project storage will use a dedicated versioned CRC-protected journal/two-slot strategy and safe write deferral.

**Why:** Dinghy's 256-byte settings slot is too small for KEEP/CHAIN/project state and unsafe partial writes must recover cleanly.

## 2026-10-23 - Verification language must be literal

**Decision:** Distinguish implemented, desktop-tested, sanitizer-tested, target-built, emulator-tested and physical-hardware-tested.

**Why:** A successful build is not equivalent to hardware validation. Physical FM-1 hardware is not currently available.
