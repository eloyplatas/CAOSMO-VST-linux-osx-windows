// Caosmo — VENV vector envelopes, attractor modulators, keytrack curve, LFO evaluation
#pragma once
#include "DSP.h"
#include <juce_core/juce_core.h>

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
// Chaotic modulator: Lorenz, Rössler, Chua, Duffing, Sprott B
struct AttrMod
{
    float x = 0.1f, y = 0.f, z = 0.f, ph = 0.f, sm = 0.f, nx = 0, ny = 0, nz = 0;
    void reset (uint32_t seed) { x = 0.1f + 0.3f * hashF (seed); y = 0.05f; z = 0.1f; ph = 0; sm = 0; }
    static float tauScale (int t) { static const float s[5] = { 0.7f, 6.f, 2.1f, 5.2f, 4.f }; return s[juce::jlimit (0, 4, t)]; }
    void step (int type, float tau, float p1, float p2)
    {
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
    static float value (double total, int wave, float skew, uint32_t seed)
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
            case 5: return hashF (seed * 7919u + cyc) * 2.f - 1.f;
            case 6: { const float a = hashF (seed * 7919u + cyc), b = hashF (seed * 7919u + cyc + 1u); const float u = 0.5f - 0.5f * sinT (0.25f + 0.5f * p);
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
    void reset (float phase, uint32_t sd) { total = start = phase; seed = sd; sl = 0; }
};
} // namespace cz
