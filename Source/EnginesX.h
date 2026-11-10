// Caosmo 0.5 — generators added after studying the SuperCollider documentation (doc.sccode.org)
// GPL v3 or later. Algorithms follow the SuperCollider UGens named below (GPL v2-or-later sources by
// Nick Collins, James McCartney, Lance Putnam, Dan Stowell, Till Bovermann and others); see docs/THIRD_PARTY.md.
#pragma once
#include "DSP.h"

namespace cz
{
//==============================================================================
// Shared helpers
inline double wrap2pi (double x) { x = std::fmod (x, 6.283185307179586); return x < 0 ? x + 6.283185307179586 : x; }
inline float catmull (float y0, float y1, float y2, float y3, float t)
{
    const float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3, b = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c = -0.5f * y0 + 0.5f * y2;
    return ((a * t + b) * t + c) * t + y1;
}

// Period tracker: nudges a time-scale correction until the oscillation's period matches the note
// (the same idea as the Mackey-Glass engine). Inconsistent (chaotic) periods are ignored.
struct PeriodTracker
{
    float corr = 1.f, dcz = 0, prev = 0, lastPer = 0; int since = 0;
    void reset() { corr = 1.f; dcz = 0; prev = 0; lastPer = 0; since = 0; }
    inline void tick (float y, float target)
    {
        dcz += (y - dcz) * 0.002f;
        const float s = y - dcz;
        ++since;
        if (prev <= 0.f && s > 0.f)
        {
            const float per = (float) since; since = 0;
            if (lastPer > 0.f && std::fabs (per - lastPer) < 0.06f * per)
            {
                const float r = per / target;
                if (r > 0.3f && r < 3.5f) corr = clampf (corr * std::pow (r, 0.3f), 0.25f, 4.f);
            }
            lastPer = per;
        }
        if (since > (int) (target * 8.f) + 4800) { since = 0; lastPer = 0; }
        prev = s;
    }
};

//==============================================================================
// 13. Gendy — Xenakis' dynamic stochastic synthesis (SuperCollider Gendy1 / Gendy4)
struct EGendy
{
    static constexpr int N = 32;
    struct Gen
    {
        float amp[N] {}, dur[N] {}; int idx = 0; double phase = 1.0; float y[4] {}; float speed = 0.01f; Rng rng;
        void reset (uint32_t seed)
        {
            rng.seed (seed);
            for (int i = 0; i < N; ++i) { amp[i] = rng.bi(); dur[i] = rng.next(); }
            idx = 0; phase = 1.0; for (auto& v : y) v = 0;
        }
    };
    Gen g[2]; float sr = 48000.f, sinPh = 0;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { g[0].reset (seed); g[1].reset (seed ^ 0x5bd1e995u); sinPh = hashF (seed); }

    // GendynUGens.cpp: Gendyn_distribution (distribution value in [-1, 1] from uniform f)
    static float dist (int which, float a, float f)
    {
        a = clampf (a, 0.0001f, 1.f);
        switch (which)
        {
            case 1: { const float c = std::atan (10.f * a); return (1.f / a) * std::tan (c * (2.f * f - 1.f)) * 0.1f; }
            case 2: { float c = 0.5f + 0.499f * a; c = std::log ((1.f - c) / c); f = (f - 0.5f) * 0.998f * a + 0.5f; return std::log ((1.f - f) / f) / c; }
            case 3: { const float c = std::tan (1.5692255f * a); float t = std::tan (1.5692255f * a * f) / c; t = std::log (t * 0.999f + 0.001f) * -0.1447648f; return 2.f * t - 1.f; }
            case 4: { const float c = std::sin (1.5707963f * a); return std::sin (kPi * (f - 0.5f) * a) / c; }
            case 5: { const float c = std::log (1.f - 0.999f * a); return 2.f * (std::log (1.f - f * 0.999f * a) / c) - 1.f; }
            default: return 2.f * f - 1.f;
        }
    }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float ascale = p[0], dscale = p[1], apar = p[2], dpar = p[3], spread = p[4], st = p[6];
        const int num = (int) std::lround (clampf (p[5], 2.f, (float) N));
        const int ad = c[0], dd = c[1]; const bool cubic = c[2] == 1;
        const float sinInc = (0.05f + 8.f * apar * apar) / sr;   // the "Sinus" distribution samples a slow internal sine
        const float a2 = 1.f - 0.5f * st, b2 = 0.5f * st;
        for (int k = 0; k < n; ++k)
        {
            sinPh += sinInc; if (sinPh >= 1.f) sinPh -= 1.f;
            const float sinA = 0.5f + 0.5f * sinT (sinPh);
            float o[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                Gen& G = g[ch];
                if (G.phase >= 1.0)
                {
                    G.phase -= 1.0;
                    G.idx = (G.idx + 1) % num;
                    float na = G.amp[G.idx] + ascale * (ad == 6 ? 2.f * sinA - 1.f : dist (ad, apar, G.rng.next()));
                    if (na > 1.f || na < -1.f) { if (na < 0.f) na += 4.f; na = std::fmod (na, 4.f); if (na > 1.f && na < 3.f) na = 2.f - na; else if (na > 1.f) na -= 4.f; }
                    G.amp[G.idx] = na;
                    float rate = G.dur[G.idx] + dscale * (dd == 6 ? 2.f * sinA - 1.f : dist (dd, dpar, G.rng.next()));
                    if (rate > 1.f || rate < 0.f) { if (rate < 0.f) rate += 2.f; rate = std::fmod (rate, 2.f); rate = 2.f - rate; }
                    G.dur[G.idx] = rate;
                    const float f = f0 * std::exp2 ((2.f * rate - 1.f) * spread / 12.f);   // durations centred on the note
                    G.speed = std::min (f * (float) num / sr, 0.9f);
                    G.y[0] = G.y[1]; G.y[1] = G.y[2]; G.y[2] = G.y[3]; G.y[3] = na;
                }
                const float t = (float) G.phase;
                o[ch] = cubic ? catmull (G.y[0], G.y[1], G.y[2], G.y[3], t) : G.y[2] + (G.y[3] - G.y[2]) * t;
                G.phase += G.speed;
            }
            L[k] = (a2 * o[0] + b2 * o[1]) * 0.6f; R[k] = (b2 * o[0] + a2 * o[1]) * 0.6f;
        }
    }
};

//==============================================================================
// 14. Maps — the SuperCollider chaotic generator family (Latoocarfian, Standard, Gbman, Cusp, Quad, LinCong,
//     FBSine, Crackle, Logistic, Henon), iterated at a rate locked to the note with N / L / C interpolation.
struct EMapZoo
{
    double x = 0.5, y = 0.5, y2 = 0; float h[4] {}; double cnt = 1.0; float lp = 0, sr = 48000.f; int lastMap = -1; float lastA = -1, lastB = -1;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { x = 0.1 + 0.3 * hashF (seed); y = 0.1 + 0.3 * hashF (seed + 7u); y2 = 0; for (auto& v : h) v = 0; cnt = 1.0; lp = 0; lastMap = -1; }
    void init (int m, float A, float B)
    {
        switch (m)
        {
            case 1: x = 0.5; y = 0.0; break;
            case 2: x = 1.2 + (A - 0.5) * 3.0; y = 2.1 + (B - 0.5) * 3.0; break;
            case 3: case 4: x = 0.0; break;
            case 5: x = 0.0; break;
            case 6: x = 0.1; y = 0.1; break;
            case 7: x = 0.3; y = 0.0; break;
            case 8: x = 0.31; break;
            case 9: x = 0.0; y = 0.0; break;
            default: x = 0.5; y = 0.5; break;
        }
        lastMap = m; lastA = A; lastB = B;
    }
    float iterate (int m, float A, float B, float C, float D)
    {
        float o = 0;
        switch (m)
        {
            case 0: { const double a = 1.0 + 4.0 * (A - 0.5), b = 3.0 + 6.0 * (B - 0.5), c = C, d = D, xp = x;   // LatoocarfianN (0.5 = SC defaults)
                      x = std::sin (y * b) + c * std::sin (xp * b); y = std::sin (xp * a) + d * std::sin (y * a); o = (float) (x / (1.0 + c)); } break;
            case 1: { const double k = 0.2 + 4.8 * A; y = wrap2pi (y + k * std::sin (x)); x = wrap2pi (x + y); o = (float) ((x - 3.141592653589793) / 3.141592653589793); } break;
            case 2: { if (std::fabs (A - lastA) > 0.01f || std::fabs (B - lastB) > 0.01f) init (2, A, B);
                      const double xp = x; x = 1.0 - y + std::fabs (xp); y = xp; o = fastTanh ((float) (x - 1.5) * 0.3f); } break;
            case 3: { const double a = 0.5 + A, b = 1.4 + B; x = a - b * std::sqrt (std::fabs (x)); if (! std::isfinite (x) || std::fabs (x) > 8.0) x = 0.0; o = (float) (x * 0.6); } break;
            case 4: { const double a = 0.5 + A, b = -2.0 + 2.0 * B, c = -1.5 + 1.5 * C; x = a * x * x + b * x + c; if (! std::isfinite (x) || std::fabs (x) > 4.0) x = 0.0; o = (float) (x * 0.5); } break;
            case 5: { const double a = 0.8 + 0.6 * A, c = 0.26 * B, mm = 0.5 + C; x = std::fmod (a * x + c, mm); if (x < 0) x += mm; o = (float) (2.0 * x / mm - 1.0); } break;
            case 6: { const double im = 2.0 * A, fb = 0.2 * B, a = 0.9 + 0.4 * C, c = D; x = std::sin (im * y + fb * x); y = wrap2pi (a * y + c); o = (float) x; } break;
            case 7: { const double pp = 1.5 + 0.55 * A; const double y0 = std::fabs (x * pp - y2 - 0.05); y2 = x; x = y0; if (! std::isfinite (x) || x > 4.0) { x = 0.3; y2 = 0; } o = (float) (x * 3.0 - 1.5); } break;
            case 8: { const double r = 3.4 + 0.6 * A; x = r * x * (1.0 - x); if (! (x > 0.0 && x < 1.0)) x = 0.31; o = (float) (2.0 * x - 1.0); } break;
            default: { const double a = 1.0 + 0.8 * A, b = 0.6 * B, xp = x; x = 1.0 - a * x * x + y; y = b * xp; if (! std::isfinite (x) || std::fabs (x) > 3.0) { x = 0; y = 0; } o = (float) (x * 0.7); } break;
        }
        return clampf (o, -1.5f, 1.5f);
    }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float oct = p[0], A = p[1], B = p[2], C = p[3], D = p[4], smooth = p[5];
        const int m = c[0], ip = c[1];
        if (m != lastMap) init (m, A, B);
        const double inc = std::min (1.0, (double) f0 * std::exp2 ((double) oct) / sr);
        const float fc = 20000.f * std::pow (0.01f, smooth), a1 = std::exp (-k2Pi * std::min (fc, sr * 0.45f) / sr), comp = std::min (4.f, std::sqrt (20000.f / fc));
        for (int k = 0; k < n; ++k)
        {
            cnt += inc;
            if (cnt >= 1.0) { cnt -= 1.0; h[0] = h[1]; h[1] = h[2]; h[2] = h[3]; h[3] = iterate (m, A, B, C, D); }
            const float t = (float) cnt;
            const float o = ip == 0 ? h[3] : (ip == 1 ? h[2] + (h[3] - h[2]) * t : catmull (h[0], h[1], h[2], h[3], t));
            lp = o + a1 * (lp - o);
            L[k] = R[k] = lp * 0.7f * comp;
        }
    }
};

//==============================================================================
// 15. Ecosystems — FitzHugh-Nagumo, Brusselator, Oregonator, Lotka-Volterra, Arneodo-Coullet-Tresser, Spruce Budworm
//     (SuperCollider SLUGens by Nick Collins and ChaosUGens by Till Bovermann), integrated with RK4 and tuned to the note.
struct EEco
{
    double s[3] {}; float sr = 48000.f, fph = 0, lastOut = 0; PeriodTracker trk; Rng rng; int lastModel = -1;
    static constexpr float kPeriod[6] = { 33.36f, 6.26f, 3.569f, 5.3f, 23.31f, 1.f };   // natural periods at default P1/P2 (time units)
    static constexpr float kHmax[6] = { 0.25f, 0.05f, 0.f, 0.05f, 0.1f, 1.f };
    void prepare (float x) { sr = x; }
    void init (int m, float P1, float P2)
    {
        switch (m)
        {
            case 0: s[0] = 0.1 + 0.2 * rng.next(); s[1] = 0; break;
            case 1: s[0] = 1.0 + rng.next(); s[1] = 1.0; break;
            case 2: s[0] = 0.2 + 0.1 * rng.next(); s[1] = 0.1; break;
            case 3: { const double a = 0.5 + 1.5 * P1, d = 0.5 + 1.5 * P2; s[0] = d * (1.6 + 0.4 * rng.next()); s[1] = a * 0.6; } break;
            case 4: s[0] = 0.5 + 0.2 * rng.next(); s[1] = 0.5; s[2] = 0.5; break;
            default: s[0] = 0.9; s[1] = 0.1; break;
        }
        lastModel = m;
    }
    void reset (uint32_t seed) { rng.seed (seed); trk.reset(); fph = 0; lastOut = 0; lastModel = -1; s[0] = s[1] = s[2] = 0; }
    static void deriv (int m, const double* x, double* d, double P1, double P2, double F)
    {
        switch (m)
        {
            case 0: { const double I = 0.3 + 1.2 * P1, e = 0.02 + 0.28 * P2 * P2; d[0] = x[0] - x[0] * x[0] * x[0] / 3.0 - x[1] + I + F; d[1] = e * (x[0] + 0.7 - 0.8 * x[1]); d[2] = 0; } break;
            case 1: { const double A = 0.5 + 1.5 * P1, B = 1.0 + A * A + 0.05 + 2.5 * P2; const double x2y = x[0] * x[0] * x[1];
                      d[0] = A + x2y - (B + 1.0) * x[0] + F; d[1] = B * x[0] - x2y; d[2] = 0; } break;
            case 2: { const double f = 0.5 + 1.9 * P1, e = 0.02 + 0.2 * P2, q = 0.02;
                      d[0] = (x[0] * (1.0 - x[0]) - f * x[1] * (x[0] - q) / (x[0] + q)) / e + F; d[1] = x[0] - x[1]; d[2] = 0; } break;
            case 3: { const double a = 0.5 + 1.5 * P1, dd = 0.5 + 1.5 * P2; d[0] = x[0] * (a * (1.0 + F) - x[1]); d[1] = x[1] * (x[0] - dd); d[2] = 0; } break;
            default: { const double al = 1.2 + 0.5 * P1, mu = 0.002 + 0.03 * P2;
                       d[0] = x[0] * (1.1 - x[0] / 2 - x[1] / 2 - x[2] / 10) + mu + F;
                       d[1] = x[1] * (-0.5 + x[0] / 2 + x[1] / 10 - x[2] / 10) + mu;
                       d[2] = x[2] * (al + 0.2 - al * x[0] - x[1] / 10 - x[2] / 10) + mu; } break;
        }
    }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float P1 = p[0], P2 = p[1], force = p[2], speed = p[3], kick = p[4], drive = p[5];
        const int m = jlimitInt (c[0], 0, 5), outSel = c[1];
        if (m != lastModel) { init (m, P1, P2); trk.reset(); }
        const float target = sr / std::max (f0, 10.f);
        const float dg = 1.f + 6.f * drive, dn = 1.f / fastTanh (dg);
        const float hmax = m == 2 ? 0.15f * (0.02f + 0.2f * P2) : kHmax[m];
        const float fScale[6] = { 0.6f, 0.8f, 2.f, 0.4f, 0.15f, 0.f };
        for (int k = 0; k < n; ++k)
        {
            fph += f0 / sr; if (fph >= 1.f) fph -= 1.f;
            const double F = force * fScale[m] * sinT (fph);
            float X, Y, Z;
            if (m == 5)
            {   // Spruce Budworm: SLUGens' Euler map, iterated at a note-locked rate
                const float rate = std::min (1.f, f0 * 6.f * speed * trk.corr / sr);
                s[2] += rate;
                if (s[2] >= 1.0)
                {
                    s[2] -= 1.0;
                    const double delta = 0.05 + 0.15 * P1, k1 = 20.0 + 15.0 * P2, k2 = 1.5, alpha = 0.1, beta = 10.1, mu = 0.3, rho = 10.1;
                    const double x = s[0], y = s[1], yy = y * y, bx = beta * x;
                    const double dx = k1 * x * (1.0 - x) - mu * y + F;
                    const double dy = k2 * y * (1.0 - y / std::max (alpha * x, 1e-4)) - rho * yy / (bx * bx + yy + 1e-9);
                    s[0] = x + delta * dx + kick * 0.02 * rng.bi(); s[1] = y + delta * dy;
                    if (! std::isfinite (s[0]) || ! std::isfinite (s[1]) || s[0] <= 1e-4 || s[0] > 3.0 || s[1] < 1e-6 || s[1] > 30.0) init (5, P1, P2);
                }
                X = (float) (s[0] - 0.5) * 1.6f; Y = (float) std::tanh (s[1] * 0.4 - 0.5); Z = 0.5f * (X - Y);
            }
            else
            {
                const double h = (double) f0 * speed * kPeriod[m] * trk.corr / sr;
                const int sub = std::min (32, 1 + (int) (h / hmax));
                const double hh = h / sub;
                for (int i = 0; i < sub; ++i)
                {
                    double k1[3], k2[3], k3[3], k4[3], t[3];
                    deriv (m, s, k1, P1, P2, F);
                    for (int j = 0; j < 3; ++j) t[j] = s[j] + 0.5 * hh * k1[j]; deriv (m, t, k2, P1, P2, F);
                    for (int j = 0; j < 3; ++j) t[j] = s[j] + 0.5 * hh * k2[j]; deriv (m, t, k3, P1, P2, F);
                    for (int j = 0; j < 3; ++j) t[j] = s[j] + hh * k3[j]; deriv (m, t, k4, P1, P2, F);
                    for (int j = 0; j < 3; ++j) s[j] += hh / 6.0 * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
                }
                if (kick > 0.f)
                {   // stochastic forcing: noise on the first variable (stochastic resonance in FHN)
                    const double kn = kick * std::sqrt (h) * (m == 0 ? 0.6 : 0.15);
                    s[0] += kn * rng.bi();
                    if (m == 3 || m == 4) for (int j = 0; j < 3; ++j) s[j] = std::max (s[j], 1e-6);
                }
                bool bad = false; for (double v : s) if (! std::isfinite (v) || std::fabs (v) > 1e3) bad = true;
                if (bad) init (m, P1, P2);
                switch (m)
                {
                    case 0: X = (float) s[0] * 0.5f; Y = (float) (s[1] - 0.6) * 0.9f; break;
                    case 1: { const double A = 0.5 + 1.5 * P1; X = std::tanh ((float) (s[0] - A) * 0.6f); Y = std::tanh ((float) (s[1] - 1.0) * 0.3f); } break;
                    case 2: X = (float) (s[0] - 0.22) * 6.f; Y = (float) (s[1] - 0.22) * 8.f; break;
                    case 3: { const double a = 0.5 + 1.5 * P1, d = 0.5 + 1.5 * P2; X = std::tanh ((float) ((s[0] - d) / (d + 0.5))); Y = std::tanh ((float) ((s[1] - a) / (a + 0.5))); } break;
                    default: X = (float) (s[0] - 1.1) * 0.9f; Y = (float) (s[1] - 0.8) * 0.8f; break;
                }
                Z = m == 4 ? (float) (s[2] - 2.5) * 0.3f : 0.5f * (X - Y);
            }
            float o = outSel == 0 ? X : (outSel == 1 ? Y : (outSel == 2 ? Z : 0.5f * (X + Y + Z)));
            trk.tick (o, target);
            o = fastTanh (o * dg) * dn * 0.8f;
            L[k] = R[k] = o * 0.45f;
        }
    }
    static int jlimitInt (int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
};

//==============================================================================
// 16. Noise — WhiteNoise, PinkNoise, BrownNoise, GrayNoise, ClipNoise, Crackle, Dust, Dust2 (SuperCollider NoiseUGens)
struct ENoise
{
    struct Src
    {
        Rng rng; uint32_t dice[16] {}; uint32_t total = 0, cnt = 1; float brown = 0; int32_t gray = 0; double cy1 = 0.3, cy2 = 0;
        void reset (uint32_t seed) { rng.seed (seed); total = 0; for (auto& d : dice) { d = rng.nextU() >> 13; total += d; } cnt = 1; brown = 0; gray = 0; cy1 = 0.2 + 0.3 * rng.next(); cy2 = 0; }
        inline float tick (int type, float dens, float chaos, float sr)
        {
            switch (type)
            {
                case 1: {   // Voss-McCartney pink noise (as PinkNoise)
                    const uint32_t c = cnt++; int kk = 0; uint32_t cc = c; while (kk < 15 && (cc & 1u) == 0) { cc >>= 1; ++kk; }
                    const uint32_t nr = rng.nextU() >> 13; total += nr - dice[kk]; dice[kk] = nr;
                    return ((float) (total + (rng.nextU() >> 13)) / (float) (17u << 19) * 2.f - 1.f) * 1.8f; }
                case 2: brown += rng.bi() * 0.125f; if (brown > 1.f) brown = 2.f - brown; else if (brown < -1.f) brown = -2.f - brown; return brown;
                case 3: gray ^= (int32_t) (1u << (rng.nextU() & 31u)); return (float) gray * 4.65661287308e-10f;
                case 4: return rng.next() < 0.5f ? -0.5f : 0.5f;
                case 5: { const double y0 = std::fabs (cy1 * chaos - cy2 - 0.05); cy2 = cy1; cy1 = y0; if (! std::isfinite (cy1) || cy1 > 4.0) { cy1 = 0.3; cy2 = 0; } return (float) (y0 * 3.0 - 1.0); }
                case 6: return rng.next() < dens / sr ? rng.next() * 1.5f : 0.f;
                case 7: return rng.next() < dens / sr ? rng.bi() * 1.5f : 0.f;
                default: return rng.bi() * 0.6f;
            }
        }
    };
    Src src[2]; float sr = 48000.f; float hold[2] {}, prevH[2] {}; double hc = 1.0; float lp[2] {}, hp[2] {}, hpx[2] {}; SVF bp[2]; SVFCoefs bpc;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { src[0].reset (seed); src[1].reset (seed ^ 0x9e3779b9u); hc = 1.0; for (int i = 0; i < 2; ++i) { hold[i] = prevH[i] = lp[i] = hp[i] = hpx[i] = 0; bp[i].reset(); } }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float tone = p[0], dens = p[1], chaos = p[2], st = p[3], bpf = p[4], Q = p[5];
        const int type = c[0], clock = c[1];
        const bool clocked = clock > 0 && type < 6;
        const double inc = std::min (1.0, 2.0 * (double) f0 / sr);
        const float lpF = tone < 0.f ? 20000.f * std::pow (0.005f, -tone) : 20000.f, hpF = tone > 0.f ? 20.f * std::pow (500.f, tone) : 0.f;
        const float la = std::exp (-k2Pi * std::min (lpF, sr * 0.45f) / sr), ha = std::exp (-k2Pi * hpF / sr);
        bpc = svfMake (2, f0, sr, Q);
        const float bg = 1.f + std::sqrt (Q) * 0.6f;
        const float a2 = 1.f - 0.5f * st, b2 = 0.5f * st;
        for (int k = 0; k < n; ++k)
        {
            float v[2];
            if (clocked)
            {
                hc += inc;
                if (hc >= 1.0) { hc -= 1.0; for (int i = 0; i < 2; ++i) { prevH[i] = hold[i]; hold[i] = src[i].tick (type, dens, chaos, sr); } }
                for (int i = 0; i < 2; ++i) v[i] = clock == 1 ? hold[i] : prevH[i] + (hold[i] - prevH[i]) * (float) hc;
            }
            else for (int i = 0; i < 2; ++i) v[i] = src[i].tick (type, dens, chaos, sr);
            const float mixed[2] = { a2 * v[0] + b2 * v[1], b2 * v[0] + a2 * v[1] };
            float o[2];
            for (int i = 0; i < 2; ++i)
            {
                float x = mixed[i];
                lp[i] = x + la * (lp[i] - x); x = lp[i];
                if (hpF > 0.f) { const float y = ha * (hp[i] + x - hpx[i]); hpx[i] = x; hp[i] = y; x = y; }
                const float b = bp[i].tick (x, bpc) * bg;
                o[i] = (1.f - bpf) * x + bpf * b;
            }
            L[k] = o[0]; R[k] = o[1];
        }
    }
};

//==============================================================================
// 17. Membrane — a bouncing ball (SuperCollider TBall) striking a modal membrane, bar or bell tuned to the note
struct EMembrane
{
    static constexpr int M = 16;
    float re[M] {}, im[M] {}; float sr = 48000.f; double h = 0, v = 0; float vRef = 1.f; bool alive = false; int burst = 0; float burstAmp = 0; Rng rng; bool pendingStrike = false;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { rng.seed (seed); pendingStrike = true; }    // keep ringing modes: a new note strikes the same body
    static const float* ratios (int body)
    {
        static const float circ[M] = { 1.f, 1.594f, 2.136f, 2.296f, 2.653f, 2.918f, 3.156f, 3.501f, 3.6f, 3.652f, 4.06f, 4.154f, 4.23f, 4.601f, 4.832f, 4.903f };
        static const float sq[M] = { 1.f, 1.581f, 2.f, 2.236f, 2.55f, 2.915f, 3.f, 3.162f, 3.536f, 3.606f, 3.808f, 4.f, 4.123f, 4.301f, 4.472f, 4.528f };
        static const float bar[M] = { 1.f, 2.756f, 5.404f, 8.933f, 13.344f, 18.638f, 24.81f, 31.87f, 39.81f, 48.63f, 58.33f, 68.92f, 80.4f, 92.75f, 106.f, 120.1f };
        static const float bell[M] = { 0.5f, 1.f, 1.2f, 1.5f, 2.f, 2.5f, 2.67f, 3.f, 4.f, 4.24f, 5.4f, 6.f, 6.67f, 8.f, 9.f, 10.2f };
        return body == 1 ? sq : (body == 2 ? bar : (body == 3 ? bell : circ));
    }
    void strike (float amp, const float* w, int nm) { for (int i = 0; i < nm; ++i) re[i] += amp * w[i]; burst = (int) (0.003f * sr); burstAmp = amp; }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float g = p[0], loss = p[1], height = p[2], scatter = p[3], decay = p[4], strikeP = p[5], tension = p[6];
        const int nm = (int) std::lround (clampf (p[7], 2.f, (float) M)); const int body = c[0];
        const float* r = ratios (body);
        float cw[M], sw[M], gk[M], wk[M], pl[M], pr[M]; float wsum = 0;
        for (int i = 0; i < M; ++i)
        {
            const float ratio = std::pow (r[i], 1.f + 0.35f * tension), f = f0 * ratio;
            const bool ok = i < nm && f < sr * 0.45f;
            const float w = k2Pi * f / sr; cw[i] = std::cos (w); sw[i] = std::sin (w);
            const float tau = decay / (1.f + 0.5f * (ratio - 1.f));
            gk[i] = ok ? std::exp (-1.f / (std::max (tau, 0.005f) * sr)) : 0.f;
            wk[i] = ok ? std::pow (ratio, -1.6f + 2.4f * strikeP) : 0.f; wsum += wk[i];
            const float pan = (i % 2 ? 0.35f : -0.35f) * std::min (1.f, (float) i / 3.f);
            pl[i] = 1.f - std::max (0.f, pan); pr[i] = 1.f + std::min (0.f, pan);
        }
        for (int i = 0; i < M; ++i) wk[i] /= std::max (wsum, 1e-3f);
        if (pendingStrike)
        {
            pendingStrike = false;
            strike (1.f, wk, M);
            vRef = std::sqrt (2.f * g * height); v = vRef * (1.f - loss); h = 0; alive = height > 0.005f && v > 0.01;
        }
        for (int k = 0; k < n; ++k)
        {
            if (alive)
            {
                v -= g / sr; h += v / sr;
                if (h <= 0.0)
                {
                    const float amp = clampf ((float) std::fabs (v) / std::max (vRef, 1e-3f), 0.f, 1.f);
                    strike (amp, wk, M);
                    v = std::fabs (v) * (1.0 - loss) * (1.0 + scatter * 0.5 * rng.bi()); h = 0;
                    if (v < 0.015 * vRef || 2.0 * v / g < 0.002) alive = false;
                }
            }
            float noise = 0;
            if (burst > 0) { noise = rng.bi() * burstAmp * strikeP * 0.4f * (float) burst / (0.003f * sr); --burst; }
            float oL = 0, oR = 0;
            for (int i = 0; i < M; ++i)
            {
                if (gk[i] == 0.f) { re[i] = im[i] = 0.f; continue; }
                const float a = re[i] + noise * wk[i], b = im[i];
                re[i] = gk[i] * (a * cw[i] - b * sw[i]); im[i] = gk[i] * (a * sw[i] + b * cw[i]);
                oL += im[i] * pl[i]; oR += im[i] * pr[i];
            }
            L[k] = oL * 1.1f; R[k] = oR * 1.1f;
        }
    }
};
} // namespace cz
