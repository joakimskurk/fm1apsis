#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
[[ -d work/dinghy ]] || { echo "run ci/prepare_dinghy.sh first" >&2; exit 2; }

VERSION=${APSIS_VERSION:-0.1-dev}
rm -rf work/apsis
cp -a work/dinghy work/apsis
rm -rf work/apsis/.git work/apsis/build

cp overlay/dinghy.c work/apsis/firmware/src/dinghy.c
cp apsis/apsis_state.h apsis/apsis_state.c \
   apsis/apsis_harmony.h apsis/apsis_harmony.c \
   apsis/apsis_gravity.h apsis/apsis_gravity.c \
   apsis/apsis_string.h apsis/apsis_string.c \
   apsis/apsis_drift.h apsis/apsis_drift.c \
   apsis/apsis_clock.h apsis/apsis_clock.c \
   apsis/apsis_spiral.h apsis/apsis_spiral.c \
   apsis/apsis_bloom.h apsis/apsis_bloom.c \
   work/apsis/firmware/src/
python3 ci/patch_release_identity.py work/apsis/tools/build.py

# Host compilation is a structural gate for the single-translation-unit FM-1 app.
( cd work/apsis && APP_VER="$VERSION" bash tests/run_host.sh ) 2>&1 | tee build/evidence/apsis_host.log

printf 'PASS: APSIS overlay + host shell\n'
