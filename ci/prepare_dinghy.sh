#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

DINGHY_COMMIT=d44df7db09b98ccad2f1233b8f83f5f09c04829e
DINGHY_TREE=8cefd253b92b544109a315fd2fb9ffcea7c429b6

rm -rf work build
mkdir -p work build/evidence

make test | tee build/evidence/core_test.log
make sanitize | tee build/evidence/core_sanitize.log

git init -q work/dinghy
(
  cd work/dinghy
  git remote add origin https://github.com/zvenson/dinghy.git
  git fetch -q --depth 1 origin "$DINGHY_COMMIT"
  git checkout -q --detach FETCH_HEAD
  actual_commit=$(git rev-parse HEAD)
  actual_tree=$(git rev-parse 'HEAD^{tree}')
  [[ "$actual_commit" == "$DINGHY_COMMIT" ]] || { echo "Dinghy commit mismatch: $actual_commit" >&2; exit 2; }
  [[ "$actual_tree" == "$DINGHY_TREE" ]] || { echo "Dinghy tree mismatch: $actual_tree" >&2; exit 3; }
)
cp UPSTREAM.lock.json build/evidence/
printf '%s\n' "$DINGHY_COMMIT" > build/evidence/dinghy_commit.txt
printf '%s\n' "$DINGHY_TREE" > build/evidence/dinghy_tree.txt
printf 'PASS: pinned Dinghy source verified\n'
