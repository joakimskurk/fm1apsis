#!/usr/bin/env bash
set -euo pipefail

mkdir -p build
SRC=(
  apsis/apsis_state.c
  apsis/apsis_harmony.c
  apsis/apsis_gravity.c
  apsis/apsis_string.c
  apsis/apsis_string_dsp.c
  apsis/apsis_string_transient_pool.c
  apsis/apsis_drift.c
  apsis/apsis_drift_dsp.c
  apsis/apsis_persistent_audio.c
)
COMMON=(-std=c11 -Wall -Wextra -Werror -pedantic -Iapsis "${SRC[@]}")

cc -O2 "${COMMON[@]}" tests/test_persistent_audio.c -o build/test_persistent_audio
build/test_persistent_audio
cc -O2 "${COMMON[@]}" tests/test_persistent_audio_segment.c -o build/test_persistent_audio_segment
build/test_persistent_audio_segment

cc -O1 -g -fsanitize=address,undefined "${COMMON[@]}" tests/test_persistent_audio.c -o build/san_persistent_audio
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  build/san_persistent_audio
cc -O1 -g -fsanitize=address,undefined "${COMMON[@]}" tests/test_persistent_audio_segment.c -o build/san_persistent_audio_segment
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  build/san_persistent_audio_segment
