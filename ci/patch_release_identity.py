#!/usr/bin/env python3
import argparse
from pathlib import Path

EXPECTED_PRODUCT = "FM-1_972"

ap = argparse.ArgumentParser()
ap.add_argument("build_py", type=Path)
a = ap.parse_args()
p = a.build_py
s = p.read_text()
orig = s

required = [
    'PRODUCT = "FM-1_990"',
    'PRODUCT = "FM-1_99" + m[2]',
    'name = "dinghy.fwsc"',
    'name = f"dinghy-{a.release}.fwsc"',
    'fm1pkg_make.SDK = a.sdk',
]
missing = [x for x in required if x not in s]
if missing:
    raise SystemExit("refusing unknown Dinghy build.py; missing: " + ", ".join(missing))

s = s.replace(
    'PRODUCT = "FM-1_990"                # Dinghy: 99N (OUR_LOADER in the installer wants FM-1_9NN). Your fork: pick your own',
    'PRODUCT = os.environ.get("APSIS_PRODUCT", "FM-1_972")  # fixed APSIS product identity',
    1,
)
s = s.replace('name = "dinghy.fwsc"', 'name = "apsis-dev.fwsc"', 1)
s = s.replace(
    'PRODUCT = "FM-1_99" + m[2]',
    'if PRODUCT != "FM-1_972":\n            raise SystemExit("APSIS_PRODUCT must remain FM-1_972")',
    1,
)
s = s.replace('name = f"dinghy-{a.release}.fwsc"', 'name = f"apsis-{a.release}.fwsc"', 1)
s = s.replace(
    'fm1pkg_make.SDK = a.sdk',
    'if PRODUCT != "FM-1_972":\n        raise SystemExit("APSIS_PRODUCT must remain FM-1_972")\n    fm1pkg_make.SDK = a.sdk',
    1,
)

if s == orig:
    raise SystemExit("no changes made")
if 'PRODUCT = "FM-1_990"' in s:
    raise SystemExit("postcondition failed: Dinghy identity remains")
if 'PRODUCT = "FM-1_99" + m[2]' in s:
    raise SystemExit("postcondition failed: release identity override remains")
if 'apsis-dev.fwsc' not in s or 'apsis-{a.release}.fwsc' not in s:
    raise SystemExit("postcondition failed: APSIS package naming missing")

p.write_text(s)
print(f"patched APSIS identity/package naming: {p} -> {EXPECTED_PRODUCT}")
