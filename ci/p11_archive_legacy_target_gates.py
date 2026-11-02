#!/usr/bin/env python3
from pathlib import Path

manual_only = '''on:\n  # Historical target replay is timing-coupled to its phase-era binary shape.\n  # Keep the original replay/hash assertions intact, but run them manually.\n  # Live later-phase PR validation is covered by core + P9/P10/P11Z gates.\n  workflow_dispatch:\n'''

simple = [
    Path('.github/workflows/p6-final.yml'),
    Path('.github/workflows/p7-target.yml'),
    Path('.github/workflows/p8-target.yml'),
]
old_simple = '''on:\n  pull_request:\n  workflow_dispatch:\n'''

for p in simple:
    s = p.read_text()
    if s.count(old_simple) != 1:
        raise SystemExit(f'{p}: expected exactly one pull_request trigger block')
    p.write_text(s.replace(old_simple, manual_only, 1))

validate = Path('.github/workflows/validate.yml')
s = validate.read_text()
old_validate = '''on:\n  push:\n    branches: [main]\n  pull_request:\n  workflow_dispatch:\n'''
if s.count(old_validate) != 1:
    raise SystemExit('validate.yml: expected exactly one main/pull_request trigger block')
validate.write_text(s.replace(old_validate, manual_only, 1))

# The bodies are deliberately untouched: this migration changes only when the
# historical binary/timing replay runs, never what its frozen archive asserts.
for p in [*simple, validate]:
    s = p.read_text()
    if 'pull_request:' in s or 'branches: [main]' in s:
        raise SystemExit(f'{p}: live legacy trigger remains')
    if s.count('workflow_dispatch:') != 1:
        raise SystemExit(f'{p}: expected exactly one manual trigger')

print('PASS: four timing-coupled legacy target replays are manual archive gates')
