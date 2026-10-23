# APSIS FM-1 product identity

Application name: `APSIS`

Repository: `joakimskurk/fm1apsis`

Package stem: `apsis`

FM-1 identity: `FM-1_972`

Decision date: 2026-10-23

Reasoning:

- Dinghy reserves/uses the `FM-1_99N` family for its own default identity path.
- `FM-1_971` is already present in the public FM-1 firmware ecosystem.
- Public GitHub and Firmware Hub searches on 2026-10-23 returned no use of `FM-1_972`.
- APSIS therefore uses `FM-1_972` unless a later collision is discovered before release.

Release builds must fail closed if a valid explicit APSIS identity is not provided. The build must never silently fall back to Dinghy's `FM-1_990` identity.
