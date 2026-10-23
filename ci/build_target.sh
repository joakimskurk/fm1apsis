#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

PRODUCT=${APSIS_PRODUCT:-FM-1_972}
[[ "$PRODUCT" == "FM-1_972" ]] || { echo "invalid APSIS_PRODUCT: $PRODUCT" >&2; exit 2; }

bash ci/prepare_dinghy.sh
bash ci/build_baseline.sh
bash ci/apply_apsis.sh
bash ci/build_apsis_target.sh

if [[ ${GITHUB_REF_TYPE:-} == tag && ${GITHUB_REF_NAME:-} == v* ]]; then
  VERSION=${GITHUB_REF_NAME#v}
else
  VERSION=${APSIS_VERSION:-0.1-dev}
fi
PKG="build/apsis-$VERSION.fwsc"

bash ci/emulator_smoke.sh "$PKG" | tee build/evidence/emulator_smoke.txt
cp -a work/emu-smoke build/evidence/emulator-smoke

printf 'PASS: APSIS Phase 1 target package %s (%s)\n' "$(basename "$PKG")" "$PRODUCT"
