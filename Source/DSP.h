// Caosmo — DSP building blocks
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <array>

namespace cz
{
constexpr float kPi  = 3.14159265358979f;
constexpr float k2Pi = 6.28318530717959f;

inline float clampf (float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float fastTanh (float x) { x = clampf (x, -3.f, 3.f); const float x2 = x * x; return x * (27.f + x2) / (27.f + 9.f * x2); }
inline float dbToGain (float db) { return db <= -59.9f ? 0.f : std::pow (10.f, db * 0.05f); }
inline float msCoef (float ms, float sr) { return ms <= 0.0001f ? 0.f : std::exp (-1.f / (ms * 0.001f * sr)); }
inline bool finiteF (float x) { return std::isfinite (x); }

inline uint32_t hash32 (uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }
inline float hashF (uint32_t x) { return (float) (hash32 (x) >> 8) * (1.f / 16777216.f); }

struct Rng
{
    uint32_t s = 0x12345678u;
    void seed (uint32_t v) { s = hash32 (v) | 1u; }
    uint32_t nextU() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float next() { return (float) (nextU() >> 8) * (1.f / 16777216.f); }
    float bi() { return next() * 2.f - 1.f; }
};

// ---------- sine table (cycles in, value out) ----------
struct SineTable
{
    static constexpr int N = 4096;
    float t[N + 1];
    SineTable() { for (int i = 0; i <= N; ++i) t[i] = std::sin (k2Pi * (float) i / (float) N); }
    static const SineTable& get() { static SineTable s; return s; }
};
inline float sinT (float cycles)
{
    const auto& T = SineTable::get();
    float p = cycles - std::floor (cycles);
    float f = p * SineTable::N;
    int i = (int) f; if (i >= SineTable::N) i = SineTable::N - 1;
    float fr = f - (float) i;
    return T.t[i] + (T.t[i + 1] - T.t[i]) * fr;
}

struct DCBlock
{
    float x1 = 0, y1 = 0, R = 0.9987f;
    void prepare (float sr) { R = 1.f - (k2Pi * 8.f / sr); }
    void reset() { x1 = y1 = 0; }
    inline float tick (float x) { float y = x - x1 + R * y1; x1 = x; y1 = y; return y; }
};

// ---------- Simper / Cytomic SVF ----------
struct SVFCoefs { float a1 = 1, a2 = 0, a3 = 0, m0 = 0, m1 = 0, m2 = 1; };
// modes: 0 LP, 1 HP, 2 BP(unity peak), 3 Notch, 4 Bell, 5 LowShelf, 6 HighShelf, 7 AllPass
inline SVFCoefs svfMake (int mode, float fc, float sr, float Q, float gainDb = 0.f)
{
    fc = clampf (fc, 8.f, sr * 0.48f);
    Q = std::max (Q, 0.05f);
    float g = std::tan (kPi * fc / sr);
    float k = 1.f / Q;
    const float A = std::pow (10.f, gainDb / 40.f);
    SVFCoefs c; c.m0 = c.m1 = c.m2 = 0;
    switch (mode)
    {
        case 0: c.m2 = 1; break;
        case 1: c.m0 = 1; c.m1 = -k; c.m2 = -1; break;
        case 2: c.m1 = k; break;
        case 3: c.m0 = 1; c.m1 = -k; break;
        case 4: k = 1.f / (Q * A); c.m0 = 1; c.m1 = k * (A * A - 1.f); break;
        case 5: g = g / std::sqrt (A); c.m0 = 1; c.m1 = k * (A - 1.f); c.m2 = A * A - 1.f; break;
        case 6: g = g * std::sqrt (A); c.m0 = A * A; c.m1 = k * (1.f - A) * A; c.m2 = 1.f - A * A; break;
        default: c.m0 = 1; c.m1 = -2.f * k; break;
    }
    c.a1 = 1.f / (1.f + g * (g + k)); c.a2 = g * c.a1; c.a3 = g * c.a2;
    return c;
}
struct SVF
{
    float ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    inline float tick (float v0, const SVFCoefs& c)
    {
        const float v3 = v0 - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        return c.m0 * v0 + c.m1 * v1 + c.m2 * v2;
    }
};

// ---------- RBJ biquad (lane / EQ) ----------
struct BQ
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] = { 0, 0 }, z2[2] = { 0, 0 };
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    inline float tick (float x, int ch)
    {
        float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
    void set (int type, float f, float sr, float Q, float gDb)
    {
        f = clampf (f, 10.f, sr * 0.48f);
        const float w = k2Pi * f / sr, cw = std::cos (w), sw = std::sin (w);
        const float A = std::pow (10.f, gDb / 40.f), al = sw / (2.f * Q);
        float B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;
        switch (type)
        {
            case 0: B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; break; // LP
            case 1: B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; break; // HP
            case 2: B0 = 1 + al * A; B1 = -2 * cw; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cw; A2 = 1 - al / A; break; // bell
            case 3: { float sA = 2 * std::sqrt (A) * al; // low shelf
                B0 = A * ((A + 1) - (A - 1) * cw + sA); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - sA);
                A0 = (A + 1) + (A - 1) * cw + sA; A1 = -2 * ((A - 1) + (A + 1) * cw); A2 = (A + 1) + (A - 1) * cw - sA; } break;
            default: { float sA = 2 * std::sqrt (A) * al; // high shelf
                B0 = A * ((A + 1) + (A - 1) * cw + sA); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - sA);
                A0 = (A + 1) - (A - 1) * cw + sA; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - sA; } break;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

// ---------- fractional delay line ----------
struct Delay
{
    std::vector<float> b; int w = 0;
    void init (int n) { b.assign ((size_t) std::max (n, 8), 0.f); w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.f); }
    inline float read (float d) const
    {
        const int N = (int) b.size();
        float rp = (float) w - d;
        while (rp < 0) rp += (float) N;
        int i0 = (int) rp; float f = rp - (float) i0;
        if (i0 >= N) i0 -= N;
        int i1 = i0 + 1; if (i1 >= N) i1 = 0;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * f;
    }
    inline void write (float x) { b[(size_t) w] = x; if (++w >= (int) b.size()) w = 0; }
};

// ---------- 12-type voice filter (stereo) ----------
struct VoiceFilter
{
    int type = 0, stages = 1;
    float sr = 48000.f;
    SVF svf[2][4]; SVFCoefs cMain, cLast;
    float lad[2][4] = {}; float lG = 0, lK = 0, ladY[2] = {};
    Delay comb[2]; float cD = 100, cFb = 0;
    SVF fs[2][3]; SVFCoefs fc[3]; float fGain[3] = { 1.f, 0.7f, 0.45f };
    float ap[2][4] = {}; float apA[4] = {}; float phFb = 0, phLast[2] = {};

    void prepare (float s) { sr = s; comb[0].init ((int) (s / 18.f) + 8); comb[1].init ((int) (s / 18.f) + 8); reset(); }
    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            for (auto& f : svf[c]) f.reset();
            for (auto& f : fs[c]) f.reset();
            for (int i = 0; i < 4; ++i) { lad[c][i] = 0; ap[c][i] = 0; }
            ladY[c] = phLast[c] = 0; comb[c].clear();
        }
    }
    void setup (int t, int slope, float cut, float res)
    {
        type = t; cut = clampf (cut, 16.f, sr * 0.47f); res = clampf (res, 0.f, 1.f);
        const float Qr = 0.707f + res * res * 22.f;
        stages = slope + 1;
        switch (t)
        {
            case 0: case 1:
                cMain = svfMake (t, cut, sr, 0.707f); cLast = svfMake (t, cut, sr, Qr); break;
            case 2:
                cMain = svfMake (2, cut, sr, 0.707f); cLast = svfMake (2, cut, sr, 0.5f + res * res * 18.f); break;
            case 3: case 7:
                cMain = cLast = svfMake (t, cut, sr, 0.5f + res * res * 18.f); break;
            case 4: stages = 1; cLast = svfMake (4, cut, sr, 1.4f, res * 18.f); break;
            case 5: stages = 1; cLast = svfMake (5, cut, sr, 0.707f, res * 18.f); break;
            case 6: stages = 1; cLast = svfMake (6, cut, sr, 0.707f, res * 18.f); break;
            case 8: { float g = std::tan (kPi * cut / sr); lG = g / (1.f + g); lK = res * 3.95f; } break;
            case 9: { cD = clampf (sr / cut, 2.f, (float) comb[0].b.size() - 3.f); cFb = res * 0.97f; } break;
            case 10:
            {
                static const float F[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 }, { 570, 840, 2410 }, { 300, 870, 2240 } };
                float pos = clampf (std::log (cut / 20.f) / std::log (1000.f), 0.f, 1.f) * 3.999f;
                int i = (int) pos; float fr = pos - (float) i;
                for (int k = 0; k < 3; ++k)
                {
                    float f = F[i][k] + (F[i + 1][k] - F[i][k]) * fr;
                    fc[k] = svfMake (2, f, sr, 2.5f + res * 10.f);
                }
            } break;
            default:
            {
                const float m[4] = { 1.f, 1.7f, 2.9f, 4.6f };
                for (int k = 0; k < 4; ++k) { float tg = std::tan (kPi * clampf (cut * m[k] * 0.5f, 20.f, sr * 0.45f) / sr); apA[k] = (tg - 1.f) / (tg + 1.f); }
                phFb = res * 0.85f;
            } break;
        }
    }
    inline float tickCh (float x, int ch)
    {
        switch (type)
        {
            case 0: case 1: case 2: case 3: case 7:
            {
                for (int s = 0; s < stages - 1; ++s) x = svf[ch][s].tick (x, (type <= 2) ? cMain : cLast);
                return svf[ch][stages - 1].tick (x, cLast);
            }
            case 4: case 5: case 6: return svf[ch][0].tick (x, cLast);
            case 8:
            {
                float in = fastTanh (x - lK * ladY[ch]);
                for (int s = 0; s < 4; ++s) { float v = (in - lad[ch][s]) * lG; float y = v + lad[ch][s]; lad[ch][s] = y + v; in = y; }
                ladY[ch] = in;
                return in * (1.f + lK * 0.45f);
            }
            case 9:
            {
                float y = x + cFb * comb[ch].read (cD);
                y = clampf (y, -8.f, 8.f);
                comb[ch].write (y);
                return y * (1.f - cFb * 0.5f);
            }
            case 10:
            {
                float o = 0;
                for (int k = 0; k < 3; ++k) o += fs[ch][k].tick (x, fc[k]) * fGain[k];
                return o * 3.f;
            }
            default:
            {
                float y = x + phFb * phLast[ch];
                for (int k = 0; k < 4; ++k) { float o = apA[k] * y + ap[ch][k]; ap[ch][k] = y - apA[k] * o; y = o; }
                phLast[ch] = y;
                return 0.5f * (x + y);
            }
        }
    }
};

} // namespace cz
