// Factory bank generator: parses presets/factory.txt, renders and levels every patch, writes presets/factory_presets.xml
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PresetManager.h"

using namespace juce;
using namespace cz;

namespace
{
    String lastError;

    bool setValue (CaosmoProcessor& p, const String& id, String val)
    {
        auto* par = p.apvts.getParameter (id);
        if (! par) { lastError = "unknown parameter " + id; return false; }
        val = val.replace ("_", " ").trim();
        if (auto* c = dynamic_cast<AudioParameterChoice*> (par))
        {
            int idx = -1;
            for (int i = 0; i < c->choices.size(); ++i) if (c->choices[i].equalsIgnoreCase (val)) { idx = i; break; }
            if (idx < 0 && val.containsOnly ("0123456789")) idx = val.getIntValue();
            if (idx < 0 || idx >= c->choices.size()) { lastError = "bad choice '" + val + "' for " + id; return false; }
            par->setValueNotifyingHost (par->convertTo0to1 ((float) idx));
            return true;
        }
        if (dynamic_cast<AudioParameterBool*> (par))
        {
            const bool on = val.equalsIgnoreCase ("on") || val.equalsIgnoreCase ("true") || val == "1";
            par->setValueNotifyingHost (on ? 1.f : 0.f); return true;
        }
        par->setValueNotifyingHost (par->convertTo0to1 (val.getFloatValue()));
        return true;
    }

    int freeSlot (CaosmoProcessor& p)
    {
        for (int i = 0; i < kNumSlots; ++i) if (p.apvts.getRawParameterValue (slotId (i, "src"))->load() < 0.5f) return i;
        return -1;
    }

    bool addMod (CaosmoProcessor& p, const String& line)
    {
        // MOD <src> > <dst> : <amt> [k=v ...]
        const String body = line.fromFirstOccurrenceOf ("MOD", false, false).trim();
        const String src = body.upToFirstOccurrenceOf (">", false, false).trim();
        const String rest = body.fromFirstOccurrenceOf (">", false, false);
        const String dst = rest.upToFirstOccurrenceOf (":", false, false).trim();
        auto toks = StringArray::fromTokens (rest.fromFirstOccurrenceOf (":", false, false), " ", "");
        toks.removeEmptyStrings();
        if (toks.isEmpty()) { lastError = "MOD without amount: " + line; return false; }
        const float amt = toks[0].getFloatValue();
        if (amt == 0.f) return true;
        const int si = sourceNames().indexOf (src, true), di = destNames().indexOf (dst, true);
        if (si <= 0) { lastError = "unknown source '" + src + "'"; return false; }
        if (di <= 0) { lastError = "unknown destination '" + dst + "'"; return false; }
        const int s = freeSlot (p); if (s < 0) { lastError = "matrix full"; return false; }
        setValue (p, slotId (s, "src"), String (si)); setValue (p, slotId (s, "dst"), String (di)); setValue (p, slotId (s, "amt"), String (amt));
        for (int i = 1; i < toks.size(); ++i)
        {
            const String k = toks[i].upToFirstOccurrenceOf ("=", false, false), v = toks[i].fromFirstOccurrenceOf ("=", false, false);
            if (! setValue (p, slotId (s, k.toRawUTF8()), v)) return false;
        }
        return true;
    }

    struct Preset { PresetInfo info; StringArray lines; bool experimental = false; };

    std::unique_ptr<CaosmoProcessor> freshProc()
    {
        auto p = std::make_unique<CaosmoProcessor>();
        p->setPlayConfigDetails (0, 2, 48000.0, 256);
        p->prepareToPlay (48000.0, 256);
        return p;
    }

    bool apply (CaosmoProcessor& p, const Preset& pr)
    {
        bool usesMacro = false;
        for (auto line : pr.lines)
        {
            line = line.trim();
            if (line.isEmpty()) continue;
            const String head = line.upToFirstOccurrenceOf (" ", false, false);
            if (head == "MOD") { if (line.contains ("Macro")) usesMacro = true; if (! addMod (p, line)) return false; continue; }
            if (head == "KEY") { p.setKeyCurve (KeyCurve::fromString (line.fromFirstOccurrenceOf (" ", false, false).trim())); continue; }
            if (head == "DIST") { p.setDistCurve (KeyCurve::fromString (line.fromFirstOccurrenceOf (" ", false, false).trim())); continue; }
            auto toks = StringArray::fromTokens (line.fromFirstOccurrenceOf (" ", false, false), " ", ""); toks.removeEmptyStrings();
            for (auto& t : toks)
            {
                const String k = t.upToFirstOccurrenceOf ("=", false, false), v = t.fromFirstOccurrenceOf ("=", false, false);
                String id;
                if (head.startsWith ("L") && head.length() == 2 && head.getLastCharacter() >= '1' && head.getLastCharacter() <= '3')
                {
                    const int l = head.getTrailingIntValue() - 1;
                    if (k.startsWith ("e."))
                    {
                        const int e = p.engineOfLane (l);
                        if (e <= 0) { lastError = "engine param before engine on " + head; return false; }
                        id = engId (l, e, k.substring (2).toRawUTF8());
                    }
                    else id = laneId (l, k.toRawUTF8());
                }
                else if (head.startsWith ("LFO")) id = lfoId (head.getTrailingIntValue() - 1, k.toRawUTF8());
                else if (head.startsWith ("VENV"))
                {
                    const int i = head.getTrailingIntValue() - 1;
                    if (k == "shape") { const int s = v.getIntValue(); p.setShape (i, factoryShape (s), factoryShapeName (s), false); continue; }
                    id = venvId (i, k.toRawUTF8());
                }
                else if (head.startsWith ("ATTR")) id = attrId (head.getTrailingIntValue() - 1, k.toRawUTF8());
                else if (head.startsWith ("TRIG")) id = trigId (head.getTrailingIntValue() - 1, k.toRawUTF8());
                else if (head.startsWith ("FX") && head.length() == 4) id = fxId (head[2] - 'A', head[3] - '1', k.toRawUTF8());
                else if (head.startsWith ("FX") && head.length() == 3) id = busId (head[2] - 'A', k.toRawUTF8());
                else if (head == "G") id = k;
                else { lastError = "unknown line head " + head; return false; }
                if (! setValue (p, id, v)) return false;
            }
        }
        if (! usesMacro)
        {
            // five working macros on every patch
            const int e = p.engineOfLane (0);
            const int np = (int) engineSpecs()[(size_t) jmax (0, e)].p.size();
            addMod (p, "MOD Macro 1 > L1 Motor 1 : 0.35");
            addMod (p, "MOD Macro 2 > L1 Cutoff : -0.35");
            addMod (p, "MOD Macro 3 > L1 FX1 Send : 0.45");
            if (np > 1) addMod (p, "MOD Macro 4 > L1 Motor 2 : 0.35");
            addMod (p, "MOD Macro 5 > Sat Drive : 0.45");
        }
        return true;
    }

    struct Meas { float rms = 0, peak = 0, motion = 0; int nans = 0; };
    Meas measure (CaosmoProcessor& p, const PresetInfo& info, bool longHold = false)
    {
        std::vector<int> notes { 57, 64 }; float hold = 1.6f;
        const String t = info.tags.joinIntoString (";");
        if (t.contains ("Bass")) notes = { 36 };
        else if (t.contains ("Pad") || t.contains ("Drone") || t.contains ("Texture")) { notes = { 48, 55, 60, 64 }; hold = 3.0f; }
        else if (t.contains ("Lead")) notes = { 60 };
        else if (t.contains ("Percussive")) { notes = { 48 }; hold = 0.6f; }
        if (longHold) hold = 6.f;
        std::vector<double> segR, segZ; double sr2 = 0; int zc = 0, segN = 0; float prevMono = 0;
        const int bs = 256; const float sr = 48000.f; const int total = (int) ((hold + 1.f) * sr), off = (int) (hold * sr);
        AudioBuffer<float> buf (2, bs); MidiBuffer mb; Meas m; double s = 0; int cnt = 0;
        for (int pos = 0; pos < total; pos += bs)
        {
            mb.clear();
            if (pos == 0) for (int n : notes) mb.addEvent (MidiMessage::noteOn (1, n, (uint8) 100), 0);
            if (pos <= off && off < pos + bs) for (int n : notes) mb.addEvent (MidiMessage::noteOff (1, n), off - pos);
            p.processBlock (buf, mb);
            for (int i = 0; i < bs; ++i)
            {
                const float l = buf.getSample (0, i), r = buf.getSample (1, i);
                if (! std::isfinite (l) || ! std::isfinite (r)) { ++m.nans; continue; }
                m.peak = std::max (m.peak, std::max (std::fabs (l), std::fabs (r)));
                if (pos + i < off)
                {
                    s += 0.5 * (l * l + r * r); ++cnt;
                    const float mono = 0.5f * (l + r);
                    sr2 += mono * mono; if ((mono > 0) != (prevMono > 0)) ++zc; prevMono = mono;
                    if (++segN == 24000) { segR.push_back (std::sqrt (sr2 / segN)); segZ.push_back (zc); sr2 = 0; zc = 0; segN = 0; }
                }
            }
        }
        m.rms = (float) std::sqrt (s / std::max (1, cnt));
        auto cv = [] (const std::vector<double>& v)
        {
            if (v.size() < 3) return 0.0;
            double mu = 0; for (size_t i = 1; i < v.size(); ++i) mu += v[i]; mu /= (double) (v.size() - 1);   // skip the attack segment
            double var = 0; for (size_t i = 1; i < v.size(); ++i) var += (v[i] - mu) * (v[i] - mu); var /= (double) (v.size() - 1);
            return mu > 1e-9 ? std::sqrt (var) / mu : 0.0;
        };
        m.motion = (float) (cv (segR) + cv (segZ));
        return m;
    }
}

int generatePresets (const File& src, const File& out)
{
    Array<Preset> list;
    StringArray all = StringArray::fromLines (src.loadFileAsString());
    const int firstExtra = all.size();
    for (auto* name : { "factory2.txt", "factory3.txt" })
    {
        auto extra = src.getSiblingFile (name);
        if (extra.existsAsFile()) all.addArray (StringArray::fromLines (extra.loadFileAsString()));
    }
    int lineNo = 0;
    for (auto line : all)
    {
        const bool exp = lineNo++ >= firstExtra;
        line = line.trim();
        if (line.isEmpty() || line.startsWith ("#")) continue;
        if (line.startsWith ("@"))
        {
            auto parts = StringArray::fromTokens (line.substring (1), "|", "");
            Preset p; p.info.name = parts[0].trim(); p.info.category = parts[1].trim();
            p.info.tags = StringArray::fromTokens (parts[2].trim(), ";", ""); p.info.tags.removeEmptyStrings();
            p.info.description = parts[3].trim(); p.info.author = "Caosmo"; p.info.factory = true; p.experimental = exp;
            list.add (p);
        }
        else if (! list.isEmpty()) list.getReference (list.size() - 1).lines.add (line);
    }
    ValueTree bank ("CaosmoFactory");
    int errors = 0;
    for (auto& pr : list)
    {
        auto p = freshProc();
        lastError = {};
        if (! apply (*p, pr)) { std::printf ("!! %s: %s\n", pr.info.name.toRawUTF8(), lastError.toRawUTF8()); ++errors; continue; }
        setValue (*p, "m_limon", "1"); setValue (*p, "m_ceil", "-0.8");
        auto m = measure (*p, pr.info, pr.experimental);
        const float target = 0.12f;
        float gainDb = m.rms > 1e-5f ? Decibels::gainToDecibels (target / m.rms) : 0.f;
        gainDb = jlimit (-18.f, 12.f, gainDb);
        auto p2 = freshProc(); apply (*p2, pr); setValue (*p2, "m_limon", "1"); setValue (*p2, "m_ceil", "-0.8");
        // level before the limiter: shift every lane's Level (the output gain sits after the limiter)
        for (int l = 0; l < kNumLanes; ++l)
        {
            const float cur = p2->apvts.getRawParameterValue (laneId (l, "level"))->load();
            setValue (*p2, laneId (l, "level"), String (jlimit (-60.f, 12.f, cur + gainDb)));
        }
        auto m2 = measure (*p2, pr.info, pr.experimental);
        const bool bad = m2.nans > 0 || m2.rms < 0.01f;
        if (bad) ++errors;
        std::printf ("%s %-24s %-16s rms %.3f -> %.3f (%+.1f dB) peak %.3f motion %.2f%s\n", bad ? "!!" : "  ", pr.info.name.toRawUTF8(), pr.info.category.toRawUTF8(), m.rms, m2.rms, gainDb, m2.peak, m2.motion,
                     pr.experimental && m2.motion < 0.08f ? "  (static?)" : "");
        auto state = p2->apvts.copyState();
        // keep only what differs from the initial patch (the loader merges onto INIT)
        auto init = p2->getInitState();
        for (int i = state.getNumChildren(); --i >= 0;)
        {
            auto c = state.getChild (i);
            if (! c.hasType ("PARAM")) continue;
            auto d = init.getChildWithProperty ("id", c.getProperty ("id"));
            if (d.isValid() && std::abs ((double) d.getProperty ("value") - (double) c.getProperty ("value")) < 1e-6) state.removeChild (i, nullptr);
        }
        bank.appendChild (PresetManager::wrap (state, pr.info), nullptr);
    }
    std::unique_ptr<XmlElement> xml (bank.createXml());
    xml->writeTo (out);
    std::printf ("%d presets written to %s, %d problems\n", bank.getNumChildren(), out.getFullPathName().toRawUTF8(), errors);
    return errors;
}
