#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
mkdir -p build

SRC=(
  apsis/apsis_state.c
  apsis/apsis_harmony.c
  apsis/apsis_gravity.c
  apsis/apsis_string.c
  apsis/apsis_string_dsp.c
  apsis/apsis_string_transient_pool.c
  apsis/apsis_string_rhythm_audio.c
  apsis/apsis_string_attractor.c
  apsis/apsis_string_attractor_audio.c
  apsis/apsis_drift.c
  apsis/apsis_drift_dsp.c
  apsis/apsis_clock.c
  apsis/apsis_clock_dsp.c
  apsis/apsis_spiral.c
  apsis/apsis_spiral_audio.c
  apsis/apsis_spiral_audio_runtime.c
  apsis/apsis_bloom.c
  apsis/apsis_keep.c
  apsis/apsis_chain.c
  apsis/apsis_mother.c
  apsis/apsis_transport.c
  apsis/apsis_late_binding.c
  apsis/apsis_runtime_m4f.c
)
COMMON=(-std=c11 -Wall -Wextra -Werror -pedantic -Iapsis "${SRC[@]}" tests/test_spiral_audio_runtime.c)

cc -O2 "${COMMON[@]}" -o build/test_spiral_audio_runtime
build/test_spiral_audio_runtime

cc -O1 -g -fsanitize=address,undefined "${COMMON[@]}" -o build/san_spiral_audio_runtime
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
  build/san_spiral_audio_runtime
