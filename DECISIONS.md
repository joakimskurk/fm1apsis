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

**Decision:** M3a/M3b begin with an allocation-free 256-entry min-heap queue. M3c may revise the capacity only after measured late-binding stress proves the need.

**Why:** Audio-side scheduling must avoid dynamic allocation and remain deterministic on FM-1, while capacity must be evidence-driven rather than guessed.

## 2026-10-23 - Browser timer jitter is not a feature

**Decision:** Preserve musical boundary semantics and late-binding intent, not incidental JavaScript `setTimeout` jitter.

**Why:** The browser uses lookahead and lead timers to work around main-thread/WebAudio constraints. Native sample scheduling can implement the same intent more directly.

## 2026-10-23 - Late binding is mandatory

**Decision:** Spiral and Bloom events keep their event time, but final pitch is resolved against the latest source-visible Gravity/Tuning state shortly before note-on.

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

## 2026-10-29 - Attached `index.html` is the existing frozen source

**Decision:** The attached Study 126 `index.html` does not create a new source revision. Its raw LF form is 601,005 bytes with SHA-256 `c5b16e6d137809163255bfa8ffb741eaa433a3703baff05fdbb40040563039bf`; normalizing LF to CRLF reproduces exactly the frozen 606,747-byte source and SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`.

**Why:** The apparent byte/hash difference is line-ending-only. Behavioural authority therefore remains the existing source lock with no parity relock.

## 2026-10-29 - Study 98 is the String acoustic reference, not the behavioural source

**Decision:** Keep locked Study 126 as the authority for String state, performance interaction, timing, Gravity relationship, routing, Classic/Dual behaviour and all later safety/click fixes. Use attached `old_index.html` (Study 98, raw LF SHA-256 `43553cc1836e080776e8d20b33d5eace32064df9d9f791743bb480ac94af92c8`) only as the acoustic/timbre target for String DSP.

When Phase 4 String DSP begins, preserve the preferred old String DNA as closely as practical on FM-1:
- the persistent low Gravity-root Foundation;
- the CLASSIC sine/drift pair around the played String pitch;
- the older overtone/pluck transient character.

Do **not** use Study 98 to roll back newer APSIS behaviour. In particular retain Study 126 Classic/Dual interaction, current two-coordinate/Gravity semantics, shared source-bus/Cathedral-send role, and the Study 120/124/125/126 de-click, guard and pulse-isolation fixes. Do not restore the removed broadband random-noise bow exciter merely to imitate the old version.

**Why:** The user explicitly prefers the old String sound, and source comparison shows that the later BODY/WIRE redesign materially changed the sound architecture. Treating Study 98 as a narrow acoustic reference lets the FM-1 port match the preferred timbre without corrupting the newer, locked musical model.

## 2026-10-29 - Native M3c uses the source desktop late-bind lead profile

**Decision:** Preserve both browser lead profiles in source-parity tests, but use the Study 126 **desktop** late-bind leads as the canonical FM-1 runtime profile:
- Spiral final pitch observation: target − 80 ms
- Bloom occurrence observation: target − 45 ms
- Bloom per-petal final pitch observation: voice target − 50 ms, causally clamped so it cannot precede Bloom occurrence

The Study 126 narrow/mobile values remain documented and tested as source facts:
- Spiral: target − 110 ms
- Bloom petal: target − 70 ms

**Why:** Study 126 changes these values from desktop to narrow browser only to give JavaScript/WebAudio main-thread scheduling more safety margin. FM-1 has neither `innerWidth` nor browser timer jitter. The existing project rule is to preserve late-binding intent rather than incidental timer compensation. Choosing the shorter source-defined desktop lead also observes state as late as the browser safely intends, which best matches the identity-bearing “latest state shortly before note-on” behaviour. This is a timing adaptation only; all frozen-vs-re-read state semantics remain source-exact.

## 2026-10-29 - M3c raises the shared event heap to 512 after measured starvation

**Decision:** The shared event queue remains one allocation-free min-heap, but its capacity is 512 entries from M3c onward. Do not restore the provisional 256-entry capacity unless the scheduling model itself changes and is re-measured.

**Evidence:** A legal worst-case M3c profile at 180 BPM, 1/4 division, 4x Spiral and 4x Bloom with long unfurl caused `scheduler_starved` at 256 entries. With 512 entries the canonical 30 s stress reached 352/512; a separate 60 s max-swing 35% burst reached 357/512 with no overflow/starvation.

**Why:** Study 126's ~1.20 s lookahead plus explicit pre-target and late-bind events materially raises the number of simultaneously live events. 512 is measured capacity, not speculative headroom.

## 2026-10-29 - M3c late-bind pools stay at 64 entries per source

**Decision:** Use fixed 64-entry Spiral and 64-entry Bloom occurrence/bind pools in M3c. Bloom petals are chained one at a time on the shared heap rather than pre-enqueuing the full 3.6 s unfurl.

**Evidence:** Canonical 30 s stress reached Spiral 59/64 and Bloom 44/64. The 60 s max-swing burst reached the same 59/64 and 44/64 without overflow.

**Why:** This preserves deterministic bounded memory while retaining exact target/bind samples. The stress evidence shows legal source ranges fit without dynamic allocation.

## 2026-10-29 - M3c canonical deterministic trace is locked

**Decision:** The canonical host replay uses seed `0x31415926` for 30 seconds under the documented worst-case profile. Current reference signatures are:
- random-decision trace: `78771ff5`
- late-binding/state-observation trace: `0e6fc422`
- queue high-water: 352/512
- Spiral slot high-water: 59/64
- Bloom slot high-water: 44/64

The same seed must replay identically; a different seed must change at least one trace hash.

**Why:** M3c needs a compact, auditable long-run identity for deterministic timing/random/state-observation behavior before DSP is allowed to consume it.
