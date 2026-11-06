// Offline test harness: renders every engine, checks level / NaN / pitch, sweeps parameters, times CPU.
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PresetManager.h"
#include "../Source/PluginEditor.h"
#include "../Source/DelugeFX.h"
#include <chrono>

using namespace juce;
using namespace cz;

static void setP (CaosmoProcessor& p, const String& id, float v)
{
    auto* par = p.apvts.getParameter (id);
    if (! par) { std::printf ("!! missing param %s\n", id.toRawUTF8()); return; }
    par->setValueNotifyingHost (par->convertTo0to1 (v));
}

struct RenderResult { float rms = 0, peak = 0, pitch = 0, clarity = 0, dc = 0; int nans = 0; };

static float yin (const std::vector<float>& x, int start, int len, float sr, float& clarity)
{
    const int maxLag = (int) (sr / 30.f), minLag = (int) (sr / 3000.f);
    std::vector<float> d ((size_t) maxLag + 1, 0.f);
    for (int t = 1; t <= maxLag; ++t) { double s = 0; for (int i = 0; i < len; ++i) { float a = x[(size_t) (start + i)] - x[(size_t) (start + i + t)]; s += a * a; } d[(size_t) t] = (float) s; }
    double run = 0; std::vector<float> cm ((size_t) maxLag + 1, 1.f);
    for (int t = 1; t <= maxLag; ++t) { run += d[(size_t) t]; cm[(size_t) t] = run > 0 ? (float) (d[(size_t) t] * t / run) : 1.f; }
    int best = -1;
    for (int t = minLag; t < maxLag; ++t) if (cm[(size_t) t] < 0.2f) { while (t + 1 < maxLag && cm[(size_t) t + 1] < cm[(size_t) t]) ++t; best = t; break; }
    if (best < 0) { clarity = 0; return 0; }
    clarity = 1.f - cm[(size_t) best];
    float a = cm[(size_t) best - 1], b = cm[(size_t) best], c = cm[(size_t) best + 1];
    float shift = (a - c) / (2.f * (a - 2 * b + c) + 1e-9f);
    return sr / ((float) best + shift);
}

static RenderResult render (CaosmoProcessor& p, std::vector<int> notes, float holdS, float relS, std::vector<float>* outL = nullptr, std::vector<float>* outR = nullptr)
{
    const int bs = 256; const float sr = 48000.f;
    const int total = (int) ((holdS + relS) * sr), offAt = (int) (holdS * sr);
    AudioBuffer<float> buf (2, bs); MidiBuffer mb;
    std::vector<float> L, R;
    for (int pos = 0; pos < total; pos += bs)
    {
        mb.clear();
        if (pos == 0) for (int n : notes) mb.addEvent (MidiMessage::noteOn (1, n, (uint8) 110), 0);
        if (pos <= offAt && offAt < pos + bs) for (int n : notes) mb.addEvent (MidiMessage::noteOff (1, n), offAt - pos);
        p.processBlock (buf, mb);
        for (int i = 0; i < bs; ++i) { L.push_back (buf.getSample (0, i)); R.push_back (buf.getSample (1, i)); }
    }
    RenderResult r; double s = 0, dc = 0; int a = (int) (0.3f * sr), b = std::min (offAt, (int) L.size());
    for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) ++r.nans; r.peak = std::max (r.peak, std::max (std::fabs (L[i]), std::fabs (R[i]))); }
    for (int i = a; i < b; ++i) { s += 0.5 * (L[(size_t) i] * L[(size_t) i] + R[(size_t) i] * R[(size_t) i]); dc += 0.5 * (L[(size_t) i] + R[(size_t) i]); }
    r.rms = (float) std::sqrt (s / std::max (1, b - a)); r.dc = (float) (dc / std::max (1, b - a));
    std::vector<float> mono (L.size()); for (size_t i = 0; i < L.size(); ++i) mono[i] = 0.5f * (L[i] + R[i]);
    if (b - a > 6000) r.pitch = yin (mono, a + 2000, 2400, sr, r.clarity);
    if (outL) *outL = L; if (outR) *outR = R;
    return r;
}

static std::unique_ptr<CaosmoProcessor> fresh()
{
    auto p = std::make_unique<CaosmoProcessor>();
    p->setPlayConfigDetails (0, 2, 48000.0, 256);
    p->prepareToPlay (48000.0, 256);
    return p;
}

static void writeWav (const File& f, const std::vector<float>& L, const std::vector<float>& R)
{
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (new FileOutputStream (f), 48000.0, 2, 24, {}, 0));
    if (! w) return;
    AudioBuffer<float> b (2, (int) L.size());
    for (size_t i = 0; i < L.size(); ++i) { b.setSample (0, (int) i, L[i]); b.setSample (1, (int) i, R[i]); }
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

int generatePresets (const File& src, const File& out);

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const String mode = argc > 1 ? argv[1] : "basic";
    const auto& specs = engineSpecs();
    File outDir = File::getCurrentWorkingDirectory().getChildFile ("test_out"); outDir.createDirectory();

    if (mode == "basic" || mode == "wav")
    {
        std::printf ("%-18s %6s %6s %6s %8s %6s %s\n", "engine", "rms", "peak", "dc", "pitch", "clar", "nans");
        for (int e = 1; e <= kNumEngines; ++e)
            for (int note : { 45, 57, 69 })
            {
                auto p = fresh(); setP (*p, laneId (0, "engine"), (float) e);
                std::vector<float> L, R;
                auto r = render (*p, { note }, 1.6f, 0.8f, &L, &R);
                const float expect = 440.f * std::exp2 ((note - 69) / 12.f);
                std::printf ("%-18s n%-3d %6.3f %6.3f %6.3f %8.1f %6.2f %d  (expect %.1f)\n", specs[(size_t) e].name, note, r.rms, r.peak, r.dc, r.pitch, r.clarity, r.nans, expect);
                if (mode == "wav" && note == 57) writeWav (outDir.getChildFile (String (e).paddedLeft ('0', 2) + "_" + specs[(size_t) e].key + ".wav"), L, R);
            }
    }
    if (mode == "sweep")
    {
        const int only = argc > 2 ? String (argv[2]).getIntValue() : 0;
        for (int e = 1; e <= kNumEngines; ++e)
        {
            if (only && e != only) continue;
            const auto& sp = specs[(size_t) e];
            for (auto& pp : sp.p)
                for (float t : { 0.f, 0.25f, 0.5f, 0.75f, 1.f })
                {
                    auto p = fresh(); setP (*p, laneId (0, "engine"), (float) e);
                    auto* par = p->apvts.getParameter (engId (0, e, pp.key));
                    par->setValueNotifyingHost (t);
                    auto r = render (*p, { 57 }, 1.2f, 0.2f);
                    const bool bad = r.nans > 0 || r.rms < 0.01f || r.peak > 3.f;
                    std::printf ("%s %-16s %-12s %8.4f rms %6.3f peak %6.3f pitch %7.1f clar %4.2f %s\n", bad ? "!!" : "  ", sp.name, pp.name,
                                 par->convertFrom0to1 (t), r.rms, r.peak, r.pitch, r.clarity, r.nans ? "NAN" : "");
                }
            for (size_t ci = 0; ci < sp.c.size(); ++ci)
                for (size_t o = 0; o < sp.c[ci].opts.size(); ++o)
                {
                    auto p = fresh(); setP (*p, laneId (0, "engine"), (float) e);
                    setP (*p, engId (0, e, sp.c[ci].key), (float) o);
                    auto r = render (*p, { 57 }, 1.2f, 0.2f);
                    const bool bad = r.nans > 0 || r.rms < 0.01f || r.peak > 3.f;
                    std::printf ("%s %-16s %-12s %-12s rms %6.3f peak %6.3f pitch %7.1f\n", bad ? "!!" : "  ", sp.name, sp.c[ci].name, sp.c[ci].opts[o], r.rms, r.peak, r.pitch);
                }
        }
    }
    if (mode == "cpu")
    {
        for (int e = 1; e <= kNumEngines; ++e)
        {
            auto p = fresh();
            for (int l = 0; l < 3; ++l) setP (*p, laneId (l, "engine"), (float) e);
            setP (*p, laneId (0, "sub1lev"), 0.5f);
            auto t0 = std::chrono::steady_clock::now();
            render (*p, { 48, 52, 55, 59, 62, 64, 67, 71 }, 2.f, 0.f);
            const double el = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            std::printf ("%-18s 8 notes x 3 lanes (+sub on L1): %5.1f%% of one core\n", specs[(size_t) e].name, el / 2.0 * 100.0);
        }
    }
    if (mode == "features")
    {
        auto run = [&] (const char* name, std::function<void (CaosmoProcessor&)> setup, std::vector<int> notes = { 57 })
        {
            auto p = fresh(); setup (*p);
            auto r = render (*p, notes, 1.2f, 1.0f);
            std::printf ("%-36s rms %6.3f peak %6.3f pitch %7.1f nans %d\n", name, r.rms, r.peak, r.pitch, r.nans);
        };
        const char* ft[] = { "LPF", "HPF", "BPF", "Notch", "Peak", "LowShelf", "HighShelf", "AllPass", "Ladder", "Comb", "Formant", "Phaser" };
        for (int t = 0; t < 12; ++t)
            run ((String ("filter ") + ft[t]).toRawUTF8(), [t] (CaosmoProcessor& p) { setP (p, laneId (0, "engine"), 2); setP (p, laneId (0, "ftype"), (float) t);
                                                       setP (p, laneId (0, "cut"), 1200.f); setP (p, laneId (0, "res"), 0.7f); setP (p, laneId (0, "fslope"), 3); });
        run ("env vactrol + LPG", [] (CaosmoProcessor& p) { setP (p, laneId (0, "emode"), 1); setP (p, laneId (0, "lpg"), 1); setP (p, laneId (0, "cut"), 6000.f); });
        run ("env LFO", [] (CaosmoProcessor& p) { setP (p, laneId (0, "emode"), 2); setP (p, laneId (0, "elrate"), 4.f); });
        run ("env one-shot", [] (CaosmoProcessor& p) { setP (p, laneId (0, "esub"), 1); setP (p, laneId (0, "esus"), 0.f); setP (p, laneId (0, "edec"), 400.f); });
        run ("3 lanes stacked", [] (CaosmoProcessor& p) { setP (p, laneId (1, "engine"), 10); setP (p, laneId (2, "engine"), 12); });
        run ("subs", [] (CaosmoProcessor& p) { setP (p, laneId (0, "sub1lev"), 1.f); setP (p, laneId (0, "sub2lev"), 1.f); });
        run ("LFO1 -> cutoff + macro -> M1", [] (CaosmoProcessor& p) {
            setP (p, laneId (0, "cut"), 2000.f); setP (p, slotId (0, "src"), (float) S_Lfo1); setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_Cut)); setP (p, slotId (0, "amt"), 0.4f);
            setP (p, "macro1", 0.8f); setP (p, slotId (1, "src"), (float) S_Macro1); setP (p, slotId (1, "dst"), (float) (kDestLaneBase + LD_M1)); setP (p, slotId (1, "amt"), 0.5f); });
        run ("poly LFO perlin -> pitch quant", [] (CaosmoProcessor& p) {
            setP (p, lfoId (0, "mode"), 1); setP (p, lfoId (0, "wave"), 7); setP (p, slotId (0, "src"), (float) S_Lfo1);
            setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_Pitch)); setP (p, slotId (0, "amt"), 0.1f); setP (p, slotId (0, "quant"), 2); }, { 57, 60, 64 });
        run ("drive all + EQ + width + mono", [] (CaosmoProcessor& p) {
            setP (p, laneId (0, "drive"), 0.6f); setP (p, laneId (0, "dflav"), 4); setP (p, laneId (0, "hpf"), 80.f); setP (p, laneId (0, "mg"), 6.f);
            setP (p, laneId (0, "width"), 1.8f); setP (p, laneId (0, "mono"), 150.f); setP (p, laneId (0, "vspread"), -1.f); }, { 48, 55, 64 });
        run ("mono legato glide", [] (CaosmoProcessor& p) { setP (p, "voices", 0); setP (p, "legato", 1); setP (p, "glide", 200.f); }, { 57, 64 });
        run ("64 voices chord", [] (CaosmoProcessor& p) { setP (p, "voices", 5); }, { 36, 40, 43, 48, 52, 55, 60, 64, 67, 72 });
        run ("lane channel filter (L1 ch2)", [] (CaosmoProcessor& p) { setP (p, laneId (0, "chan"), 2); });
        run ("velocity -> level, keytrack -> cut", [] (CaosmoProcessor& p) {
            setP (p, slotId (0, "src"), (float) S_Velocity); setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_Cut)); setP (p, slotId (0, "amt"), 0.3f);
            setP (p, slotId (1, "src"), (float) S_Key); setP (p, slotId (1, "dst"), (float) (kDestLaneBase + LD_Res)); setP (p, slotId (1, "amt"), 0.3f); });
    }
    if (mode == "grid" && argc > 4)
    {
        const int e = String (argv[2]).getIntValue(); const String a = argv[3], b = argv[4];
        const int note = argc > 5 ? String (argv[5]).getIntValue() : 57;
        const float expect = 440.f * std::exp2 ((note - 69) / 12.f);
        std::printf ("rows %s, cols %s  (ratio pitch/expected, '.' = unpitched)\n", a.toRawUTF8(), b.toRawUTF8());
        for (float ta : { 0.f, 0.2f, 0.4f, 0.6f, 0.8f, 1.f })
        {
            std::printf ("%4.1f |", ta);
            for (float tb : { 0.f, 0.2f, 0.4f, 0.6f, 0.8f, 1.f })
            {
                auto p = fresh(); setP (*p, laneId (0, "engine"), (float) e);
                p->apvts.getParameter (engId (0, e, a.toRawUTF8()))->setValueNotifyingHost (ta);
                p->apvts.getParameter (engId (0, e, b.toRawUTF8()))->setValueNotifyingHost (tb);
                auto r = render (*p, { note }, 1.2f, 0.1f);
                if (r.pitch > 0 && r.clarity > 0.6f) std::printf (" %5.2f", r.pitch / expect); else std::printf ("   .  ");
                std::printf ("/%.2f", r.rms);
            }
            std::printf ("\n");
        }
    }
    if (mode == "set" && argc > 2)   // set <engine> key=val key=val ... [note=57]
    {
        const int e = String (argv[2]).getIntValue(); int note = 57;
        auto p = fresh(); setP (*p, laneId (0, "engine"), (float) e);
        for (int i = 3; i < argc; ++i)
        {
            const String kv = argv[i]; const String k = kv.upToFirstOccurrenceOf ("=", false, false); const float v = kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
            if (k == "note") note = (int) v; else if (k.startsWith ("L1_") || ! k.containsChar ('.')) { if (p->apvts.getParameter (k)) setP (*p, k, v); else setP (*p, engId (0, e, k.toRawUTF8()), v); }
        }
        auto r = render (*p, { note }, 1.6f, 0.3f);
        std::printf ("rms %.3f peak %.3f pitch %.1f (expect %.1f) clar %.2f nans %d\n", r.rms, r.peak, r.pitch, 440.f * std::exp2 ((note - 69) / 12.f), r.clarity, r.nans);
    }
    if (mode == "ui")
    {
        auto p = fresh();
        { PresetManager pm (*p); pm.load (argc > 2 ? String (argv[2]).getIntValue() : 2); }
        p->prepareToPlay (48000.0, 256);
        render (*p, { 48, 55, 60, 64 }, 1.0f, 0.0f);   // fill scopes and meters
        std::unique_ptr<AudioProcessorEditor> ed (p->createEditor());
        auto* ce = dynamic_cast<CaosmoEditor*> (ed.get());
        auto snap = [&] (const String& n)
        {
            for (int i = 0; i < 3; ++i) { MessageManager::getInstance()->runDispatchLoopUntil (120); }
            auto img = ed->createComponentSnapshot (ed->getLocalBounds());
            PNGImageFormat png; auto f = outDir.getChildFile ("ui_" + n + ".png"); f.deleteFile();
            FileOutputStream os (f); png.writeImageToStream (img, os);
        };
        for (int pg = 0; pg < 4; ++pg) { ce->showPage (pg); snap (String (pg)); }
        ce->showPage (3);
        {
            std::function<MasterPage* (Component*)> find = [&] (Component* c) -> MasterPage* {
                if (auto* m = dynamic_cast<MasterPage*> (c)) return m;
                for (auto* ch : c->getChildren()) if (auto* m = find (ch)) return m;
                return nullptr; };
            if (auto* mp = find (ed.get())) { setP (*p, "dl_on", 1); mp->setView (2); snap ("deluge"); mp->setView (0); }
        }
        ce->testOverlay (0); snap ("browser"); ce->testOverlay (-1);
        ce->testOverlay (1); snap ("save"); ce->testOverlay (-1);
        ce->testOverlay (2); snap ("settings"); ce->testOverlay (-1);
        std::printf ("ui snapshots written\n");
    }
    if (mode == "features2")
    {
        auto run = [&] (const char* name, std::function<void (CaosmoProcessor&)> setup, std::vector<int> notes = { 57 }, float hold = 1.2f, float rel = 1.0f)
        {
            auto p = fresh(); setup (*p);
            auto r = render (*p, notes, hold, rel);
            std::printf ("%-40s rms %6.3f peak %6.3f pitch %7.1f nans %d\n", name, r.rms, r.peak, r.pitch, r.nans);
            return r;
        };
        for (int t = 1; t < kNumFxTypes; ++t)
            run ((String ("FX A slot1 ") + kFxTypeNames[t]).toRawUTF8(), [t] (CaosmoProcessor& p) {
                setP (p, fxId (0, 0, "type"), (float) t); setP (p, laneId (0, "fx1"), 0.f); setP (p, "macro1", 1.f); });
        run ("FX chain delay>reverb>granular freeze", [] (CaosmoProcessor& p) {
            setP (p, fxId (1, 0, "type"), 2); setP (p, fxId (1, 1, "type"), 1); setP (p, fxId (1, 2, "type"), 4); setP (p, fxId (1, 2, "opt"), 1);
            setP (p, laneId (0, "fx2"), -6.f); });
        run ("master sat tube x4 + comp", [] (CaosmoProcessor& p) {
            setP (p, "m_satdrive", 0.7f); setP (p, "m_satflav", 1); setP (p, "m_satos", 2); setP (p, "m_thr", -30.f); setP (p, "m_ratio", 6.f); setP (p, "m_cgain", 6.f); setP (p, "m_glue", 0.5f); });
        auto r = run ("limiter -3 dBTP on hot stack", [] (CaosmoProcessor& p) {
            setP (p, laneId (1, "engine"), 2); setP (p, laneId (2, "engine"), 12); setP (p, "out_gain", 0.f); setP (p, "m_limon", 1); setP (p, "m_ceil", -3.f);
            for (int l = 0; l < 3; ++l) setP (p, laneId (l, "level"), 9.f); }, { 48, 55, 60, 64 });
        std::printf ("   -> limiter peak %.3f (ceiling %.3f)\n", r.peak, std::pow (10.f, -3.f / 20.f));
        run ("lane env = VENV 3 (gates) sustain-loop", [] (CaosmoProcessor& p) { setP (p, laneId (0, "emode"), 3); setP (p, laneId (0, "evsrc"), 2); setP (p, laneId (0, "esub"), 2); setP (p, laneId (0, "evtime"), 0.5f); });
        run ("VENV1 poly -> cutoff", [] (CaosmoProcessor& p) { setP (p, laneId (0, "cut"), 300.f);
            setP (p, slotId (0, "src"), (float) S_Venv1); setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_Cut)); setP (p, slotId (0, "amt"), 0.6f); });
        run ("VENV2 global, host sync", [] (CaosmoProcessor& p) { setP (p, venvId (1, "voicing"), 0); setP (p, venvId (1, "sync"), 1);
            setP (p, slotId (0, "src"), (float) S_Venv1 + 1); setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_Level)); setP (p, slotId (0, "amt"), -0.4f); });
        for (int t = 0; t < 5; ++t)
            run ((String ("attractor ") + kAttrTypeNames[t] + " -> pitch & motor1").toRawUTF8(), [t] (CaosmoProcessor& p) {
                setP (p, attrId (0, "type"), (float) t); setP (p, attrId (0, "rate"), 3.f);
                setP (p, slotId (0, "src"), (float) S_Attr1); setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_Pitch)); setP (p, slotId (0, "amt"), 0.05f);
                setP (p, slotId (1, "src"), (float) S_Attr1); setP (p, slotId (1, "dst"), (float) (kDestLaneBase + LD_M1)); setP (p, slotId (1, "amt"), 0.3f); });
        run ("attractor poly + LFO Oct link pendulum", [] (CaosmoProcessor& p) {
            setP (p, attrId (1, "mode"), 1); setP (p, lfoId (0, "link"), 3); setP (p, lfoId (0, "linkrel"), 3);
            for (int i = 0; i < 4; ++i) { setP (p, slotId (i, "src"), (float) (S_Lfo1 + i * 2)); setP (p, slotId (i, "dst"), (float) (kDestLaneBase + LD_Cut)); setP (p, slotId (i, "amt"), 0.1f); }
            setP (p, slotId (5, "src"), (float) (S_Attr1 + 1)); setP (p, slotId (5, "dst"), (float) (kDestLaneBase + LD_Res)); setP (p, slotId (5, "amt"), 0.3f); }, { 50, 57, 62 });
        run ("loop lock perlin, fade-in, jitter", [] (CaosmoProcessor& p) {
            setP (p, "lfo_loop", 1); setP (p, "lfo_looplen", 0.3f); setP (p, lfoId (2, "wave"), 7); setP (p, lfoId (2, "rate"), 5.f);
            setP (p, slotId (0, "src"), (float) (S_Lfo1 + 2)); setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_Pitch)); setP (p, slotId (0, "amt"), 0.04f);
            setP (p, slotId (0, "fin"), 500.f); setP (p, slotId (0, "jit"), 0.5f); });
        run ("EQ morph mod by LFO", [] (CaosmoProcessor& p) {
            setP (p, slotId (0, "src"), (float) S_Lfo1); setP (p, slotId (0, "dst"), (float) (kDestLaneBase + LD_EqMorph)); setP (p, slotId (0, "amt"), 0.5f); });
        run ("HQ off (additive)", [] (CaosmoProcessor& p) { setP (p, laneId (0, "engine"), 8); setP (p, "hq", 0); });
        run ("FX param mod + bus level mod", [] (CaosmoProcessor& p) { setP (p, laneId (0, "fx1"), -3.f);
            setP (p, slotId (0, "src"), (float) S_Lfo1); setP (p, slotId (0, "dst"), (float) kDestFxParam); setP (p, slotId (0, "amt"), 0.3f);
            setP (p, slotId (1, "src"), (float) S_ModWheel); setP (p, slotId (1, "dst"), (float) kDestBusLevel); setP (p, slotId (1, "amt"), -0.5f); });
        {   // MPE: two notes on member channels, per-note bend
            auto p = fresh(); setP (*p, "mpe", 1);
            const int bs = 256; AudioBuffer<float> buf (2, bs); MidiBuffer mb; double s = 0; int cnt = 0; int nans = 0;
            for (int pos = 0; pos < 48000; pos += bs)
            {
                mb.clear();
                if (pos == 0) { mb.addEvent (MidiMessage::noteOn (2, 57, (uint8) 100), 0); mb.addEvent (MidiMessage::noteOn (3, 64, (uint8) 100), 0); }
                if (pos == 12032) { mb.addEvent (MidiMessage::pitchWheel (2, 12000), 0); mb.addEvent (MidiMessage::channelPressureChange (3, 100), 0); }
                p->processBlock (buf, mb);
                for (int i = 0; i < bs; ++i) { float v = buf.getSample (0, i); if (! std::isfinite (v)) ++nans; s += v * v; ++cnt; }
            }
            std::printf ("%-40s rms %6.3f nans %d voices %d\n", "MPE lower zone, member bend/pressure", std::sqrt (s / cnt), nans, p->ui.activeVoices.load());
        }
        {   // VENV shapes + keytrack curve round-trip through state
            auto p = fresh();
            p->setShape (0, factoryShape (41), factoryShapeName (41));
            KeyCurve k; k.n = 3; k.x[0] = 0; k.y[0] = 1; k.x[1] = 0.5f; k.y[1] = -1; k.x[2] = 1; k.y[2] = 1; p->setKeyCurve (k);
            MemoryBlock mbk; p->getStateInformation (mbk);
            auto q = fresh(); q->setStateInformation (mbk.getData(), (int) mbk.getSize());
            std::printf ("state round-trip: shape '%s' pts %d, key pts %d\n", q->venvTree (0).getProperty ("name").toString().toRawUTF8(), q->getShape (0).n, q->getKeyCurve().n);
        }
    }
    if (mode == "genpresets")
        return generatePresets (File::getCurrentWorkingDirectory().getChildFile ("presets/factory.txt"),
                                File::getCurrentWorkingDirectory().getChildFile ("presets/factory_presets.xml")) > 0 ? 1 : 0;
    if (mode == "loadpresets")
    {
        auto probe = fresh(); PresetManager pm0 (*probe);
        int bad = 0; const int n = pm0.all().size();
        for (int i = 0; i < n; ++i)
        {
            auto p = fresh(); PresetManager pm (*p);
            if (! pm.load (i)) { std::printf ("!! load failed %d\n", i); ++bad; continue; }
            p->prepareToPlay (48000.0, 256);
            auto r = render (*p, { 48, 60 }, 1.5f, 0.5f);
            const bool b = r.nans > 0 || r.peak < 0.005f || r.peak > 0.95f;
            if (b) ++bad;
            if (b || i % 16 == 0) std::printf ("%s %3d %-24s rms %.3f peak %.3f current='%s'\n", b ? "!!" : "  ", i, pm.all()[i].name.toRawUTF8(), r.rms, r.peak, pm.currentName().toRawUTF8());
        }
        std::printf ("%d presets loaded, %d problems\n", n, bad);
    }
    if (mode == "demos")
    {
        auto probe = fresh(); PresetManager pm0 (*probe);
        const auto& all = pm0.all();
        for (int i = (argc > 2 ? String (argv[2]).getIntValue() : 0); i < all.size(); i += 8)
        {
            auto p = fresh(); PresetManager pm (*p); pm.load (i); p->prepareToPlay (48000.0, 256);
            const String t = all[i].tags.joinIntoString (";");
            std::vector<int> notes = t.contains ("Bass") ? std::vector<int> { 36 } : (t.contains ("Pad") || t.contains ("Drone") || t.contains ("Texture") ? std::vector<int> { 48, 55, 60, 64 } : std::vector<int> { 60, 67 });
            std::vector<float> L, R;
            render (*p, notes, i >= 128 ? 9.0f : 3.0f, 3.0f, &L, &R);
            writeWav (outDir.getChildFile ("demo_" + String (i).paddedLeft ('0', 3) + "_" + File::createLegalFileName (all[i].name).replace (" ", "_") + ".wav"), L, R);
            std::printf ("demo %s\n", all[i].name.toRawUTF8());
        }
    }
    if (mode == "delugegain")
    {
        for (float amp : { 0.05f, 0.2f, 0.5f })
        {
            DelugeRack rack; rack.prepare (48000.0); DelugeParams dp; dp.on = true;
            std::vector<float> l (48000), r (48000), l0, r0;
            for (int i = 0; i < 48000; ++i) l[(size_t) i] = r[(size_t) i] = amp * std::sin (6.2831853f * 220.f * i / 48000.f);
            l0 = l; r0 = r;
            for (int pos = 0; pos < 48000; pos += 256) rack.process (l.data() + pos, r.data() + pos, std::min (256, 48000 - pos), dp, 120.0);
            double a = 0, b = 0; for (int i = 24000; i < 48000; ++i) { a += l0[(size_t) i] * l0[(size_t) i]; b += l[(size_t) i] * l[(size_t) i]; }
            std::printf ("neutral rack gain at amp %.2f: %.4f (%.2f dB)\n", amp, std::sqrt (b / a), 10.0 * std::log10 (b / a));
        }
    }
    if (mode == "deluge")
    {
        auto base = [] (CaosmoProcessor& p) { setP (p, "dl_on", 1); };
        auto run = [&] (const char* name, std::function<void (CaosmoProcessor&)> setup, std::vector<int> notes = { 57 }, float hold = 1.0f, float rel = 1.5f)
        {
            auto p = fresh(); base (*p); setup (*p);
            std::vector<float> L, R;
            auto t0 = std::chrono::steady_clock::now();
            auto r = render (*p, notes, hold, rel, &L, &R);
            const double el = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            double tail = 0; const size_t a = (size_t) ((hold + 0.6f) * 48000.f); for (size_t i = a; i < L.size(); ++i) tail += L[i] * L[i];
            std::printf ("%-34s rms %6.3f peak %6.3f tail %8.4f nans %d  cpu %4.1f%%\n", name, r.rms, r.peak, std::sqrt (tail / std::max<size_t> (1, L.size() - a)), r.nans, el / (hold + rel) * 100.0);
            return std::make_pair (L, R);
        };
        run ("bypass (dl_on=0)", [] (CaosmoProcessor& p) { setP (p, "dl_on", 0); });
        run ("on, all neutral", [] (CaosmoProcessor&) {});
        const char* mods[] = { "", "Flanger", "Chorus", "Phaser", "Stereo Chorus", "Warble", "Dimension", "Grain" };
        for (int m = 1; m <= 7; ++m)
            run ((String ("mod ") + mods[m]).toRawUTF8(), [m] (CaosmoProcessor& p) { setP (p, "dl_modtype", (float) m); setP (p, "dl_moddepth", 40.f); setP (p, "dl_modfb", 30.f); });
        run ("EQ bass +20 treble -20", [] (CaosmoProcessor& p) { setP (p, "dl_bass", 20.f); setP (p, "dl_treble", -20.f); });
        run ("decimation 35", [] (CaosmoProcessor& p) { setP (p, "dl_srr", 35.f); });
        run ("bitcrush 40", [] (CaosmoProcessor& p) { setP (p, "dl_crush", 40.f); });
        run ("saturation 8", [] (CaosmoProcessor& p) { setP (p, "dl_sat", 8.f); });
        run ("delay digital 1/16", [] (CaosmoProcessor& p) { setP (p, "dl_dlyamt", 30.f); });
        run ("delay analog free", [] (CaosmoProcessor& p) { setP (p, "dl_dlyamt", 35.f); setP (p, "dl_dlysync", 0); setP (p, "dl_dlyanalog", 1); });
        for (int m = 0; m < 3; ++m)
            run ((String ("reverb model ") + String (m)).toRawUTF8(), [m] (CaosmoProcessor& p) { setP (p, "dl_rvbmodel", (float) m); setP (p, "dl_rvbamt", 35.f); setP (p, "dl_rvbroom", 40.f); });
        run ("compressor thr 40 ratio 40", [] (CaosmoProcessor& p) { setP (p, "dl_cmpthr", 40.f); setP (p, "dl_cmpratio", 40.f); }, { 48, 55, 60, 64 });
        run ("stutter quantized", [] (CaosmoProcessor& p) { setP (p, "dl_stutter", 1); });
        run ("everything at once", [] (CaosmoProcessor& p) {
            setP (p, "dl_modtype", 5); setP (p, "dl_moddepth", 35.f); setP (p, "dl_bass", 10.f); setP (p, "dl_srr", 20.f); setP (p, "dl_crush", 25.f); setP (p, "dl_sat", 5.f);
            setP (p, "dl_dlyamt", 30.f); setP (p, "dl_dlyanalog", 1); setP (p, "dl_rvbmodel", 1); setP (p, "dl_rvbamt", 30.f); setP (p, "dl_cmpthr", 30.f); }, { 48, 55, 60, 64 });
        {   // delay timing: an impulse through a 1/16 synced delay at 120 BPM should repeat after 125 ms
            auto p = fresh(); setP (*p, "dl_on", 1); setP (*p, "dl_dlyamt", 30.f); setP (*p, "dl_dlysync", 6); setP (*p, "dl_dlypp", 0);
            setP (*p, laneId (0, "esub"), 1); setP (*p, laneId (0, "esus"), 0.f); setP (*p, laneId (0, "edec"), 15.f); setP (*p, laneId (0, "eatt"), 0.3f);
            std::vector<float> L, R; render (*p, { 69 }, 0.05f, 1.0f, &L, &R);
            // find the first two energy bursts
            std::vector<float> env (L.size() / 240 + 1, 0.f);
            for (size_t i = 0; i < L.size(); ++i) env[i / 240] = std::max (env[i / 240], std::fabs (L[i]) + std::fabs (R[i]));
            std::printf ("delay 1/16 @120 BPM, burst starts (ms):");
            bool in = false; int cnt = 0;
            for (size_t b = 0; b < env.size() && cnt < 8; ++b) { const bool on = env[b] > 0.01f; if (on && ! in) { std::printf (" %.0f", b * 5.0); ++cnt; } in = on; }
            std::printf ("  (expect spacing 125)\n");
        }
        {   // stutter loop length: quantized at the default knob should repeat 1/16 notes (125 ms at 120 BPM)
            auto p = fresh(); setP (*p, "dl_on", 1);
            setP (*p, laneId (0, "esub"), 1); setP (*p, laneId (0, "esus"), 0.f); setP (*p, laneId (0, "edec"), 15.f); setP (*p, laneId (0, "eatt"), 0.3f);
            const int bs = 256; AudioBuffer<float> buf (2, bs); MidiBuffer mb; std::vector<float> L;
            for (int pos = 0; pos < 48000; pos += bs)
            {
                mb.clear();
                if (pos == 0) mb.addEvent (MidiMessage::noteOn (1, 69, (uint8) 110), 0);
                if (pos == 0) setP (*p, "dl_stutter", 1);
                p->processBlock (buf, mb);
                for (int i = 0; i < bs; ++i) L.push_back (std::fabs (buf.getSample (0, i)));
            }
            std::vector<float> env (L.size() / 240 + 1, 0.f);
            for (size_t i = 0; i < L.size(); ++i) env[i / 240] = std::max (env[i / 240], L[i]);
            std::printf ("stutter @120 BPM, burst starts (ms):");
            bool in = false; int cnt = 0;
            for (size_t b = 0; b < env.size() && cnt < 8; ++b) { const bool on = env[b] > 0.01f; if (on && ! in) { std::printf (" %.0f", b * 5.0); ++cnt; } in = on; }
            std::printf ("\n");
        }
    }
    if (mode == "delugedemos")
    {
        struct D { const char* preset; const char* file; std::vector<std::pair<const char*, float>> set; std::vector<int> notes; float hold; };
        std::vector<D> demos = {
            { "Glass Synchrony", "deluge_warble_mutable", { { "dl_modtype", 5 }, { "dl_moddepth", 35 }, { "dl_rvbmodel", 1 }, { "dl_rvbamt", 32 }, { "dl_rvbroom", 40 } }, { 48, 55, 60, 64 }, 6.f },
            { "Gear Bass", "deluge_analog_delay_bitcrush", { { "dl_crush", 30 }, { "dl_dlyamt", 30 }, { "dl_dlyanalog", 1 }, { "dl_dlysync", 5 }, { "dl_sat", 4 } }, { 36 }, 3.f },
            { "Hyphae", "deluge_dimension_digital_reverb", { { "dl_modtype", 6 }, { "dl_moddepth", 30 }, { "dl_rvbmodel", 2 }, { "dl_rvbamt", 30 }, { "dl_cmpthr", 25 } }, { 48, 55, 60, 67 }, 7.f },
            { "Klock Tower", "deluge_stutter_decimation", { { "dl_stutter", 1 }, { "dl_srr", 25 }, { "dl_dlyamt", 22 } }, { 45, 52 }, 5.f },
            { "Pressure Front", "deluge_grain_freeverb", { { "dl_modtype", 7 }, { "dl_moddepth", 30 }, { "dl_rvbamt", 30 } }, { 48, 55, 60, 64 }, 7.f },
        };
        auto probe = fresh(); PresetManager pm0 (*probe);
        for (auto& d : demos)
        {
            int idx = -1; for (int i = 0; i < pm0.all().size(); ++i) if (pm0.all()[i].name == d.preset) idx = i;
            auto p = fresh(); PresetManager pm (*p); pm.load (idx); p->prepareToPlay (48000.0, 256);
            setP (*p, "dl_on", 1); for (auto& kv : d.set) setP (*p, kv.first, kv.second);
            std::vector<float> L, R; auto r = render (*p, d.notes, d.hold, 3.f, &L, &R);
            writeWav (outDir.getChildFile (String (d.file) + ".wav"), L, R);
            std::printf ("%-32s from '%s': rms %.3f peak %.3f nans %d\n", d.file, d.preset, r.rms, r.peak, r.nans);
        }
    }
    return 0;
}
