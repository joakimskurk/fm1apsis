#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The whole firmware (firmware/src/app.c) on this computer in Dinghy's host mode (tests/host.c): each script in
# tests/scripts/apsis_*.txt (and the default one) plays, the sound goes to build/host/<script>.wav, the screens to
# PNG. Each WAV's SHA-256 is checked against tests/sound.sha256, so a change that alters the sound shows up as one.
# When the sound is meant to change: UPDATE=1 sh tests/sound.sh, and say why in the commit.
# Linux (the host build puts the canvas in an ELF section; on macOS run it in a Linux container).
set -e
cd "$(dirname "$0")/.."
mkdir -p build/host
cc -O2 -Wall -Wno-unused-function -DDINGHY_HOST -DAPP_VER="\"dev\"" -Ifirmware/hal -Ifirmware/src -Ifirmware/gen \
   -Iapsis -o build/host/apsis firmware/src/app.c
: > build/host/sound.sha256
run() {   # $1 name, $2 script (or none)
    (cd build/host && ./apsis "$1" ${2:+"../../$2"} > "$1.log")
    python3 - "build/host/$1.wav" <<'PY'
import math, struct, sys
d = open(sys.argv[1], "rb").read()[44:]
v = struct.unpack("<%dh" % (len(d) // 2), d)
rms = math.sqrt(sum(x * x for x in v) / max(1, len(v)))
db = lambda x: 20 * math.log10(x / 32767) if x else -999
print(f"  {sys.argv[1]}: {len(v) // 2} frames, peak {max(map(abs, v))} ({db(max(map(abs, v))):.1f} dBFS), "
      f"rms {rms:.0f} ({db(rms):.1f} dBFS)")
PY
    echo "$(sha256sum "build/host/$1.wav" | cut -d' ' -f1)  $1" >> build/host/sound.sha256
}
run default ""
for s in tests/scripts/apsis_*.txt; do run "$(basename "$s" .txt)" "$s"; done
for p in build/host/*.ppm; do
    [ -f "$p" ] && python3 -c "import sys; from PIL import Image; Image.open(sys.argv[1]).resize((480, 480), Image.NEAREST).save(sys.argv[1][:-4] + '.png')" "$p" && rm "$p"
done
if [ "${UPDATE:-0}" = 1 ]; then
    cp build/host/sound.sha256 tests/sound.sha256
    echo "tests/sound.sha256 updated"
elif diff -u tests/sound.sha256 build/host/sound.sha256; then
    echo "SOUND UNCHANGED"
else
    echo "SOUND CHANGED (if meant: UPDATE=1 sh tests/sound.sh)"
    exit 1
fi
