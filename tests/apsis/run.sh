#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# APSIS's own host tests: every tests/apsis/test_*.c against the apsis/ sources the firmware ships, once plainly
# (-Werror) and once under AddressSanitizer + UndefinedBehaviorSanitizer. Linux or macOS, from the repo root:
#   sh tests/apsis/run.sh            (SAN=0: skip the sanitizer pass; CC=clang MSAN=1: MemorySanitizer, Linux)
set -e
cd "$(dirname "$0")/../.."
OUT=build/apsis-tests
CC="${CC:-cc}"
fail=0

pass() {   # $1 name, $2 flags
    mkdir -p "$OUT/$1"
    for f in apsis/*.c; do
        b=$(basename "$f" .c)
        case $b in apsis_project_fm1|apsis_target_div64) continue ;; esac   # (the app's flash; the chip's division)
        $CC $2 -Iapsis -c "$f" -o "$OUT/$1/$b.o"
    done
    for t in tests/apsis/test_*.c; do
        b=$(basename "$t" .c)
        if ! $CC $2 -Iapsis "$OUT/$1"/apsis_*.o "$t" -o "$OUT/$1/$b" 2> "$OUT/$1/$b.log"; then
            echo "FAIL (build) $1 $b"; cat "$OUT/$1/$b.log"; fail=1; continue
        fi
        if "$OUT/$1/$b" > "$OUT/$1/$b.out" 2>&1; then
            echo "ok   $1 $b"
        else
            echo "FAIL $1 $b"; tail -5 "$OUT/$1/$b.out"; fail=1
        fi
    done
}

pass plain "-std=c11 -O2 -Wall -Wextra -Werror -pedantic"
[ "${SAN:-1}" = 0 ] || pass san "-std=c11 -O1 -g -Wall -Wextra -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all"
[ "${MSAN:-0}" = 0 ] || pass msan "-std=c11 -O1 -g -Wall -Wextra -pedantic -fsanitize=memory -fsanitize-memory-track-origins=2"

[ "$fail" = 0 ] && echo "APSIS TESTS PASSED" || echo "APSIS TESTS FAILED"
exit "$fail"
