# M3c late-binding contract — Study 126

Status: source-derived contract for Phase 3 / M3c. This document is normative for M3c tests and implementation.

Authority:
- locked browser source: `apsis.html`, Study 126 · String Pulse Isolation
- SHA-256: `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- Study 98 remains String acoustic reference only and has no authority over this timing contract

M3c remains silent. This contract describes **state observation and event timing**, not DSP.

## 1. Browser scheduler context

Study 126 `audio.tick()` fills roughly 1.20 s of future WebAudio time. Spiral attractor decisions and Bloom occurrence scheduling are therefore planned ahead, while pitch is deliberately rebound closer to the audible target.

Relevant source landmarks in the locked Study 126 file:
- `bloomGravitySnapshotAt()` around lines 1591–1610
- Bloom voicing planner around lines 1629–1749
- `freqForStep()` around lines 1476–1482
- `scheduleSpiralBoundVoice()` around lines 2503–2521
- `scheduleBloomBoundVoice()` around lines 2532–2547
- `playBloom()` / `scheduleBloom()` around lines 2582–2614
- `audio.tick()` around lines 2750–2830
- `commitPendingGravity()` / `queueGravityCandidate()` around lines 3739–3764

Native APSIS must preserve these state-observation rules on the shared sample timeline. It must **not** reproduce browser main-thread jitter, late timer execution or WebAudio implementation accidents.

## 2. Spiral: event identity is frozen, final pitch is late-bound

### 2.1 Occurrence/planning state

At the Spiral subdivision in `tick()`:
- the attractor cell is advanced;
- gate and energy are resolved;
- the route ordinal `index` is chosen from the current derived sequence length;
- trigger position, energy and duration are attached to the occurrence;
- `spiralStep` advances only for a gated occurrence.

The event's **route ordinal** is the stable identity that survives to late binding. The originally derived pitch is only provisional.

### 2.2 Browser pitch-bind lead

`scheduleSpiralBoundVoice()` prepares the WebAudio voice at the target note-on time, then schedules a pitch-only rebind:
- desktop (`innerWidth > 700`): target − **80 ms**
- narrow browser (`innerWidth <= 700`): target − **110 ms**

The timer may change pitch but cannot move note-on.

### 2.3 State re-read at Spiral bind

At the late-bind point Study 126 re-reads:
- `derivedSequence()`;
- the **current sequence length**;
- current committed Gravity/frame state used by `derivedSequence()`;
- current `state.spiral.register`;
- current scale lock / tonic / scale / global temperament state reached through `freqForStep()`.

The frozen route ordinal is reduced modulo the **new** derived-sequence length. If the current derived sequence is empty, no final rebind is produced.

### 2.4 State Spiral does *not* project

Spiral does **not** call `bloomGravitySnapshotAt()` and does not inspect `state.framePending`.

Therefore a quantized Gravity candidate that is merely pending at the Spiral bind time is invisible to Spiral. It affects the bound Spiral pitch only if it has actually committed before the bind observation point.

This asymmetry versus Bloom is intentional source behaviour and must be regression-tested.

### 2.5 Native rule for browser timer lateness

The browser has a safety fallback: if the main-thread timer runs too late to rebind a prepared oscillator, the provisional pitch can survive. Native APSIS must **not** emulate this nondeterministic timer failure. The M3c bind event is deterministic and records the pitch state observed at its defined bind sample.

## 3. Bloom: occurrence freezes the plan; every petal binds harmony/tuning again

Bloom has two distinct late-binding stages.

### 3.1 Bloom occurrence stage

`tick()` computes the Bloom target as:
- qualifying Clock/Bloom point
- plus `state.bloom.delay` clamped to 0…320 ms.

`scheduleBloom()` runs its occurrence callback at:
- target − **45 ms**.

At that callback it evaluates `bloomGravitySnapshotAt(target)` and calls `playBloom()`.

### 3.2 `bloomGravitySnapshotAt(target)` projection rule

The snapshot begins with the **currently committed**:
- tonic;
- scale;
- degree;
- voicing/ring;
- inversion;
- voice count.

If a normal quantized `state.framePending` exists and its commit target `p.at` is at or before the requested Bloom target (with the source's +0.5 ms tolerance), the snapshot projects only:
- pending degree;
- pending ring/voicing.

It does **not** project tonic, scale, inversion or voices.

It also does **not** inspect Mother's pending structural queue. Mother-held changes become visible only after Mother actually commits them.

### 3.3 State frozen at Bloom occurrence

`playBloom()` chooses a new voicing plan once per Bloom occurrence. The following are occurrence-time decisions/data and must remain stable for all petals of that Bloom:
- the chosen voicing plan, including its tone ordinals and octave/register offsets;
- bounded-random-walk / random candidate choice and resulting voicing-state advance;
- occurrence voice count as represented by the chosen plan length;
- Bloom duration;
- unfurl amount and therefore each petal's target `voiceAt`;
- width-derived pan per petal;
- occurrence-derived amplitude/level per petal.

The plan is explicitly copied before each petal timer is scheduled. **Register is therefore frozen through the plan; it is not re-read at petal bind.**

### 3.4 Per-petal pitch-bind lead and occurrence causality

Each Bloom petal requests a pitch-bind timer relative to its own `voiceAt`:
- desktop (`innerWidth > 700`): voice target − **50 ms**
- narrow browser (`innerWidth <= 700`): voice target − **70 ms**

However, those per-petal timers are not created until `playBloom()` runs inside the Bloom occurrence callback at target −45 ms. `scheduleBloomBoundVoice()` uses `Math.max(0, ...)`, so a petal bind can never execute before the occurrence that created it.

Therefore the deterministic native equivalent is:
- `occurrenceSample = bloomTarget − 45 ms`
- `petalBindSample = max(occurrenceSample, voiceTarget − nativePetalLead)`

For the first petal, `voiceTarget == bloomTarget`. Under both browser lead profiles its nominal bind (−50 or −70 ms) lies before the −45 ms occurrence, so the first petal effectively observes state **at occurrence time**, approximately target −45 ms. Later petals can bind at their normal target-relative lead once their `voiceAt` is sufficiently later than the occurrence target.

Because `voiceAt = occurrence target + phase × unfurl`, later petals can bind substantially later than the Bloom occurrence and can observe different state.

### 3.5 State re-read at each Bloom petal bind

At every petal bind Study 126 re-evaluates:
- `bloomGravitySnapshotAt(voiceAt)` using the target-aware pending-Gravity projection above;
- current Bloom `depth` while rebuilding the base chord under the frozen plan;
- current Bloom tuning reference;
- current Bloom tuning mode;
- current Bloom tuning amount;
- current global temperament mode when Bloom tuning follows global temperament;
- current global temperament strength when Bloom tuning follows global temperament.

The **frozen plan** is then mapped onto the newly observed base chord. The plan itself, its octave/register offsets and its random choice are not recomputed.

Result: different petals of one Bloom may legitimately resolve to different final pitches/tuning if Gravity or tuning state changes during unfurl.

## 4. Pending Gravity and Mother interaction

There are two different future-state paths and M3c must not merge them:

1. **normal quantized Gravity** (`state.framePending` in Study 126)
   - Bloom may project pending degree/ring when the pending commit target is <= the Bloom/petal target;
   - Spiral never projects it and sees it only after actual commit.

2. **Mother-held structural edits**
   - are not part of `bloomGravitySnapshotAt()`;
   - neither source may synthesize them before Mother actually commits;
   - visibility is therefore determined by the real shared-timeline ordering of Mother commit versus the later Spiral/Bloom observation event.

## 5. Source mute behaviour

Both late-bind paths re-check whether their source is on when the bind callback fires. Native symbolic M3c should retain enough information to mark a planned bind as skipped/cancelled if the source is disabled before bind. No audio is produced in M3c.

## 6. Browser width leads versus native FM-1

The source has two browser safety-lead profiles because the browser main thread uses `innerWidth` as a proxy for the narrow/mobile environment. FM-1 has no browser viewport and no main-thread timer jitter.

M3c tests encode **both source values** so the browser contract is not lost. FM-1 uses the source desktop profile as recorded in `DECISIONS.md`: Spiral 80 ms and Bloom petal 50 ms, with Bloom occurrence fixed at 45 ms. Bloom occurrence causality still applies, so the first petal cannot bind before the 45 ms occurrence.

## 7. Required regression expectations before runtime implementation

The M3c regression slice must prove at minimum:

1. Spiral route ordinal is frozen at occurrence but final sequence length/pitch is re-read at bind.
2. Spiral observes a Gravity change that committed before bind.
3. Spiral does **not** observe a merely pending same-target Gravity change.
4. Spiral final pitch re-reads register and global tuning state.
5. Bloom occurrence freezes the voicing plan/register/random choice.
6. Bloom occurrence snapshot can project normal pending degree/ring for its target.
7. Bloom does not project Mother-held pending Gravity before Mother commit.
8. Each Bloom petal re-reads target-aware Gravity independently.
9. Each Bloom petal re-reads tuning independently while retaining the occurrence-frozen plan.
10. A later petal can resolve differently from an earlier petal after a mid-unfurl state change.
11. Desktop/narrow source lead constants are represented explicitly.
12. First-petal Bloom bind cannot precede the 45 ms occurrence; later petals use `max(occurrence, voiceAt−lead)`.
13. Added bind events remain bounded in the fixed 256-entry queue and do not create a second module clock.
14. M3c output remains exactly silent.

## 8. Non-goals

M3c does not implement:
- Spiral oscillators, filters, envelopes or timbre;
- Bloom oscillators, envelopes, filters or timbre;
- String/Drift/Clock DSP;
- Cathedral/Space DSP;
- browser timer-lateness failures;
- visual animation timing beyond what is needed to identify the same musical occurrence.

Those remain later-phase work. String DSP will use Study 98 only as its acoustic target inside the Study 126 behavioural contract.
