#!/usr/bin/env python3
"""P11E capacity/high-water inventory from frozen source-derived stress gates.

This is a measurement/reporting tool. It does not infer physical FM-1 timing,
stack use or hardware contention, and it does not authorize a capacity change.
"""

import argparse
import json
import re
from pathlib import Path


def one(pattern: str, text: str, label: str):
    matches = re.findall(pattern, text, re.M)
    if len(matches) != 1:
        raise SystemExit(f"FAIL: expected one {label} record, got {len(matches)}")
    return matches[0]


def ints(values):
    if isinstance(values, str):
        values = (values,)
    return tuple(int(v, 0) for v in values)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runtime", type=Path, required=True)
    ap.add_argument("--attractor", type=Path, required=True)
    ap.add_argument("--string", type=Path, required=True)
    ap.add_argument("--body", type=Path, required=True)
    ap.add_argument("--clock", type=Path, required=True)
    ap.add_argument("--spiral", type=Path, required=True)
    ap.add_argument("--bloom", type=Path, required=True)
    ap.add_argument("--bloom-bound", type=Path, required=True)
    ap.add_argument("--out-json", type=Path, required=True)
    ap.add_argument("--out-md", type=Path, required=True)
    a = ap.parse_args()

    runtime = a.runtime.read_text(errors="replace")
    attractor = a.attractor.read_text(errors="replace")
    string = a.string.read_text(errors="replace")
    body = a.body.read_text(errors="replace")
    clock = a.clock.read_text(errors="replace")
    spiral = a.spiral.read_text(errors="replace")
    bloom = a.bloom.read_text(errors="replace")
    bound = a.bloom_bound.read_text(errors="replace")

    rq, rqcap, rs, rscap, rb, rbcap = ints(one(
        r"M3c extreme burst \(queue=(\d+)/(\d+), spiral-slots=(\d+)/(\d+), bloom-slots=(\d+)/(\d+)\)",
        runtime, "M3c runtime extreme"))
    aq, aqcap, apend, apendcap = ints(one(
        r"M4c extreme shared timeline \(queue=(\d+)/(\d+), pending=(\d+)/(\d+),",
        attractor, "M4c attractor extreme"))
    sl, sp, ssteal, sdrop = ints(one(
        r"M4b1 pool logical=(\d+) physical=(\d+) steals=(\d+) drops=(\d+)",
        string, "String transient pool"))
    bl, bp, bsteal, bdrop = ints(one(
        r"M4b composite pool logical=(\d+) physical=(\d+) steals=(\d+) drops=(\d+)",
        body, "String BODY/composite pool"))
    chw, chcap, ctrig, cdrop = ints(one(
        r"worst Clock pool high-water=(\d+)/(\d+) triggers=(\d+) drops=(\d+)",
        clock, "Clock pool"))
    stress_seconds, spiral_hits, spiral_logical, spiral_legs, spiral_oscs, spiral_steals, spiral_drops = ints(one(
        r"M4F_STRESS seconds=(\d+) hits=(\d+) logical_hw=(\d+) leg_hw=(\d+) osc_hw=(\d+) steals=(\d+) drops=(\d+)",
        spiral, "Spiral DSP stress"))
    btrig, bpetal, bpetalcap, bosc, bdrops, bpeak, bhash, bsize = one(
        r"M4G_STRESS triggers=(\d+) petals=(\d+)/(\d+) oscs=(\d+) drops=(\d+) peak=(\d+) hash=([0-9a-fA-F]+) sizeof=(\d+)",
        bloom, "Bloom DSP stress")
    btrig, bpetal, bpetalcap, bosc, bdrops, bpeak, bsize = ints(
        (btrig, bpetal, bpetalcap, bosc, bdrops, bpeak, bsize))
    proven, current, life, span43, span44, hostslot, hostsnap = ints(one(
        r"P11E_BLOOM_BOUND proven_max=(\d+) current_cap=(\d+) lifetime_samples=(\d+) "
        r"min_span_43_samples=(\d+) min_span_44_samples=(\d+) "
        r"host_runtime_slot_bytes=(\d+) host_audio_snapshot_bytes=(\d+)",
        bound, "Bloom concurrency bound"))

    # Freeze the exact already-established stress results. A changed result is
    # evidence that must be investigated, not silently normalized by the report.
    expected = {
        "runtime": (357, 512, 59, 64, 44, 64),
        "attractor": (415, 512, 60, 64),
        "string": (80, 112, 480, 0),
        "body": (80, 108, 410, 0),
        "clock": (23, 32, 176, 0),
        "spiral": (8, 384, 15, 30, 90, 0, 0),
        "bloom": (186, 125, 136, 375, 0, 266),
        "bloom_bound": (44, 64, 158540),
    }
    actual = {
        "runtime": (rq, rqcap, rs, rscap, rb, rbcap),
        "attractor": (aq, aqcap, apend, apendcap),
        "string": (sl, sp, ssteal, sdrop),
        "body": (bl, bp, bsteal, bdrop),
        "clock": (chw, chcap, ctrig, cdrop),
        "spiral": (stress_seconds, spiral_hits, spiral_logical, spiral_legs,
                   spiral_oscs, spiral_steals, spiral_drops),
        "bloom": (btrig, bpetal, bpetalcap, bosc, bdrops, bpeak),
        "bloom_bound": (proven, current, life),
    }
    for key, exp in expected.items():
        if actual[key] != exp:
            raise SystemExit(f"FAIL: {key} capacity result {actual[key]} != frozen {exp}")

    if not (span43 < life < span44):
        raise SystemExit(
            f"FAIL: Bloom concurrency inequality lost: span43={span43}, life={life}, span44={span44}"
        )
    if rb != proven:
        raise SystemExit(f"FAIL: runtime Bloom high-water {rb} != proven maximum {proven}")

    max_queue = max(rq, aq)
    report = {
        "phase": "P11E",
        "scope": "source-derived bounded capacity/high-water inventory",
        "no_capacity_change": True,
        "runtime_queue": {
            "capacity": rqcap,
            "m3c_high_water": rq,
            "m4c_high_water": aq,
            "observed_max": max_queue,
            "reserve": rqcap - max_queue,
            "classification": "unknown reserve; no shrink without a stronger upper-bound proof",
        },
        "runtime_spiral_slots": {
            "capacity": rscap,
            "high_water": rs,
            "reserve": rscap - rs,
            "classification": "minimal reserve; veto shrink",
        },
        "runtime_bloom_slots": {
            "capacity": rbcap,
            "observed_high_water": rb,
            "proven_legal_max": proven,
            "reserve_above_proven_max": rbcap - proven,
            "lifetime_samples": life,
            "min_43_interval_span_samples": span43,
            "min_44_interval_span_samples": span44,
            "classification": "proven structural reserve; candidate for a separate falsification patch",
        },
        "string_transient_pool": {
            "logical_capacity": 80,
            "logical_high_water": sl,
            "physical_capacity": 112,
            "physical_high_water": sp,
            "steals": ssteal,
            "drops": sdrop,
            "body_composite_logical_high_water": bl,
            "body_composite_physical_high_water": bp,
            "body_composite_steals": bsteal,
            "body_composite_drops": bdrop,
            "classification": "saturated under legal burst; veto shrink",
        },
        "string_attractor_pending": {
            "capacity": apendcap,
            "high_water": apend,
            "reserve": apendcap - apend,
            "classification": "minimal reserve; veto shrink",
        },
        "clock_pool": {
            "capacity": chcap,
            "high_water": chw,
            "reserve": chcap - chw,
            "triggers": ctrig,
            "drops": cdrop,
            "classification": "measured reserve but not a priority RAM owner",
        },
        "spiral_dsp": {
            "leg_capacity": 32,
            "stress_seconds": stress_seconds,
            "hits": spiral_hits,
            "logical_high_water": spiral_logical,
            "leg_high_water": spiral_legs,
            "oscillator_high_water": spiral_oscs,
            "steals": spiral_steals,
            "drops": spiral_drops,
            "leg_reserve": 32 - spiral_legs,
            "classification": "one complete two-leg hit of reserve; veto shrink",
        },
        "bloom_dsp": {
            "petal_capacity": bpetalcap,
            "petal_high_water": bpetal,
            "reserve": bpetalcap - bpetal,
            "oscillator_high_water": bosc,
            "drops": bdrops,
            "peak": bpeak,
            "hash": bhash.lower(),
            "sizeof_host": bsize,
            "classification": "intentional bounded margin already source-stressed; no P11E shrink",
        },
        "host_projection_only": {
            "runtime_bloom_slot_bytes": hostslot,
            "bloom_audio_snapshot_bytes": hostsnap,
            "note": "Host sizeof values are projection aids only; P11F must measure the real JieLi target delta.",
        },
        "next_candidate": {
            "name": "APSIS_RUNTIME_BLOOM_SLOTS",
            "current": current,
            "proven_legal_max": proven,
            "rule": "Do not change in P11E. A later P11F may falsify a conservative reduced capacity on the real target with all frozen gates.",
        },
        "not_proven": [
            "physical FM-1 ISR/callback timing margin",
            "runtime stack high-water on physical hardware",
            "LCD DMA contention on physical hardware",
            "NOR timing or power-interruption behavior",
        ],
    }

    a.out_json.parent.mkdir(parents=True, exist_ok=True)
    a.out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    md = [
        "# APSIS Phase 11E capacity / high-water inventory",
        "",
        "P11E changes **no firmware capacity**. It collects the existing source-derived stress gates and adds an exact Bloom scheduler concurrency bound.",
        "",
        "| Structure | High-water / proven max | Capacity | Reserve | P11E decision |",
        "|---|---:|---:|---:|---|",
        f"| M3c/M4c shared event queue | {max_queue} | {rqcap} | {rqcap-max_queue} | unknown reserve; no shrink |",
        f"| M3c Spiral late slots | {rs} | {rscap} | {rscap-rs} | minimal; veto shrink |",
        f"| M3c Bloom late slots | **{proven} proven max** | {rbcap} | **{rbcap-proven}** | **candidate for separate P11F falsification** |",
        f"| String transient logical | {sl} | 80 | 0 | saturated; veto shrink |",
        f"| String transient physical | {sp} | 112 | 0 | saturated; veto shrink |",
        f"| String Attractor pending | {apend} | {apendcap} | {apendcap-apend} | minimal; veto shrink |",
        f"| Clock finite pool | {chw} | {chcap} | {chcap-chw} | measured reserve; low priority |",
        f"| Spiral DSP legs | {spiral_legs} | 32 | {32-spiral_legs} | one hit reserve; veto shrink |",
        f"| Bloom DSP petals | {bpetal} | {bpetalcap} | {bpetalcap-bpetal} | retain current stress margin |",
        "",
        "## Exact Bloom late-slot proof",
        "",
        f"- real M3c extreme trace reaches **{rb}** simultaneous Bloom slots;",
        f"- legal maximum slot lifetime is **{life:,} samples** (3595 ms at 44.1 kHz);",
        f"- the shortest span of 43 fastest-pulse intervals is **{span43:,} samples**, so 44 concurrent occurrences are reachable;",
        f"- the shortest span of 44 fastest-pulse intervals is **{span44:,} samples**, which exceeds the slot lifetime;",
        "- `apsis_transport_schedule_next()` emits at most one Bloom event for a pulse; n=3/rate=4/cycles=1 reaches one per pulse after the source first-cycle guard;",
        "- therefore **45 simultaneous Bloom scheduler slots are impossible under the locked legal contract, and 44 is both reachable and the exact upper bound**.",
        "",
        "This proof is about scheduler occupancy, not physical CPU timing.",
        "",
        "## Candidate selection",
        "",
        "P11E authorizes **no size change**. It identifies only one high-value RAM candidate with a proven structural reserve: `APSIS_RUNTIME_BLOOM_SLOTS`, which also sizes `ApsisBloomAudioRuntime.snapshot[]`.",
        "",
        f"Host layout reports {hostslot} B per runtime Bloom slot and {hostsnap} B per audio snapshot. These are projection aids only; any later P11F reduction must measure the actual JieLi `.data/.bss` delta and preserve exact frozen idle/active behavior.",
        "",
        "## Explicit vetoes",
        "",
        "- Do not shrink String transient logical or physical capacity: the legal 30-ms burst saturates both and already relies on stealing.",
        "- Do not shrink Spiral DSP legs or M3c Spiral slots: their measured reserve is minimal.",
        "- Do not shrink String Attractor pending capacity: 60/64 is already reached.",
        "- Do not shrink the shared event queue from high-water alone: 415/512 is observed but no global mathematical upper bound has been proved.",
        "- Do not infer stack or ISR margin from these host/source tests.",
        "",
        "## Next gate",
        "",
        "P11F may test a conservative reduced Bloom-slot capacity on an isolated staging branch. The patch is acceptable only if the real JieLi RAM saving is measured and every relevant frozen host, target, idle-emulator and active-lifecycle gate remains green.",
        "",
    ]
    a.out_md.write_text("\n".join(md))

    print(json.dumps({
        "queue_max": max_queue,
        "string_physical": [sp, 112],
        "spiral_slots": [rs, rscap],
        "spiral_legs": [spiral_legs, 32],
        "attractor_pending": [apend, apendcap],
        "bloom_slots_observed": rb,
        "bloom_slots_proven_max": proven,
        "bloom_slots_capacity": rbcap,
        "bloom_petals": [bpetal, bpetalcap],
        "candidate": "APSIS_RUNTIME_BLOOM_SLOTS",
    }, sort_keys=True))
    print("PASS: P11E bounded capacity inventory generated; no capacity changed")


if __name__ == "__main__":
    main()
