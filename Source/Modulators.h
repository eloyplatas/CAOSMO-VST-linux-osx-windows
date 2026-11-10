// Caosmo — VENV vector envelopes, attractor modulators, keytrack curve, LFO evaluation
#pragma once
#include "DSP.h"
#include <juce_core/juce_core.h>
#include <climits>

namespace cz
{
//==============================================================================
// Drawable shape: breakpoints x∈[0,1] ascending, y∈[0,1], c = curvature of the segment to the next point (-1..1)
struct VShape
{
    static constexpr int kMax = 64;
    int n = 2; float x[kMax] {}, y[kMax] {}, c[kMax] {};
    VShape() { x[0] = 0; y[0] = 0; x[1] = 1; y[1] = 0; }

    static inline float bend (float t, float c)
    {
        if (c > 0.001f) return std::pow (t, 1.f + c * 5.f);
        if (c < -0.001f) return 1.f - std::pow (1.f - t, 1.f - c * 5.f);
        return t;
    }
    float evalSeg (int s, float p) const
    {
        const float w = x[s + 1] - x[s];
        float t = w > 1e-6f ? (p - x[s]) / w : 1.f;
        t = clampf (t, 0.f, 1.f);
        return y[s] + (y[s + 1] - y[s]) * bend (t, c[s]);
    }
    float eval (float p) const
    {
        if (n < 2) return n == 1 ? y[0] : 0.f;
        if (p <= x[0]) return y[0];
        if (p >= x[n - 1]) return y[n - 1];
        int s = 0; while (s < n - 2 && p > x[s + 1]) ++s;
        return evalSeg (s, p);
    }
    void sanitise()
    {
        n = juce::jlimit (2, kMax, n);
        x[0] = 0.f; x[n - 1] = 1.f;
        for (int i = 1; i < n; ++i) x[i] = std::max (x[i], x[i - 1]);
        for (int i = 0; i < n; ++i) { y[i] = clampf (y[i], 0.f, 1.f); c[i] = clampf (c[i], -1.f, 1.f); }
    }
    juce::String toString() const
    {
        juce::String s;
        for (int i = 0; i < n; ++i) s << juce::String (x[i], 4) << "," << juce::String (y[i], 4) << "," << juce::String (c[i], 3) << (i < n - 1 ? ";" : "");
        return s;
    }
    static VShape fromString (const juce::String& str)
    {
        VShape s; s.n = 0;
        for (auto& tok : juce::StringArray::fromTokens (str, ";", ""))
        {
            auto p = juce::StringArray::fromTokens (tok, ",", "");
            if (p.size() < 2 || s.n >= kMax) continue;
            s.x[s.n] = p[0].getFloatValue(); s.y[s.n] = p[1].getFloatValue(); s.c[s.n] = p.size() > 2 ? p[2].getFloatValue() : 0.f; ++s.n;
        }
        if (s.n < 2) return VShape();
        s.sanitise(); return s;
    }
};

// 100 factory shapes in banks A–J
int numFactoryShapes();
VShape factoryShape (int index);
juce::String factoryShapeName (int index);

//==============================================================================
struct VenvPlayer
{
    float pos = 0, out = 0; bool running = false, gate = false; int seg = 0;
    void trigger() { pos = 0; running = true; gate = true; seg = 0; }
    void release() { gate = false; }
    void kill() { running = false; gate = false; out = 0; pos = 0; seg = 0; }
    bool done() const { return ! running; }
    // inc: position advance per call; mode 0 1-shot, 1 sustain, 2 sustain-loop
    inline float tick (const VShape& s, float inc, int mode, float pa, float pd, float ps, const float* zl)
    {
        if (! running) return out;
        pd = std::max (pd, pa); ps = std::max (ps, pd);
        pos += inc;
        if (gate && mode == 1 && pos >= ps) pos = ps;
        else if (gate && mode == 2 && pos >= ps) { if (ps - pd > 0.002f) { pos = pd + std::fmod (pos - ps, ps - pd); seg = 0; } else pos = ps; }
        if (pos >= 1.f) { pos = 1.f; running = false; }
        if (pos < s.x[seg]) seg = 0;
        while (seg < s.n - 2 && pos > s.x[seg + 1]) ++seg;
        float y = s.n >= 2 ? s.evalSeg (seg, pos) : 0.f;
        const int z = pos < pa ? 0 : (pos < pd ? 1 : (pos < ps ? 2 : 3));
        out = clampf (y * zl[z], 0.f, 1.f);
        return out;
    }
};

// ---------- lane envelope: ADSR / Vactrol / LFO ----------
struct Env
{
    enum Stage { Idle, Delay, Attack, Decay, Sustain, Release };
    float sr = 48000.f;
    int mode = 0; bool oneShot = false;
    Stage st = Idle; float out = 0, t = 0, relStart = 0, atkStart = 0;
    float del = 0, att = 0.005f, dec = 0.3f, sus = 0.8f, rel = 0.4f; int cA = 0, cD = 0, cR = 0;
    // vactrol
    bool gate = false; float vf = 0, vs = 0, hist = 0, driftMul = 1.f;
    float rf = 5, ff = 80, rs = 120, fsl = 900, vmix = 0.5f, vmem = 0.3f, vshape = 0.4f, vdrift = 0.1f;
    float kRF = 0, kFF = 0, kRS = 0, kFS = 0;
    // lfo env
    double lph = 0; float lrate = 2.f; int lshape = 0; float lfade = 1.f; bool lgate = false; float lsm = 0;
    // VENV mode
    VenvPlayer vp; const VShape* vsh = nullptr; float vinc = 0; int vmode = 1; float vpa = 0.15f, vpd = 0.4f, vps = 0.7f; float vz[4] { 1, 1, 1, 1 };

    static inline float shapeRise (float x, int c) { return c == 0 ? x : (c == 1 ? 1.f - (1.f - x) * (1.f - x) * (1.f - x) : x * x * x); }
    static inline float shapeFall (float x, int c) { return c == 0 ? 1.f - x : (c == 1 ? (1.f - x) * (1.f - x) * (1.f - x) : 1.f - x * x * x); }

    void noteOn (float rnd)
    {
        if (mode == 0) { atkStart = out; t = 0; st = del > 0.0005f ? Delay : Attack; }
        else if (mode == 1) { gate = true; driftMul = 1.f + vdrift * 0.35f * rnd; st = Attack; }
        else if (mode == 2) { lph = 0; lgate = true; lfade = 1.f; st = Attack; }
        else { vp.trigger(); st = Attack; }
    }
    void noteOff()
    {
        if (mode == 0) { if (! oneShot && st != Idle) { relStart = out; t = 0; st = Release; } }
        else if (mode == 1) { gate = false; }
        else if (mode == 2) { if (! oneShot) lgate = false; }
        else { if (vmode != 0) vp.release(); }
    }
    void kill() { st = Idle; out = 0; gate = false; vf = vs = 0; lgate = false; lsm = 0; vp.kill(); }
    bool idle() const { return st == Idle; }
    void updateVactrol()
    {
        float dm = std::max (0.2f, driftMul), mem = 1.f + vmem * 4.f * hist;
        kRF = 1.f - msCoef (rf * dm, sr); kFF = 1.f - msCoef (ff * dm * mem, sr);
        kRS = 1.f - msCoef (rs * dm, sr); kFS = 1.f - msCoef (fsl * dm * mem, sr);
    }
    inline float tick()
    {
        if (mode == 0)
        {
            switch (st)
            {
                case Idle: out = 0; break;
                case Delay: t += 1.f / (del * sr); out = 0; if (t >= 1.f) { t = 0; atkStart = 0; st = Attack; } break;
                case Attack: t += 1.f / (std::max (att, 0.0003f) * sr);
                    if (t >= 1.f) { t = 0; out = 1.f; st = Decay; } else out = atkStart + (1.f - atkStart) * shapeRise (t, cA); break;
                case Decay: t += 1.f / (std::max (dec, 0.0005f) * sr);
                    if (t >= 1.f) { t = 0; out = sus; if (oneShot) { relStart = out; st = Release; } else st = Sustain; }
                    else out = sus + (1.f - sus) * shapeFall (t, cD); break;
                case Sustain: out += (sus - out) * 0.002f; if (oneShot) { relStart = out; t = 0; st = Release; } break;
                case Release: t += 1.f / (std::max (rel, 0.0005f) * sr);
                    if (t >= 1.f) { t = 0; out = 0; st = Idle; } else out = relStart * shapeFall (t, cR); break;
            }
            return out;
        }
        if (mode == 1)
        {
            const float g = gate ? 1.f : 0.f;
            vf += (g - vf) * (g > vf ? kRF : kFF);
            vs += (g - vs) * (g > vs ? kRS : kFS);
            float light = vmix * vs + (1.f - vmix) * vf;
            hist += (light - hist) * 0.00002f;
            out = std::pow (clampf (light, 0.f, 1.f), 1.f + vshape * 2.f);
            if (! gate && light < 0.0002f) { st = Idle; out = 0; }
            return out;
        }
        if (mode == 3)
        {
            out = vsh ? vp.tick (*vsh, vinc, vmode, vpa, vpd, vps, vz) : 0.f;
            if (vp.done()) { st = Idle; out = 0; }
            return out;
        }
        // LFO envelope
        lph += (double) lrate / sr;
        if (lph >= 1.0) { if (oneShot) { st = Idle; lsm = out = 0; return 0; } lph -= 1.0; }
        const float p = (float) lph;
        float v;
        switch (lshape)
        {
            case 0: v = 0.5f - 0.5f * sinT (p + 0.25f); break;
            case 1: v = 1.f - std::fabs (2.f * p - 1.f); break;
            case 2: v = 1.f - p; break;
            case 3: v = p < 0.5f ? 1.f : 0.f; break;
            default: v = p; break;
        }
        if (! lgate) { lfade -= 1.f / (std::max (rel, 0.001f) * sr); if (lfade <= 0) { lfade = 0; st = Idle; } }
        lsm += (v * lfade - lsm) * 0.02f;
        out = lsm;
        return out;
    }
};


//==============================================================================
struct KeyCurve
{
    static constexpr int kMax = 32;
    int n = 2; float x[kMax] { 0.f, 1.f }, y[kMax] { -1.f, 1.f };
    float eval (float p) const
    {
        if (p <= x[0]) return y[0];
        for (int i = 0; i < n - 1; ++i) if (p <= x[i + 1]) { const float w = x[i + 1] - x[i]; return y[i] + (y[i + 1] - y[i]) * (w > 1e-6f ? (p - x[i]) / w : 1.f); }
        return y[n - 1];
    }
    juce::String toString() const { juce::String s; for (int i = 0; i < n; ++i) s << juce::String (x[i], 4) << "," << juce::String (y[i], 4) << (i < n - 1 ? ";" : ""); return s; }
    static KeyCurve fromString (const juce::String& str)
    {
        KeyCurve k; k.n = 0;
        for (auto& tok : juce::StringArray::fromTokens (str, ";", ""))
        {
            auto p = juce::StringArray::fromTokens (tok, ",", "");
            if (p.size() < 2 || k.n >= kMax) continue;
            k.x[k.n] = clampf (p[0].getFloatValue(), 0.f, 1.f); k.y[k.n] = clampf (p[1].getFloatValue(), -1.f, 1.f); ++k.n;
        }
        if (k.n < 2) return KeyCurve();
        k.x[0] = 0; k.x[k.n - 1] = 1; for (int i = 1; i < k.n; ++i) k.x[i] = std::max (k.x[i], k.x[i - 1]);
        return k;
    }
};

//==============================================================================
// v0.5 random distributions, after SuperCollider's random patterns (Pwhite, Pgauss, Plprand, Phprand, Pmeanrand,
// Pexprand, Pcauchy, Pbeta, Ppoisson, Pprob). Every function maps uniform numbers to a value in [0, 1].
struct DistTable
{
    static constexpr int N = 256;
    float inv[N + 1] {};                       // inverse CDF of the drawn density
    DistTable() { for (int i = 0; i <= N; ++i) inv[i] = (float) i / N; }
    static DistTable fromCurve (const KeyCurve& k)
    {
        DistTable t; float pdf[N], cdf[N + 1]; cdf[0] = 0;
        for (int i = 0; i < N; ++i) { pdf[i] = 0.002f + std::max (0.f, (k.eval ((i + 0.5f) / N) + 1.f) * 0.5f); cdf[i + 1] = cdf[i] + pdf[i]; }
        int j = 0;
        for (int i = 0; i <= N; ++i)
        {
            const float target = cdf[N] * (float) i / N;
            while (j < N - 1 && cdf[j + 1] < target) ++j;
            const float w = cdf[j + 1] - cdf[j];
            t.inv[i] = clampf (((float) j + (w > 0 ? (target - cdf[j]) / w : 0.f)) / N, 0.f, 1.f);
        }
        return t;
    }
    float eval (float u) const { const float f = clampf (u, 0.f, 1.f) * N; const int i = std::min ((int) f, N - 1); return inv[i] + (inv[i + 1] - inv[i]) * (f - (float) i); }
};

inline float distSample (int d, float u1, float u2, const DistTable* drawn)
{
    switch (d)
    {
        case 1: { const float r = std::sqrt (-2.f * std::log (std::max (u1, 1e-7f))); return clampf (0.5f + 0.17f * r * std::cos (k2Pi * u2), 0.f, 1.f); }   // Gauss
        case 2: return std::min (u1, u2);                                                    // Low  (Plprand)
        case 3: return std::max (u1, u2);                                                    // High (Phprand)
        case 4: return 0.5f * (u1 + u2);                                                     // Mean (Pmeanrand)
        case 5: return -std::log (1.f - u1 * 0.99326f) / 5.f;                                 // Exponential, skewed towards 0
        case 6: return clampf (0.5f + 0.08f * std::tan (kPi * (u1 - 0.5f) * 0.98f), 0.f, 1.f); // Cauchy: heavy tails
        case 7: return 0.5f - 0.5f * std::cos (kPi * u1);                                    // Beta(0.5, 0.5): U-shaped
        case 8: {                                                                            // Poisson, lambda = 4, scaled to 0..1
            static const float cdf[13] = { 0.0183f, 0.0916f, 0.2381f, 0.4335f, 0.6288f, 0.7851f, 0.8893f, 0.9489f, 0.9786f, 0.9919f, 0.9972f, 0.9991f, 1.f };
            int k = 0; while (k < 12 && u1 > cdf[k]) ++k; return (float) k / 12.f; }
        case 9: return drawn ? drawn->eval (u1) : u1;                                        // Drawn (Pprob)
        default: return u1;                                                                  // Uniform (Pwhite)
    }
}

//==============================================================================
// Chaotic modulator: Lorenz, Rössler, Chua, Duffing, Sprott B
// v0.5: + Henon, Latoocarfian, Standard and Gingerbread maps, FitzHugh-Nagumo, Lotka-Volterra and Arneodo-Coullet-Tresser
struct AttrMod
{
    float x = 0.1f, y = 0.f, z = 0.f, ph = 0.f, sm = 0.f, nx = 0, ny = 0, nz = 0;
    int lastType = -1; float acc = 0, prevN[3] {}, curN[3] {}, initP2 = -1, initP1 = -1;
    void reset (uint32_t seed) { x = 0.1f + 0.3f * hashF (seed); y = 0.05f; z = 0.1f; ph = 0; sm = 0; lastType = -1; acc = 0; }
    static float tauScale (int t)
    {
        static const float s[12] = { 0.7f, 6.f, 2.1f, 5.2f, 4.f, 8.f, 8.f, 8.f, 8.f, 33.36f, 5.3f, 23.31f };
        return s[juce::jlimit (0, 11, t)];
    }
    void initType (int t, float p1, float p2)
    {
        switch (t)
        {
            case 5: case 6: x = 0.1f; y = 0.1f; break;
            case 7: x = 0.5f; y = 0.2f + p2 * 5.f; break;
            case 8: x = 1.2f + (p1 - 0.5f) * 3.f; y = 2.1f + (p2 - 0.5f) * 3.f; break;
            case 9: x = 0.1f; y = 0.f; break;
            case 10: { const float a = 0.5f + 1.5f * p1, d = 0.5f + 1.5f * p2; x = d * 1.8f; y = a * 0.6f; } break;
            case 11: x = 0.6f; y = 0.5f; z = 0.5f; break;
            default: x = 0.1f; y = 0.05f; z = 0.1f; break;
        }
        lastType = t; initP1 = p1; initP2 = p2; acc = 0;
        for (int i = 0; i < 3; ++i) prevN[i] = curN[i] = 0;
    }
    void stepMap (int type, float tau, float p1, float p2)
    {
        if ((type == 8 && (std::fabs (p1 - initP1) > 0.01f || std::fabs (p2 - initP2) > 0.01f)) || (type == 7 && std::fabs (p2 - initP2) > 0.01f)) initType (type, p1, p2);
        acc += tau;
        int guard = 0;
        while (acc >= 1.f && guard++ < 256)
        {
            acc -= 1.f;
            for (int i = 0; i < 3; ++i) prevN[i] = curN[i];
            float a, b, xp = x;
            switch (type)
            {
                case 5: a = 1.f + 0.4f * p1; b = 0.15f + 0.2f * p2; x = 1.f - a * x * x + y; y = b * xp; if (! finiteF (x) || std::fabs (x) > 3.f) { x = 0.1f; y = 0.1f; }
                        curN[0] = x / 1.5f; curN[1] = y / 0.45f; curN[2] = 0.5f * (x / 1.5f + y / 0.45f); break;
                case 6: a = 1.f + 4.f * (p1 - 0.5f); b = 3.f + 6.f * (p2 - 0.5f); x = std::sin (y * b) + 0.8f * std::sin (xp * b); y = std::sin (xp * a) + 0.8f * std::sin (y * a);
                        curN[0] = x / 1.8f; curN[1] = y / 1.8f; curN[2] = x * y / 3.24f; break;
                case 7: a = 0.3f + 4.f * p1; y = (float) wrap2piM (y + a * std::sin (x)); x = (float) wrap2piM (x + y);
                        curN[0] = (x - kPi) / kPi; curN[1] = (y - kPi) / kPi; curN[2] = std::sin (x + y); break;
                default: x = 1.f - y + std::fabs (xp); y = xp;
                         curN[0] = fastTanh ((x - 1.5f) * 0.3f); curN[1] = fastTanh ((y - 1.5f) * 0.3f); curN[2] = fastTanh ((x - y) * 0.3f); break;
            }
        }
        const float t = clampf (acc, 0.f, 1.f);
        nx = clampf (prevN[0] + (curN[0] - prevN[0]) * t, -1.f, 1.f); ny = clampf (prevN[1] + (curN[1] - prevN[1]) * t, -1.f, 1.f); nz = clampf (prevN[2] + (curN[2] - prevN[2]) * t, -1.f, 1.f);
    }
    static double wrap2piM (double v) { v = std::fmod (v, 6.283185307179586); return v < 0 ? v + 6.283185307179586 : v; }
    void stepOde (int type, float tau, float p1, float p2)
    {
        int sub = 1 + (int) (tau / 0.02f); sub = std::min (sub, 64);
        const float h = tau / (float) sub;
        for (int i = 0; i < sub; ++i)
        {
            float dx, dy, dz = 0;
            if (type == 9) { const float I = 0.3f + 1.2f * p1, e = 0.02f + 0.28f * p2 * p2; dx = x - x * x * x / 3.f - y + I; dy = e * (x + 0.7f - 0.8f * y); }
            else if (type == 10) { const float a = 0.5f + 1.5f * p1, d = 0.5f + 1.5f * p2; dx = x * (a - y); dy = y * (x - d); }
            else { const float al = 1.2f + 0.5f * p1, mu = 0.002f + 0.03f * p2;
                   dx = x * (1.1f - x / 2 - y / 2 - z / 10) + mu; dy = y * (-0.5f + x / 2 + y / 10 - z / 10) + mu; dz = z * (al + 0.2f - al * x - y / 10 - z / 10) + mu; }
            x += dx * h; y += dy * h; z += dz * h;
            if (type >= 10) { x = std::max (x, 1e-5f); y = std::max (y, 1e-5f); z = std::max (z, 1e-5f); }
        }
        if (! finiteF (x) || ! finiteF (y) || ! finiteF (z) || std::fabs (x) > 200.f || std::fabs (y) > 200.f || std::fabs (z) > 200.f) initType (type, p1, p2);
        if (type == 9) { nx = x * 0.5f; ny = (y - 0.6f) / 1.2f; nz = fastTanh (x - y); }
        else if (type == 10) { const float a = 0.5f + 1.5f * p1, d = 0.5f + 1.5f * p2; nx = fastTanh ((x - d) / (d + 0.5f)); ny = fastTanh ((y - a) / (a + 0.5f)); nz = fastTanh (x * y - a * d); }
        else { nx = (x - 1.1f) * 0.9f; ny = (y - 0.8f) * 0.8f; nz = (z - 2.5f) * 0.3f; }
        nx = clampf (nx, -1.f, 1.f); ny = clampf (ny, -1.f, 1.f); nz = clampf (nz, -1.f, 1.f);
    }
    void step (int type, float tau, float p1, float p2)
    {
        if (type >= 5)
        {
            if (type != lastType) initType (type, p1, p2);
            if (type <= 8) stepMap (type, tau, p1, p2); else stepOde (type, tau, p1, p2);
            return;
        }
        if (lastType >= 5) initType (type, p1, p2);
        lastType = type;
        int sub = 1 + (int) (tau / 0.008f); sub = std::min (sub, 64);
        const float h = tau / (float) sub;
        for (int i = 0; i < sub; ++i)
        {
            float dx, dy, dz;
            switch (type)
            {
                case 0: { const float sg = 5.f + p1 * 15.f, rho = 20.f + p2 * 25.f; dx = sg * (y - x); dy = x * (rho - z) - y; dz = x * y - 8.f / 3.f * z; } break;
                case 1: { const float a = 0.1f + p1 * 0.2f, c = 4.f + p2 * 5.f; dx = -y - z; dy = x + a * y; dz = 0.2f + z * (x - c); } break;
                case 2: { const float al = 8.f + p1 * 3.f, be = 12.f + p2 * 6.f, m0 = -8.f / 7.f, m1 = -5.f / 7.f;
                          const float fx = m1 * x + 0.5f * (m0 - m1) * (std::fabs (x + 1.f) - std::fabs (x - 1.f));
                          dx = al * (y - x - fx); dy = x - y + z; dz = -be * y; } break;
                case 3: { const float g = 0.2f + p1 * 0.4f, d = 0.1f + p2 * 0.3f; ph += h * 1.2f;
                          dx = y; dy = -d * y + x - x * x * x + g * std::cos (ph); dz = 0; z = std::sin (ph); } break;
                default: { const float a = 0.6f + p1 * 0.8f, b = 0.5f + p2; dx = a * y * z; dy = x - y; dz = b - x * y; } break;
            }
            x += dx * h; y += dy * h; if (type != 3) z += dz * h;
        }
        if (ph > 1000.f) ph = std::fmod (ph, 6.2831853f);
        if (! finiteF (x) || std::fabs (x) > 200.f || std::fabs (y) > 200.f || std::fabs (z) > 200.f) reset (7u);
        switch (type)
        {
            case 0: nx = x / 20.f; ny = y / 27.f; nz = (z - (20.f + p2 * 25.f) + 3.f) / 18.f; break;
            case 1: nx = x / 10.f; ny = y / 10.f; nz = z / 12.f - 0.3f; break;
            case 2: nx = x / 2.2f; ny = y / 0.4f; nz = z / 3.f; break;
            case 3: nx = x / 1.4f; ny = y / 1.2f; nz = z; break;
            default: nx = x / 3.f; ny = y / 3.f; nz = z / 3.f; break;
        }
        nx = clampf (nx, -1.f, 1.f); ny = clampf (ny, -1.f, 1.f); nz = clampf (nz, -1.f, 1.f);
    }
    float output (int outSel, float smoothK)
    {
        float v = outSel == 0 ? nx : outSel == 1 ? ny : outSel == 2 ? nz : clampf ((nx + ny + nz) * 0.6f, -1.f, 1.f);
        sm = v + smoothK * (sm - v);
        return sm;
    }
};

//==============================================================================
// Stateless LFO value at a continuous phase position (cycles since reset)
struct LfoEval
{
    static inline float warp (float p, float skew)
    {
        skew = clampf (skew, 0.02f, 0.98f);
        return p < skew ? 0.5f * p / skew : 0.5f + 0.5f * (p - skew) / (1.f - skew);
    }
    static inline bool isRandom (int wave) { return wave >= 5; }
    static inline bool isStateful (int wave) { return wave >= 8; }
    static float value (double total, int wave, float skew, uint32_t seed, int dist = 0, const DistTable* drawn = nullptr)
    {
        const double fl = std::floor (total);
        const float ph = (float) (total - fl);
        const uint32_t cyc = (uint32_t) (int64_t) fl;
        const float p = warp (ph, skew);
        switch (wave)
        {
            case 0: return sinT (p);
            case 1: return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
            case 2: return 2.f * p - 1.f;
            case 3: return 1.f - 2.f * p;
            case 4: return p < 0.5f ? 1.f : -1.f;
            case 5: return distSample (dist, hashF (seed * 7919u + cyc), hashF (seed * 104729u + cyc), drawn) * 2.f - 1.f;
            case 6: { const float a = distSample (dist, hashF (seed * 7919u + cyc), hashF (seed * 104729u + cyc), drawn),
                                  b = distSample (dist, hashF (seed * 7919u + cyc + 1u), hashF (seed * 104729u + cyc + 1u), drawn);
                      const float u = 0.5f - 0.5f * sinT (0.25f + 0.5f * p);
                      return (a + (b - a) * u) * 2.f - 1.f; }
            default:
            {   // 1-D gradient noise over two octaves
                auto grad = [seed] (double t)
                {
                    const double fi = std::floor (t); const float f = (float) (t - fi); const uint32_t i = (uint32_t) (int64_t) fi;
                    const float g0 = hashF (seed * 31u + i) * 2.f - 1.f, g1 = hashF (seed * 31u + i + 1u) * 2.f - 1.f;
                    const float u = f * f * (3.f - 2.f * f);
                    return (1.f - u) * g0 * f + u * g1 * (f - 1.f);
                };
                return clampf (2.4f * (grad (total) + 0.5f * grad (total * 2.0 + 17.0)), -1.f, 1.f);
            }
        }
    }
};

struct LfoState
{
    double total = 0, start = 0; uint32_t seed = 1; float sl = 0, fade = 1.f, last = 0;
    // v0.5 stateful random waves: Brownian (Pbrown / LFBrownNoise), Geo Brownian (Pgbrown),
    // Noise Ring (Wiard noise ring, DNoiseRing) and Markov (Pfsm / Dfsm). Replayed from the start after a Loop Lock reset.
    int64_t sCyc = INT64_MIN; float walk = 0, sPrev = 0, sCur = 0; uint32_t reg = 0; int mst = 0; float levels[8] {}; int sWave = -1, sDist = -1;
    void reset (float phase, uint32_t sd) { total = start = phase; seed = sd; sl = 0; sCyc = INT64_MIN; }

    void initState (int wave, int dist, const DistTable* drawn)
    {
        walk = 0; sPrev = sCur = 0; mst = 0;
        reg = hash32 (seed * 2654435761u) & 0xFFFFu; if (reg == 0) reg = 0x5A3Cu;
        for (int i = 0; i < 8; ++i) levels[i] = distSample (dist, hashF (seed * 31u + (uint32_t) i), hashF (seed * 37u + (uint32_t) i), drawn) * 2.f - 1.f;
        if (wave == 9) walk = -0.5f;
        sWave = wave; sDist = dist;
    }
    void stepState (int wave, float skew, int dist, const DistTable* drawn, int64_t c)
    {
        const uint32_t cu = (uint32_t) c;
        const float u1 = hashF (seed * 7919u + cu), u2 = hashF (seed * 104729u + cu), u3 = hashF (seed * 15485863u + cu);
        sPrev = sCur;
        switch (wave)
        {
            case 8: {   // Brownian: reflected random walk, step drawn from the distribution
                const float stepSz = 0.03f + 0.6f * skew * skew;
                walk += (distSample (dist, u1, u2, drawn) * 2.f - 1.f) * stepSz;
                if (walk > 1.f) walk = 2.f - walk; else if (walk < -1.f) walk = -2.f - walk;
                sCur = walk; } break;
            case 9: {   // Geo Brownian: the walk moves in proportion to its value (log domain)
                const float stepSz = 0.05f + 1.2f * skew * skew;
                walk += (distSample (dist, u1, u2, drawn) * 2.f - 1.f) * stepSz;
                const float lo = -3.f; if (walk > 0.f) walk = -walk; else if (walk < lo) walk = 2.f * lo - walk;
                sCur = (std::exp (walk) - 0.0498f) / 0.9502f * 2.f - 1.f; } break;
            case 10: {  // Noise ring: 16-bit shift register; the bit leaving the ring flips with probability Skew^2
                const uint32_t outBit = (reg >> 15) & 1u;
                const uint32_t nb = u1 < skew * skew ? (outBit ^ 1u) : outBit;
                reg = ((reg << 1) | nb) & 0xFFFFu;
                sCur = (float) (reg & 0xFFu) / 127.5f - 1.f; } break;
            default: {  // Markov chain over 8 levels: low Skew stays near, high Skew jumps anywhere
                const float k = 0.3f + 4.f * (1.f - skew);
                float w[8], tot = 0; for (int i = 0; i < 8; ++i) { w[i] = std::exp (-k * (float) std::abs (i - mst)); tot += w[i]; }
                float u = u3 * tot; int nx = 7; for (int i = 0; i < 8; ++i) { if (u < w[i]) { nx = i; break; } u -= w[i]; }
                mst = nx; sCur = levels[mst]; } break;
        }
    }
    float statefulValue (int wave, float skew, int dist, const DistTable* drawn)
    {
        const double fl = std::floor (total); const int64_t cyc = (int64_t) fl; const float ph = (float) (total - fl);
        if (wave != sWave || dist != sDist || sCyc == INT64_MIN || cyc < sCyc || cyc - sCyc > 4096)
        {
            const int64_t base = (int64_t) std::floor (start);
            initState (wave, dist, drawn);
            sCyc = (cyc >= base && cyc - base <= 4096) ? base - 1 : cyc - 1;
        }
        while (sCyc < cyc) { ++sCyc; stepState (wave, skew, dist, drawn, sCyc); }
        if (wave == 8 || wave == 9) { const float u = 0.5f - 0.5f * sinT (0.25f + 0.5f * ph); return sPrev + (sCur - sPrev) * u; }
        return sCur;
    }
};

//==============================================================================
// v0.5 stochastic trigger generator (SuperCollider Dust, GaussTrig, CoinGate on Impulse, LorenzTrig, Impulse)
struct TrigGen
{
    double toNext = 0; float gate = 0, env = 0, sh = 0, out = 0; bool odd = false; float lx = 1.f, ly = 1.f, lz = 20.f; Rng rng;
    void reset (uint32_t s) { rng.seed (s); toNext = 0; gate = env = sh = out = 0; odd = false; lx = 1.f; ly = 1.f; lz = 20.f; }
    // returns true if it fired during these n samples
    bool tick (int mode, float rateHz, float dev, float prob, int shape, float lenMs, int dist, const DistTable* drawn, float level, float sr, int n)
    {
        rateHz = clampf (rateHz, 0.01f, 400.f);
        const float dt = (float) n / sr;
        bool fire = false;
        if (mode == 0) fire = rng.next() < 1.f - std::exp (-rateHz * dt);           // Dust: Poisson process
        else
        {
            if (mode == 3)
            {   // LorenzTrig: the instantaneous rate follows a Lorenz attractor between Rate*(1-Dev) and Rate*(1+3*Dev)
                const float hh = std::min (0.01f, dt * 8.f);
                for (int i = 0; i < 2; ++i) { const float dx = 10.f * (ly - lx), dy = lx * (28.f - lz) - ly, dz = lx * ly - 8.f / 3.f * lz; lx += dx * hh; ly += dy * hh; lz += dz * hh; }
                if (! finiteF (lx) || std::fabs (lx) > 100.f) { lx = 1.f; ly = 1.f; lz = 20.f; }
                const float u = clampf (0.5f + lx / 40.f, 0.f, 1.f);
                rateHz *= (1.f - dev) + 4.f * dev * u;
            }
            toNext -= dt;
            if (toNext <= 0.0)
            {
                fire = true;
                double iv = 1.0 / rateHz;
                if (mode == 1) { const float r = std::sqrt (-2.f * std::log (std::max (rng.next(), 1e-7f))) * std::cos (k2Pi * rng.next()); iv *= std::max (0.05, 1.0 + 0.5 * dev * r); }
                if (mode == 4) { odd = ! odd; iv *= odd ? 1.0 + dev * 0.5 : 1.0 - dev * 0.5; }   // Clock with swing
                toNext += iv; if (toNext < 0.0) toNext = iv;
            }
        }
        if (fire && rng.next() >= prob) fire = false;                                   // CoinGate
        const float lenS = std::max (lenMs * 0.001f, 0.001f);
        if (fire) { gate = lenS; env = 1.f; sh = distSample (dist, rng.next(), rng.next(), drawn); }
        gate = std::max (0.f, gate - dt);
        env *= std::exp (-dt / lenS);
        switch (shape)
        {
            case 0: out = gate > 0.f ? 1.f : 0.f; break;
            case 1: out = env; break;
            case 2: out = sh; break;
            default: out = sh * env; break;
        }
        out *= level;
        return fire;
    }
};
} // namespace cz
