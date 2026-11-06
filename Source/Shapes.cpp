// Caosmo — 100 procedural factory shapes for the VENV bank (A–J × 10)
#include "Modulators.h"

namespace cz
{
namespace
{
    struct B
    {
        VShape s; B() { s.n = 0; }
        B& p (float x, float y, float c = 0.f) { if (s.n < VShape::kMax) { s.x[s.n] = x; s.y[s.n] = y; s.c[s.n] = c; ++s.n; } return *this; }
        VShape done() { if (s.n == 0) p (0, 0); if (s.x[s.n - 1] < 1.f) p (1.f, s.y[s.n - 1]); s.sanitise(); return s; }
    };
    const char* kNames[100] = {
        // A — envelopes
        "Classic ADSR", "Pluck", "Swell", "AR Gate", "Organ", "Reverse", "Double Hit", "Slow Bloom", "Ping", "Plateau",
        // B — ramps & curves
        "Ramp Up", "Ramp Down", "Exp Rise", "Exp Fall", "Log Rise", "Log Fall", "S-Curve", "Triangle", "Trapezoid", "Half Sine",
        // C — gates
        "Gate 2", "Gate 3", "Gate 4", "Gate 5", "Gate 6", "Gate 8", "Dotted", "Swing", "Stutter", "Gallop",
        // D — stairs
        "Stairs Up 4", "Stairs Up 8", "Stairs Down 4", "Stairs Down 8", "Random Steps 1", "Random Steps 2", "Random Steps 3", "Random Steps 4", "Pyramid", "Arp Steps",
        // E — bounces
        "Bounce 3", "Bounce 4", "Bounce 5", "Bounce 6", "Bounce 8", "Bounce 10", "Bounce 12", "Rebound", "Dribble", "Ricochet",
        // F — waves
        "Sine 1", "Sine 2", "Sine 3", "Sine 5", "Saw 2", "Saw 4", "Square 2", "Square 4", "Tri 3", "Chirp",
        // G — smooth random
        "Drift 1", "Drift 2", "Drift 3", "Drift 4", "Drift 5", "Wander 1", "Wander 2", "Wander 3", "Wander 4", "Wander 5",
        // H — spikes
        "Spike", "Spike Late", "Twin Spike", "Spike Train", "Click", "Snap", "Sparks", "Hammer", "Flick", "Strobe",
        // I — breaths & swells
        "Breath", "Sigh", "Tide", "Heartbeat", "Tremolo Decay", "Vibrato Swell", "Pulse Fade", "Wave Crest", "Lungs", "Ember Glow",
        // J — chaos traces
        "Lorenz 1", "Lorenz 2", "Lorenz 3", "Rossler 1", "Rossler 2", "Duffing 1", "Duffing 2", "Logistic 1", "Logistic 2", "Henon 1" };

    VShape gates (int n, float duty, float swing = 0.f)
    {
        B b; b.p (0, 0);
        for (int i = 0; i < n; ++i)
        {
            float x0 = (float) i / n; if (i & 1) x0 += swing / n;
            const float x1 = x0 + duty / n;
            b.p (x0 + 0.002f, 1.f).p (std::min (x1, 0.998f), 1.f).p (std::min (x1 + 0.002f, 0.999f), 0.f);
        }
        return b.p (1, 0).done();
    }
    VShape stairs (int n, bool up, uint32_t seed = 0)
    {
        B b;
        for (int i = 0; i < n; ++i)
        {
            float v = seed ? hashF (seed * 13u + (uint32_t) i) : (up ? (float) (i + 1) / n : 1.f - (float) i / n);
            b.p ((float) i / n, v).p ((float) (i + 1) / n - 0.003f, v);
        }
        return b.p (1, seed ? 0.f : (up ? 1.f : 0.f)).done();
    }
    VShape bounce (int n, float decay)
    {
        B b; b.p (0, 0); float x = 0, amp = 1, w = 0.3f; float tot = 0; float ww = w;
        for (int i = 0; i < n; ++i) { tot += ww; ww *= 0.72f; }
        for (int i = 0; i < n && b.s.n < 60; ++i)
        {
            const float seg = w / tot;
            b.p (x + seg * 0.5f, amp, -0.6f).p (x + seg, 0.f, 0.6f);
            x += seg; amp *= decay; w *= 0.72f;
        }
        return b.p (1, 0).done();
    }
    VShape wave (int type, int cycles)
    {
        B b; const int steps = std::min (60, cycles * (type == 0 ? 12 : 2));
        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float) i / steps, ph = t * cycles - std::floor (t * cycles);
            float v = 0;
            if (type == 0) v = 0.5f - 0.5f * std::cos (k2Pi * t * cycles);
            b.p (t, type == 0 ? v : 0);
            (void) ph;
        }
        if (type == 0) return b.done();
        B c;
        for (int k = 0; k < cycles; ++k)
        {
            const float x0 = (float) k / cycles, x1 = (float) (k + 1) / cycles, xm = 0.5f * (x0 + x1);
            if (type == 1) c.p (x0, 0).p (x1 - 0.002f, 1.f);                                 // saw
            else if (type == 2) c.p (x0, 1).p (xm - 0.002f, 1).p (xm, 0).p (x1 - 0.002f, 0); // square
            else c.p (x0, 0).p (xm, 1).p (x1 - 0.001f, 0);                                    // triangle
        }
        return c.p (1, 0).done();
    }
    VShape drift (uint32_t seed, int pts, float rough)
    {
        B b;
        for (int i = 0; i < pts; ++i) b.p ((float) i / (pts - 1), clampf (0.5f + (hashF (seed * 101u + (uint32_t) i) - 0.5f) * (0.6f + rough), 0.f, 1.f), (hashF (seed + (uint32_t) i * 7u) - 0.5f) * rough);
        return b.done();
    }
    VShape chaosTrace (int kind, uint32_t seed)
    {
        B b; float x = 0.1f + 0.1f * hashF (seed), y = 0.f, z = 0.f; const int pts = 48;
        std::vector<float> v;
        for (int i = 0; i < pts * 20; ++i)
        {
            switch (kind)
            {
                case 0: { const float h = 0.004f; float dx = 10 * (y - x), dy = x * (28 - z) - y, dz = x * y - 8.f / 3.f * z; x += dx * h; y += dy * h; z += dz * h; } break;
                case 1: { const float h = 0.03f; float dx = -y - z, dy = x + 0.2f * y, dz = 0.2f + z * (x - 5.7f); x += dx * h; y += dy * h; z += dz * h; } break;
                case 2: { const float h = 0.03f; z += h * 1.2f; float dx = y, dy = -0.25f * y + x - x * x * x + 0.4f * std::cos (z); x += dx * h; y += dy * h; } break;
                case 3: { x = 3.9f * (x < 0.f || x > 1.f ? 0.3f : x) * (1.f - (x < 0.f || x > 1.f ? 0.3f : x)); } break;
                default: { float nx = 1.f - 1.4f * x * x + y; y = 0.3f * x; x = nx; } break;
            }
            if (i % (kind >= 3 ? 1 : 20) == 0 && (int) v.size() < pts) v.push_back (kind == 0 ? x / 40.f + 0.5f : kind == 1 ? x / 22.f + 0.5f : kind == 2 ? x / 3.f + 0.5f : kind == 3 ? x : x / 3.f + 0.5f);
        }
        for (int i = 0; i < (int) v.size(); ++i) b.p ((float) i / (v.size() - 1), clampf (v[(size_t) i], 0.f, 1.f), 0.f);
        return b.done();
    }
}

int numFactoryShapes() { return 100; }
juce::String factoryShapeName (int i) { i = juce::jlimit (0, 99, i); return juce::String::charToString ((juce::juce_wchar) ('A' + i / 10)) + juce::String (i % 10 + 1) + " " + kNames[i]; }

VShape factoryShape (int i)
{
    i = juce::jlimit (0, 99, i);
    const int bank = i / 10, k = i % 10;
    switch (bank)
    {
        case 0:
            switch (k)
            {
                case 0: return B().p (0, 0, -0.4f).p (0.12f, 1, 0.5f).p (0.35f, 0.7f).p (0.7f, 0.7f, 0.5f).p (1, 0).done();
                case 1: return B().p (0, 0).p (0.01f, 1, 0.7f).p (0.45f, 0.05f, 0.4f).p (1, 0).done();
                case 2: return B().p (0, 0, 0.6f).p (0.6f, 1).p (0.75f, 0.9f).p (1, 0, -0.3f).done();
                case 3: return B().p (0, 0).p (0.02f, 1).p (0.7f, 1).p (0.72f, 0).p (1, 0).done();
                case 4: return B().p (0, 0).p (0.005f, 1).p (0.7f, 1).p (0.705f, 0).p (1, 0).done();
                case 5: return B().p (0, 0, 0.8f).p (0.65f, 1).p (0.67f, 0).p (1, 0).done();
                case 6: return B().p (0, 0).p (0.01f, 1, 0.5f).p (0.15f, 0.2f).p (0.18f, 0.9f, 0.5f).p (0.5f, 0.6f).p (0.7f, 0.6f, 0.4f).p (1, 0).done();
                case 7: return B().p (0, 0, 0.8f).p (0.45f, 0.4f, -0.5f).p (0.7f, 1).p (0.85f, 0.8f, 0.5f).p (1, 0).done();
                case 8: return B().p (0, 0).p (0.003f, 1, 0.85f).p (0.25f, 0.f).p (1, 0).done();
                default: return B().p (0, 0, -0.6f).p (0.2f, 0.85f).p (0.8f, 0.85f).p (1, 0, 0.6f).done();
            }
        case 1:
            switch (k)
            {
                case 0: return B().p (0, 0).p (1, 1).done();
                case 1: return B().p (0, 1).p (1, 0).done();
                case 2: return B().p (0, 0, 0.8f).p (1, 1).done();
                case 3: return B().p (0, 1, 0.8f).p (1, 0).done();
                case 4: return B().p (0, 0, -0.8f).p (1, 1).done();
                case 5: return B().p (0, 1, -0.8f).p (1, 0).done();
                case 6: return B().p (0, 0, 0.6f).p (0.5f, 0.5f, -0.6f).p (1, 1).done();
                case 7: return B().p (0, 0).p (0.5f, 1).p (1, 0).done();
                case 8: return B().p (0, 0).p (0.25f, 1).p (0.75f, 1).p (1, 0).done();
                default: return B().p (0, 0, -0.5f).p (0.5f, 1, 0.5f).p (1, 0).done();
            }
        case 2:
            switch (k)
            {
                case 0: return gates (2, 0.5f);  case 1: return gates (3, 0.5f); case 2: return gates (4, 0.5f); case 3: return gates (5, 0.45f);
                case 4: return gates (6, 0.4f);  case 5: return gates (8, 0.4f); case 6: return gates (3, 0.75f); case 7: return gates (4, 0.4f, 0.3f);
                case 8: return gates (12, 0.3f);
                default: { B b; b.p (0, 0); const float xs[] = { 0.f, 0.1f, 0.18f, 0.5f, 0.6f, 0.68f };
                           for (float x0 : xs) b.p (x0 + 0.002f, 1).p (x0 + 0.06f, 1).p (x0 + 0.062f, 0); return b.p (1, 0).done(); }
            }
        case 3:
            switch (k)
            {
                case 0: return stairs (4, true);  case 1: return stairs (8, true); case 2: return stairs (4, false); case 3: return stairs (8, false);
                case 4: return stairs (8, true, 11); case 5: return stairs (8, true, 23); case 6: return stairs (12, true, 37); case 7: return stairs (16, true, 51);
                case 8: { B b; const float v[] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.f, 0.8f, 0.6f, 0.4f, 0.2f }; for (int i = 0; i < 9; ++i) b.p (i / 9.f, v[i]).p ((i + 1) / 9.f - 0.003f, v[i]); return b.p (1, 0.2f).done(); }
                default: { B b; const float v[] = { 0.f, 0.33f, 0.58f, 1.f, 0.58f, 0.33f, 0.f, 0.58f }; for (int i = 0; i < 8; ++i) b.p (i / 8.f, v[i]).p ((i + 1) / 8.f - 0.003f, v[i]); return b.p (1, 0).done(); }
            }
        case 4:
        {
            static const int n[] = { 3, 4, 5, 6, 8, 10, 12, 6, 9, 7 };
            static const float d[] = { 0.6f, 0.65f, 0.7f, 0.72f, 0.75f, 0.8f, 0.82f, 0.4f, 0.85f, 0.55f };
            return bounce (n[k], d[k]);
        }
        case 5:
            switch (k)
            {
                case 0: return wave (0, 1); case 1: return wave (0, 2); case 2: return wave (0, 3); case 3: return wave (0, 5);
                case 4: return wave (1, 2); case 5: return wave (1, 4); case 6: return wave (2, 2); case 7: return wave (2, 4); case 8: return wave (3, 3);
                default: { B b; for (int i = 0; i <= 60; ++i) { const float t = i / 60.f; b.p (t, 0.5f - 0.5f * std::cos (k2Pi * (1.f + 7.f * t * t) * t)); } return b.done(); }
            }
        case 6: return drift (1000u + (uint32_t) k * 77u, k < 5 ? 9 : 17, k < 5 ? 0.2f : 0.7f);
        case 7:
            switch (k)
            {
                case 0: return B().p (0, 0).p (0.02f, 1, 0.7f).p (0.12f, 0).p (1, 0).done();
                case 1: return B().p (0, 0).p (0.6f, 0).p (0.62f, 1, 0.7f).p (0.75f, 0).p (1, 0).done();
                case 2: return B().p (0, 0).p (0.01f, 1, 0.7f).p (0.1f, 0).p (0.4f, 0).p (0.41f, 0.8f, 0.7f).p (0.5f, 0).p (1, 0).done();
                case 3: { B b; for (int i = 0; i < 8; ++i) { const float x0 = i / 8.f; b.p (x0, 0).p (x0 + 0.005f, 1.f - i * 0.1f, 0.8f).p (x0 + 0.06f, 0); } return b.p (1, 0).done(); }
                case 4: return B().p (0, 0).p (0.002f, 1).p (0.02f, 0).p (1, 0).done();
                case 5: return B().p (0, 1, 0.9f).p (0.08f, 0).p (1, 0).done();
                case 6: { B b; for (int i = 0; i < 14; ++i) { const float x0 = hashF (900u + (uint32_t) i) * 0.95f; b.p (x0, 0); } VShape s = b.done(); B c; c.p (0, 0);
                          for (int i = 0; i < 12; ++i) { const float x0 = (i + hashF (300u + (uint32_t) i) * 0.6f) / 12.f; c.p (x0, 0).p (x0 + 0.004f, hashF (500u + (uint32_t) i)).p (x0 + 0.02f, 0); } (void) s; return c.p (1, 0).done(); }
                case 7: return B().p (0, 0).p (0.003f, 1).p (0.05f, 0.3f, 0.6f).p (0.3f, 0).p (1, 0).done();
                case 8: return B().p (0, 0.3f).p (0.1f, 1, 0.8f).p (0.15f, 0.3f).p (1, 0.3f).done();
                default: return gates (16, 0.25f);
            }
        case 8:
            switch (k)
            {
                case 0: return B().p (0, 0, -0.5f).p (0.4f, 1, 0.5f).p (1, 0).done();
                case 1: return B().p (0, 0.6f, -0.5f).p (0.2f, 1, 0.6f).p (1, 0).done();
                case 2: return B().p (0, 0.2f, -0.5f).p (0.25f, 0.8f, 0.5f).p (0.5f, 0.3f, -0.5f).p (0.75f, 1.f, 0.5f).p (1, 0.2f).done();
                case 3: { B b; for (int i = 0; i < 4; ++i) { const float x0 = i / 4.f; b.p (x0, 0.1f).p (x0 + 0.03f, 1, 0.6f).p (x0 + 0.08f, 0.2f).p (x0 + 0.11f, 0.7f, 0.6f).p (x0 + 0.2f, 0.1f); } return b.p (1, 0.1f).done(); }
                case 4: { B b; for (int i = 0; i <= 48; ++i) { const float t = i / 48.f; b.p (t, (1.f - t) * (0.5f + 0.5f * std::cos (k2Pi * 8.f * t))); } return b.done(); }
                case 5: { B b; for (int i = 0; i <= 48; ++i) { const float t = i / 48.f; b.p (t, clampf (t * 1.4f, 0.f, 1.f) * (0.75f + 0.25f * std::cos (k2Pi * 6.f * t))); } return b.done(); }
                case 6: { B b; for (int i = 0; i < 10; ++i) { const float x0 = i / 10.f; const float a = 1.f - i / 10.f; b.p (x0, 0).p (x0 + 0.01f, a).p (x0 + 0.06f, a * 0.5f).p (x0 + 0.098f, 0); } return b.p (1, 0).done(); }
                case 7: return B().p (0, 0.2f, -0.4f).p (0.3f, 0.6f, 0.4f).p (0.55f, 1.f, 0.7f).p (0.7f, 0.1f, -0.3f).p (1, 0.2f).done();
                case 8: return B().p (0, 0, -0.6f).p (0.35f, 1, 0.6f).p (0.5f, 0.6f, -0.6f).p (0.85f, 0.9f, 0.6f).p (1, 0).done();
                default: return B().p (0, 0, -0.8f).p (0.5f, 0.6f, -0.2f).p (0.8f, 1).p (1, 0, 0.7f).done();
            }
        default:
        {
            static const int kind[] = { 0, 0, 0, 1, 1, 2, 2, 3, 3, 4 };
            return chaosTrace (kind[k], 17u + (uint32_t) k * 31u);
        }
    }
}
} // namespace cz
