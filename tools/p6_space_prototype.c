#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FS 44100u
#define PI 3.14159265358979323846
#define FDN_N 8
#define SCH_COMBS 8
#define SCH_AP 4
#define POOL_BUDGET 344064u
#define EXISTING_POOL 30720u
#define INPUT_HISTORY_BYTES 24696u /* stereo int16, 140 ms */
#define ECHO_BYTES 211680u /* stereo int16, 1.2 s */

typedef struct {
    double peak;
    double rms;
    double corr;
    double hf_ratio;
    double metal;
    double db_at_decay;
    uint32_t hash;
} Metrics;

static uint32_t rng_state = 0x1265a11du;

static double rnd_pm1(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return ((double)(rng_state >> 8) / 8388607.5) - 1.0;
}

static int16_t q15(double x)
{
    if (x > 0.999969) x = 0.999969;
    if (x < -1.0) x = -1.0;
    return (int16_t)lrint(x * 32767.0);
}

static double f15(int16_t x)
{
    return (double)x / 32768.0;
}

static uint32_t hash_i16(uint32_t h, int16_t v)
{
    h ^= (uint16_t)v;
    return h * 16777619u;
}

static Metrics metrics(const double *l, const double *r, size_t n, double decay)
{
    Metrics m = {0};
    double ss = 0, sl = 0, sr = 0, sll = 0, srr = 0, slr = 0, hf = 0;
    uint32_t h = 2166136261u;
    size_t i;

    for (i = 0; i < n; ++i) {
        double a = fabs(l[i]);
        if (fabs(r[i]) > a) a = fabs(r[i]);
        if (a > m.peak) m.peak = a;
        ss += l[i] * l[i] + r[i] * r[i];
        sl += l[i];
        sr += r[i];
        sll += l[i] * l[i];
        srr += r[i] * r[i];
        slr += l[i] * r[i];
        if (i) {
            double dl = l[i] - l[i - 1u];
            double dr = r[i] - r[i - 1u];
            hf += dl * dl + dr * dr;
        }
        h = hash_i16(h, q15(l[i]));
        h = hash_i16(h, q15(r[i]));
    }

    m.rms = sqrt(ss / (2.0 * n));
    {
        double cov = slr - sl * sr / n;
        double vl = sll - sl * sl / n;
        double vr = srr - sr * sr / n;
        m.corr = (vl > 1e-20 && vr > 1e-20) ? cov / sqrt(vl * vr) : 0.0;
    }
    m.hf_ratio = ss > 1e-20 ? sqrt(hf / ss) : 0.0;
    m.hash = h;

    /* Metallic-periodicity proxy: maximum normalized autocorrelation at
     * 20..180 ms lags over a 1..3 second late window. */
    {
        size_t a0 = FS;
        size_t a1 = n < 3u * FS ? n : 3u * FS;
        double best = 0.0;
        if (a1 > a0 + 8000u) {
            unsigned lag;
            for (lag = 882u; lag <= 7938u; lag += 147u) {
                double num = 0, d0 = 0, d1 = 0;
                for (i = a0; i + lag < a1; i += 3u) {
                    double x = 0.5 * (l[i] + r[i]);
                    double y = 0.5 * (l[i + lag] + r[i + lag]);
                    num += x * y;
                    d0 += x * x;
                    d1 += y * y;
                }
                if (d0 > 1e-20 && d1 > 1e-20) {
                    double c = fabs(num / sqrt(d0 * d1));
                    if (c > best) best = c;
                }
            }
        }
        m.metal = best;
    }

    /* RMS in the last 100 ms before the requested decay time, relative to
     * peak. This is a simple common proxy, not a claim that source DECAY is
     * literally an RT60 parameter. */
    {
        size_t center = (size_t)(decay * FS);
        size_t w = FS / 10u;
        size_t start;
        double es = 0;
        size_t count = 0;
        if (center > n) center = n;
        start = center > w ? center - w : 0u;
        for (i = start; i < center; ++i) {
            es += l[i] * l[i] + r[i] * r[i];
            count += 2u;
        }
        if (count && es > 1e-20 && m.peak > 1e-20) {
            double rr = sqrt(es / count);
            m.db_at_decay = 20.0 * log10(rr / m.peak);
        } else {
            m.db_at_decay = -200.0;
        }
    }
    return m;
}

/* Deterministic statistical rendering of the Study-126 Cathedral IR formula.
 * The browser uses Math.random(), so this is a comparison reference rather
 * than an attempt to reproduce one browser session bit-for-bit. */
static void render_ref(double decay, double *l, double *r, size_t n)
{
    double lp[2] = {0}, lp2[2] = {0}, lp3[2] = {0};
    size_t len = (size_t)(decay * FS);
    int c;
    memset(l, 0, n * sizeof(*l));
    memset(r, 0, n * sizeof(*r));
    rng_state = 0x1265a11du;
    if (len > n) len = n;

    for (c = 0; c < 2; ++c) {
        size_t i;
        static const double ts[5] = {.031, .054, .083, .127, .181};
        static const double gs[5] = {.62, .44, .34, .23, .16};
        int k;
        for (i = 0; i < len; ++i) {
            double t = (double)i / FS;
            double env = exp(-5.15 * t / decay);
            double late = 1.0 - exp(-t * 18.0);
            double shimmer = .91 + .09 * sin((i * (c ? 1.007 : .993)) * .0019);
            double white = rnd_pm1();
            double y;
            lp[c] = lp[c] * .78 + white * .22;
            lp2[c] = lp2[c] * .68 + lp[c] * .32;
            lp3[c] = lp3[c] * .58 + lp2[c] * .42;
            y = lp3[c] * env * late * shimmer * .54;
            if (c) r[i] = y;
            else l[i] = y;
        }
        for (k = 0; k < 5; ++k) {
            size_t ix = (size_t)(ts[k] * FS);
            if (ix < n) {
                double g = gs[k] * (c ? (.92 + k * .01) : 1.0);
                if (c) r[ix] += g;
                else l[ix] += g;
            }
        }
    }
}

typedef struct {
    int16_t *buf;
    unsigned len;
    unsigned size;
    unsigned w;
    double lp;
    double phase;
    double rate;
} FLine;

static const unsigned fdn_len[FDN_N] = {1871, 1999, 2137, 2281, 2441, 2593, 2767, 2953};
static const double fdn_rates[FDN_N] = {.071, .083, .097, .109, .127, .139, .157, .181};

static void had8(double *v)
{
    int step;
    for (step = 1; step < 8; step *= 2) {
        int i;
        for (i = 0; i < 8; i += 2 * step) {
            int j;
            for (j = 0; j < step; ++j) {
                double a = v[i + j];
                double b = v[i + j + step];
                v[i + j] = a + b;
                v[i + j + step] = a - b;
            }
        }
    }
}

static size_t fdn_bytes(void)
{
    size_t s = 0;
    int i;
    for (i = 0; i < FDN_N; ++i) s += (fdn_len[i] + 16u) * sizeof(int16_t);
    return s;
}

static void render_fdn(double decay, double damping, double *l, double *r, size_t n)
{
    FLine q[FDN_N] = {{0}};
    double alpha = 1.0 - exp(-2.0 * PI * (damping * .65) / FS);
    double norm = 1.0 / sqrt(8.0);
    double post_l = 0, post_r = 0;
    static const double out_l[8] = {1, 1, -1, 1, -1, 1, 1, -1};
    static const double out_r[8] = {1, -1, 1, 1, 1, -1, 1, -1};
    int k;
    size_t i;

    memset(l, 0, n * sizeof(*l));
    memset(r, 0, n * sizeof(*r));
    for (k = 0; k < FDN_N; ++k) {
        q[k].size = fdn_len[k] + 16u;
        q[k].len = fdn_len[k];
        q[k].buf = calloc(q[k].size, sizeof(int16_t));
        assert(q[k].buf);
        q[k].phase = k * .73;
        q[k].rate = 2.0 * PI * fdn_rates[k] / FS;
    }

    for (i = 0; i < n; ++i) {
        double d[8], m[8];
        double outl = 0, outr = 0;
        for (k = 0; k < FDN_N; ++k) {
            double mod = 9.0 * sin(q[k].phase);
            double pos;
            unsigned i0, i1;
            double f;
            q[k].phase += q[k].rate;
            if (q[k].phase > 2.0 * PI) q[k].phase -= 2.0 * PI;
            pos = (double)q[k].w - (double)q[k].len - mod;
            while (pos < 0) pos += q[k].size;
            while (pos >= q[k].size) pos -= q[k].size;
            i0 = (unsigned)pos;
            i1 = (i0 + 1u) % q[k].size;
            f = pos - i0;
            d[k] = f15(q[k].buf[i0]) * (1.0 - f) + f15(q[k].buf[i1]) * f;
            q[k].lp += alpha * (d[k] - q[k].lp);
            m[k] = q[k].lp;
            outl += out_l[k] * m[k];
            outr += out_r[k] * m[k];
        }
        post_l += alpha * (outl * .10 - post_l);
        post_r += alpha * (outr * .10 - post_r);
        l[i] = (post_l + .13 * post_r) / 1.13;
        r[i] = (post_r + .13 * post_l) / 1.13;

        had8(m);
        for (k = 0; k < FDN_N; ++k) {
            double delay_s = (double)q[k].len / FS;
            double g = pow(10.0, (-36.0 / 20.0) * (delay_s / decay));
            double input = i == 0 ? .55 * ((k & 1) ? -.72 : .72) : 0.0;
            double x = input + g * m[k] * norm;
            q[k].buf[q[k].w] = q15(x);
            q[k].w = (q[k].w + 1u) % q[k].size;
        }
    }
    for (k = 0; k < FDN_N; ++k) free(q[k].buf);
}

typedef struct {
    int16_t *b;
    unsigned len;
    unsigned w;
    double lp;
} Comb;

typedef struct {
    int16_t *b;
    unsigned len;
    unsigned w;
} AP;

static const unsigned comb_l[SCH_COMBS] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
static const unsigned comb_r[SCH_COMBS] = {1139, 1211, 1300, 1379, 1445, 1514, 1580, 1640};
static const unsigned ap_l[SCH_AP] = {556, 441, 341, 225};
static const unsigned ap_r[SCH_AP] = {579, 464, 364, 248};

static size_t sch_bytes(void)
{
    size_t s = 0;
    int i;
    for (i = 0; i < SCH_COMBS; ++i) s += (comb_l[i] + comb_r[i]) * sizeof(int16_t);
    for (i = 0; i < SCH_AP; ++i) s += (ap_l[i] + ap_r[i]) * sizeof(int16_t);
    return s;
}

static double allpass(AP *a, double x, double g)
{
    double z = f15(a->b[a->w]);
    double y = -g * x + z;
    double w = x + g * y;
    a->b[a->w] = q15(w);
    a->w = (a->w + 1u) % a->len;
    return y;
}

static double comb(Comb *c, double x, double g, double alpha)
{
    double y = f15(c->b[c->w]);
    double w;
    c->lp += alpha * (y - c->lp);
    w = x + g * c->lp;
    if (fabs(w) < (2.5 / 32768.0)) w = 0.0;
    c->b[c->w] = q15(w);
    c->w = (c->w + 1u) % c->len;
    return y;
}

static void render_sch(double decay, double damping, double *l, double *r, size_t n)
{
    Comb cl[SCH_COMBS] = {{0}}, cr[SCH_COMBS] = {{0}};
    AP al[SCH_AP] = {{0}}, ar[SCH_AP] = {{0}};
    double alpha = 1.0 - exp(-2.0 * PI * (damping * .65) / FS);
    int k;
    size_t i;

    memset(l, 0, n * sizeof(*l));
    memset(r, 0, n * sizeof(*r));
    for (k = 0; k < SCH_COMBS; ++k) {
        cl[k].len = comb_l[k];
        cr[k].len = comb_r[k];
        cl[k].b = calloc(cl[k].len, sizeof(int16_t));
        cr[k].b = calloc(cr[k].len, sizeof(int16_t));
        assert(cl[k].b && cr[k].b);
    }
    for (k = 0; k < SCH_AP; ++k) {
        al[k].len = ap_l[k];
        ar[k].len = ap_r[k];
        al[k].b = calloc(al[k].len, sizeof(int16_t));
        ar[k].b = calloc(ar[k].len, sizeof(int16_t));
        assert(al[k].b && ar[k].b);
    }

    for (i = 0; i < n; ++i) {
        double x = i == 0 ? .45 : 0.0;
        double il = x, ir = x, yl = 0, yr = 0;
        for (k = 0; k < 2; ++k) {
            il = allpass(&al[k], il, .68);
            ir = allpass(&ar[k], ir, .68);
        }
        for (k = 0; k < SCH_COMBS; ++k) {
            double dl = (double)cl[k].len / FS;
            double dr = (double)cr[k].len / FS;
            double gl = pow(10.0, (-45.0 / 20.0) * (dl / decay));
            double gr = pow(10.0, (-45.0 / 20.0) * (dr / decay));
            yl += comb(&cl[k], il, gl, alpha);
            yr += comb(&cr[k], ir, gr, alpha);
        }
        yl /= SCH_COMBS;
        yr /= SCH_COMBS;
        yl = allpass(&al[2], yl, .62);
        yr = allpass(&ar[2], yr, .62);
        yl = allpass(&al[3], yl, .56);
        yr = allpass(&ar[3], yr, .56);
        l[i] = yl * .55;
        r[i] = yr * .55;
    }

    for (k = 0; k < SCH_COMBS; ++k) {
        free(cl[k].b);
        free(cr[k].b);
    }
    for (k = 0; k < SCH_AP; ++k) {
        free(al[k].b);
        free(ar[k].b);
    }
}

static void printm(const char *name, double decay, size_t bytes, Metrics m)
{
    printf("%s decay=%.1f pool=%zu peak=%.6f rms=%.8f corr=%+.4f hf=%.4f metal=%.4f enddb=%.2f hash=%08x\n",
           name, decay, bytes, m.peak, m.rms, m.corr, m.hf_ratio, m.metal,
           m.db_at_decay, m.hash);
}

int main(void)
{
    const double ds[3] = {4.0, 8.4, 12.0};
    const uint32_t ref_hash[3] = {0xba35441au, 0x1df1df0fu, 0x65ae55fdu};
    const uint32_t fdn_hash[3] = {0xecbca731u, 0xf1f7e1b5u, 0x90d4d8ebu};
    const uint32_t sch_hash[3] = {0x34d8a97cu, 0x2f85b094u, 0x7f2ef54eu};
    size_t fbytes = fdn_bytes(), sbytes = sch_bytes();
    size_t ftotal = EXISTING_POOL + INPUT_HISTORY_BYTES + ECHO_BYTES + fbytes;
    size_t stotal = EXISTING_POOL + INPUT_HISTORY_BYTES + ECHO_BYTES + sbytes;
    int j;

    printf("P6 memory: existing=%u input_history=%u echo=%u FDN_tail=%zu FDN_total=%zu FDN_headroom=%zu SCH_tail=%zu SCH_total=%zu SCH_headroom=%zu\n",
           EXISTING_POOL, INPUT_HISTORY_BYTES, ECHO_BYTES,
           fbytes, ftotal, (size_t)POOL_BUDGET - ftotal,
           sbytes, stotal, (size_t)POOL_BUDGET - stotal);
    assert(ftotal < POOL_BUDGET);
    assert(stotal < POOL_BUDGET);

    for (j = 0; j < 3; ++j) {
        double d = ds[j];
        size_t n = (size_t)((d + 1.0) * FS);
        double *l = calloc(n, sizeof(double));
        double *r = calloc(n, sizeof(double));
        Metrics mr, mf, ms;
        if (!l || !r) return 2;

        render_ref(d, l, r, n);
        mr = metrics(l, r, n, d);
        printm("REF", d, 0, mr);

        render_fdn(d, 6200.0, l, r, n);
        mf = metrics(l, r, n, d);
        printm("FDN", d, fbytes, mf);

        render_sch(d, 6200.0, l, r, n);
        ms = metrics(l, r, n, d);
        printm("SCH", d, sbytes, ms);

        assert(mr.hash == ref_hash[j]);
        assert(mf.hash == fdn_hash[j]);
        assert(ms.hash == sch_hash[j]);
        assert(fabs(mf.db_at_decay - mr.db_at_decay) < 3.0);
        assert(fabs(mf.corr) < 0.10);
        assert(fabs(mf.hf_ratio - mr.hf_ratio) < 0.06);
        assert(mf.metal < 0.20);
        assert(mf.metal < ms.metal * 0.35);

        free(l);
        free(r);
    }

    puts("PASS: P6 deterministic Space candidates fit pool; FDN tracks reference proxies and beats Schroeder metallicity");
    return 0;
}
