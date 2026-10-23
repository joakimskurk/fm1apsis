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

( cd work/apsis && APSIS_PRODUCT="$PRODUCT" bash ./build.sh --release "$VERSION" ) 2>&1 | tee build/evidence/apsis_target.log
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
