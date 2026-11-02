#!/usr/bin/env python3
import argparse
import json
from pathlib import Path

EXPECTED = {
    "product": "FM-1_972",
    "image": 140152,
    "image_max": 581564,
    "ram": 89684,
    "ram_max": 98304,
    "pool": 328740,
    "pool_max": 344064,
    "phase9_pool": 309532,
    "runtime": 21928,
    "motion_store": 19208,
    "idle_cursor": 123392,
    "idle_queue": 28,
    "idle_pcm": "c4fa18954cd5d5ecc895ec9457d673b8bdbe0f946c0d2aa3efa3d28f77c60f93",
    "late_trace": "811c9dc5",
}


def fail(msg: str) -> None:
    raise SystemExit(f"FAIL: {msg}")


def pct(used: int, maximum: int) -> float:
    return round(100.0 * used / maximum, 3)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", type=Path, required=True)
    ap.add_argument("--emulator", type=Path, required=True)
    ap.add_argument("--sizes", type=Path, required=True)
    ap.add_argument("--out-json", type=Path, required=True)
    ap.add_argument("--out-md", type=Path, required=True)
    args = ap.parse_args()

    target = json.loads(args.target.read_text())
    emu = json.loads(args.emulator.read_text())
    sizes = json.loads(args.sizes.read_text())

    for key in ("product", "image", "image_max", "ram", "ram_max", "pool", "pool_max"):
        expected = EXPECTED[key]
        if target.get(key) != expected:
            fail(f"target {key}={target.get(key)!r} != {expected!r}")

    if sizes.get("apsis_runtime_bytes") != EXPECTED["runtime"]:
        fail(f"ApsisRuntime={sizes.get('apsis_runtime_bytes')} != {EXPECTED['runtime']}")
    if sizes.get("motion_store_bytes") != EXPECTED["motion_store"]:
        fail(f"MotionStore={sizes.get('motion_store_bytes')} != {EXPECTED['motion_store']}")

    motion_pool_delta = target["pool"] - EXPECTED["phase9_pool"]
    if motion_pool_delta != sizes["motion_store_bytes"]:
        fail(
            "Phase-10 POOL delta does not equal MotionStore: "
            f"{motion_pool_delta} != {sizes['motion_store_bytes']}"
        )

    emulator_expect = {
        "wav_raw_sha256": EXPECTED["idle_pcm"],
        "runtime_cursor_low32": EXPECTED["idle_cursor"],
        "runtime_queue_high_water": EXPECTED["idle_queue"],
        "runtime_queue_overflowed": 0,
        "runtime_late_flags": 0,
        "runtime_late_trace_hash": EXPECTED["late_trace"],
        "dynamic_home_surface": True,
    }
    for key, expected in emulator_expect.items():
        if emu.get(key) != expected:
            fail(f"emulator {key}={emu.get(key)!r} != {expected!r}")

    resources = {}
    for name in ("image", "ram", "pool"):
        used = target[name]
        maximum = target[f"{name}_max"]
        resources[name] = {
            "used": used,
            "max": maximum,
            "headroom": maximum - used,
            "used_percent": pct(used, maximum),
            "headroom_percent": round(100.0 - pct(used, maximum), 3),
        }

    report = {
        "phase": "P11A",
        "baseline": "Phase-10 merged production target",
        "product": target["product"],
        "resources": resources,
        "sizes": sizes,
        "pool_accounting": {
            "phase9_pool_bytes": EXPECTED["phase9_pool"],
            "phase10_pool_bytes": target["pool"],
            "phase10_pool_delta_bytes": motion_pool_delta,
            "motion_store_bytes": sizes["motion_store_bytes"],
            "unexplained_pool_growth_bytes": motion_pool_delta - sizes["motion_store_bytes"],
        },
        "idle_regression": {
            "pcm_sha256": emu["wav_raw_sha256"],
            "cursor": emu["runtime_cursor_low32"],
            "queue_high_water": emu["runtime_queue_high_water"],
            "overflow": emu["runtime_queue_overflowed"],
            "late_flags": emu["runtime_late_flags"],
            "late_trace": emu["runtime_late_trace_hash"],
        },
        "package": {
            "bytes": target.get("package_bytes"),
            "sha256": target.get("package_sha256"),
        },
        "not_measured_without_additional_instrumentation_or_hardware": [
            "target stack high-water / worst-case stack headroom",
            "app_audio ISR/callback cycle cost and deadline margin on physical FM-1",
            "worst-case combined DSP + UI + control timing on physical FM-1",
            "LCD DMA wall-time/contention on physical FM-1",
            "NOR erase/program timing and power-interruption behaviour on physical FM-1",
            "DAC/listening quality and Cathedral perceptual tuning",
        ],
    }

    args.out_json.parent.mkdir(parents=True, exist_ok=True)
    args.out_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    md = [
        "# APSIS Phase 11A baseline inventory",
        "",
        "Measured from the merged Phase-10 production stack. This is a baseline, not an optimisation result.",
        "",
        "## Target resources",
        "",
        "| Resource | Used | Max | Headroom | Used |",
        "|---|---:|---:|---:|---:|",
    ]
    for name in ("image", "ram", "pool"):
        r = resources[name]
        md.append(
            f"| {name.upper()} | {r['used']:,} B | {r['max']:,} B | "
            f"{r['headroom']:,} B | {r['used_percent']:.3f}% |"
        )
    md += [
        "",
        "## Key ownership facts",
        "",
        f"- `sizeof(ApsisRuntime)` = **{sizes['apsis_runtime_bytes']:,} B**.",
        f"- MotionStore = **{sizes['motion_store_bytes']:,} B** ({sizes['motion_points_per_lane']} points/lane at {sizes['motion_capture_hz']} Hz capture bound).",
        f"- Phase-9 → Phase-10 POOL delta = **{motion_pool_delta:,} B**.",
        f"- Unexplained Phase-10 POOL growth = **{report['pool_accounting']['unexplained_pool_growth_bytes']} B**.",
        "- Therefore the complete Phase-10 POOL increase is accounted for by MotionStore; the advanced Attractor/Gravity editors add no POOL allocation.",
        "",
        "## Frozen idle regression",
        "",
        f"- PCM SHA-256 `{emu['wav_raw_sha256']}`",
        f"- cursor {emu['runtime_cursor_low32']:,}",
        f"- queue high-water {emu['runtime_queue_high_water']}/512",
        f"- overflow {emu['runtime_queue_overflowed']}; late flags {emu['runtime_late_flags']}; late trace `{emu['runtime_late_trace_hash']}`",
        "",
        "## Explicitly not measured yet",
        "",
    ]
    md.extend(f"- {item}" for item in report["not_measured_without_additional_instrumentation_or_hardware"])
    md += [
        "",
        "Phase 11 optimisation work must preserve this baseline unless a changed invariant is explicitly re-approved with evidence.",
        "",
    ]
    args.out_md.write_text("\n".join(md))

    print(json.dumps(report, sort_keys=True))
    print("PASS: P11A merged Phase-10 resource/ownership baseline is exact")


if __name__ == "__main__":
    main()
