#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
[[ -d work/dinghy ]] || { echo "run ci/prepare_dinghy.sh first" >&2; exit 2; }
mkdir -p build/evidence

( cd work/dinghy && bash tests/run_host.sh ) 2>&1 | tee build/evidence/dinghy_host.log
( cd work/dinghy && bash ./build.sh ) 2>&1 | tee build/evidence/dinghy_target.log

printf 'PASS: unchanged Dinghy host + target baseline\n'
