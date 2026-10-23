# CHANGELOG

Human-readable milestone history. Git remains the detailed source history.

## 2026-10-23 - M3a checkpoint started

Active branch: `m3-transport-core`.

Added / in progress:
- sample-referenced APSIS transport core
- fixed 256-entry event min-heap
- long-running whole-sample + Q32 fractional timeline
- browser swing/division/Clock-cycle semantics
- event generation for Clock-linked APSIS systems
- Gravity boundary prediction
- transport regression test suite
- native String timing-state representation corrections found during scheduler audit

Status: implemented/committed but not yet fully CI-validated or target/emulator integrated. See `PROJECT_STATE.md`.

## 2026-10-23 - M2c merged

Merge commit: `4f690260c5d4d45ff10ab8eef2360b3bb0a3d361`.

Completed Phase 2 state semantics for:
- KEEP partial musical memory
- CHAIN cell/lane/morph model
- MOTHER held structural-change queue and horizon behaviour

Verification:
- strict desktop tests PASS
- ASAN/UBSAN PASS
- unchanged Dinghy baseline PASS
- APSIS single-TU host PASS
- real JieLi target PASS
- emulator smoke PASS

M2c package SHA-256: `5d318396d34cae6fd7c0fea4d175b1931ebf637194eb60bce7bf514433a94d02`.

## 2026-10-23 - M2b completed

Added source-derived:
- Gravity point defaults/model
- ring/voicing helpers
- Bloom voice-leading planner
- related regression coverage

## 2026-10-23 - M2a completed

Added source-derived:
- complete initial APSIS state/defaults
- scale/tuning helpers
- String pitch model
- Drift delta model
- Clock structural model
- Spiral route and derived sequence
- deterministic attractor state

## 2026-10-23 - Phase 1 completed

Established:
- pinned Dinghy baseline
- APSIS overlay shell
- unique target identity `FM-1_972`
- host build
- real JieLi FM-1 target build
- staged CI
- FM-1 emulator smoke

## 2026-10-23 - Project reference freeze

Locked:
- browser `apsis.html` as behavioural source
- source SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- port contract
- architecture direction
- parity matrix
- source/behaviour maps
- FM-1 UI-mapping proposals
