#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

PRODUCT=${APSIS_PRODUCT:-FM-1_972}
DINGHY_COMMIT=d44df7db09b98ccad2f1233b8f83f5f09c04829e
DINGHY_TREE=8cefd253b92b544109a315fd2fb9ffcea7c429b6

if [[ "$PRODUCT" != "FM-1_972" ]]; then
  echo "invalid APSIS_PRODUCT: $PRODUCT (APSIS is reserved as FM-1_972)" >&2
  exit 2
fi

if [[ ${GITHUB_REF_TYPE:-} == tag && ${GITHUB_REF_NAME:-} == v* ]]; then
  VERSION=${GITHUB_REF_NAME#v}
else
  VERSION=${APSIS_VERSION:-0.1-dev}
fi

rm -rf work build
mkdir -p work build/evidence

make test | tee build/evidence/core_test.log
make sanitize | tee build/evidence/core_sanitize.log

# 1. Reconstruct the exact upstream baseline and prove both commit and tree.
git init -q work/dinghy
(
  cd work/dinghy
  git remote add origin https://github.com/zvenson/dinghy.git
  git fetch -q --depth 1 origin "$DINGHY_COMMIT"
  git checkout -q --detach FETCH_HEAD
  [[ $(git rev-parse HEAD) == "$DINGHY_COMMIT" ]]
  [[ $(git rev-parse 'HEAD^{tree}') == "$DINGHY_TREE" ]]
)
cp UPSTREAM.lock.json build/evidence/

# 2. Baseline must pass before APSIS touches it.
( cd work/dinghy && bash tests/run_host.sh ) 2>&1 | tee build/evidence/dinghy_host.log
( cd work/dinghy && bash ./build.sh ) 2>&1 | tee build/evidence/dinghy_target.log

# 3. Apply only the APSIS overlay/core to a clean copy.
cp -a work/dinghy work/apsis
rm -rf work/apsis/.git
cp overlay/dinghy.c work/apsis/firmware/src/dinghy.c
cp apsis/apsis_state.h apsis/apsis_state.c apsis/apsis_harmony.h apsis/apsis_harmony.c work/apsis/firmware/src/
python3 ci/patch_release_identity.py work/apsis/tools/build.py

# 4. Host proves the one-translation-unit integration before target build.
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) 2>&1 | tee build/evidence/apsis_host.log

# 5. Build the real target package with APSIS' fixed identity.
( cd work/apsis && APSIS_PRODUCT="$PRODUCT" bash ./build.sh --release "$VERSION" ) 2>&1 | tee build/evidence/apsis_target.log
PKG="work/apsis/build/apsis-$VERSION.fwsc"
[[ -f "$PKG" ]] || { echo "missing target package: $PKG" >&2; exit 3; }
cp "$PKG" build/
sha256sum "build/$(basename "$PKG")" | tee build/SHA256SUMS

python3 ci/inspect_target_build.py \
  --log build/evidence/apsis_target.log \
  --package "build/$(basename "$PKG")" \
  --product "$PRODUCT" \
  --out build/target_report.json \
  | tee build/evidence/target_report.txt

bash ci/emulator_smoke.sh "build/$(basename "$PKG")" | tee build/evidence/emulator_smoke.txt
cp -a work/emu-smoke build/evidence/emulator-smoke

printf 'PASS: APSIS Phase 1 target package %s (%s)\n' "$(basename "$PKG")" "$PRODUCT"
