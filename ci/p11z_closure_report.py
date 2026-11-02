#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path


def one(pattern, text, label):
    matches = re.findall(pattern, text, re.M)
    if len(matches) != 1:
        raise SystemExit(f"FAIL: expected one {label} record, got {len(matches)}")
    return matches[0]


def ints(v):
    if isinstance(v, str):
        v = (v,)
    return tuple(int(x, 0) for x in v)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--runtime', type=Path, required=True)
    ap.add_argument('--attractor', type=Path, required=True)
    ap.add_argument('--string', type=Path, required=True)
    ap.add_argument('--body', type=Path, required=True)
    ap.add_argument('--clock', type=Path, required=True)
    ap.add_argument('--spiral', type=Path, required=True)
    ap.add_argument('--bloom', type=Path, required=True)
    ap.add_argument('--bloom-bound', type=Path, required=True)
    ap.add_argument('--target', type=Path, required=True)
    ap.add_argument('--idle', type=Path, required=True)
    ap.add_argument('--active-target', type=Path, required=True)
    ap.add_argument('--active', type=Path, required=True)
    ap.add_argument('--out-json', type=Path, required=True)
    ap.add_argument('--out-md', type=Path, required=True)
    a = ap.parse_args()

    runtime = a.runtime.read_text(errors='replace')
    attractor = a.attractor.read_text(errors='replace')
    string = a.string.read_text(errors='replace')
    body = a.body.read_text(errors='replace')
    clock = a.clock.read_text(errors='replace')
    spiral = a.spiral.read_text(errors='replace')
    bloom = a.bloom.read_text(errors='replace')
    bound = a.bloom_bound.read_text(errors='replace')
    target = json.loads(a.target.read_text())
    idle = json.loads(a.idle.read_text())
    active_target = json.loads(a.active_target.read_text())
    active = json.loads(a.active.read_text())

    rq, rqcap, rs, rscap, rb, rbcap = ints(one(
        r"M3c extreme burst \(queue=(\d+)/(\d+), spiral-slots=(\d+)/(\d+), bloom-slots=(\d+)/(\d+)\)",
        runtime, 'runtime extreme'))
    aq, aqcap, apend, apendcap = ints(one(
        r"M4c extreme shared timeline \(queue=(\d+)/(\d+), pending=(\d+)/(\d+),",
        attractor, 'attractor extreme'))
    sl, sp, ssteal, sdrop = ints(one(
        r"M4b1 pool logical=(\d+) physical=(\d+) steals=(\d+) drops=(\d+)",
        string, 'String transient'))
    bl, bp, bsteal, bdrop = ints(one(
        r"M4b composite pool logical=(\d+) physical=(\d+) steals=(\d+) drops=(\d+)",
        body, 'String BODY transient'))
    chw, chcap, ctrig, cdrop = ints(one(
        r"worst Clock pool high-water=(\d+)/(\d+) triggers=(\d+) drops=(\d+)",
        clock, 'Clock pool'))
    spiral_vals = ints(one(
        r"M4F_STRESS seconds=(\d+) hits=(\d+) logical_hw=(\d+) leg_hw=(\d+) osc_hw=(\d+) steals=(\d+) drops=(\d+)",
        spiral, 'Spiral stress'))
    btrig, bpetal, bpetalcap, bosc, bdrops, bpeak, bhash, bsize = one(
        r"M4G_STRESS triggers=(\d+) petals=(\d+)/(\d+) oscs=(\d+) drops=(\d+) peak=(\d+) hash=([0-9a-fA-F]+) sizeof=(\d+)",
        bloom, 'Bloom stress')
    bloom_vals = ints((btrig, bpetal, bpetalcap, bosc, bdrops, bpeak, bsize))
    proven, current, life, span43, span44, hostslot, hostsnap = ints(one(
        r"P11E_BLOOM_BOUND proven_max=(\d+) current_cap=(\d+) lifetime_samples=(\d+) "
        r"min_span_43_samples=(\d+) min_span_44_samples=(\d+) "
        r"host_runtime_slot_bytes=(\d+) host_audio_snapshot_bytes=(\d+)",
        bound, 'Bloom exact bound'))

    expected_capacity = {
        'runtime': (357, 512, 59, 64, 44, 48),
        'attractor': (415, 512, 60, 64),
        'string': (80, 112, 480, 0),
        'body': (80, 108, 410, 0),
        'clock': (23, 32, 176, 0),
        'spiral': (8, 384, 15, 30, 90, 0, 0),
        'bloom': (186, 125, 136, 375, 0, 266, 22864),
        'bloom_bound': (44, 48, 158540),
    }
    actual_capacity = {
        'runtime': (rq, rqcap, rs, rscap, rb, rbcap),
        'attractor': (aq, aqcap, apend, apendcap),
        'string': (sl, sp, ssteal, sdrop),
        'body': (bl, bp, bsteal, bdrop),
        'clock': (chw, chcap, ctrig, cdrop),
        'spiral': spiral_vals,
        'bloom': bloom_vals,
        'bloom_bound': (proven, current, life),
    }
    for k, exp in expected_capacity.items():
        if actual_capacity[k] != exp:
            raise SystemExit(f"FAIL: closure capacity {k} {actual_capacity[k]} != {exp}")
    if not (span43 < life < span44):
        raise SystemExit('FAIL: exact Bloom concurrency inequality lost')
    if rb != proven:
        raise SystemExit('FAIL: runtime Bloom high-water no longer equals proven legal maximum')

    expected_target = {
        'product': 'FM-1_972', 'image': 140472, 'image_max': 581564,
        'ram': 87524, 'ram_max': 98304, 'pool': 319176, 'pool_max': 344064,
        'ram_headroom': 10780, 'pool_headroom': 24888, 'package_bytes': 610110,
        'package_sha256': 'f9ca07eaee94843dcfa94ee7d05cdfade412ef221a1a8984243454a095ee3168',
    }
    for k, v in expected_target.items():
        if target.get(k) != v:
            raise SystemExit(f"FAIL: closure production target {k}={target.get(k)!r} != {v!r}")

    expected_idle = {
        'wav_raw_sha256': 'c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93',
        'runtime_cursor_low32': 123392,
        'runtime_queue_high_water': 28,
        'runtime_queue_overflowed': 0,
        'runtime_late_flags': 0,
        'runtime_late_trace_hash': '811c9dc5',
    }
    for k, v in expected_idle.items():
        if idle.get(k) != v:
            raise SystemExit(f"FAIL: closure idle {k}={idle.get(k)!r} != {v!r}")

    expected_active_target = {
        'product': 'FM-1_972', 'image': 137736, 'image_max': 581564,
        'ram': 87572, 'ram_max': 98304, 'pool': 319176, 'pool_max': 344064,
        'ram_headroom': 10732, 'pool_headroom': 24888, 'package_bytes': 610110,
        'package_sha256': '4685179ba074dfa090fc3a0603830d50c40cddae8dca97755f144cd0a885af63',
    }
    for k, v in expected_active_target.items():
        if active_target.get(k) != v:
            raise SystemExit(f"FAIL: closure active target {k}={active_target.get(k)!r} != {v!r}")

    # The active emulator script itself carries the complete lifecycle assertions.
    # Freeze the top-level telemetry here as a second independent closure check.
    for k, v in {
        'runtime_queue_high_water': 30,
        'runtime_queue_overflowed': 0,
        'runtime_late_flags': 0,
    }.items():
        if active.get(k) != v:
            raise SystemExit(f"FAIL: closure active {k}={active.get(k)!r} != {v!r}")

    report = {
        'phase': 'P11Z',
        'status': 'software_target_emulator_ready_for_phase12_hardware_validation',
        'production_authority': 'P11W',
        'capacity': {
            'runtime_queue_high_water': max(rq, aq),
            'runtime_queue_capacity': rqcap,
            'spiral_slots': [rs, rscap],
            'bloom_slots': [rb, rbcap],
            'bloom_proven_legal_max': proven,
            'string_transient': {'logical': [sl, 80], 'physical': [sp, 112], 'steals': ssteal, 'drops': sdrop},
            'string_attractor_pending': [apend, apendcap],
            'clock_pool': [chw, chcap],
            'spiral_leg_high_water': [spiral_vals[3], 32],
            'bloom_petal_high_water': [bloom_vals[1], bloom_vals[2]],
        },
        'production_target': target,
        'idle': {k: idle[k] for k in expected_idle},
        'active_target': active_target,
        'active_runtime': {k: active[k] for k in ('runtime_queue_high_water','runtime_queue_overflowed','runtime_late_flags')},
        'phase12_hardware_only': [
            'physical app_audio ISR/callback cycle margin under worst-case combined load',
            'physical runtime stack high-water and guard margin',
            'LCD DMA/audio bus contention and physical control/LED/display behavior',
            'real NOR erase/program timing and power-interruption behavior',
            'DAC/listening quality and Cathedral perceptual tuning',
            'real FM-1 install/rescue/update path',
        ],
    }
    a.out_json.parent.mkdir(parents=True, exist_ok=True)
    a.out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')

    md = [
        '# APSIS Phase 11Z closure report', '',
        '**Software / real-target / pinned-emulator closure: PASS.**', '',
        'P11Z introduces no production optimization. It revalidates the accepted P11W production stack as the final pre-hardware Phase-11 checkpoint.', '',
        '## Frozen production target', '',
        f"- image **{target['image']:,} / {target['image_max']:,} B**",
        f"- ordinary RAM **{target['ram']:,} / {target['ram_max']:,} B**; headroom **{target['ram_headroom']:,} B**",
        f"- POOL **{target['pool']:,} / {target['pool_max']:,} B**; headroom **{target['pool_headroom']:,} B**",
        f"- package SHA-256 `{target['package_sha256']}`", '',
        '## Bounded-capacity closure', '',
        f'- shared event queue high-water **{max(rq, aq)}/{rqcap}**',
        f'- Spiral late slots **{rs}/{rscap}**',
        f'- Bloom late slots **{rb}/{rbcap}**, exact legal maximum **{proven}**',
        f'- String transient pool **{sl}/80 logical, {sp}/112 physical**, steals {ssteal}, drops {sdrop}',
        f'- String Attractor pending **{apend}/{apendcap}**',
        f'- Clock pool **{chw}/{chcap}**',
        f'- Spiral legs **{spiral_vals[3]}/32**',
        f'- Bloom petals **{bloom_vals[1]}/{bloom_vals[2]}**', '',
        '## Frozen emulator closure', '',
        f"- idle PCM `{idle['wav_raw_sha256']}`",
        f"- idle cursor {idle['runtime_cursor_low32']:,}; queue {idle['runtime_queue_high_water']}/512; overflow {idle['runtime_queue_overflowed']}; late {idle['runtime_late_flags']}",
        f"- active target image/RAM/POOL **{active_target['image']:,} / {active_target['ram']:,} / {active_target['pool']:,} B**",
        f"- active queue {active['runtime_queue_high_water']}/512; overflow {active['runtime_queue_overflowed']}; late {active['runtime_late_flags']}", '',
        '## Deferred to Phase 12 — physical FM-1 only', '',
    ]
    md.extend(f'- {x}' for x in report['phase12_hardware_only'])
    md += ['', 'No physical-hardware claim is made by P11Z.', '']
    a.out_md.write_text('\n'.join(md))
    print(json.dumps(report, sort_keys=True))
    print('PASS: P11Z Phase-11 closure matrix is exact; remaining gates are physical Phase 12')


if __name__ == '__main__':
    main()
