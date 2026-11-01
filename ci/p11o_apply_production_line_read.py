#!/usr/bin/env python3
from pathlib import Path

SOURCE = Path('apsis/apsis_space_dsp.c')
s = SOURCE.read_text()

old = '''static int32_t space_line_read(const ApsisSpaceFdn *fdn,
                               const ApsisSpaceFdnLine *line,
                               int32_t modulation_q8)
{
    int32_t delay_q8, read_q8, span_q8;
    uint32_t needed, i0, i1, frac;
    int32_t a, b;
    const int16_t *base;
    if (!fdn || !line || !fdn->storage_bound) return 0;
    delay_q8 = (int32_t)line->base_delay * 256 + modulation_q8;
    if (delay_q8 < 256) delay_q8 = 256;
    needed = (uint32_t)(delay_q8 + 255) >> 8;
    if (line->fill_count < needed) return 0;

    span_q8 = (int32_t)line->capacity * 256;
    read_q8 = (int32_t)line->write_pos * 256 - delay_q8;
    if (read_q8 < 0) read_q8 += span_q8;
    if (read_q8 >= span_q8) read_q8 -= span_q8;
    i0 = (uint32_t)read_q8 >> 8;
    frac = (uint32_t)read_q8 & 255u;
    i1 = i0 + 1u;
    if (i1 >= line->capacity) i1 = 0u;
    base = fdn->storage + line->offset;
    a = base[i0];
    if (!frac) return a;
    b = base[i1];
    return (a * (int32_t)(256u - frac) + b * (int32_t)frac) >> 8;
}'''

new = '''static int32_t space_line_read(const ApsisSpaceFdn *fdn,
                               const ApsisSpaceFdnLine *line,
                               int32_t modulation_q8)
{
    int32_t delay_q8, read_q8, span_q8;
    uint32_t needed, i0, i1, frac;
    int32_t a, b;
    const int16_t *base;
    if (!fdn || !line || !fdn->storage_bound) return 0;
    delay_q8 = (int32_t)line->base_delay * 256 + modulation_q8;
    if (line->fill_count < line->capacity) {
        needed = (uint32_t)(delay_q8 + 255) >> 8;
        if (line->fill_count < needed) return 0;
    }

    span_q8 = (int32_t)line->capacity * 256;
    read_q8 = (int32_t)line->write_pos * 256 - delay_q8;
    if (read_q8 < 0) read_q8 += span_q8;
    i0 = (uint32_t)read_q8 >> 8;
    frac = (uint32_t)read_q8 & 255u;
    i1 = i0 + 1u;
    if (i1 >= line->capacity) i1 = 0u;
    base = fdn->storage + line->offset;
    a = base[i0];
    if (!frac) return a;
    b = base[i1];
    return (a * (int32_t)(256u - frac) + b * (int32_t)frac) >> 8;
}'''

if s.count(old) != 1:
    raise SystemExit(f'FAIL: P11O production line-read authority anchor count={s.count(old)}')
SOURCE.write_text(s.replace(old, new, 1))
print('PASS: applied exact P11N-proved line-read candidate to production source')
