# APSIS Browser Reference Lock

This document freezes the browser source used to plan the first FM-1 port.

## Locked source

- file: `apsis.html`
- size: `606747` bytes
- lines: `5742`
- SHA-256: `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`
- frozen for FM-1 planning: 2026-10-23

The HTML itself is not copied into this repository during the planning phase. The hash above is the behavioural-source identity. Any later browser revision must receive a new hash and an explicit parity-diff review before it becomes the FM-1 reference.

### 2026-10-29 attached-source verification

The user supplied `index.html` again while M3a was being finalized. The attachment uses LF line endings and therefore has a different raw byte identity:

- LF attachment size: `601005` bytes
- LF attachment lines: `5742`
- LF attachment SHA-256: `c5b16e6d137809163255bfa8ffb741eaa433a3703baff05fdbb40040563039bf`

This is **not** a new browser revision. Replacing each LF with CRLF produces exactly:

- `606747` bytes
- SHA-256 `012f4fcc3e14e91af9a87f61839d9b29dde76c29652d5b5120dd19b4f1ba8d21`

which is byte-for-byte the existing locked reference identity. The attached `index.html` can therefore be used directly for source inspection after normalizing line endings; no parity relock is required.

A separate attached `old_index.html` (Study 98) is **not** a behavioural replacement source. It is retained only as an explicit String timbre/acoustic reference; see `STRING_TIMBRE_REFERENCE.md` and `DECISIONS.md`.

## Important source landmarks

Line numbers refer to the locked 5742-line source above.

| Approx. line | Landmark |
| ---: | --- |
| 988 | `DEFAULTS` and top-level module defaults |
| 999 | top-level live `state` |
| 1032 | KEEP `snapshot()` contract |
| 1308 | MOTHER structural hold / pending logic |
| 1406 | DRIFT `beatDelta` / `beatPair` |
| 1483 | tuning / frequency helpers |
| 1840 | `derivedSequence()` for SPIRAL / Gravity relationship |
| 2003 | WebAudio engine root |
| 2533 | BLOOM bound-voice scheduling / synthesis |
| 2750 | central Clock scheduler / event ordering |
| 3316 | STRING drawing / geometry |
| 3391 | CLOCK drawing / three rings |
| 3573 | SPIRAL drawing / geometry |
| 3904 | GRAVITY drawing / 8x3 orbit |
| 4106 | BLOOM drawing / flower gesture |
| 4257 | primary pointer performance dispatcher |
| 4551 | KEEP snapshot apply / lane recall |
| 4615 | CHAIN max links and normalization |
| 4697 | CHAIN link application / morph |
| 4927 | parameter/depth panels including Chain and module edit surfaces |
| 5602 | MOTHER surface and five-source arcs |

## Rule for reference changes

Do not silently update the FM-1 port when `apsis.html` changes.

For a new browser revision:

1. record filename/size/line count/SHA-256;
2. diff identity-bearing behaviours against this lock;
3. classify changes as bugfix, UI-only, musical-model change or audio-model change;
4. update parity tests first;
5. then update native implementation.

This keeps the FM-1 port reproducible in the same way the pinned Dinghy commit keeps the platform reproducible.
