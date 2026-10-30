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
  apsis/apsis_bloom_audio.c
  apsis/apsis_bloom_audio_runtime.c
  apsis/apsis_keep.c
  apsis/apsis_chain.c
  apsis/apsis_mother.c
  apsis/apsis_transport.c
  apsis/apsis_late_binding.c
  apsis/apsis_persistent_audio.c
  apsis/apsis_runtime_m5a.c
  apsis/apsis_runtime_m5a_probe.c
)
COMMON=(-std=c11 -Wall -Wextra -Werror -pedantic -Iapsis "${SRC[@]}")

cc -O2 "${COMMON[@]}" tests/test_structural_audio_runtime.c \
  -o build/test_structural_audio_runtime
build/test_structural_audio_runtime

cc -O1 -g -fsanitize=address,undefined "${COMMON[@]}" \
  tests/test_structural_audio_runtime.c -o build/san_structural_audio_runtime
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
  build/san_structural_audio_runtime

cc -O2 "${COMMON[@]}" tests/test_structural_audio_capacity.c \
  -o build/test_structural_audio_capacity
build/test_structural_audio_capacity

cc -O1 -g -fsanitize=address,undefined "${COMMON[@]}" \
  tests/test_structural_audio_capacity.c -o build/san_structural_audio_capacity
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
  build/san_structural_audio_capacity
