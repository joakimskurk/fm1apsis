#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build

COMMON=(
  -std=c11 -Wall -Wextra -Werror -pedantic -Iapsis
  apsis/apsis_state.c
  apsis/apsis_harmony.c
  apsis/apsis_gravity.c
  apsis/apsis_drift.c
  apsis/apsis_spiral_audio.c
  tests/test_spiral_audio_plan.c
)

cc -O2 "${COMMON[@]}" -o build/test_spiral_audio_plan
build/test_spiral_audio_plan

cc -O1 -g -fsanitize=address,undefined "${COMMON[@]}" -o build/san_spiral_audio_plan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
  build/san_spiral_audio_plan
