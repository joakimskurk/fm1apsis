#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

PRODUCT=${APSIS_PRODUCT:-FM-1_972}
[[ "$PRODUCT" == "FM-1_972" ]] || { echo "invalid APSIS_PRODUCT: $PRODUCT" >&2; exit 2; }
[[ -d work/apsis ]] || { echo "run ci/apply_apsis.sh first" >&2; exit 3; }

if [[ ${GITHUB_REF_TYPE:-} == tag && ${GITHUB_REF_NAME:-} == v* ]]; then
  VERSION=${GITHUB_REF_NAME#v}
else
  VERSION=${APSIS_VERSION:-0.1-dev}
fi

# Phase 6's embedded Space wrapper advances the room at half the 44.1-kHz
# source rate. Retiming is intentionally applied only to the generated FM-1
# sources, never to the host/reference engine. Older phases have no target
# bridge file and therefore pass through unchanged. Guard the patch so repeated
# local target builds of the same generated tree remain harmless.
if [[ -f work/apsis/firmware/src/apsis_space_target.c && \
      -f work/apsis/firmware/src/apsis_space_room.c ]] && \
   grep -q '#define ROOM_FAST_GUARD_HP_LP_ALPHA_Q15 784u' \
      work/apsis/firmware/src/apsis_space_room.c; then
  python3 ci/patch_apsis_p6_half_rate.py | tee -a build/evidence/apsis_target.log
fi

( cd work/apsis && APSIS_PRODUCT="$PRODUCT" bash ./build.sh --release "$VERSION" ) 2>&1 | tee -a build/evidence/apsis_target.log
PKG="work/apsis/build/apsis-$VERSION.fwsc"
[[ -f "$PKG" ]] || { echo "missing target package: $PKG" >&2; exit 4; }
cp "$PKG" build/
sha256sum "build/$(basename "$PKG")" | tee build/SHA256SUMS

python3 ci/inspect_target_build.py \
  --log build/evidence/apsis_target.log \
  --package "build/$(basename "$PKG")" \
  --product "$PRODUCT" \
  --out build/target_report.json \
  | tee build/evidence/target_report.txt

printf 'PASS: APSIS target package %s (%s)\n' "$(basename "$PKG")" "$PRODUCT"