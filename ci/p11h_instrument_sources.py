#!/usr/bin/env python3
"""Create test-only instrumented copies of P11H workload sources.

Production APSIS sources are never modified. The generated copies include
`tests/p11h_probe.h` and increment deterministic operation counters used by the
host workload harness. Exact-string anchors deliberately fail closed if the
production implementation moves.
"""
from pathlib import Path
import argparse
import shutil


def replace_once(text: str, old: str, new: str, label: str) -> str:
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"FAIL: {label}: expected one anchor, found {n}")
    return text.replace(old, new, 1)


def instrument(src: Path, dst: Path, replacements):
    text = src.read_text()
    first_nl = text.find("\n")
    if first_nl < 0:
        raise SystemExit(f"FAIL: no include line in {src}")
    text = text[: first_nl + 1] + '#include "p11h_probe.h"\n' + text[first_nl + 1 :]
    for label, old, new in replacements:
        text = replace_once(text, old, new, f"{src.name}:{label}")
    dst.write_text(text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    instrument(Path("apsis/apsis_space_target.c"), out / "apsis_space_target.c", [
        ("target render count",
         "    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;\n\n    /* Source engines",
         "    if (frames > APSIS_SPACE_ROOM_BLOCK_MAX) frames = APSIS_SPACE_ROOM_BLOCK_MAX;\n"
         "    apsis_p11h_probe.target_render_calls++;\n"
         "    apsis_p11h_probe.output_frames += frames;\n\n    /* Source engines"),
        ("room frame count",
         "    room_frames = complete_groups + (remainder ? 1u : 0u);\n    if (!room_frames) return;\n",
         "    room_frames = complete_groups + (remainder ? 1u : 0u);\n    if (!room_frames) return;\n"
         "    apsis_p11h_probe.room_frames += room_frames;\n"),
    ])

    instrument(Path("apsis/apsis_space_room.c"), out / "apsis_space_room.c", [
        ("room mul count",
         "static int32_t space_room_mul_q15(int32_t a, int32_t b)\n{\n    return space_room_round_shift((int64_t)a * b, 15u);\n}",
         "static int32_t space_room_mul_q15(int32_t a, int32_t b)\n{\n"
         "    apsis_p11h_probe.room_mul_q15_calls++;\n"
         "    return space_room_round_shift((int64_t)a * b, 15u);\n}"),
        ("biquad count",
         "    if (!s || !c) return x_q15;\n\n    /* Keep 15 extra signal bits",
         "    if (!s || !c) return x_q15;\n"
         "    apsis_p11h_probe.room_biquad_calls++;\n\n    /* Keep 15 extra signal bits"),
        ("delay read count",
         "    int64_t mixed;\n    if (!storage || !capacity_frames || channel > 1u) return 0;",
         "    int64_t mixed;\n"
         "    apsis_p11h_probe.delay_read_calls++;\n"
         "    if (!storage || !capacity_frames || channel > 1u) return 0;"),
        ("stereo write count",
         "    uint32_t w;\n    if (!storage || !capacity_frames || !write_frame || !fill_frames) return;",
         "    uint32_t w;\n"
         "    apsis_p11h_probe.stereo_write_calls++;\n"
         "    if (!storage || !capacity_frames || !write_frame || !fill_frames) return;"),
        ("smooth count",
         "static void space_room_smooth_params(ApsisSpaceRoom *room)\n{\n    if (!room || !room->params_initialized) return;",
         "static void space_room_smooth_params(ApsisSpaceRoom *room)\n{\n"
         "    if (!room || !room->params_initialized) return;\n"
         "    apsis_p11h_probe.smooth_param_steps++;"),
        ("room render count",
         "    if (!room || !state || !out_stereo || !room->storage_bound) return;\n    space_room_targets(room, state);",
         "    if (!room || !state || !out_stereo || !room->storage_bound) return;\n"
         "    apsis_p11h_probe.room_render_calls++;\n"
         "    space_room_targets(room, state);"),
        ("room frame iteration",
         "        for (i = 0u; i < n; ++i) {\n            const SpaceRoomBiquadCoeff *guard_lp;",
         "        for (i = 0u; i < n; ++i) {\n"
         "            apsis_p11h_probe.room_frame_iterations++;\n"
         "            const SpaceRoomBiquadCoeff *guard_lp;"),
    ])

    instrument(Path("apsis/apsis_space_dsp.c"), out / "apsis_space_dsp.c", [
        ("fdn mul count",
         "static int32_t space_mul_q15(int32_t a, int32_t b)\n{\n    int64_t p = (int64_t)a * b;",
         "static int32_t space_mul_q15(int32_t a, int32_t b)\n{\n"
         "    apsis_p11h_probe.fdn_mul_q15_calls++;\n"
         "    int64_t p = (int64_t)a * b;"),
        ("fdn line read count",
         "    const int16_t *base;\n    if (!fdn || !line || !fdn->storage_bound) return 0;",
         "    const int16_t *base;\n"
         "    apsis_p11h_probe.fdn_line_read_calls++;\n"
         "    if (!fdn || !line || !fdn->storage_bound) return 0;"),
        ("fdn render count",
         "    if (!fdn || !fdn->storage_bound || !out_stereo) return;\n    alpha = space_damping_alpha(damping_hz);",
         "    if (!fdn || !fdn->storage_bound || !out_stereo) return;\n"
         "    apsis_p11h_probe.fdn_render_calls++;\n"
         "    apsis_p11h_probe.fdn_frames += frames;\n"
         "    alpha = space_damping_alpha(damping_hz);"),
        ("fdn line write count",
         "        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {\n            ApsisSpaceFdnLine *line = &fdn->line[k];\n            int16_t *base = fdn->storage + line->offset;",
         "        for (k = 0u; k < APSIS_SPACE_FDN_LINES; ++k) {\n"
         "            apsis_p11h_probe.fdn_line_write_calls++;\n"
         "            ApsisSpaceFdnLine *line = &fdn->line[k];\n"
         "            int16_t *base = fdn->storage + line->offset;"),
    ])

    instrument(Path("apsis/apsis_transport.c"), out / "apsis_transport.c", [
        ("schedule call count",
         "    if (!t || !s || !q || !t->running) return 0u;\n\n    n = apsis_transport_clock_n(s->clock.n);",
         "    if (!t || !s || !q || !t->running) return 0u;\n"
         "    apsis_p11h_probe.transport_schedule_calls++;\n\n"
         "    n = apsis_transport_clock_n(s->clock.n);"),
        ("events added count",
         "    for (i = 0u; i < count; ++i)\n        if (!apsis_transport_queue_push(q, local[i])) return 0u;\n\n    t->melody_step++;",
         "    for (i = 0u; i < count; ++i)\n"
         "        if (!apsis_transport_queue_push(q, local[i])) return 0u;\n"
         "    apsis_p11h_probe.transport_events_added += count;\n\n"
         "    t->melody_step++;"),
    ])

    print(f"PASS: wrote test-only instrumented sources to {out}")


if __name__ == "__main__":
    main()
