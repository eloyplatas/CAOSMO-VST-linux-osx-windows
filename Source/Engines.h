// Caosmo — the twelve chaos engines
#pragma once
#include "DSP.h"
#include "EnginesX.h"

namespace cz
{
// Calibration constants (measured with the test harness)
struct Cal
{
    static constexpr float sprottRate = 1.009f;  // measured with the harness at default Chaos/Damp
    static constexpr float chuaRate   = 0.605f;  // 0.472 cycles/τ at α=10 β=15 → Omega 3.5 lands on the note
};

//==============================================================================
// 1. Kuramoto swarm of coupled phase oscillators
struct EKuramoto
{
    static constexpr int N = 12;
    float c_[N] {}, s_[N] {}, g[N] {}; float sr = 48000.f;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed)
    {
        for (int i = 0; i < N; ++i) { float th = hashF (seed * 31u + (uint32_t) i) * k2Pi; c_[i] = std::cos (th); s_[i] = std::sin (th); g[i] = 0.f; }
    }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float K = p[0], spread = p[1];
        const int nOsc = (int) std::lround (clampf (p[2], 3.f, 12.f));
        const int outM = c[0], rm = c[1];
        float ratio[N]; float sum = 0;
        for (int i = 0; i < N; ++i)
        {
            float t = nOsc > 1 ? std::min (1.f, (float) i / (float) (nOsc - 1)) : 0.5f;
            float r;
            switch (rm)
            {
                case 0: r = 1.f + spread * (1.5f * t * t - 0.5f); break;
                case 1: r = 1.f + spread * (t - 0.5f); break;
                case 2: r = 1.f + spread * (std::fmod ((float) i * 0.6180339f, 1.f) - 0.5f); break;
                case 3: r = 1.f + spread * (float) i; break;
                default: r = 1.f + spread * (hashF (1013u + (uint32_t) i) - 0.5f) * 1.2f; break;
            }
            ratio[i] = std::max (0.05f, r);
            if (i < nOsc) sum += ratio[i];
        }
        const float norm = (float) nOsc / sum;
        float w[N];
        for (int i = 0; i < N; ++i) w[i] = k2Pi * f0 * ratio[i] * norm / sr;
        const float Keff = K * k2Pi * f0 * (0.03f + spread) * 0.18f / sr;
        const float gc = 1.f - msCoef (20.f, sr);
        for (int k = 0; k < n; ++k)
        {
            float rc = 0, rs = 0, G = 0;
            for (int i = 0; i < N; ++i) { g[i] += ((i < nOsc ? 1.f : 0.f) - g[i]) * gc; rc += g[i] * c_[i]; rs += g[i] * s_[i]; G += g[i]; }
            const float inv = 1.f / std::max (G, 1e-3f); rc *= inv; rs *= inv;
            for (int i = 0; i < N; ++i)
            {
                float d = clampf (w[i] + Keff * (rs * c_[i] - rc * s_[i]), -1.4f, 1.4f);
                float d2 = d * d;
                float cd = 1.f - d2 * 0.5f + d2 * d2 * (1.f / 24.f) - d2 * d2 * d2 * (1.f / 720.f);
                float sd = d * (1.f - d2 * (1.f / 6.f) + d2 * d2 * (1.f / 120.f));
                float nc = c_[i] * cd - s_[i] * sd, ns = s_[i] * cd + c_[i] * sd;
                float m = 1.5f - 0.5f * (nc * nc + ns * ns);
                c_[i] = nc * m; s_[i] = ns * m;
            }
            float o = (outM == 0) ? rs : rs * std::sqrt (rc * rc + rs * rs) * 1.4f;
            L[k] = R[k] = o;
        }
    }
};

//==============================================================================
// 2. Forced Duffing double-well oscillator
struct EDuffing
{
    float x = 0.3f, v = 0, ph = 0, nz = 0, sr = 48000.f; Rng rng;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { x = 0.3f + 0.2f * hashF (seed); v = 0; ph = 0; nz = 0; rng.seed (seed); }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float gamma = p[0], delta = p[1], omega = p[2], stiff = p[3], fb = p[4];
        const int outM = c[0], dw = c[1];
        const float h = std::min (k2Pi * f0 / 1.41421356f / sr, 0.6f);
        const float drInc = omega * f0 / sr;
        const float gd = gamma * 0.9f;
        const float nzA = 1.f - std::exp (-k2Pi * std::min (omega * f0, sr * 0.4f) / sr);
        const float ws = std::sqrt (stiff);
        for (int k = 0; k < n; ++k)
        {
            ph += drInc; if (ph >= 1.f) ph -= 1.f;
            float d;
            switch (dw)
            {
                case 0: d = sinT (ph); break;
                case 1: d = ph < 0.5f ? 1.f : -1.f; break;
                case 2: d = 1.f - 4.f * std::fabs (ph - 0.5f); break;
                case 3: d = 2.f * ph - 1.f; break;
                default: nz += (rng.bi() - nz) * nzA; d = nz * 2.5f; break;
            }
            const float F = gd * (d + fb * 1.6f * x * ws);
            auto acc = [&] (float X, float V) { return -delta * V + X - stiff * X * X * X + F; };
            const float k1x = v,                   k1v = acc (x, v);
            const float k2x = v + 0.5f * h * k1v,  k2v = acc (x + 0.5f * h * k1x, v + 0.5f * h * k1v);
            const float k3x = v + 0.5f * h * k2v,  k3v = acc (x + 0.5f * h * k2x, v + 0.5f * h * k2v);
            const float k4x = v + h * k3v,         k4v = acc (x + h * k3x, v + h * k3v);
            x += h / 6.f * (k1x + 2.f * k2x + 2.f * k3x + k4x);
            v += h / 6.f * (k1v + 2.f * k2v + 2.f * k3v + k4v);
            if (! finiteF (x) || ! finiteF (v) || std::fabs (x) > 8.f) { x = 0.3f; v = 0; }
            const float po = x * ws * 0.6f, ve = v * ws * 0.42f;
            const float o = outM == 0 ? po : (outM == 1 ? ve : 0.6f * (po + ve));
            L[k] = R[k] = o;
        }
    }
};

//==============================================================================
// 3. Sprott-type jerk flow
struct ESprott
{
    float x = 0.1f, y = 0, z = 0, sr = 48000.f;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { x = 0.1f + 0.1f * hashF (seed); y = 0; z = 0; }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float chaos = p[0], damp = p[1], omega = p[2], fb = p[3];
        const int outM = c[0];
        // the jerk flow only stays bounded for 0.55 < a < ~1.1: chaos/damp/fb are mapped inside that window
        const float cn = clampf ((chaos - 0.1f) / 2.9f, 0.f, 1.f), dn = clampf ((damp - 0.1f) / 2.9f, 0.f, 1.f);
        const float a = clampf (std::max (0.98f - 0.5f * std::sqrt (cn) + 0.12f * (dn - 0.31f), 0.565f + 0.33f * fb), 0.565f, 0.98f);
        const float h = std::min (k2Pi * f0 * omega * Cal::sprottRate / sr, 0.5f);
        auto dz = [&] (float X, float Y, float Z) { return -a * Z - Y + (std::fabs (X) - 1.f) + fb * 0.35f * std::sin (X * 1.7f); };
        for (int k = 0; k < n; ++k)
        {
            const float k1x = y, k1y = z, k1z = dz (x, y, z);
            const float k2x = y + 0.5f * h * k1y, k2y = z + 0.5f * h * k1z, k2z = dz (x + 0.5f * h * k1x, y + 0.5f * h * k1y, z + 0.5f * h * k1z);
            const float k3x = y + 0.5f * h * k2y, k3y = z + 0.5f * h * k2z, k3z = dz (x + 0.5f * h * k2x, y + 0.5f * h * k2y, z + 0.5f * h * k2z);
            const float k4x = y + h * k3y, k4y = z + h * k3z, k4z = dz (x + h * k3x, y + h * k3y, z + h * k3z);
            x += h / 6.f * (k1x + 2 * k2x + 2 * k3x + k4x);
            y += h / 6.f * (k1y + 2 * k2y + 2 * k3y + k4y);
            z += h / 6.f * (k1z + 2 * k2z + 2 * k3z + k4z);
            if (! finiteF (x) || std::fabs (x) > 30.f || std::fabs (y) > 30.f || std::fabs (z) > 30.f) { x = 0.1f; y = z = 0; }
            float o;
            switch (outM)
            {
                case 0: o = x * 0.35f; break;
                case 1: o = y * 0.45f; break;
                case 2: o = z * 0.45f; break;
                default: o = (x * 0.35f + y * 0.45f + z * 0.45f) * 0.5f; break;
            }
            L[k] = R[k] = clampf (o, -1.5f, 1.5f);
        }
    }
};

//==============================================================================
// 4. Lorenz + Rössler (pitch-inert)
struct ELorRos
{
    float lx = 1, ly = 1, lz = 20, rx = 1, ry = 0, rz = 0, sr = 48000.f;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { lx = 0.5f + hashF (seed); ly = 1; lz = 20; rx = 1.f + hashF (seed + 3u); ry = 0; rz = 0; }
    static inline float pick (int m, float a, float b, float c) { return m == 0 ? a : m == 1 ? b : m == 2 ? c : (a + b + c) * 0.45f; }
    void render (const float* p, const int* c, float, float* L, float* R, int n)
    {
        const float lorL = p[0], rosL = p[1], sig = p[2], cc = p[3];
        const float rho = 28.f, beta = 8.f / 3.f, ra = 0.2f, rb = 0.2f;
        const float hL = 140.f / sr, hR = 1150.f / sr;
        for (int k = 0; k < n; ++k)
        {
            { // Lorenz RK4
                auto f = [&] (float X, float Y, float Z, float& dx, float& dy, float& dz) { dx = sig * (Y - X); dy = X * (rho - Z) - Y; dz = X * Y - beta * Z; };
                float a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4;
                f (lx, ly, lz, a1, b1, c1);
                f (lx + 0.5f * hL * a1, ly + 0.5f * hL * b1, lz + 0.5f * hL * c1, a2, b2, c2);
                f (lx + 0.5f * hL * a2, ly + 0.5f * hL * b2, lz + 0.5f * hL * c2, a3, b3, c3);
                f (lx + hL * a3, ly + hL * b3, lz + hL * c3, a4, b4, c4);
                lx += hL / 6.f * (a1 + 2 * a2 + 2 * a3 + a4); ly += hL / 6.f * (b1 + 2 * b2 + 2 * b3 + b4); lz += hL / 6.f * (c1 + 2 * c2 + 2 * c3 + c4);
                if (! finiteF (lx) || std::fabs (lx) > 200.f) { lx = 1; ly = 1; lz = 20; }
            }
            { // Rössler RK4
                auto f = [&] (float X, float Y, float Z, float& dx, float& dy, float& dz) { dx = -Y - Z; dy = X + ra * Y; dz = rb + Z * (X - cc); };
                float a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4;
                f (rx, ry, rz, a1, b1, c1);
                f (rx + 0.5f * hR * a1, ry + 0.5f * hR * b1, rz + 0.5f * hR * c1, a2, b2, c2);
                f (rx + 0.5f * hR * a2, ry + 0.5f * hR * b2, rz + 0.5f * hR * c2, a3, b3, c3);
                f (rx + hR * a3, ry + hR * b3, rz + hR * c3, a4, b4, c4);
                rx += hR / 6.f * (a1 + 2 * a2 + 2 * a3 + a4); ry += hR / 6.f * (b1 + 2 * b2 + 2 * b3 + b4); rz += hR / 6.f * (c1 + 2 * c2 + 2 * c3 + c4);
                if (! finiteF (rx) || std::fabs (rx) > 200.f || std::fabs (rz) > 200.f) { rx = 1; ry = 0; rz = 0; }
            }
            const float lo = pick (c[0], lx / 18.f, ly / 24.f, (lz - 24.f) / 16.f);
            const float ro = pick (c[1], rx / 10.f, ry / 10.f, rz / 12.f - 0.15f);
            const float o = 0.8f * (lo * lorL + ro * rosL);
            L[k] = R[k] = o;
        }
    }
};

//==============================================================================
// 5. Logistic / Hénon maps (pitch-inert)
struct EMaps
{
    float lx = 0.31f, hx = 0.1f, hy = 0, lp = 0, dph = 0, sr = 48000.f; float hist[4] {}; double cnt = 1.0;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { lx = 0.2f + 0.6f * hashF (seed); hx = 0.1f; hy = 0; lp = 0; dph = hashF (seed + 9u); for (auto& h : hist) h = 0; cnt = 1.0; }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float morph = p[0], r = p[1], a = p[2], b = p[3], bright = p[4], drift = p[5];
        const int clock = c[0];
        // v0.5 note clocks: iterate at f0 * 2^Iter Oct (as SuperCollider's LogisticN/HenonN/L/C) instead of once per sample
        const double inc = clock == 0 ? 1.0 : std::min (1.0, (double) f0 * std::exp2 ((double) p[6]) / sr);
        const float fc = 20000.f * std::pow (0.01f, bright);
        const float a1 = std::exp (-k2Pi * std::min (fc, sr * 0.45f) / sr);
        const float comp = std::min (6.f, std::sqrt (20000.f / fc));   // keep level when smoothing
        const float dInc = 0.17f / sr;
        for (int k = 0; k < n; ++k)
        {
            dph += dInc; if (dph >= 1.f) dph -= 1.f;
            cnt += inc;
            if (cnt >= 1.0)
            {
                cnt -= 1.0;
                const float s = sinT (dph);
                const float re = std::min (4.f, r + drift * 0.04f * s), ae = a + drift * 0.08f * s;
                lx = re * lx * (1.f - lx);
                if (! (lx > 0.f && lx < 1.f)) lx = 0.31f;
                const float nh = 1.f - ae * hx * hx + hy; hy = b * hx; hx = nh;
                if (! finiteF (hx) || std::fabs (hx) > 3.f) { hx = 0.1f; hy = 0; }
                hist[0] = hist[1]; hist[1] = hist[2]; hist[2] = hist[3];
                hist[3] = (1.f - morph) * (2.f * lx - 1.f) + morph * hx * 0.75f;
            }
            const float t = (float) cnt;
            const float o = clock <= 1 ? hist[3] : (clock == 2 ? hist[2] + (hist[3] - hist[2]) * t : catmull (hist[0], hist[1], hist[2], hist[3], t));
            lp = o + a1 * (lp - o);
            L[k] = R[k] = lp * 0.7f * comp;
        }
    }
};

//==============================================================================
// 6. Chua double scroll
struct EChua
{
    float x = 0.1f, y = 0, z = 0, sr = 48000.f;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { x = 0.1f + 0.1f * hashF (seed); y = 0; z = 0; }
    void render (const float* p, const int*, float f0, float* L, float* R, int n)
    {
        const float al = p[0], be = p[1], om = p[2];
        const float m0 = -8.f / 7.f, m1 = -5.f / 7.f;
        const float h = std::min (f0 * om * Cal::chuaRate / sr, 0.25f);
        auto F = [&] (float X, float Y, float Z, float& dx, float& dy, float& dz)
        {
            const float fx = m1 * X + 0.5f * (m0 - m1) * (std::fabs (X + 1.f) - std::fabs (X - 1.f));
            dx = al * (Y - X - fx); dy = X - Y + Z; dz = -be * Y;
        };
        for (int k = 0; k < n; ++k)
        {
            float a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4;
            F (x, y, z, a1, b1, c1);
            F (x + 0.5f * h * a1, y + 0.5f * h * b1, z + 0.5f * h * c1, a2, b2, c2);
            F (x + 0.5f * h * a2, y + 0.5f * h * b2, z + 0.5f * h * c2, a3, b3, c3);
            F (x + h * a3, y + h * b3, z + h * c3, a4, b4, c4);
            x += h / 6.f * (a1 + 2 * a2 + 2 * a3 + a4); y += h / 6.f * (b1 + 2 * b2 + 2 * b3 + b4); z += h / 6.f * (c1 + 2 * c2 + 2 * c3 + c4);
            if (! finiteF (x) || std::fabs (x) > 20.f || std::fabs (z) > 50.f) { x = 0.1f; y = z = 0; }
            L[k] = R[k] = x * 0.3f;
        }
    }
};

//==============================================================================
// 7. Gray-Scott reaction-diffusion ring (pitch-inert)
struct EGray
{
    static constexpr int N = 24;
    float u[N] {}, v[N] {}, nu[N] {}, nv[N] {}; float sr = 48000.f, dph = 0, hold = 0; Rng rng; SVF bp; SVFCoefs bpc;
    float wA[N] {}, wB[N] {};
    void prepare (float s)
    {
        sr = s;
        for (int i = 0; i < N; ++i) { wA[i] = std::sin (k2Pi * 2.f * (float) i / N); wB[i] = std::cos (k2Pi * 5.f * (float) i / N); }
    }
    void seedSpot (int at) { for (int d = -1; d <= 1; ++d) { int j = (at + d + N) % N; u[j] = 0.5f; v[j] = 0.25f; } }
    void reset (uint32_t seed)
    {
        rng.seed (seed);
        for (int i = 0; i < N; ++i) { u[i] = 1.f; v[i] = 0.f; }
        for (int k = 0; k < 3; ++k) seedSpot ((int) (rng.next() * N));
        bp.reset(); dph = rng.next();
    }
    void render (const float* p, const int*, float f0, float* L, float* R, int n)
    {
        const float F = p[0], K = p[1], drift = p[2], bpf = p[3], Q = p[4];
        const float Du = 0.32f, Dv = 0.16f, s = 2.f;
        bpc = svfMake (2, f0, sr, Q);
        for (int k = 0; k < n; ++k)
        {
            dph += 0.21f / sr; if (dph >= 1.f) dph -= 1.f;
            const float Fe = F * (1.f + drift * 0.18f * sinT (dph)), Ke = K * (1.f + drift * 0.05f * sinT (dph * 1.37f + 0.3f));
            float sumV = 0, o = 0;
            for (int i = 0; i < N; ++i)
            {
                const int im = (i + N - 1) % N, ip = (i + 1) % N;
                const float lu = u[im] + u[ip] - 2.f * u[i], lv = v[im] + v[ip] - 2.f * v[i];
                const float uvv = u[i] * v[i] * v[i];
                nu[i] = clampf (u[i] + Du * lu + s * (-uvv + Fe * (1.f - u[i])), 0.f, 1.5f);
                nv[i] = clampf (v[i] + Dv * lv + s * (uvv - (Fe + Ke) * v[i]), 0.f, 1.5f);
            }
            for (int i = 0; i < N; ++i)
            {
                const float dv = nv[i] - v[i];
                u[i] = nu[i]; v[i] = nv[i]; sumV += v[i];
                o += v[i] * wA[i] + dv * wB[i] * 40.f;
            }
            if (sumV < 0.05f && drift > 0.f && rng.next() < drift * 0.02f) seedSpot ((int) (rng.next() * N));
            o *= 0.9f;
            const float b = bp.tick (o, bpc) * (1.f + std::sqrt (Q) * 0.6f);
            const float out = (1.f - bpf) * o + bpf * b;
            L[k] = R[k] = fastTanh (out * 0.9f);
        }
    }
};

//==============================================================================
// 8. Additive bank steered by a sub-audio Lorenz
struct ESpectral
{
    static constexpr int N = 32;
    float c_[N] {}, s_[N] {}, amp[N] {}, tgt[N] {}; float lx = 1, ly = 1, lz = 20, sr = 48000.f;
    bool hq = true; static constexpr int T = 31; float hb[T] {}, hist[T] {};    // Engine HQ: 2x oversampling + half-band decimator
    void prepare (float s)
    {
        sr = s;
        for (int k = 0; k < T; ++k)
        {
            const float m = (float) (k - T / 2); const float sinc = m == 0.f ? 0.5f : std::sin (kPi * 0.5f * m) / (kPi * m);
            const float win = 0.42f - 0.5f * std::cos (k2Pi * k / (T - 1)) + 0.08f * std::cos (2.f * k2Pi * k / (T - 1));
            hb[k] = sinc * win;
        }
        float sum = 0; for (float v : hb) sum += v; for (float& v : hb) v /= sum;
    }
    void reset (uint32_t seed)
    {
        lx = 0.5f + hashF (seed); ly = 1.f; lz = 20.f;
        for (int i = 0; i < N; ++i) { float th = hashF (seed * 17u + (uint32_t) i) * k2Pi; c_[i] = std::cos (th); s_[i] = std::sin (th); amp[i] = 0; }
    }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        if (! hq) { core (p, c, f0, L, n, sr); for (int k = 0; k < n; ++k) R[k] = L[k]; return; }
        float up[64];
        core (p, c, f0, up, 2 * n, 2.f * sr);
        for (int k = 0; k < n; ++k)
        {
            for (int j = 0; j < 2; ++j) { for (int t = T - 1; t > 0; --t) hist[t] = hist[t - 1]; hist[0] = up[2 * k + j]; }
            float o = 0; for (int t = 0; t < T; ++t) o += hist[t] * hb[t];
            L[k] = R[k] = o;
        }
    }
    void core (const float* p, const int*, float f0, float* L, int n, float sr)
    {
        const float chaos = p[0], dens = p[1], smooth = p[2], drive = p[3];
        // sub-audio Lorenz step
        const float h = 1.5f * chaos * (float) n / sr;
        { const float sg = 10.f, rho = 28.f, bt = 8.f / 3.f;
          for (int it = 0; it < 2; ++it)
          { float dx = sg * (ly - lx), dy = lx * (rho - lz) - ly, dz = lx * ly - bt * lz; lx += dx * h * 0.5f; ly += dy * h * 0.5f; lz += dz * h * 0.5f; }
          if (! finiteF (lx)) { lx = 1; ly = 1; lz = 20; } }
        const float X = lx / 18.f, Y = ly / 24.f, Z = (lz - 24.f) / 16.f;
        float norm = 0;
        for (int i = 0; i < N; ++i)
        {
            const float roll = 1.f / std::pow ((float) (i + 1), 0.85f);
            float a = 0.5f + 0.5f * std::sin (3.1f * X * (float) (i % 7 + 1) * 0.5f + 2.3f * Y * (float) (i % 5 + 1) * 0.5f + 1.7f * Z * (float) ((i * 3) % 4 + 1) * 0.5f + (float) i * 1.3f);
            float dmul = clampf (dens - (float) i, 0.f, 1.f);
            if ((float) (i + 1) * f0 > sr * 0.45f) dmul = 0;
            tgt[i] = a * roll * dmul;
            norm += roll * dmul;
        }
        const float sk = 1.f - std::exp (-(float) n / (sr * (0.002f + smooth * smooth * 0.6f)));
        float inc[N], nAmp[N];
        for (int i = 0; i < N; ++i) { nAmp[i] = amp[i] + (tgt[i] - amp[i]) * sk; inc[i] = (nAmp[i] - amp[i]) / (float) n; }
        float cw[N], sw[N];
        for (int i = 0; i < N; ++i) { const float w = k2Pi * f0 * (float) (i + 1) / sr; cw[i] = std::cos (w); sw[i] = std::sin (w); }
        const float g = 2.2f / std::max (norm, 0.5f), dg = std::max (drive * 2.f, 0.05f), dn = 1.f / fastTanh (dg);
        for (int k = 0; k < n; ++k)
        {
            float o = 0;
            for (int i = 0; i < N; ++i)
            {
                amp[i] += inc[i];
                o += amp[i] * s_[i];
                const float nc = c_[i] * cw[i] - s_[i] * sw[i], ns = s_[i] * cw[i] + c_[i] * sw[i];
                c_[i] = nc; s_[i] = ns;
            }
            L[k] = fastTanh (o * g * dg) * dn * 0.6f;
        }
        for (int i = 0; i < N; ++i) { amp[i] = nAmp[i]; const float m = 1.5f - 0.5f * (c_[i] * c_[i] + s_[i] * s_[i]); c_[i] *= m; s_[i] *= m; }
    }
};

//==============================================================================
// 9. 2-D morphing wavetable woven into grains, rewritten by a chaos modulator
struct EWaveGrain
{
    static constexpr int G = 16;
    struct Grain { bool on = false; float age = 0, len = 1, ph = 0, mph = 0, inc = 0, gl = 1, gr = 1, x = 0, y = 0, fold = 0; };
    Grain g[G]; float sr = 48000.f, spawnAcc = 1e9f, nextSpawn = 0, prevOut = 0;
    float ax = 0.1f, ay = 0, az = 0, adph = 0, sx = 0, sy = 0, sf = 0; Rng rng;

    void prepare (float s) { sr = s; }
    void reset (uint32_t seed)
    {
        rng.seed (seed); for (auto& q : g) q.on = false;
        spawnAcc = 1e9f; ax = 0.1f + 0.2f * rng.next(); ay = 0.1f; az = 0.1f; adph = 0; sx = sy = sf = 0; prevOut = 0;
    }
    static inline float blep (float t, float dt)
    {
        if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }
    static inline float wave (float p, float dt, float x, float y, float pw, float fold)
    {
        p -= std::floor (p);
        dt = clampf (dt, 1e-5f, 0.45f);
        const float sn = sinT (p);
        const float tri = 1.f - 4.f * std::fabs (p - 0.5f);
        const float saw = 2.f * p - 1.f - blep (p, dt);
        float q = p - pw; q -= std::floor (q);
        const float sq = (p < pw ? 1.f : -1.f) + blep (p, dt) - blep (q, dt);
        float m = (1 - x) * (1 - y) * sn + x * (1 - y) * tri + (1 - x) * y * saw + x * y * sq;
        if (fold > 0.001f) m = (1.f - fold) * m + fold * sinT (0.25f * m * (1.f + 6.f * fold));
        return m;
    }
    void stepChaos (int type, float rate, int n)
    {
        static const float scale[4] = { 0.7f, 6.f, 3.f, 1.4f };
        float h = rate * scale[type] * (float) n / sr;
        int sub = 1 + (int) (h / 0.01f); h /= (float) sub; sub = std::min (sub, 32);
        for (int s = 0; s < sub; ++s)
        {
            float dx, dy, dz;
            switch (type)
            {
                case 0: dx = 10.f * (ay - ax); dy = ax * (28.f - az) - ay; dz = ax * ay - 8.f / 3.f * az; break;
                case 1: dx = -ay - az; dy = ax + 0.2f * ay; dz = 0.2f + az * (ax - 5.7f); break;
                case 2: { float fx = -5.f / 7.f * ax + 0.5f * (-8.f / 7.f + 5.f / 7.f) * (std::fabs (ax + 1) - std::fabs (ax - 1));
                          dx = 9.f * (ay - ax - fx); dy = ax - ay + az; dz = -14.286f * ay; } break;
                default: adph += h * 1.2f / k2Pi; dx = ay; dy = -0.25f * ay + ax - ax * ax * ax + 0.4f * std::cos (k2Pi * adph); dz = 0; break;
            }
            ax += dx * h; ay += dy * h; az += dz * h;
        }
        if (! finiteF (ax) || std::fabs (ax) > 100.f) { ax = 0.1f; ay = 0.1f; az = 0.1f; }
    }
    void axes (int type, float& nx, float& ny, float& nf) const
    {
        switch (type)
        {
            case 0: nx = ax / 18.f; ny = ay / 24.f; nf = (az - 24.f) / 16.f; break;
            case 1: nx = ax / 10.f; ny = ay / 10.f; nf = az / 12.f - 0.3f; break;
            case 2: nx = ax / 2.2f; ny = ay / 0.4f; nf = az / 3.f; break;
            default: nx = ax / 1.4f; ny = ay / 1.2f; nf = ax * ay; break;
        }
        nx = clampf (nx, -1.f, 1.f); ny = clampf (ny, -1.f, 1.f); nf = clampf (nf, -1.f, 1.f);
    }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float X = p[0], Y = p[1], pw = p[2], fold = p[3], sizeMs = p[4], ratio = p[5], crate = p[6];
        const float amX = p[7], amY = p[8], amF = p[9], tau = p[10];
        const int count = (int) std::lround (clampf (p[11], 1.f, 16.f));
        const float shape = p[12], scatter = p[13], jitter = p[14], spray = p[15], fb = p[16];
        const int ctype = c[0], gmode = c[1]; const bool rev = c[2] == 1;

        stepChaos (ctype, crate, n);
        float nx, ny, nf; axes (ctype, nx, ny, nf);
        const float tk = 1.f - std::exp (-(float) n / (sr * (0.005f + tau * 0.8f)));
        sx += (nx - sx) * tk; sy += (ny - sy) * tk; sf += (nf - sf) * tk;
        const float cX = clampf (X + amX * sx, 0.f, 1.f), cY = clampf (Y + amY * sy, 0.f, 1.f), cF = clampf (fold + amF * sf, 0.f, 1.f);

        const float lenS = std::max (sizeMs * 0.001f * sr, 16.f);
        const float interval = lenS / (float) count;
        const float baseInc = f0 / sr;
        const float fAmt = std::min (1.f, std::fabs (std::log2 (ratio)) * 3.f);
        const float norm = 0.85f / std::sqrt ((float) count);

        for (int k = 0; k < n; ++k)
        {
            spawnAcc += 1.f;
            if (spawnAcc >= nextSpawn)
            {
                spawnAcc = 0; nextSpawn = interval * (1.f + scatter * rng.bi() * 0.9f);
                int slot = -1; float oldest = -1;
                for (int i = 0; i < G; ++i) { if (! g[i].on) { slot = i; break; } if (g[i].age / g[i].len > oldest) { oldest = g[i].age / g[i].len; slot = i; } }
                Grain& q = g[slot];
                q.on = true; q.age = 0; q.len = lenS;
                q.inc = baseInc * std::exp2 (jitter * rng.bi() * 0.5f / 12.f) * (rev ? -1.f : 1.f);
                q.ph = scatter * rng.next(); q.mph = 0;
                const float pan = clampf (spray * rng.bi(), -1.f, 1.f);
                q.gl = std::sqrt (0.5f * (1.f - pan)) * 1.41421f; q.gr = std::sqrt (0.5f * (1.f + pan)) * 1.41421f;
                q.x = cX; q.y = cY; q.fold = cF;
            }
            float oL = 0, oR = 0;
            for (int i = 0; i < G; ++i)
            {
                Grain& q = g[i];
                if (! q.on) continue;
                const float t = q.age / q.len;
                const float sw = sinT (0.5f * t);
                const float w = (1.f - shape) * sw * sw + shape * std::min (1.f, sw * 3.f);
                const float gx = gmode == 0 ? q.x : cX, gy = gmode == 0 ? q.y : cY, gf = gmode == 0 ? q.fold : cF;
                q.mph += std::fabs (q.inc);
                if (q.mph >= 1.f) { q.mph -= 1.f; if (fAmt > 0.f) q.ph = 0.f; }
                q.ph += q.inc * ratio + fb * 0.04f * prevOut;
                float v = wave (q.ph, std::fabs (q.inc * ratio), gx, gy, pw, gf);
                if (fAmt > 0.f) v *= (1.f - fAmt) + fAmt * sinT (0.5f * q.mph);
                v *= w;
                oL += v * q.gl; oR += v * q.gr;
                q.age += 1.f; if (q.age >= q.len) q.on = false;
            }
            oL *= norm; oR *= norm;
            prevOut = 0.5f * (oL + oR);
            L[k] = oL; R[k] = oR;
        }
    }
};

//==============================================================================
// 10. Bowed string (digital waveguide, friction bow, body)
struct EBowed
{
    struct Str
    {
        Delay neck, bridge; float lp = 0, apz = 0;
        void init (int n) { neck.init (n); bridge.init (n); lp = apz = 0; }
        void clear() { neck.clear(); bridge.clear(); lp = apz = 0; }
        inline float tick (float bowV, float slope, float force, float raspN, float nD, float bD, float lossG, float pole, float apA)
        {
            const float bOut = bridge.read (bD), nOut = neck.read (nD);
            lp = (1.f - pole) * bOut + pole * lp;
            const float s = lp * lossG;
            const float ap = apA * s + apz; apz = s - apA * ap;
            const float bridgeRefl = -ap, nutRefl = -nOut;
            const float dv = bowV - (bridgeRefl + nutRefl) + raspN;
            float q = std::fabs (dv * slope) + 0.75f; q *= q;
            const float tbl = std::min (1.f, 1.f / (q * q)) * force;
            const float nv = dv * tbl;
            neck.write (clampf (bridgeRefl + nv, -4.f, 4.f));
            bridge.write (clampf (nutRefl + nv, -4.f, 4.f));
            return bOut;
        }
    };
    Str s[2]; float sr = 48000.f, bowV = 0; Rng rng; SVF body[2][3]; SVFCoefs bc[3]; bool inited = false;
    void prepare (float sRate)
    {
        sr = sRate; const int n = (int) (sr / 22.f) + 16;
        s[0].init (n); s[1].init (n);
        for (int i = 0; i < 3; ++i) bc[i] = svfMake (2, i == 0 ? 230.f : (i == 1 ? 600.f : 1600.f), sr, 2.5f);
    }
    void reset (uint32_t seed) { s[0].clear(); s[1].clear(); bowV = 0; rng.seed (seed); for (auto& c : body) for (auto& b : c) b.reset(); }
    void render (const float* p, const int*, float f0, float* L, float* R, int n)
    {
        const float press = p[0], speed = p[1], pos = p[2], loss = p[3], rasp = p[4], bright = p[5], stiff = p[6], bodyAmt = p[7];
        const float slope = 4.f - 3.4f * press;
        const float force = clampf ((press - 0.05f) / 0.1f, 0.f, 1.f);
        const float lossG = 0.995f - loss * 0.1f;
        const float pole = 0.85f - 0.45f * bright;
        const float apA = -stiff * 0.75f;
        const float comp = pole / (1.f - pole) + (1.f - apA) / (1.f + apA) - 0.1f;
        const float beta = 0.04f + pos * 0.46f;
        const float targetV = speed * 0.25f;
        const float bk = 1.f - msCoef (8.f, sr);
        const float maxD = (float) s[0].neck.b.size() - 4.f;
        float nD[2], bD[2];
        for (int c = 0; c < 2; ++c)
        {
            const float f = std::max (f0, 23.f) * (c == 0 ? 0.99884f : 1.00116f);
            const float base = clampf (sr / f - comp, 4.f, 2.f * maxD - 4.f);
            bD[c] = clampf (base * beta, 1.5f, maxD); nD[c] = clampf (base * (1.f - beta), 1.5f, maxD);
        }
        for (int k = 0; k < n; ++k)
        {
            bowV += (targetV - bowV) * bk;
            float o[2];
            for (int c = 0; c < 2; ++c)
            {
                const float rn = rasp * 0.4f * rng.bi() * bowV;
                const float x = s[c].tick (bowV, slope, force, rn, nD[c], bD[c], lossG, pole, apA);
                float bsum = 0; for (int i = 0; i < 3; ++i) bsum += body[c][i].tick (x, bc[i]);
                o[c] = ((1.f - bodyAmt) * x + bodyAmt * bsum * 0.9f) * 1.6f;
            }
            L[k] = o[0]; R[k] = o[1];
        }
    }
};

//==============================================================================
// 11. Mackey-Glass relative: delay loop with steep non-linearity, self-tuning
struct EMackey
{
    Delay buf; float y = 0.3f, yPrev = 0, sr = 48000.f, corr = 0.45f, dcz = 0, prevSig = 0, lastPer = 0; int since = 0, locked = 0; Rng rng;
    void prepare (float s) { sr = s; buf.init ((int) (s / 16.f) + 16); }
    void reset (uint32_t seed) { buf.clear(); y = 0.5f; yPrev = 0; corr = 0.45f; since = 0; lastPer = 0; dcz = 0; prevSig = 0; locked = 0; rng.seed (seed); }
    static inline float powAbs (float x, float e) { const float a = std::fabs (x); return a < 1e-9f ? 0.f : std::exp2 (e * std::log2 (a)); }
    void render (const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        const float drive = p[0], speed = p[1], tap = p[2], loss = p[3], sharp = p[4], bright = p[5], ember = p[6], tap2 = p[7], res = p[8];
        const int curve = c[0], octave = c[1];
        const float target = sr / std::max (f0, 18.f);
        const float maxD = (float) buf.b.size() - 4.f;
        const float D = clampf (target * corr, 4.f, maxD);
        const float beta = (1.f + drive * 3.f) * (1.f + res * 1.5f);
        const float gam = beta * (0.25f + loss * 0.35f);            // γ/β stays in the oscillating half
        const float dt = (0.3f + speed * 6.f) * (curve == 1 ? 1.6f : 1.f) / D;
        const float pe = 5.5f + sharp * 9.f, pc = 5.5f + sharp * 18.f;
        const float d2 = std::max (2.f, D * (0.05f + 0.95f * tap));
        for (int k = 0; k < n; ++k)
        {
            const float u = buf.read (D), u2 = buf.read (d2);
            const float in = (u + tap2 * u2) / (1.f + tap2);
            const float f = in / (1.f + powAbs (in, curve == 0 ? pe : pc));
            y += dt * (beta * f - gam * y);
            if (ember > 0.f && rng.next() < ember * ember * 0.0015f) y += rng.bi() * ember * 0.8f;
            y = clampf (y, -6.f, 6.f);
            if (! finiteF (y)) y = 0.3f;
            buf.write (y);
            // pitch tracker on DC-removed signal
            dcz += (y - dcz) * 0.0015f;
            const float sig = y - dcz;
            ++since;
            if (prevSig <= 0.f && sig > 0.f)
            {
                const float per = (float) since; since = 0;
                if (lastPer > 0.f && std::fabs (per - lastPer) < 0.06f * per)
                {
                    float meas = per, ratioM = meas / target;
                    // Fixed: once locked, a period-doubling is heard as a sub-octave underneath, not retuned
                    if (octave == 0 && locked > 8 && ratioM > 1.8f && ratioM < 2.25f) meas *= 0.5f;
                    ratioM = meas / target;
                    if (ratioM > 0.3f && ratioM < 4.f) corr = clampf (corr * std::pow (target / meas, 0.3f), 0.1f, 3.f);
                    if (std::fabs (ratioM - 1.f) < 0.03f) ++locked; else if (ratioM < 1.8f || ratioM > 2.25f) locked = 0;
                }
                lastPer = per;
            }
            prevSig = sig;
            const float o = sig + bright * (y - yPrev) * 3.f;
            yPrev = y;
            const float out = fastTanh (o * (0.6f + bright * 1.2f)) * 0.8f;
            L[k] = R[k] = out;
        }
    }
};

//==============================================================================
// 12. Circle map (Arnold tongues) with coupled slave gear
struct ECircle
{
    float tA = 0, tB = 0, fr = 0, sA = 0, sB = 0, fr2 = 0, lp = 0, sr = 48000.f; Rng rng;
    void prepare (float s) { sr = s; }
    void reset (uint32_t seed) { tA = tB = 0; sA = sB = 0; fr = fr2 = 0; lp = 0; rng.seed (seed); }
    void render (const float* p, const int*, float f0, float* L, float* R, int n)
    {
        const float lock = p[0], ratio = p[1], bias = p[2], slew = p[3], tongue = p[4], shape = p[5], slave = p[6], couple = p[7];
        const float it = std::min (2.f * f0 / sr, 0.9f);
        const float Om = 0.5f + (ratio - 0.5f) * 1.25f;
        const float Kc = lock / 0.9f * (1.f + tongue * 0.4f) / k2Pi;
        const float bOff = bias - 0.5f;
        const float drv = 1.f + shape * 8.f, dn = 1.f / fastTanh (drv);
        const float fc = 18000.f * std::pow (0.015f, slew);
        const float la = std::exp (-k2Pi * std::min (fc, sr * 0.45f) / sr);
        for (int k = 0; k < n; ++k)
        {
            fr += it;
            while (fr >= 1.f)
            {
                fr -= 1.f; tA = tB;
                const float jit = tongue * tongue * 0.04f * rng.bi();
                tB = tB + Om + jit - Kc * sinT (tB + bOff) + couple * 0.05f / k2Pi * sinT (0.5f * sB - tB);
                const float off = std::floor (tA); tA -= off; tB -= off;
            }
            fr2 += it * 2.f;
            while (fr2 >= 1.f)
            {
                fr2 -= 1.f; sA = sB;
                sB = sB + Om * slave - Kc * sinT (sB + bOff) + couple * 0.2f / k2Pi * sinT (2.f * tB - sB);
                const float off = std::floor (sA); sA -= off; sB -= off;
            }
            const float ph1 = tA + (tB - tA) * fr, ph2 = sA + (sB - sA) * fr2;
            const float w1 = fastTanh (sinT (ph1) * drv) * dn, w2 = fastTanh (sinT (ph2) * drv) * dn;
            const float o = (0.8f - 0.3f * couple) * w1 + (0.2f + 0.3f * couple) * w2;
            lp = o + la * (lp - o);
            L[k] = R[k] = lp * 0.7f;
        }
    }
};

//==============================================================================
struct EngineBank
{
    EKuramoto kur; EDuffing duf; ESprott spr; ELorRos lr; EMaps maps; EChua chua; EGray gray; ESpectral spec; EWaveGrain wg; EBowed bow; EMackey mg; ECircle cm;
    EGendy gen; EMapZoo mz; EEco eco; ENoise nz; EMembrane mb;
    bool started[18] = {};
    void setHQ (bool on) { spec.hq = on; }
    void prepare (float sr)
    {
        kur.prepare (sr); duf.prepare (sr); spr.prepare (sr); lr.prepare (sr); maps.prepare (sr); chua.prepare (sr);
        gray.prepare (sr); spec.prepare (sr); wg.prepare (sr); bow.prepare (sr); mg.prepare (sr); cm.prepare (sr);
        gen.prepare (sr); mz.prepare (sr); eco.prepare (sr); nz.prepare (sr); mb.prepare (sr);
        for (auto& b : started) b = false;
    }
    // freeMode: engines with a NOTE menu set to "Free" keep their state across notes
    void noteOn (int type, uint32_t seed, bool freeMode)
    {
        if (type <= 0 || type > 17) return;
        if (freeMode && started[type]) return;
        started[type] = true;
        switch (type)
        {
            case 1: kur.reset (seed); break;  case 2: duf.reset (seed); break;  case 3: spr.reset (seed); break;
            case 4: lr.reset (seed); break;   case 5: maps.reset (seed); break; case 6: chua.reset (seed); break;
            case 7: gray.reset (seed); break; case 8: spec.reset (seed); break; case 9: wg.reset (seed); break;
            case 10: bow.reset (seed); break; case 11: mg.reset (seed); break;  case 12: cm.reset (seed); break;
            case 13: gen.reset (seed); break; case 14: mz.reset (seed); break;  case 15: eco.reset (seed); break;
            case 16: nz.reset (seed); break;  case 17: mb.reset (seed); break;
            default: break;
        }
    }
    void render (int type, const float* p, const int* c, float f0, float* L, float* R, int n)
    {
        switch (type)
        {
            case 1: kur.render (p, c, f0, L, R, n); break;   case 2: duf.render (p, c, f0, L, R, n); break;
            case 3: spr.render (p, c, f0, L, R, n); break;   case 4: lr.render (p, c, f0, L, R, n); break;
            case 5: maps.render (p, c, f0, L, R, n); break;  case 6: chua.render (p, c, f0, L, R, n); break;
            case 7: gray.render (p, c, f0, L, R, n); break;  case 8: spec.render (p, c, f0, L, R, n); break;
            case 9: wg.render (p, c, f0, L, R, n); break;    case 10: bow.render (p, c, f0, L, R, n); break;
            case 11: mg.render (p, c, f0, L, R, n); break;   case 12: cm.render (p, c, f0, L, R, n); break;
            case 13: gen.render (p, c, f0, L, R, n); break;  case 14: mz.render (p, c, f0, L, R, n); break;
            case 15: eco.render (p, c, f0, L, R, n); break;  case 16: nz.render (p, c, f0, L, R, n); break;
            case 17: mb.render (p, c, f0, L, R, n); break;
            default: for (int k = 0; k < n; ++k) L[k] = R[k] = 0; break;
        }
    }
};
} // namespace cz
