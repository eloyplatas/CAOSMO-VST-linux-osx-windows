// Caosmo — FX bus effects and master chain
#pragma once
#include "DSP.h"
#include "Params.h"
#include <juce_dsp/juce_dsp.h>

namespace cz
{
//==============================================================================
struct FxReverb
{
    static constexpr int N = 8;
    Delay line[N], pre[2]; Delay ap[2][4]; float lp[N] {}; float sr = 48000.f, mph[N] {};
    void prepare (float s)
    {
        sr = s;
        for (auto& l : line) l.init ((int) (s * 0.32f));
        pre[0].init ((int) (s * 0.21f)); pre[1].init ((int) (s * 0.21f));
        for (auto& c : ap) for (auto& a : c) a.init ((int) (s * 0.02f));
        reset();
    }
    void reset() { for (auto& l : line) l.clear(); for (auto& p : pre) p.clear(); for (auto& c : ap) for (auto& a : c) a.clear(); for (auto& v : lp) v = 0; for (int i = 0; i < N; ++i) mph[i] = 0.13f * (float) i; }
    void process (float* L, float* R, int n, const float* p)
    {
        static const float base[N] = { 29.7f, 37.1f, 41.1f, 43.7f, 47.9f, 53.3f, 59.9f, 67.7f };
        static const float apMs[4] = { 4.7f, 3.6f, 2.8f, 1.9f };
        const float size = p[0], damp = p[1], preMs = p[2] * 200.f, mod = p[3], level = p[4];
        const float scale = 0.45f + size * 1.9f;
        const float t60 = 0.3f + size * size * 12.f;
        float len[N], g[N];
        for (int i = 0; i < N; ++i) { len[i] = base[i] * 0.001f * sr * scale; g[i] = std::pow (10.f, -3.f * len[i] / (t60 * sr)); }
        const float dc = std::exp (-k2Pi * (18000.f * std::pow (0.08f, damp)) / sr);
        const float preS = std::max (1.f, preMs * 0.001f * sr);
        const float mDepth = mod * 0.0006f * sr;
        for (int k = 0; k < n; ++k)
        {
            float in[2] = { L[k], R[k] };
            for (int c = 0; c < 2; ++c)
            {
                pre[c].write (in[c]);
                float x = pre[c].read (preS);
                for (int a = 0; a < 4; ++a)
                {
                    const float d = apMs[a] * 0.001f * sr * (c ? 1.07f : 1.f);
                    const float z = ap[c][a].read (d);
                    const float y = -0.6f * x + z;
                    ap[c][a].write (x + 0.6f * y);
                    x = y;
                }
                in[c] = x;
            }
            float o[N];
            for (int i = 0; i < N; ++i)
            {
                mph[i] += (0.11f + 0.07f * (float) i) / sr; if (mph[i] >= 1.f) mph[i] -= 1.f;
                float v = line[i].read (len[i] + mDepth * (1.f + sinT (mph[i])));
                lp[i] = v + dc * (lp[i] - v);
                o[i] = lp[i] * g[i];
            }
            // fast Walsh-Hadamard mix
            for (int h = 1; h < N; h <<= 1)
                for (int i = 0; i < N; i += h << 1)
                    for (int j = i; j < i + h; ++j) { const float a = o[j], b = o[j + h]; o[j] = a + b; o[j + h] = a - b; }
            const float nrm = 0.35355339f;
            for (int i = 0; i < N; ++i) line[i].write (o[i] * nrm + ((i & 1) ? in[1] : in[0]) * 0.5f);
            float oL = 0, oR = 0;
            for (int i = 0; i < N; i += 2) { oL += lp[i]; oR += lp[i + 1]; }
            L[k] = oL * 0.55f * level; R[k] = oR * 0.55f * level;
        }
    }
};

struct FxDelay
{
    Delay d[2]; float sr = 48000.f, lp[2] {}, tSm = 0.3f, ph = 0, drift = 0, dTarget = 0; Rng rng;
    void prepare (float s) { sr = s; d[0].init ((int) (s * 2.2f)); d[1].init ((int) (s * 2.2f)); reset(); }
    void reset() { d[0].clear(); d[1].clear(); lp[0] = lp[1] = 0; tSm = -1.f; }
    void process (float* L, float* R, int n, const float* p, bool ping)
    {
        const float tMs = std::pow (2000.f, p[0]), fb = p[1] * 0.98f, damp = p[2], wob = p[3], level = p[4];
        const float target = clampf (tMs * 0.001f * sr, 2.f, (float) d[0].b.size() - 600.f);
        if (tSm < 0) tSm = target;
        const float dc = std::exp (-k2Pi * (16000.f * std::pow (0.05f, damp)) / sr);
        const float ts = 1.f - std::exp (-1.f / (0.08f * sr));
        for (int k = 0; k < n; ++k)
        {
            tSm += (target - tSm) * ts;
            ph += 0.6f / sr; if (ph >= 1.f) ph -= 1.f;
            if ((k & 63) == 0) dTarget = rng.bi();
            drift += (dTarget - drift) * 0.0005f;
            const float m = wob * 0.004f * sr * (0.6f * sinT (ph) + 0.4f * drift);
            float a = d[0].read (tSm + m + 1.f), b = d[1].read (tSm * (ping ? 1.f : 1.013f) + m + 1.f);
            lp[0] = a + dc * (lp[0] - a); lp[1] = b + dc * (lp[1] - b);
            if (ping) { d[0].write (clampf (0.5f * (L[k] + R[k]) + fb * lp[1], -4.f, 4.f)); d[1].write (clampf (fb * lp[0], -4.f, 4.f)); }
            else { d[0].write (clampf (L[k] + fb * lp[0], -4.f, 4.f)); d[1].write (clampf (R[k] + fb * lp[1], -4.f, 4.f)); }
            L[k] = a * level; R[k] = b * level;
        }
    }
};

struct FxFreqShift
{
    // Olli Niemitalo's 90° IIR allpass pair
    struct AP { float a = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0; inline float tick (float x) { const float y = a * (x + y2) - x2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; } };
    AP pa[2][4], pb[2][4]; float dly[2] {}, ph = 0, fbS[2] {}, sr = 48000.f;
    void prepare (float s)
    {
        sr = s;
        const float A[4] = { 0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f };
        const float Bc[4] = { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 4; ++i) { pa[c][i].a = A[i] * A[i]; pb[c][i].a = Bc[i] * Bc[i]; }
        reset();
    }
    void reset() { for (int c = 0; c < 2; ++c) { for (auto& a : pa[c]) a.x1 = a.x2 = a.y1 = a.y2 = 0; for (auto& a : pb[c]) a.x1 = a.x2 = a.y1 = a.y2 = 0; dly[c] = fbS[c] = 0; } }
    void process (float* L, float* R, int n, const float* p)
    {
        float s = 2.f * p[0] - 1.f; const float shift = s * s * s * 2000.f, fb = p[1] * 0.9f, level = p[2];
        const float inc = shift / sr;
        for (int k = 0; k < n; ++k)
        {
            ph += inc; ph -= std::floor (ph);
            const float co = sinT (ph + 0.25f), si = sinT (ph);
            float* io[2] = { L + k, R + k };
            for (int c = 0; c < 2; ++c)
            {
                float x = *io[c] + fb * fbS[c];
                float i1 = x, q1 = x;
                for (auto& a : pa[c]) i1 = a.tick (i1);
                for (auto& a : pb[c]) q1 = a.tick (q1);
                const float I = dly[c]; dly[c] = i1;
                const float o = I * co - q1 * si;
                fbS[c] = clampf (o, -2.f, 2.f);
                *io[c] = o * level;
            }
        }
    }
};

struct FxGranular
{
    static constexpr int G = 32;
    struct Gr { bool on = false; float pos = 0, inc = 1, age = 0, len = 1, gl = 1, gr = 1; };
    std::vector<float> buf[2]; int w = 0, size = 1; Gr g[G]; float sr = 48000.f, acc = 0, next = 0; Rng rng;
    void prepare (float s) { sr = s; size = (int) (s * 4.f); buf[0].assign ((size_t) size, 0.f); buf[1].assign ((size_t) size, 0.f); reset(); }
    void reset() { std::fill (buf[0].begin(), buf[0].end(), 0.f); std::fill (buf[1].begin(), buf[1].end(), 0.f); for (auto& q : g) q.on = false; w = 0; acc = 0; next = 0; }
    inline float rd (int c, float pos) const
    {
        while (pos < 0) pos += (float) size; while (pos >= (float) size) pos -= (float) size;
        const int i0 = (int) pos; const int i1 = (i0 + 1) % size; const float f = pos - (float) i0;
        return buf[c][(size_t) i0] + (buf[c][(size_t) i1] - buf[c][(size_t) i0]) * f;
    }
    void process (float* L, float* R, int n, const float* p, bool freeze)
    {
        const float lenS = 10.f * std::pow (50.f, p[0]) * 0.001f * sr, dens = std::pow (40.f, p[1]), scatter = p[2];
        const float pitch = std::exp2 (std::round (p[3] * 48.f - 24.f) / 12.f), level = p[4];
        const float interval = sr / dens;
        const float norm = 1.f / std::sqrt (std::max (1.f, dens * lenS / sr));
        for (int k = 0; k < n; ++k)
        {
            if (! freeze) { buf[0][(size_t) w] = L[k]; buf[1][(size_t) w] = R[k]; if (++w >= size) w = 0; }
            if (++acc >= next)
            {
                acc = 0; next = interval * (0.5f + rng.next());
                for (auto& q : g) if (! q.on)
                {
                    q.on = true; q.age = 0; q.len = lenS; q.inc = pitch;
                    const float back = lenS * std::max (1.f, pitch) + 64.f + scatter * rng.next() * sr * 1.5f;
                    q.pos = (float) w - std::min (back, (float) size - lenS * 4.f - 8.f);
                    const float pan = rng.bi() * 0.8f; q.gl = std::sqrt (0.5f * (1.f - pan)) * 1.414f; q.gr = std::sqrt (0.5f * (1.f + pan)) * 1.414f;
                    break;
                }
            }
            float oL = 0, oR = 0;
            for (auto& q : g)
            {
                if (! q.on) continue;
                const float t = q.age / q.len; const float win = 0.5f - 0.5f * sinT (t + 0.25f) * 1.f;
                const float wv = sinT (0.5f * t); const float wn = wv * wv; (void) win;
                oL += rd (0, q.pos) * wn * q.gl; oR += rd (1, q.pos) * wn * q.gr;
                q.pos += q.inc; q.age += 1.f; if (q.age >= q.len) q.on = false;
            }
            L[k] = oL * norm * level; R[k] = oR * norm * level;
        }
    }
};

struct FxHowl
{
    Delay d[2]; SVF bp[2]; float lp[2] {}, sr = 48000.f;
    void prepare (float s) { sr = s; d[0].init ((int) (s / 30.f) + 8); d[1].init ((int) (s / 30.f) + 8); reset(); }
    void reset() { d[0].clear(); d[1].clear(); bp[0].reset(); bp[1].reset(); lp[0] = lp[1] = 0; }
    void process (float* L, float* R, int n, const float* p)
    {
        const float f = 40.f * std::pow (100.f, p[0]), fb = p[1] * 1.2f, damp = p[2], drive = p[3], level = p[4];
        const SVFCoefs c = svfMake (2, f, sr, 4.f);
        const float D = clampf (sr / f, 2.f, (float) d[0].b.size() - 3.f);
        const float dc = std::exp (-k2Pi * (14000.f * std::pow (0.06f, damp)) / sr);
        const float g = 1.f + drive * 6.f;
        for (int k = 0; k < n; ++k)
        {
            float* io[2] = { L + k, R + k };
            for (int ch = 0; ch < 2; ++ch)
            {
                const float del = d[ch].read (D * (ch ? 1.003f : 1.f));
                float s = 0.5f * *io[ch] + fb * bp[ch].tick (del, c);
                s = fastTanh (s * g) / std::sqrt (g);
                lp[ch] = s + dc * (lp[ch] - s);
                d[ch].write (lp[ch]);
                *io[ch] = lp[ch] * level * 1.5f;
            }
        }
    }
};

struct FxSlot
{
    int type = 0; FxReverb rv; FxDelay dl; FxFreqShift fs; FxGranular gr; FxHowl hw; float rph = 0, sr = 48000.f;
    std::atomic<float> inPk { 0 }, outPk { 0 };
    void prepare (float s) { sr = s; rv.prepare (s); dl.prepare (s); fs.prepare (s); gr.prepare (s); hw.prepare (s); }
    void setType (int t)
    {
        if (t == type) return;
        type = t;
        switch (t) { case 1: rv.reset(); break; case 2: dl.reset(); break; case 3: fs.reset(); break; case 4: gr.reset(); break; case 5: hw.reset(); break; default: break; }
    }
    void process (float* L, float* R, int n, const float* p, bool opt)
    {
        float ip = 0; for (int k = 0; k < n; ++k) ip = std::max (ip, std::max (std::fabs (L[k]), std::fabs (R[k])));
        switch (type)
        {
            case 1: rv.process (L, R, n, p); break;
            case 2: dl.process (L, R, n, p, opt); break;
            case 3: fs.process (L, R, n, p); break;
            case 4: gr.process (L, R, n, p, opt); break;
            case 5: hw.process (L, R, n, p); break;
            case 6:
            {
                const float inc = std::pow (4000.f, p[0]) / sr, level = p[1];
                for (int k = 0; k < n; ++k) { rph += inc; rph -= std::floor (rph); const float m = sinT (rph) * level; L[k] *= m; R[k] *= m; }
            } break;
            default: break;
        }
        float op = 0; for (int k = 0; k < n; ++k) op = std::max (op, std::max (std::fabs (L[k]), std::fabs (R[k])));
        inPk.store (std::max (ip, inPk.load() * 0.97f)); outPk.store (std::max (op, outPk.load() * 0.97f));
    }
};

//==============================================================================
struct EqBank
{
    BQ hp[4], ls, bell, hs, lp[4]; int hpN = 0, lpN = 0; bool lsOn = false, bellOn = false, hsOn = false; float cache[11];
    EqBank() { for (auto& c : cache) c = -12345.f; }
    void reset() { for (auto& b : hp) b.reset(); for (auto& b : lp) b.reset(); ls.reset(); bell.reset(); hs.reset(); }
    // v: hpf, hpfs, lsf, lsg, mf, mg, mq, hsf, hsg, lpf, lpfs
    void set (const float* v, float s)
    {
        bool changed = false; for (int i = 0; i < 11; ++i) if (std::fabs (v[i] - cache[i]) > 1e-6f) { changed = true; cache[i] = v[i]; }
        if (! changed) return;
        hpN = v[0] > 20.5f ? (int) v[1] + 1 : 0; for (int i = 0; i < hpN; ++i) hp[i].set (1, v[0], s, 0.7071f, 0);
        lsOn = std::fabs (v[3]) > 0.05f; if (lsOn) ls.set (3, v[2], s, 0.7071f, v[3]);
        bellOn = std::fabs (v[5]) > 0.05f; if (bellOn) bell.set (2, v[4], s, v[6], v[5]);
        hsOn = std::fabs (v[8]) > 0.05f; if (hsOn) hs.set (4, v[7], s, 0.7071f, v[8]);
        lpN = v[9] < 19900.f ? (int) v[10] + 1 : 0; for (int i = 0; i < lpN; ++i) lp[i].set (0, v[9], s, 0.7071f, 0);
    }
    inline float tick (float x, int c)
    {
        for (int i = 0; i < hpN; ++i) x = hp[i].tick (x, c);
        if (lsOn) x = ls.tick (x, c);
        if (bellOn) x = bell.tick (x, c);
        if (hsOn) x = hs.tick (x, c);
        for (int i = 0; i < lpN; ++i) x = lp[i].tick (x, c);
        return x;
    }
};

struct MasterParams
{
    bool bypass = false, eqOn = true, satOn = true, compOn = true, widthOn = true, limOn = false, extSc = false, autoRel = true;
    float eq[11] {}; float satDrive = 0, satMix = 1; int satFlav = 0, satOs = 1;
    float thr = 0, ratio = 1, att = 10, rel = 150, knee = 6, makeup = 0, glue = 0, scHpf = 20;
    float ms = 1, mono = 0, ceil = -1, lrel = 100, outGain = 1.f;
};

struct MasterChain
{
    float sr = 48000.f; int maxBlock = 512;
    EqBank eq;
    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4;
    float gr = 0, fbL = 0, fbR = 0; SVF scHp[2]; SVFCoefs scC; bool scOn = false; float scCache = -1;
    SVF monoHp; SVFCoefs monoC; float monoCache = -1;
    std::vector<float> laBuf[2]; int laW = 0, laLen = 1; std::vector<float> gBuf; float limG = 1.f, grAvg = 0;
    float prevTp[2] {}, prev2[2] {};
    std::atomic<float> compGR { 0 }, limGR { 0 }, tpPeak { 0 }, corr { 0 };
    float cLR = 0, cLL = 1e-9f, cRR = 1e-9f;

    void prepare (float s, int block)
    {
        sr = s; maxBlock = std::max (block, 16);
        os2 = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        os4 = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        os2->initProcessing ((size_t) maxBlock); os4->initProcessing ((size_t) maxBlock);
        laLen = std::max (8, (int) (s * 0.0015f));
        laBuf[0].assign ((size_t) laLen + 1, 0.f); laBuf[1].assign ((size_t) laLen + 1, 0.f); gBuf.assign ((size_t) laLen + 1, 1.f);
        reset();
    }
    void reset()
    {
        eq.reset(); gr = 0; fbL = fbR = 0; scHp[0].reset(); scHp[1].reset(); monoHp.reset(); laW = 0; limG = 1.f; grAvg = 0;
        for (auto& b : laBuf) std::fill (b.begin(), b.end(), 0.f); std::fill (gBuf.begin(), gBuf.end(), 1.f);
        if (os2) os2->reset(); if (os4) os4->reset();
    }
    int latency() const { return laLen; }

    static inline float tpEstimate (float x0, float x1, float x2, float x3)
    {   // peak of Catmull-Rom interpolation between x1 and x2 at 1/4, 1/2, 3/4
        float m = std::max (std::fabs (x1), std::fabs (x2));
        for (float t : { 0.25f, 0.5f, 0.75f })
        {
            const float t2 = t * t, t3 = t2 * t;
            const float v = 0.5f * ((2.f * x1) + (-x0 + x2) * t + (2.f * x0 - 5.f * x1 + 4.f * x2 - x3) * t2 + (-x0 + 3.f * x1 - 3.f * x2 + x3) * t3);
            m = std::max (m, std::fabs (v));
        }
        return m;
    }

    void process (float* L, float* R, const float* scL, const float* scR, int n, const MasterParams& p)
    {
        const bool colour = ! p.bypass;
        // EQ
        if (colour && p.eqOn) { eq.set (p.eq, sr); for (int k = 0; k < n; ++k) { L[k] = eq.tick (L[k], 0); R[k] = eq.tick (R[k], 1); } }
        // Saturator
        if (colour && p.satOn && p.satDrive > 0.0005f)
        {
            const float g = 1.f + p.satDrive * 20.f, mix = p.satMix, comp = 1.f / (1.f + p.satDrive * 2.5f);
            const int f = p.satFlav; const float bias = 0.35f * p.satDrive;
            auto shape = [&] (float x)
            {
                const float u = x * g;
                return f == 0 ? (fastTanh (u) - 0.08f * fastTanh (u) * fastTanh (u) * fastTanh (u)) * comp
                              : (fastTanh (u + bias) - fastTanh (bias)) * comp;
            };
            float* ch[2] = { L, R };
            juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
            if (p.satOs == 0)
            {
                for (int k = 0; k < n; ++k) { L[k] = L[k] * (1.f - mix) + shape (L[k]) * mix; R[k] = R[k] * (1.f - mix) + shape (R[k]) * mix; }
            }
            else
            {
                float dryL[64], dryR[64];
                for (int k = 0; k < n; ++k) { dryL[k] = L[k]; dryR[k] = R[k]; }
                auto& o = p.satOs == 1 ? *os2 : *os4;
                auto up = o.processSamplesUp (blk);
                for (size_t c = 0; c < up.getNumChannels(); ++c) { auto* d = up.getChannelPointer (c); for (size_t i = 0; i < up.getNumSamples(); ++i) d[i] = shape (d[i]); }
                o.processSamplesDown (blk);
                for (int k = 0; k < n; ++k) { L[k] = dryL[k] * (1.f - mix) + L[k] * mix; R[k] = dryR[k] * (1.f - mix) + R[k] * mix; }
            }
        }
        // Compressor
        float grMax = 0;
        if (colour && p.compOn && (p.ratio > 1.001f || p.makeup > 0.01f))
        {
            if (std::fabs (p.scHpf - scCache) > 0.01f) { scCache = p.scHpf; scOn = p.scHpf > 21.f; if (scOn) scC = svfMake (1, p.scHpf, sr, 0.7071f); }
            const float aK = std::exp (-1.f / (p.att * 0.001f * sr)), rK = std::exp (-1.f / (p.rel * 0.001f * sr));
            const float mk = dbToGain (p.makeup);
            for (int k = 0; k < n; ++k)
            {
                float dl = (p.extSc && scL) ? scL[k] : L[k], dr = (p.extSc && scR) ? scR[k] : R[k];
                dl = (1.f - p.glue) * dl + p.glue * fbL; dr = (1.f - p.glue) * dr + p.glue * fbR;
                if (scOn) { dl = scHp[0].tick (dl, scC); dr = scHp[1].tick (dr, scC); }
                const float lvl = juce::Decibels::gainToDecibels (std::max (std::fabs (dl), std::fabs (dr)), -100.f);
                const float over = lvl - p.thr;
                float red;
                if (2.f * over < -p.knee) red = 0;
                else if (p.knee > 0.01f && 2.f * std::fabs (over) <= p.knee) { const float t = over + 0.5f * p.knee; red = (1.f / p.ratio - 1.f) * t * t / (2.f * p.knee); }
                else red = (1.f / p.ratio - 1.f) * over;
                const float target = -red; // dB of reduction (positive)
                gr = target > gr ? target + aK * (gr - target) : target + rK * (gr - target);
                const float gg = dbToGain (-gr) * mk;
                L[k] *= gg; R[k] *= gg; fbL = L[k]; fbR = R[k];
                grMax = std::max (grMax, gr);
            }
        }
        compGR.store (grMax);
        // Width
        if (colour && p.widthOn)
        {
            if (std::fabs (p.mono - monoCache) > 0.01f) { monoCache = p.mono; if (p.mono > 1.f) monoC = svfMake (1, p.mono, sr, 0.7071f); }
            for (int k = 0; k < n; ++k)
            {
                const float m = 0.5f * (L[k] + R[k]); float s = 0.5f * (L[k] - R[k]) * p.ms;
                if (p.mono > 1.f) s = monoHp.tick (s, monoC);
                L[k] = m + s; R[k] = m - s;
            }
        }
        // Limiter (look-ahead, true-peak estimate)
        float limMin = 1.f;
        if (p.limOn)
        {
            const float ceil = dbToGain (p.ceil);
            for (int k = 0; k < n; ++k)
            {
                const float tl = tpEstimate (prev2[0], prevTp[0], L[k], L[k]), tr = tpEstimate (prev2[1], prevTp[1], R[k], R[k]);
                prev2[0] = prevTp[0]; prevTp[0] = L[k]; prev2[1] = prevTp[1]; prevTp[1] = R[k];
                const float pk = std::max (tl, tr);
                const float need = pk > ceil ? ceil / pk : 1.f;
                laBuf[0][(size_t) laW] = L[k]; laBuf[1][(size_t) laW] = R[k]; gBuf[(size_t) laW] = need;
                if (++laW > laLen) laW = 0;
                float wmin = 1.f; for (float v : gBuf) wmin = std::min (wmin, v);
                const float relMs = p.autoRel ? 30.f + 600.f * clampf (grAvg / 8.f, 0.f, 1.f) : p.lrel;
                const float rk = std::exp (-1.f / (relMs * 0.001f * sr));
                const float ak = std::exp (-1.f / (0.3f * (float) laLen));
                limG = wmin < limG ? wmin + ak * (limG - wmin) : wmin + rk * (limG - wmin);
                limG = std::min (limG, 1.f);
                const float dbr = -juce::Decibels::gainToDecibels (limG, -60.f);
                grAvg += (dbr - grAvg) * 0.0005f;
                const float oL = laBuf[0][(size_t) laW] * limG, oR = laBuf[1][(size_t) laW] * limG;
                L[k] = clampf (oL, -ceil, ceil); R[k] = clampf (oR, -ceil, ceil);
                limMin = std::min (limMin, limG);
            }
        }
        else
        {
            // limiter off: keep the same look-ahead delay so the plugin's latency never changes
            for (int k = 0; k < n; ++k)
            {
                laBuf[0][(size_t) laW] = L[k]; laBuf[1][(size_t) laW] = R[k]; gBuf[(size_t) laW] = 1.f;
                if (++laW > laLen) laW = 0;
                L[k] = laBuf[0][(size_t) laW]; R[k] = laBuf[1][(size_t) laW];
            }
            limG = 1.f;
        }
        limGR.store (-juce::Decibels::gainToDecibels (limMin, -60.f));
        // output gain + meters
        float tp = 0;
        for (int k = 0; k < n; ++k)
        {
            L[k] *= p.outGain; R[k] *= p.outGain;
            tp = std::max (tp, std::max (std::fabs (L[k]), std::fabs (R[k])));
            cLR = 0.9995f * cLR + 0.0005f * L[k] * R[k]; cLL = 0.9995f * cLL + 0.0005f * L[k] * L[k]; cRR = 0.9995f * cRR + 0.0005f * R[k] * R[k];
        }
        tpPeak.store (std::max (tp, tpPeak.load() * 0.95f));
        corr.store (cLR / std::sqrt (cLL * cRR + 1e-12f));
    }
};
} // namespace cz
