#include "apsis_drift.h"

/* 257-point source-derived lookup for
 * delta = 0.05 + 3.95 * amount^1.65 Hz, amount in [0,1].
 * Values are milli-Hz at i/256. Linear interpolation keeps the target path
 * libm-free and deterministic on FM-1 while matching the browser law closely.
 */
static const uint16_t DELTA_MILLIHZ[257] = {
50,50,51,53,54,56,58,60,63,66,69,72,75,79,83,87,
91,95,99,104,109,114,119,124,129,135,141,147,153,159,165,171,
178,184,191,198,205,212,220,227,235,242,250,258,266,274,283,291,
299,308,317,326,335,344,353,362,372,381,391,401,411,421,431,441,
451,461,472,483,493,504,515,526,537,548,560,571,583,594,606,618,
630,642,654,666,678,691,703,716,728,741,754,767,780,793,806,820,
833,846,860,874,888,901,915,929,944,958,972,986,1001,1016,1030,1045,
1060,1075,1090,1105,1120,1135,1151,1166,1181,1197,1213,1229,1244,1260,1276,1292,
1309,1325,1341,1358,1374,1391,1407,1424,1441,1458,1475,1492,1509,1526,1544,1561,
1579,1596,1614,1632,1649,1667,1685,1703,1721,1739,1758,1776,1794,1813,1831,1850,
1869,1888,1907,1925,1944,1964,1983,2002,2021,2041,2060,2080,2099,2119,2139,2159,
2179,2199,2219,2239,2259,2279,2300,2320,2341,2361,2382,2403,2423,2444,2465,2486,
2507,2528,2550,2571,2592,2614,2635,2657,2678,2700,2722,2744,2766,2788,2810,2832,
2854,2876,2899,2921,2944,2966,2989,3012,3034,3057,3080,3103,3126,3149,3172,3196,
3219,3242,3266,3289,3313,3336,3360,3384,3408,3432,3456,3480,3504,3528,3552,3577,
3601,3625,3650,3675,3699,3724,3749,3773,3798,3823,3848,3874,3899,3924,3949,3975,
4000
};

uint16_t apsis_drift_delta_millihz(uint16_t amount_q15)
{
    uint32_t scaled = (uint32_t)amount_q15 * 256u;
    uint32_t index = scaled / APSIS_Q15_ONE;
    uint32_t rem;
    uint32_t a, b;
    if (index >= 256u) return DELTA_MILLIHZ[256];
    rem = scaled - index * APSIS_Q15_ONE;
    a = DELTA_MILLIHZ[index];
    b = DELTA_MILLIHZ[index + 1u];
    return (uint16_t)(a + ((b - a) * rem + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE);
}

uint16_t apsis_drift_audible_delta_millihz(const ApsisDriftState *d)
{
    uint32_t delta, depth, level;
    if (!d || !d->sound) return 0u;
    delta = apsis_drift_delta_millihz(d->amount_q15);
    depth = d->depth_q15 < APSIS_Q15_MILLI(150) ? APSIS_Q15_MILLI(150) : d->depth_q15;
    level = d->level_q15;
    delta = (delta * depth + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    delta = (delta * level + APSIS_Q15_ONE / 2u) / APSIS_Q15_ONE;
    return (uint16_t)delta;
}
