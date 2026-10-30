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
  apsis/apsis_bloom_audio.c
  apsis/apsis_bloom_audio_runtime.c
  tests/test_bloom_audio_runtime.c
)

cc -O2 "${COMMON[@]}" -o build/test_bloom_audio_runtime
build/test_bloom_audio_runtime

cc -O1 -g -fsanitize=address,undefined "${COMMON[@]}" -o build/san_bloom_audio_runtime
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
  build/san_bloom_audio_runtime
