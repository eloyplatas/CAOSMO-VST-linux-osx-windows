#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace cz
{
namespace
{
    const int kVoiceCounts[] = { 1, 4, 8, 16, 32, 64 };
    const float kDriftP[] = { 0.f, 3.f, 8.f, 20.f };
    const float kDriftF[] = { 0.f, 1.f, 3.f, 7.f };
    const float kDriftL[] = { 0.f, 0.5f, 1.5f, 3.f };
    const float kVsSeq[8] = { -1.f, 1.f, -0.5f, 0.5f, -0.75f, 0.75f, -0.25f, 0.25f };
    const int kDefaultShapes[kNumVenv] = { 0, 2, 22, 90 };

    bool nativeBipolar (int s)
    {
        return (s >= S_Lfo1 && s < S_Lfo1 + kNumLfo) || (s >= S_Attr1 && s < S_Attr1 + kNumAttr) || s == S_Key || s == S_PitchBend || s == S_NoteRandom || s == S_MpeBend;
    }
    float quantise (float semis, int mode)
    {
        if (mode <= 0) return semis;
        const float r = std::round (semis);
        if (mode == 1 || mode == 5) return r;
        static const int maj[] = { 0, 2, 4, 5, 7, 9, 11 }, minr[] = { 0, 2, 3, 5, 7, 8, 10 }, pen[] = { 0, 2, 4, 7, 9 };
        const int* sc = mode == 2 ? maj : (mode == 3 ? minr : pen);
        const int ns = mode == 4 ? 5 : 7;
        const int ri = (int) r; const int oct = (int) std::floor (ri / 12.0); const int deg = ri - oct * 12;
        int best = sc[0], bd = 99;
        for (int i = 0; i < ns; ++i) { const int d = std::abs (deg - sc[i]); if (d < bd) { bd = d; best = sc[i]; } }
        if (12 - deg < bd) best = 12;
        return (float) (oct * 12 + best);
    }
    inline bool isLanePitchDest (int d) { return d >= kDestLaneBase && d < kDestLfoRate && ((d - kDestLaneBase) % LD_Count) == LD_Pitch; }
    String packEq (const float* v) { String s; for (int i = 0; i < kEqN; ++i) s << String (v[i], 4) << (i < kEqN - 1 ? "," : ""); return s; }
}

//==============================================================================
CaosmoProcessor::CaosmoProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", AudioChannelSet::stereo(), false)),
      apvts (*this, &undo, "CAOSMO", createLayout())
{
    bindRefs();
    for (int i = 0; i < kMaxVoices; ++i) voices.push_back (std::make_unique<Voice>());
    for (auto& b : fx) for (auto& s : b) s = std::make_unique<FxSlot>();
    ensureStateChildren();
    for (int l = 0; l < kNumLanes; ++l) apvts.addParameterListener (laneId (l, "eqsnap"), this);
    apvts.state.addListener (this);
    syncShapesFromTree(); syncEqFromTree();
    initState = apvts.copyState();
    undo.clearUndoHistory();
}

CaosmoProcessor::~CaosmoProcessor()
{
    cancelPendingUpdate();
    apvts.state.removeListener (this);
    for (int l = 0; l < kNumLanes; ++l) apvts.removeParameterListener (laneId (l, "eqsnap"), this);
}

void CaosmoProcessor::bindRefs()
{
    outGain.bind (apvts, "out_gain"); voicesP.bind (apvts, "voices"); legato.bind (apvts, "legato");
    glide.bind (apvts, "glide"); glideMode.bind (apvts, "glide_mode"); tune.bind (apvts, "tune"); pbRange.bind (apvts, "pb_range");
    pbNote.bind (apvts, "pb_note"); mpeMode.bind (apvts, "mpe"); hqP.bind (apvts, "hq");
    loopOn.bind (apvts, "lfo_loop"); loopLen.bind (apvts, "lfo_looplen"); loopSync.bind (apvts, "lfo_loopsync"); loopDiv.bind (apvts, "lfo_loopdiv");
    for (int i = 0; i < kNumMacros; ++i) macro[i].bind (apvts, "macro" + String (i + 1));
    static const char* eqk[kEqN] = { "hpf", "hpfs", "lsf", "lsg", "mf", "mg", "mq", "hsf", "hsg", "lpf", "lpfs" };

    const auto& specs = engineSpecs();
    for (int l = 0; l < kNumLanes; ++l)
    {
        auto& R = lr[l];
        auto B = [&] (PRef& r, const char* k) { r.bind (apvts, laneId (l, k)); };
        B (R.engine, "engine"); B (R.power, "power"); B (R.chan, "chan"); B (R.driftp, "driftp"); B (R.driftf, "driftf"); B (R.driftl, "driftl");
        B (R.pitch, "pitch"); B (R.fine, "fine"); B (R.sub1oct, "sub1oct"); B (R.sub1lev, "sub1lev"); B (R.sub2oct, "sub2oct"); B (R.sub2lev, "sub2lev");
        B (R.ftype, "ftype"); B (R.fslope, "fslope"); B (R.cut, "cut"); B (R.res, "res");
        B (R.emode, "emode"); B (R.esub, "esub"); B (R.edel, "edel"); B (R.eatt, "eatt"); B (R.edec, "edec"); B (R.esus, "esus"); B (R.erel, "erel");
        B (R.ecatt, "ecatt"); B (R.ecdec, "ecdec"); B (R.ecrel, "ecrel");
        B (R.vrf, "vrf"); B (R.vff, "vff"); B (R.vrs, "vrs"); B (R.vfs, "vfs"); B (R.vmix, "vmix"); B (R.vmem, "vmem"); B (R.vshape, "vshape"); B (R.vdrift, "vdrift");
        B (R.elrate, "elrate"); B (R.elshape, "elshape"); B (R.evsrc, "evsrc"); B (R.evtime, "evtime"); B (R.lpg, "lpg");
        B (R.eqon, "eqon"); for (int k = 0; k < kEqN; ++k) B (R.eq[k], eqk[k]); B (R.eqsnap, "eqsnap"); B (R.eqmorph, "eqmorph");
        B (R.mute, "mute"); B (R.solo, "solo"); B (R.level, "level"); B (R.pan, "pan"); B (R.width, "width"); B (R.vspread, "vspread");
        B (R.mono, "mono"); B (R.drive, "drive"); B (R.dflav, "dflav");
        B (R.fx[0], "fx1"); B (R.fx[1], "fx2"); B (R.fx[2], "fx3");
        for (int e = 1; e <= kNumEngines; ++e)
        {
            const auto& sp = specs[(size_t) e];
            R.np[e] = (int) sp.p.size(); R.nc[e] = (int) sp.c.size();
            for (int k = 0; k < R.np[e]; ++k) R.ep[e][k].bind (apvts, engId (l, e, sp.p[(size_t) k].key));
            for (int k = 0; k < R.nc[e]; ++k) R.ec[e][k].bind (apvts, engId (l, e, sp.c[(size_t) k].key));
        }
    }
    for (int i = 0; i < kNumLfo; ++i)
    {
        auto& L = lfoR[i];
        auto B = [&] (PRef& r, const char* k) { r.bind (apvts, lfoId (i, k)); };
        B (L.on, "on"); B (L.wave, "wave"); B (L.rate, "rate"); B (L.sync, "sync"); B (L.div, "div"); B (L.level, "level"); B (L.phase, "phase");
        B (L.slew, "slew"); B (L.skew, "skew"); B (L.afade, "afade"); B (L.rfade, "rfade"); B (L.mode, "mode"); B (L.retrig, "retrig");
        if ((i & 1) == 0) { B (L.link, "link"); B (L.linkrel, "linkrel"); }
    }
    for (int i = 0; i < kNumVenv; ++i)
    {
        auto& V = venvR[i];
        auto B = [&] (PRef& r, const char* k) { r.bind (apvts, venvId (i, k)); };
        B (V.on, "on"); B (V.time, "time"); B (V.sync, "sync"); B (V.div, "div"); B (V.level, "level"); B (V.za, "za"); B (V.zd, "zd"); B (V.zs, "zs"); B (V.zr, "zr");
        B (V.pa, "pa"); B (V.pd, "pd"); B (V.ps, "ps"); B (V.mode, "mode"); B (V.voicing, "voicing"); B (V.retrig, "retrig");
    }
    for (int i = 0; i < kNumAttr; ++i)
    {
        auto& A = attrR[i];
        auto B = [&] (PRef& r, const char* k) { r.bind (apvts, attrId (i, k)); };
        B (A.on, "on"); B (A.type, "type"); B (A.rate, "rate"); B (A.sync, "sync"); B (A.div, "div"); B (A.p1, "p1"); B (A.p2, "p2");
        B (A.smooth, "smooth"); B (A.level, "level"); B (A.out, "out"); B (A.mode, "mode"); B (A.retrig, "retrig");
    }
    for (int i = 0; i < kNumSlots; ++i)
    {
        auto& S = slotR[i];
        auto B = [&] (PRef& r, const char* k) { r.bind (apvts, slotId (i, k)); };
        B (S.src, "src"); B (S.dst, "dst"); B (S.amt, "amt"); B (S.pol, "pol"); B (S.curve, "curve"); B (S.lag, "lag");
        B (S.fin, "fin"); B (S.fout, "fout"); B (S.jit, "jit"); B (S.quant, "quant"); B (S.cc, "cc");
    }
    for (int b = 0; b < kNumBuses; ++b)
    {
        busLevel[b].bind (apvts, busId (b, "level"));
        for (int s = 0; s < kSlotsPerBus; ++s)
        {
            auto& F = fxR[b][s];
            F.type.bind (apvts, fxId (b, s, "type")); F.on.bind (apvts, fxId (b, s, "on")); F.opt.bind (apvts, fxId (b, s, "opt"));
            for (int k = 0; k < kFxParams; ++k) F.p[k].bind (apvts, fxId (b, s, ("p" + String (k + 1)).toRawUTF8()));
        }
    }
    auto M = [&] (PRef& r, const char* id) { r.bind (apvts, id); };
    M (mr.bypass, "m_bypass"); M (mr.eqon, "m_eqon"); for (int k = 0; k < kEqN; ++k) mr.eq[k].bind (apvts, String ("m_") + eqk[k]);
    M (mr.analyzer, "m_analyzer"); M (mr.saton, "m_saton"); M (mr.satdrive, "m_satdrive"); M (mr.satmix, "m_satmix"); M (mr.satflav, "m_satflav"); M (mr.satos, "m_satos");
    M (mr.compon, "m_compon"); M (mr.thr, "m_thr"); M (mr.ratio, "m_ratio"); M (mr.att, "m_att"); M (mr.rel, "m_rel"); M (mr.knee, "m_knee");
    M (mr.cgain, "m_cgain"); M (mr.glue, "m_glue"); M (mr.schpf, "m_schpf"); M (mr.sc, "m_sc");
    {
        auto Bd = [&] (PRef& r, const char* id) { r.bind (apvts, id); };
        Bd (dr.on, "dl_on"); Bd (dr.modtype, "dl_modtype"); Bd (dr.modrate, "dl_modrate"); Bd (dr.moddepth, "dl_moddepth"); Bd (dr.modfb, "dl_modfb"); Bd (dr.modoff, "dl_modoff");
        Bd (dr.bass, "dl_bass"); Bd (dr.treble, "dl_treble"); Bd (dr.bassf, "dl_bassf"); Bd (dr.treblef, "dl_treblef");
        Bd (dr.srr, "dl_srr"); Bd (dr.crush, "dl_crush"); Bd (dr.sat, "dl_sat");
        Bd (dr.dlyamt, "dl_dlyamt"); Bd (dr.dlyrate, "dl_dlyrate"); Bd (dr.dlysync, "dl_dlysync"); Bd (dr.dlysynctype, "dl_dlysynctype"); Bd (dr.dlypp, "dl_dlypp"); Bd (dr.dlyanalog, "dl_dlyanalog");
        Bd (dr.rvbmodel, "dl_rvbmodel"); Bd (dr.rvbamt, "dl_rvbamt"); Bd (dr.rvbroom, "dl_rvbroom"); Bd (dr.rvbdamp, "dl_rvbdamp"); Bd (dr.rvbwidth, "dl_rvbwidth");
        Bd (dr.rvbhpf, "dl_rvbhpf"); Bd (dr.rvblpf, "dl_rvblpf"); Bd (dr.rvbpan, "dl_rvbpan");
        Bd (dr.cmpthr, "dl_cmpthr"); Bd (dr.cmpratio, "dl_cmpratio"); Bd (dr.cmpatt, "dl_cmpatt"); Bd (dr.cmprel, "dl_cmprel"); Bd (dr.cmpsc, "dl_cmpsc"); Bd (dr.cmpblend, "dl_cmpblend");
        Bd (dr.stutter, "dl_stutter"); Bd (dr.sturate, "dl_sturate"); Bd (dr.stuquant, "dl_stuquant"); Bd (dr.sturev, "dl_sturev"); Bd (dr.stupp, "dl_stupp"); Bd (dr.vol, "dl_vol");
    }
    M (mr.widthon, "m_widthon"); M (mr.ms, "m_ms"); M (mr.mono, "m_mono"); M (mr.limon, "m_limon"); M (mr.ceil, "m_ceil"); M (mr.lrel, "m_lrel"); M (mr.lrelmode, "m_lrelmode");
}

//==============================================================================
void CaosmoProcessor::ensureStateChildren()
{
    auto vs = apvts.state.getOrCreateChildWithName ("VENVSHAPES", nullptr);
    while (vs.getNumChildren() < kNumVenv)
    {
        const int i = vs.getNumChildren();
        ValueTree c ("SHAPE");
        c.setProperty ("pts", factoryShape (kDefaultShapes[i]).toString(), nullptr);
        c.setProperty ("name", factoryShapeName (kDefaultShapes[i]), nullptr);
        vs.appendChild (c, nullptr);
    }
    auto kt = apvts.state.getOrCreateChildWithName ("KEYTRACK", nullptr);
    if (! kt.hasProperty ("pts")) kt.setProperty ("pts", KeyCurve().toString(), nullptr);
    auto eq = apvts.state.getOrCreateChildWithName ("EQSNAPS", nullptr);
    while (eq.getNumChildren() < kNumLanes) { ValueTree c ("LANE"); c.setProperty ("used", 1, nullptr); eq.appendChild (c, nullptr); }
}

ValueTree CaosmoProcessor::venvTree (int i) { return apvts.state.getChildWithName ("VENVSHAPES").getChild (i); }
ValueTree CaosmoProcessor::keyTree() { return apvts.state.getChildWithName ("KEYTRACK"); }
VShape CaosmoProcessor::getShape (int i) { return VShape::fromString (venvTree (i).getProperty ("pts").toString()); }
void CaosmoProcessor::setShape (int i, const VShape& s, const String& name, bool undoable)
{
    auto t = venvTree (i); if (! t.isValid()) return;
    t.setProperty ("pts", s.toString(), undoable ? &undo : nullptr);
    if (name.isNotEmpty()) t.setProperty ("name", name, undoable ? &undo : nullptr);
    syncShapesFromTree();
}
KeyCurve CaosmoProcessor::getKeyCurve() { return KeyCurve::fromString (keyTree().getProperty ("pts").toString()); }
void CaosmoProcessor::setKeyCurve (const KeyCurve& k) { keyTree().setProperty ("pts", k.toString(), &undo); syncShapesFromTree(); }

void CaosmoProcessor::syncShapesFromTree()
{
    VShape s[kNumVenv]; for (int i = 0; i < kNumVenv; ++i) s[i] = getShape (i);
    const KeyCurve k = getKeyCurve();
    { const SpinLock::ScopedLockType sl (shapeLock); for (int i = 0; i < kNumVenv; ++i) pShapes[i] = s[i]; pKey = k; }
    shapesPending = true;
    shapesDirtyFromTree = false;
}

void CaosmoProcessor::syncEqFromTree()
{
    auto eq = apvts.state.getChildWithName ("EQSNAPS");
    for (int l = 0; l < kNumLanes; ++l)
    {
        auto c = eq.getChild (l);
        eqSel[l] = lr[l].eqsnap.geti();
        int used = c.isValid() ? (int) c.getProperty ("used", 1) : 1;
        for (int s = 0; s < 4; ++s)
        {
            auto toks = StringArray::fromTokens (c.isValid() ? c.getProperty ("s" + String (s)).toString() : String(), ",", "");
            for (int k = 0; k < kEqN; ++k)
                eqSnap[l][s][k].store (toks.size() == kEqN ? toks[k].getFloatValue() : lr[l].eq[k].p->convertFrom0to1 (lr[l].eq[k].p->getDefaultValue()));
        }
        eqUsed[l].store (used | (1 << eqSel[l]));
    }
    eqReload = false;
}

void CaosmoProcessor::writeEqToTree (int l)
{
    auto c = apvts.state.getChildWithName ("EQSNAPS").getChild (l);
    if (! c.isValid()) return;
    c.setProperty ("used", eqUsed[l].load(), nullptr);
    for (int s = 0; s < 4; ++s) { float v[kEqN]; for (int k = 0; k < kEqN; ++k) v[k] = eqSnap[l][s][k].load(); c.setProperty ("s" + String (s), packEq (v), nullptr); }
}

void CaosmoProcessor::eqResetSnapshots (int l)
{
    eqSel[l] = 0;
    if (auto* p = lr[l].eqsnap.p) p->setValueNotifyingHost (0.f);
    for (int k = 0; k < kEqN; ++k) { auto* p = lr[l].eq[k].p; p->setValueNotifyingHost (p->getDefaultValue()); }
    if (auto* p = lr[l].eqmorph.p) p->setValueNotifyingHost (0.f);
    for (int s = 0; s < 4; ++s) for (int k = 0; k < kEqN; ++k) eqSnap[l][s][k].store (lr[l].eq[k].get());
    eqUsed[l].store (1);
    writeEqToTree (l);
}

void CaosmoProcessor::parameterChanged (const String& id, float)
{
    if (id.endsWith ("eqsnap")) { eqSelPending = true; triggerAsyncUpdate(); }
}

void CaosmoProcessor::handleAsyncUpdate()
{
    if (shapesDirtyFromTree) { ensureStateChildren(); syncShapesFromTree(); }
    if (eqReload) syncEqFromTree();
    if (eqSelPending.exchange (false))
    {
        for (int l = 0; l < kNumLanes; ++l)
        {
            const int ns = lr[l].eqsnap.geti();
            if (ns == eqSel[l]) continue;
            const int old = eqSel[l];
            for (int k = 0; k < kEqN; ++k) eqSnap[l][old][k].store (lr[l].eq[k].get());
            int used = eqUsed[l].load() | (1 << old);
            eqSel[l] = ns;
            if (used & (1 << ns))
            {
                for (int k = 0; k < kEqN; ++k) { auto* p = lr[l].eq[k].p; p->setValueNotifyingHost (p->convertTo0to1 (eqSnap[l][ns][k].load())); }
            }
            else
            {
                used |= (1 << ns);
                for (int k = 0; k < kEqN; ++k) eqSnap[l][ns][k].store (lr[l].eq[k].get());
            }
            eqUsed[l].store (used);
            // place the morph on the selected snapshot so what you hear is what you edit
            int cnt = 0, idx = 0; for (int s = 0; s < 4; ++s) if (used & (1 << s)) { if (s == ns) idx = cnt; ++cnt; }
            if (cnt > 1) lr[l].eqmorph.p->setValueNotifyingHost ((float) idx / (float) (cnt - 1));
            writeEqToTree (l);
        }
    }
}

void CaosmoProcessor::eqValues (int l, float morphMod, float* out) const
{
    const int sel = lr[l].eqsnap.geti();
    const int used = eqUsed[l].load() | (1 << sel);
    float live[kEqN]; for (int k = 0; k < kEqN; ++k) live[k] = lr[l].eq[k].get();
    int list[4], cnt = 0; for (int s = 0; s < 4; ++s) if (used & (1 << s)) list[cnt++] = s;
    if (cnt <= 1) { for (int k = 0; k < kEqN; ++k) out[k] = live[k]; return; }
    const float m = clampf (lr[l].eqmorph.get() + morphMod, 0.f, 1.f) * (float) (cnt - 1);
    const int i0 = std::min ((int) m, cnt - 1), i1 = std::min (i0 + 1, cnt - 1); const float t = m - (float) i0;
    auto val = [&] (int s, int k) { return s == sel ? live[k] : eqSnap[l][s][k].load (std::memory_order_relaxed); };
    for (int k = 0; k < kEqN; ++k)
    {
        const float a = val (list[i0], k), b = val (list[i1], k);
        if (k == 1 || k == 10) out[k] = t < 0.5f ? a : b;
        else if (k == 0 || k == 2 || k == 4 || k == 7 || k == 9) out[k] = std::exp (std::log (std::max (a, 1.f)) * (1.f - t) + std::log (std::max (b, 1.f)) * t);
        else out[k] = a + (b - a) * t;
    }
}

bool CaosmoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::stereo()) return false;
    if (layouts.inputBuses.size() > 0)
    {
        const auto in = layouts.getChannelSet (true, 0);
        if (! in.isDisabled() && in != AudioChannelSet::mono() && in != AudioChannelSet::stereo()) return false;
    }
    return true;
}

void CaosmoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; maxBlock = std::max (samplesPerBlock, kBlock);
    const float s = (float) sr;
    for (auto& vp : voices)
    {
        auto& v = *vp; v.active = false;
        for (auto& lv : v.lane)
        {
            lv.eng.prepare (s); lv.sub.prepare (s); lv.filt.prepare (s); lv.env.sr = s; lv.env.kill();
            lv.dcL.prepare (s); lv.dcR.prepare (s); lv.active = false;
        }
    }
    for (int l = 0; l < kNumLanes; ++l) { laneL[l].assign (kBlock, 0.f); laneR[l].assign (kBlock, 0.f); bus[l] = LaneBus(); }
    for (int b = 0; b < kNumBuses; ++b) { sendL[b].assign (kBlock, 0.f); sendR[b].assign (kBlock, 0.f); }
    scBufL.assign ((size_t) maxBlock, 0.f); scBufR.assign ((size_t) maxBlock, 0.f);
    for (auto& b : fx) for (auto& f : b) f->prepare (s);
    master.prepare (s, kBlock);
    deluge.prepare (sr);
    for (int i = 0; i < kNumLfo; ++i) gLfo[i].reset (0.f, (uint32_t) (i + 1) * 977u);
    for (auto& g : gVenv) g.kill();
    for (int i = 0; i < kNumAttr; ++i) gAttr[i].reset ((uint32_t) i * 13u + 5u);
    heldCount = 0; for (auto& h : held) h = 0; lastNote = -1; loopPos = 0;
    setLatencySamples (master.latency()); // constant: the look-ahead delay runs whether or not the limiter is on
    if (shapesDirtyFromTree) syncShapesFromTree();
}

//==============================================================================
int CaosmoProcessor::laneMaskFor (int chan) const
{
    int m = 0;
    const bool mpe = isMpe();
    for (int l = 0; l < kNumLanes; ++l)
    {
        if (lr[l].engine.geti() == 0 || ! lr[l].power.on()) continue;
        const int ch = lr[l].chan.geti();
        if (mpe || ch == 0 || ch == chan) m |= (1 << l);
    }
    return m;
}

void CaosmoProcessor::startVoice (Voice& v, int note, float vel, int chan, bool fingered)
{
    const int mask = laneMaskFor (chan);
    const bool wasActive = v.active;
    v.active = true; v.gate = true; v.sustained = false;
    v.note = note; v.chan = chan; v.vel = vel; v.relVel = 0.f; v.age = ++ageCounter;
    v.pressure = isMpe() ? chanPress[chan] : 0.f; v.slide = isMpe() ? chanSlide[chan] : slideGlobal; v.bend = isMpe() ? chanBend[chan] : 0.f;
    const bool doGlide = glide.get() > 0.5f && lastNote >= 0 && (glideMode.geti() == 1 || fingered);
    v.targetNote = (float) note;
    if (! doGlide) v.curNote = (float) note;
    else if (! (voicesP.geti() == 0 && wasActive)) v.curNote = (float) lastNote;
    v.noteRand = hashF ((uint32_t) ageCounter * 2654435761u) * 2.f - 1.f;
    v.vsIdx = vspreadCounter++;
    v.rng.seed ((uint32_t) ageCounter * 7919u + (uint32_t) note);
    Rng& r = v.rng;

    const auto& specs = engineSpecs();
    for (int l = 0; l < kNumLanes; ++l)
    {
        auto& lv = v.lane[l]; auto& R = lr[l];
        if (! (mask & (1 << l))) { lv.active = false; lv.env.kill(); continue; }
        const int type = R.engine.geti();
        const auto& sp = specs[(size_t) type];
        const bool freeMode = sp.noteChoice >= 0 && R.ec[type][sp.noteChoice].geti() == 1;
        const bool fresh = ! lv.active;
        lv.active = true; lv.type = type;
        const uint32_t seed = hash32 ((uint32_t) note * 977u + (uint32_t) l * 131u + 1u);
        lv.eng.noteOn (type, seed, freeMode);
        lv.sub.noteOn (type, seed ^ 0xA5A5u, freeMode);
        if (fresh) { lv.filt.reset(); lv.dcL.reset(); lv.dcR.reset(); lv.env.kill(); lv.levelSm = 0.f; }
        lv.driftP = kDriftP[R.driftp.geti()] * r.bi();
        lv.driftF = kDriftF[R.driftf.geti()] * r.bi();
        lv.driftL = dbToGain (kDriftL[R.driftl.geti()] * r.bi());
        lv.env.mode = R.emode.geti(); lv.env.oneShot = R.esub.geti() == 1;
        lv.env.del = R.edel.get() * 0.001f; lv.env.vdrift = R.vdrift.get();
        lv.env.vsh = &aShapes[jlimit (0, kNumVenv - 1, R.evsrc.geti())];
        const int es = R.esub.geti(); lv.env.vmode = es == 0 ? 1 : (es == 1 ? 0 : 2);
        lv.env.noteOn (r.bi());
    }
    for (int i = 0; i < kNumLfo; ++i)
    {
        if (lfoR[i].mode.geti() == 1) { v.lfo[i].reset (lfoR[i].phase.get(), hash32 ((uint32_t) ageCounter + (uint32_t) i * 31u)); v.lfo[i].fade = lfoR[i].afade.get() < 0.5f ? 1.f : 0.f; }
        else if (lfoR[i].retrig.on()) gLfo[i].reset (lfoR[i].phase.get(), (uint32_t) (i + 1) * 977u);
    }
    for (int i = 0; i < kNumVenv; ++i) if (venvR[i].voicing.geti() == 1) v.venv[i].trigger();
    for (int i = 0; i < kNumAttr; ++i)
    {
        if (attrR[i].mode.geti() == 1) v.attr[i].reset (hash32 ((uint32_t) note * 31u + (uint32_t) i + (uint32_t) ageCounter));
        else if (attrR[i].retrig.on()) gAttr[i].reset ((uint32_t) i * 13u + 5u);
    }
    for (int s = 0; s < kNumSlots; ++s) { v.slotFade[s] = slotR[s].fin.get() < 0.5f ? 1.f : 0.f; v.slotJit[s] = 0; }
    v.lagInit = false;
    lastNote = note;
}

void CaosmoProcessor::noteOn (int note, float vel, int chan)
{
    const bool wasHeld = heldCount > 0;
    if (! held[note]) { held[note] = 1; heldStack[heldCount++] = note; }
    if (! wasHeld) vspreadCounter = 0;
    for (int i = 0; i < kNumVenv; ++i) if (venvR[i].voicing.geti() == 0 && (! wasHeld || venvR[i].retrig.on())) gVenv[i].trigger();
    if (laneMaskFor (chan) == 0) { lastNote = note; return; }
    const int maxV = kVoiceCounts[jlimit (0, 5, voicesP.geti())];

    if (maxV == 1)
    {
        Voice& v = *voices[0];
        if (legato.on() && v.active && v.gate)
        {
            v.note = note; v.targetNote = (float) note; v.chan = chan;
            if (glide.get() <= 0.5f) v.curNote = (float) note;
            lastNote = note; return;
        }
        startVoice (v, note, vel, chan, wasHeld);
        return;
    }
    Voice* pick = nullptr;
    for (int i = 0; i < maxV; ++i) if (! voices[(size_t) i]->active) { pick = voices[(size_t) i].get(); break; }
    if (! pick) for (int i = 0; i < maxV; ++i) { auto* v = voices[(size_t) i].get(); if (! v->gate && (! pick || v->age < pick->age)) pick = v; }
    if (! pick) for (int i = 0; i < maxV; ++i) { auto* v = voices[(size_t) i].get(); if (! pick || v->age < pick->age) pick = v; }
    startVoice (*pick, note, vel, chan, wasHeld);
}

void CaosmoProcessor::releaseVoice (Voice& v)
{
    v.gate = false; v.sustained = false;
    for (auto& lv : v.lane) if (lv.active) lv.env.noteOff();
    for (auto& p : v.venv) p.release();
}

void CaosmoProcessor::noteOff (int note, float relVel, int chan)
{
    if (held[note])
    {
        held[note] = 0;
        int j = 0; for (int i = 0; i < heldCount; ++i) if (heldStack[i] != note) heldStack[j++] = heldStack[i];
        heldCount = j;
    }
    if (heldCount == 0 && ! sustainDown) for (auto& g : gVenv) g.release();
    if (voicesP.geti() == 0)
    {
        Voice& v = *voices[0];
        if (! (v.active && v.gate && v.note == note)) return;
        v.relVel = relVel;
        bool channelFilter = false; for (auto& R : lr) if (R.chan.geti() != 0) channelFilter = true;
        if (heldCount > 0 && ! channelFilter && ! isMpe())
        {
            const int prev = heldStack[heldCount - 1];
            if (legato.on()) { v.note = prev; v.targetNote = (float) prev; if (glide.get() <= 0.5f) v.curNote = (float) prev; lastNote = prev; }
            else startVoice (v, prev, v.vel, v.chan, true);
            return;
        }
        if (sustainDown) v.sustained = true; else releaseVoice (v);
        return;
    }
    for (auto& vp : voices)
    {
        auto& v = *vp;
        if (v.active && v.gate && ! v.sustained && v.note == note && v.chan == chan)
        {
            v.relVel = relVel;
            if (sustainDown) v.sustained = true; else releaseVoice (v);
        }
    }
}

void CaosmoProcessor::handleMidi (const MidiMessage& m)
{
    const int ch = jlimit (1, 16, m.getChannel());
    const bool member = isMpe() && ! isMasterChan (ch);
    if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity(), ch);
    else if (m.isNoteOff()) noteOff (m.getNoteNumber(), m.getFloatVelocity(), ch);
    else if (m.isPitchWheel())
    {
        const float v = (float) (m.getPitchWheelValue() - 8192) / 8192.f;
        if (member) chanBend[ch] = v; else pitchBend = v;
    }
    else if (m.isChannelPressure())
    {
        const float v = (float) m.getChannelPressureValue() / 127.f;
        if (member) chanPress[ch] = v; else aftertouch = v;
    }
    else if (m.isAftertouch())
    {
        const float v = (float) m.getAfterTouchValue() / 127.f;
        for (auto& vp : voices) if (vp->active && vp->note == m.getNoteNumber()) vp->pressure = v;
    }
    else if (m.isController())
    {
        const int c = m.getControllerNumber(); const float val = (float) m.getControllerValue() / 127.f;
        const int ls = learnSlot.load();
        if (ls >= 0 && c != 64 && c < 120) { learnedCC = c; learnedSlot = ls; learnSlot = -1; }
        cc[c & 127] = val;
        if (c == 74) { if (member) chanSlide[ch] = val; else slideGlobal = val; }
        if (member && c != 64) return;
        if (c == 1) modWheel = val;
        else if (c == 11) expression = val;
        else if (c == 64)
        {
            const bool down = val >= 0.5f;
            if (sustainDown && ! down)
            {
                for (auto& vp : voices) if (vp->active && vp->sustained) releaseVoice (*vp);
                if (heldCount == 0) for (auto& g : gVenv) g.release();
            }
            sustainDown = down;
        }
        else if (c == 120) { for (auto& vp : voices) { vp->active = false; for (auto& lv : vp->lane) { lv.active = false; lv.env.kill(); } } heldCount = 0; for (auto& h : held) h = 0; }
        else if (c == 123) { for (auto& vp : voices) if (vp->active) releaseVoice (*vp); heldCount = 0; for (auto& h : held) h = 0; }
    }
}

//==============================================================================
void CaosmoProcessor::computeSources (const Voice* v, float* src) const
{
    src[S_None] = 0.f;
    for (int i = 0; i < kNumLfo; ++i) src[S_Lfo1 + i] = lfoR[i].mode.geti() == 1 ? (v ? v->lfoVal[i] : 0.f) : gLfoVal[i];
    for (int i = 0; i < kNumVenv; ++i) src[S_Venv1 + i] = venvR[i].voicing.geti() == 1 ? (v ? v->venvVal[i] : 0.f) : gVenvVal[i];
    for (int i = 0; i < kNumAttr; ++i) src[S_Attr1 + i] = attrR[i].mode.geti() == 1 ? (v ? v->attrVal[i] : 0.f) : gAttrVal[i];
    for (int l = 0; l < kNumLanes; ++l) src[S_Env1 + l] = v ? v->lane[l].envOut : 0.f;
    src[S_Velocity] = v ? v->vel : 0.f;
    src[S_RelVelocity] = v ? v->relVel : 0.f;
    src[S_Key] = v ? aKey.eval (clampf (v->curNote / 127.f, 0.f, 1.f)) : 0.f;
    src[S_ModWheel] = modWheel; src[S_Aftertouch] = aftertouch; src[S_PitchBend] = pitchBend;
    src[S_Expression] = expression; src[S_Sustain] = sustainDown ? 1.f : 0.f; src[S_CC] = 0.f;
    src[S_MpePressure] = v ? v->pressure : 0.f; src[S_MpeSlide] = v ? v->slide : slideGlobal; src[S_MpeBend] = v ? v->bend : 0.f;
    for (int i = 0; i < kNumMacros; ++i) src[S_Macro1 + i] = macro[i].get();
    src[S_NoteRandom] = v ? v->noteRand : 0.f;
}

void CaosmoProcessor::accumulateMods (const float* src, float* mod, Voice* v, int n, bool global)
{
    const float s = (float) sr;
    float* lag = v ? v->slotLag : gSlotLag;
    bool& lagInit = v ? v->lagInit : gLagInit;
    for (int i = 0; i < kNumSlots; ++i)
    {
        const auto& S = slotR[i];
        const int si = S.src.geti(), di = S.dst.geti();
        if (si <= 0 || di <= 0 || si >= S_Count || di >= kNumDests) continue;
        const bool polySrc = isPolySourceKind (si)
                             || (si >= S_Lfo1 && si < S_Lfo1 + kNumLfo && lfoR[si - S_Lfo1].mode.geti() == 1)
                             || (si >= S_Venv1 && si < S_Venv1 + kNumVenv && venvR[si - S_Venv1].voicing.geti() == 1)
                             || (si >= S_Attr1 && si < S_Attr1 + kNumAttr && attrR[si - S_Attr1].mode.geti() == 1);
        if (global && polySrc) continue;
        const float amt = S.amt.get();
        if (amt == 0.f) continue;
        float x = si == S_CC ? cc[jlimit (0, 127, S.cc.geti())] : src[si];
        const bool bip = nativeBipolar (si);
        const int pol = S.pol.geti();
        if (pol == 1 && bip) x = 0.5f * (x + 1.f); else if (pol == 2 && ! bip) x = 2.f * x - 1.f;
        const float sg = x < 0 ? -1.f : 1.f, ax = std::min (1.f, std::fabs (x));
        switch (S.curve.geti())
        {
            case 1: x = sg * ax * ax; break;
            case 2: x = sg * std::sqrt (ax); break;
            case 3: x = sg * ax * ax * (3.f - 2.f * ax); break;
            default: break;
        }
        float val = amt * x;
        const float jit = S.jit.get();
        if (v)
        {
            float& f = v->slotFade[i];
            if (v->gate) { const float fi = S.fin.get(); f = fi < 0.5f ? 1.f : std::min (1.f, f + (float) n / (fi * 0.001f * s)); }
            else { const float fo = S.fout.get(); if (fo >= 0.5f) f = std::max (0.f, f - (float) n / (fo * 0.001f * s)); }
            val *= f;
            if (jit > 0.f) { float& j = v->slotJit[i]; j += (v->rng.bi() - j) * 0.15f; val += jit * 0.3f * j; }
        }
        else if (jit > 0.f) { float& j = gSlotJit[i]; j += (gRng.bi() - j) * 0.15f; val += jit * 0.3f * j; }
        const float lagMs = S.lag.get();
        if (lagMs > 0.5f)
        {
            const float c = std::exp (-(float) n / (lagMs * 0.001f * s));
            if (! lagInit) lag[i] = val;
            lag[i] = val + c * (lag[i] - val); val = lag[i];
        }
        else lag[i] = val;
        if (isLanePitchDest (di)) { const int q = S.quant.geti(); if (q > 0) val = quantise (val * 72.f, q) / 72.f; }
        mod[di] += val;
    }
    lagInit = true;
}

float CaosmoProcessor::lfoCyclesPerSample (int i, float rateMod) const
{
    const auto& L = lfoR[i];
    if (L.sync.geti() == 0) return L.rate.mod (rateMod) / (float) sr;
    const int d = jlimit (0, kNumLfoDivs - 1, L.div.geti());
    return (float) (bpm / 60.0) / kLfoDivBeats[d] * std::exp2 (rateMod * 6.f) / (float) sr;
}

float CaosmoProcessor::modRateHz (const PRef& rate, const PRef& sync, const PRef& div, float rateMod) const
{
    if (sync.geti() == 0) return rate.mod (rateMod);
    const int d = jlimit (0, kNumLfoDivs - 1, div.geti());
    return (float) (bpm / 60.0) / kLfoDivBeats[d] * std::exp2 (rateMod * 6.f);
}

void CaosmoProcessor::advanceLfos (LfoState* st, float* out, bool poly, bool gate, int n)
{
    const float s = (float) sr;
    for (int i = 0; i < kNumLfo; ++i)
    {
        const auto& L = lfoR[i];
        const bool isPoly = L.mode.geti() == 1;
        if (! L.on.on() || isPoly != poly) { out[i] = 0.f; continue; }
        const auto& lk = link[i];
        const bool linked = lk.master >= 0 && lfoR[lk.master].mode.geti() == L.mode.geti() && lfoR[lk.master].on.on();
        if (linked)
        {
            const double mt = st[lk.master].total;
            switch (lk.rel)
            {
                case 0: st[i].total = mt; break;
                case 1: st[i].total = mt + (double) lk.k / (double) lk.n; break;
                case 2: st[i].total = mt / (double) (lk.k + 1); break;
                case 3: { const double fl = std::floor (mt), ph = mt - fl; st[i].total = fl + ((lk.k & 1) ? 1.0 - ph : ph); } break;
                default: st[i].total = mt; break;
            }
        }
        else if (! poly && L.sync.geti() == 1 && playing)
        {
            const int d = jlimit (0, kNumLfoDivs - 1, L.div.geti());
            st[i].total = ppq / (double) kLfoDivBeats[d] + L.phase.get();
        }
        else st[i].total += (double) lfoCyclesPerSample (i, globalMod[kDestLfoRate + i]) * n;
        float v = LfoEval::value (st[i].total, L.wave.geti(), L.skew.get(), st[i].seed);
        if (linked && lk.rel == 4 && (lk.k & 1)) v = -v;
        const float sl = L.slew.get();
        const float slewK = sl <= 0.f ? 0.f : std::exp (-(float) n / (s * sl * sl * 0.5f));
        st[i].sl = v + slewK * (st[i].sl - v);
        float& f = st[i].fade;
        if (poly)
        {
            if (gate) { const float a = L.afade.get(); f = a < 0.5f ? 1.f : std::min (1.f, f + (float) n / (a * 0.001f * s)); }
            else { const float r = L.rfade.get(); if (r >= 0.5f) f = std::max (0.f, f - (float) n / (r * 0.001f * s)); }
        }
        else f = 1.f;
        out[i] = st[i].sl * L.level.mod (globalMod[kDestLfoLevel + i]) * f;
    }
}

//==============================================================================
void CaosmoProcessor::renderChunk (float* outL, float* outR, const float* scL, const float* scR, int n)
{
    const float s = (float) sr;
    const auto& specs = engineSpecs();
    const bool hq = hqP.on();

    // ---- LFO links ----
    for (auto& l : link) l = Link();
    for (int m : { 0, 2, 4, 6 })
    {
        if (link[m].master >= 0) continue;
        const int L = lfoR[m].link.geti();
        const int span = L == 0 ? 1 : (L == 1 ? 2 : (L == 2 ? 4 : 8));
        for (int j = 1; j < span && m + j < kNumLfo; ++j) link[m + j] = { m, j, span, lfoR[m].linkrel.geti() };
    }

    // ---- global modulators ----
    advanceLfos (gLfo, gLfoVal, false, heldCount > 0, n);
    if (loopOn.on())
    {
        const double len = loopSync.geti() == 1 ? (double) kLoopDivBeats[jlimit (0, kNumLoopDivs - 1, loopDiv.geti())] * 60.0 / bpm * sr
                                                 : (double) loopLen.mod (globalMod[kDestLoopLen]) * sr;
        loopPos += n;
        if (loopPos >= len)
        {
            loopPos = std::fmod (loopPos, std::max (len, 1.0));
            for (int i = 0; i < kNumLfo; ++i)
                if (LfoEval::isRandom (lfoR[i].wave.geti()))
                {
                    gLfo[i].total = gLfo[i].start;
                    for (auto& vp : voices) if (vp->active) vp->lfo[i].total = vp->lfo[i].start;
                }
        }
    }
    auto venvInc = [&] (int i, float tm)
    {
        const auto& V = venvR[i];
        float T;
        if (V.sync.geti() == 0) T = V.time.mod (tm);
        else T = kLfoDivBeats[jlimit (0, kNumLfoDivs - 1, V.div.geti())] * 60.f / (float) bpm * std::exp2 (-tm * 4.f);
        return (float) n / (std::max (T, 0.005f) * s);
    };
    auto venvTick = [&] (VenvPlayer& p, int i, float tm, float lm)
    {
        const auto& V = venvR[i];
        const float zl[4] = { V.za.get(), V.zd.get(), V.zs.get(), V.zr.get() };
        return p.tick (aShapes[i], venvInc (i, tm), V.mode.geti(), V.pa.get(), V.pd.get(), V.ps.get(), zl) * V.level.mod (lm);
    };
    for (int i = 0; i < kNumVenv; ++i)
    {
        if (! venvR[i].on.on() || venvR[i].voicing.geti() == 1) { gVenvVal[i] = 0; continue; }
        gVenvVal[i] = venvTick (gVenv[i], i, globalMod[kDestVenvTime + i], globalMod[kDestVenvLevel + i]);
        ui.venvPos[i].store (gVenv[i].pos); ui.venvVal[i].store (gVenvVal[i]);
    }
    auto attrTick = [&] (AttrMod& a, int i)
    {
        const auto& A = attrR[i];
        const int t = jlimit (0, 4, A.type.geti());
        const float hz = modRateHz (A.rate, A.sync, A.div, globalMod[kDestAttrRate + i]);
        a.step (t, hz * AttrMod::tauScale (t) * (float) n / s, A.p1.get(), A.p2.get());
        const float sm = A.smooth.get();
        const float k = sm <= 0.f ? 0.f : std::exp (-(float) n / (s * (sm * sm * 0.5f + 1e-4f)));
        return a.output (A.out.geti(), k) * A.level.mod (globalMod[kDestAttrLevel + i]);
    };
    for (int i = 0; i < kNumAttr; ++i)
    {
        if (! attrR[i].on.on() || attrR[i].mode.geti() == 1) { gAttrVal[i] = 0; continue; }
        gAttrVal[i] = attrTick (gAttr[i], i);
        const int w = ui.attrW[i].load(); ui.attrTrail[i][w][0] = gAttr[i].nx; ui.attrTrail[i][w][1] = gAttr[i].ny; ui.attrTrail[i][w][2] = gAttr[i].nz;
        ui.attrW[i].store ((w + 1) & 255); ui.attrVal[i].store (gAttrVal[i]);
    }

    float gsrc[S_Count]; computeSources (nullptr, gsrc);
    std::fill (std::begin (globalMod), std::end (globalMod), 0.f);
    accumulateMods (gsrc, globalMod, nullptr, n, true);

    bool anySolo = false;
    for (int l = 0; l < kNumLanes; ++l)
    {
        std::fill (laneL[l].begin(), laneL[l].begin() + n, 0.f); std::fill (laneR[l].begin(), laneR[l].begin() + n, 0.f);
        if (lr[l].solo.on()) anySolo = true;
        newest[l] = nullptr;
    }
    for (int b = 0; b < kNumBuses; ++b) { std::fill (sendL[b].begin(), sendL[b].begin() + n, 0.f); std::fill (sendR[b].begin(), sendR[b].begin() + n, 0.f); }
    Voice* newestAll = nullptr;
    for (auto& vp : voices)
    {
        if (! vp->active) continue;
        if (! newestAll || vp->age > newestAll->age) newestAll = vp.get();
        for (int l = 0; l < kNumLanes; ++l) if (vp->lane[l].active && (! newest[l] || vp->age > newest[l]->age)) newest[l] = vp.get();
    }

    const float tuneHz = tune.get();
    const float pbSemis = pitchBend * pbRange.get();
    const bool mpe = isMpe();
    int nActive = 0;
    float liveMod[kNumDests]; bool haveLive = false;
    float liveSrc[S_Count];

    for (auto& vp : voices)
    {
        Voice& v = *vp;
        if (! v.active) continue;
        ++nActive;
        advanceLfos (v.lfo, v.lfoVal, true, v.gate, n);
        for (int i = 0; i < kNumVenv; ++i)
            v.venvVal[i] = (venvR[i].on.on() && venvR[i].voicing.geti() == 1) ? venvTick (v.venv[i], i, globalMod[kDestVenvTime + i], globalMod[kDestVenvLevel + i]) : 0.f;
        for (int i = 0; i < kNumAttr; ++i)
            v.attrVal[i] = (attrR[i].on.on() && attrR[i].mode.geti() == 1) ? attrTick (v.attr[i], i) : 0.f;
        if (mpe) { v.bend = chanBend[v.chan]; v.pressure = chanPress[v.chan]; v.slide = chanSlide[v.chan]; }
        else v.slide = slideGlobal;

        float vsrc[S_Count]; computeSources (&v, vsrc);
        float mod[kNumDests]; std::fill (std::begin (mod), std::end (mod), 0.f);
        accumulateMods (vsrc, mod, &v, n, false);

        const float gms = glide.mod (mod[kDestGlide]);
        if (gms > 0.5f) v.curNote += (v.targetNote - v.curNote) * (1.f - std::exp (-(float) n / (gms * 0.001f * s)));
        else v.curNote = v.targetNote;
        const float noteBend = mpe ? v.bend * pbNote.get() : 0.f;

        bool anyLane = false;
        for (int l = 0; l < kNumLanes; ++l)
        {
            auto& lv = v.lane[l]; auto& R = lr[l];
            if (! lv.active) continue;
            const int type = R.engine.geti();
            if (type <= 0 || type > kNumEngines || (! R.power.on() && bus[l].powerSm < 1e-4f)) { lv.active = false; lv.env.kill(); continue; }
            if (type != lv.type)
            {
                lv.type = type;
                const uint32_t seed = hash32 ((uint32_t) v.note * 977u + (uint32_t) l * 131u + 1u);
                lv.eng.noteOn (type, seed, false); lv.sub.noteOn (type, seed ^ 0xA5A5u, false);
            }
            const float* md = mod + kDestLaneBase + l * LD_Count;

            Env& e = lv.env;
            const int em = R.emode.geti();
            if (em != e.mode) { e.kill(); e.mode = em; if (v.gate) e.noteOn (0.f); else { lv.active = false; continue; } }
            const int es = R.esub.geti();
            e.oneShot = es == 1;
            e.del = R.edel.get() * 0.001f; e.att = R.eatt.mod (md[LD_Att]) * 0.001f; e.dec = R.edec.mod (md[LD_Dec]) * 0.001f;
            e.sus = R.esus.mod (md[LD_Sus]); e.rel = R.erel.mod (md[LD_Rel]) * 0.001f;
            e.cA = R.ecatt.geti(); e.cD = R.ecdec.geti(); e.cR = R.ecrel.geti();
            if (em == 1)
            {
                e.rf = R.vrf.get(); e.ff = R.vff.get(); e.rs = R.vrs.get(); e.fsl = R.vfs.get();
                e.vmix = R.vmix.get(); e.vmem = R.vmem.get(); e.vshape = R.vshape.get(); e.vdrift = R.vdrift.get();
                e.updateVactrol();
            }
            else if (em == 2) { e.lrate = R.elrate.get(); e.lshape = R.elshape.geti(); }
            else if (em == 3)
            {
                const int vi = jlimit (0, kNumVenv - 1, R.evsrc.geti());
                const auto& V = venvR[vi];
                e.vsh = &aShapes[vi]; e.vinc = 1.f / (std::max (R.evtime.get(), 0.005f) * s);
                e.vmode = es == 0 ? 1 : (es == 1 ? 0 : 2);
                e.vpa = V.pa.get(); e.vpd = V.pd.get(); e.vps = V.ps.get();
                e.vz[0] = V.za.get(); e.vz[1] = V.zd.get(); e.vz[2] = V.zs.get(); e.vz[3] = V.zr.get();
            }

            const float semis = v.curNote - 69.f + R.pitch.mod (md[LD_Pitch]) + (R.fine.mod (md[LD_Fine]) + lv.driftP) * 0.01f + pbSemis + noteBend;
            const float f0 = clampf (tuneHz * std::exp2 (semis / 12.f), 1.f, s * 0.45f);

            float p[kMaxEngP]; int c[kMaxEngC] = { 0, 0, 0 };
            for (int k = 0; k < R.np[type]; ++k) p[k] = k < kMotorDests ? R.ep[type][k].mod (md[LD_M1 + k]) : R.ep[type][k].get();
            for (int k = 0; k < R.nc[type]; ++k) c[k] = R.ec[type][k].geti();
            float* eL = scratch[0]; float* eR = scratch[1]; float* sL = scratch[2]; float* sR = scratch[3];
            lv.eng.setHQ (hq); lv.sub.setHQ (hq);
            lv.eng.render (type, p, c, f0, eL, eR, n);
            const float s1 = R.sub1lev.mod (md[LD_Sub1]);
            if (s1 > 0.0005f)
            {
                lv.sub.render (type, p, c, f0 * std::exp2 (-(float) (R.sub1oct.geti() + 1)), sL, sR, n);
                for (int k = 0; k < n; ++k) { eL[k] += sL[k] * s1; eR[k] += sR[k] * s1; }
            }
            const float s2 = R.sub2lev.mod (md[LD_Sub2]);
            if (s2 > 0.0005f)
            {
                const float f2 = f0 * std::exp2 (-(float) (R.sub2oct.geti() + 1));
                const float iL = f2 * 0.998269f / s, iR = f2 * 1.001734f / s;
                for (int k = 0; k < n; ++k)
                {
                    lv.sub2ph += iL; if (lv.sub2ph >= 1.f) lv.sub2ph -= 1.f;
                    lv.sub2phR += iR; if (lv.sub2phR >= 1.f) lv.sub2phR -= 1.f;
                    eL[k] += sinT (lv.sub2ph) * s2 * 0.7f; eR[k] += sinT (lv.sub2phR) * s2 * 0.7f;
                }
            }

            float cut = R.cut.mod (md[LD_Cut]) * std::exp2 (lv.driftF / 12.f);
            if (R.lpg.geti() == 1) cut = 20.f * std::pow (std::max (cut, 20.f) / 20.f, clampf (e.out, 0.f, 1.f));
            lv.filt.setup (R.ftype.geti(), R.fslope.geti(), cut, R.res.mod (md[LD_Res]));

            const float lvl = dbToGain (R.level.mod (md[LD_Level])) * lv.driftL * v.vel;
            const float lk = 1.f - std::exp (-1.f / (0.004f * s));
            const float vsAmt = clampf (R.vspread.get() + md[LD_VSpread] * 2.f, -1.f, 1.f);
            const float pos = -kVsSeq[v.vsIdx & 7] * vsAmt;
            const float gl = pos > 0 ? 1.f - pos : 1.f, gr = pos < 0 ? 1.f + pos : 1.f;
            const bool isNewest = newest[l] == &v;

            float* bl = laneL[l].data(); float* br = laneR[l].data();
            for (int k = 0; k < n; ++k)
            {
                float xl = lv.dcL.tick (eL[k]), xr = lv.dcR.tick (eR[k]);
                if (isNewest) ui.engine[l].push (0.5f * (xl + xr));
                xl = lv.filt.tickCh (xl, 0); xr = lv.filt.tickCh (xr, 1);
                const float env = e.tick();
                lv.levelSm += (lvl - lv.levelSm) * lk;
                const float g = env * lv.levelSm;
                bl[k] += xl * g * gl; br[k] += xr * g * gr;
            }
            if (isNewest && type == 9)
                for (int gi = 0; gi < 16; ++gi)
                {
                    const auto& q = lv.eng.wg.g[gi];
                    const float t = q.on ? q.age / q.len : -1.f;
                    ui.grains[l][gi][0].store (t); ui.grains[l][gi][1].store (q.on ? sinT (0.5f * t) : 0.f); ui.grains[l][gi][2].store (q.gr - q.gl);
                }
            lv.envOut = e.out;
            if (e.idle()) lv.active = false; else anyLane = true;
        }
        if (&v == newestAll) { std::copy (std::begin (mod), std::end (mod), liveMod); std::copy (std::begin (vsrc), std::end (vsrc), liveSrc); haveLive = true; }
        if (! anyLane) v.active = false;
    }
    ui.activeVoices.store (nActive, std::memory_order_relaxed);
    if (! haveLive) { std::copy (std::begin (globalMod), std::end (globalMod), liveMod); std::copy (std::begin (gsrc), std::end (gsrc), liveSrc); }
    for (int d = 0; d < kNumDests; ++d) ui.modLive[d].store (liveMod[d], std::memory_order_relaxed);
    for (int i = 0; i < S_Count; ++i) ui.srcLive[i].store (liveSrc[i], std::memory_order_relaxed);
    for (int i = 0; i < kNumLfo; ++i) ui.lfoVal[i].store (liveSrc[S_Lfo1 + i], std::memory_order_relaxed);
    if (newestAll)
    {
        for (int i = 0; i < kNumVenv; ++i) if (venvR[i].voicing.geti() == 1) { ui.venvPos[i].store (newestAll->venv[i].pos); ui.venvVal[i].store (newestAll->venvVal[i]); }
        for (int i = 0; i < kNumAttr; ++i) if (attrR[i].mode.geti() == 1)
        {
            const auto& a = newestAll->attr[i]; const int w = ui.attrW[i].load();
            ui.attrTrail[i][w][0] = a.nx; ui.attrTrail[i][w][1] = a.ny; ui.attrTrail[i][w][2] = a.nz; ui.attrW[i].store ((w + 1) & 255);
            ui.attrVal[i].store (newestAll->attrVal[i]);
        }
        ui.mpePress.store (newestAll->pressure); ui.mpeSlide.store (newestAll->slide); ui.mpeGlide.store (newestAll->bend);
    }

    // ---- lane buses ----
    for (int l = 0; l < kNumLanes; ++l)
    {
        auto& R = lr[l]; auto& B = bus[l];
        const float* gm = globalMod + kDestLaneBase + l * LD_Count;
        const bool muted = R.mute.on() || (anySolo && ! R.solo.on());
        const float muteT = muted ? 0.f : 1.f, powT = R.power.on() ? 1.f : 0.f;
        const float sk = 1.f - std::exp (-1.f / (0.006f * s));
        const bool eq = R.eqon.on();
        if (eq) { float ev[kEqN]; eqValues (l, gm[LD_EqMorph], ev); B.eq.set (ev, s); }
        const float drv = R.drive.mod (gm[LD_Drive]); const int fl = R.dflav.geti();
        const float g = 1.f + drv * 14.f, dm = std::min (1.f, drv * 10.f), comp = 1.f / (1.f + drv * 1.2f);
        const float width = R.width.mod (gm[LD_Width]);
        const float monoF = R.mono.get();
        if (std::fabs (monoF - B.monoCache) > 0.01f) { B.monoCache = monoF; if (monoF > 1.f) B.monoC = svfMake (1, monoF, s, 0.7071f); }
        const float pan = R.pan.mod (gm[LD_Pan]);
        const float pl = pan > 0 ? 1.f - pan : 1.f, pr = pan < 0 ? 1.f + pan : 1.f;
        float sg[kNumBuses]; for (int f = 0; f < kNumBuses; ++f) sg[f] = dbToGain (R.fx[f].mod (gm[LD_Fx1 + f]));
        float* bl = laneL[l].data(); float* br = laneR[l].data();
        float peak = 0;
        for (int k = 0; k < n; ++k)
        {
            float x[2] = { bl[k], br[k] };
            if (eq) { x[0] = B.eq.tick (x[0], 0); x[1] = B.eq.tick (x[1], 1); }
            if (drv > 0.001f)
            {
                for (int c = 0; c < 2; ++c)
                {
                    const float in = x[c] * g; float y;
                    switch (fl)
                    {
                        case 0: y = fastTanh (in + drv * 0.15f) - fastTanh (drv * 0.15f); break;
                        case 1: y = clampf (in, -1.f, 1.f); break;
                        case 2: y = sinT (0.25f * in); break;
                        case 3: { const float t = fastTanh (in); y = t - 0.15f * drv * t * t * t; } break;
                        default: { float u = in * 0.25f + 0.25f; u -= std::floor (u); y = 1.f - 4.f * std::fabs (u - 0.5f); } break;
                    }
                    x[c] = (1.f - dm) * x[c] + dm * y * comp;
                }
            }
            const float m = 0.5f * (x[0] + x[1]); float sd = 0.5f * (x[0] - x[1]) * width;
            if (monoF > 1.f) sd = B.monoHp.tick (sd, B.monoC);
            B.muteSm += (muteT - B.muteSm) * sk; B.powerSm += (powT - B.powerSm) * sk;
            const float og = B.muteSm * B.powerSm;
            const float L = (m + sd) * pl * og, Rr = (m - sd) * pr * og;
            outL[k] += L; outR[k] += Rr;
            for (int f = 0; f < kNumBuses; ++f) if (sg[f] > 0.f) { sendL[f][k] += L * sg[f]; sendR[f][k] += Rr * sg[f]; }
            ui.laneL[l].push (L); ui.laneR[l].push (Rr);
            peak = std::max (peak, std::max (std::fabs (L), std::fabs (Rr)));
        }
        ui.lanePeak[l].store (std::max (peak, ui.lanePeak[l].load() * 0.95f), std::memory_order_relaxed);
    }

    // ---- FX buses (summed before the master chain) ----
    for (int b = 0; b < kNumBuses; ++b)
    {
        float* L = sendL[b].data(); float* R = sendR[b].data();
        bool silent = true; for (int k = 0; k < n && silent; ++k) if (L[k] != 0.f || R[k] != 0.f) silent = false;
        if (silent && busTail[b] < 1e-6f) { ui.busPk[b].store (ui.busPk[b].load() * 0.95f); continue; }   // nothing in, tails finished
        for (int sl = 0; sl < kSlotsPerBus; ++sl)
        {
            auto& F = fxR[b][sl]; auto& slot = *fx[b][sl];
            const int t = F.type.geti();
            if (t == 0 || ! F.on.on()) continue;
            slot.setType (t);
            float p[kFxParams]; for (int k = 0; k < kFxParams; ++k) p[k] = F.p[k].mod (globalMod[kDestFxParam + (b * kSlotsPerBus + sl) * kFxParams + k]);
            slot.process (L, R, n, p, F.opt.on());
        }
        const float g = dbToGain (busLevel[b].mod (globalMod[kDestBusLevel + b]));
        float pk = 0;
        for (int k = 0; k < n; ++k) { outL[k] += L[k] * g; outR[k] += R[k] * g; pk = std::max (pk, std::fabs (L[k] * g)); }
        float tail = 0; for (int k = 0; k < n; ++k) tail = std::max (tail, std::max (std::fabs (L[k]), std::fabs (R[k])));
        busTail[b] = tail;
        ui.busPk[b].store (std::max (pk, ui.busPk[b].load() * 0.95f));
    }

    // ---- master chain ----
    MasterParams mp;
    const float* mm = globalMod + kDestMaster;
    mp.bypass = mr.bypass.on(); mp.eqOn = mr.eqon.on();
    for (int k = 0; k < kEqN; ++k) mp.eq[k] = mr.eq[k].get();
    mp.eq[3] = mr.eq[3].mod (mm[MD_LowG]); mp.eq[5] = mr.eq[5].mod (mm[MD_MidG]); mp.eq[8] = mr.eq[8].mod (mm[MD_HighG]);
    mp.satOn = mr.saton.on(); mp.satDrive = mr.satdrive.mod (mm[MD_SatDrive]); mp.satMix = mr.satmix.mod (mm[MD_SatMix]);
    mp.satFlav = mr.satflav.geti(); mp.satOs = mr.satos.geti();
    mp.compOn = mr.compon.on(); mp.thr = mr.thr.mod (mm[MD_CompThr]); mp.ratio = mr.ratio.get(); mp.att = mr.att.get(); mp.rel = mr.rel.get();
    mp.knee = mr.knee.get(); mp.makeup = mr.cgain.get(); mp.glue = mr.glue.get(); mp.scHpf = mr.schpf.get(); mp.extSc = mr.sc.on();
    mp.widthOn = mr.widthon.on(); mp.ms = mr.ms.mod (mm[MD_Width]); mp.mono = mr.mono.get();
    mp.limOn = mr.limon.on(); mp.ceil = mr.ceil.get(); mp.lrel = mr.lrel.get(); mp.autoRel = mr.lrelmode.geti() == 1;
    mp.outGain = dbToGain (outGain.mod (mm[MD_Out]));
    // ---- DELUGE FX insert (effects from the Synthstrom Audible Deluge firmware) ----
    {
        auto& P = dlp;
        P.on = dr.on.on(); P.modType = dr.modtype.geti(); P.modRate = dr.modrate.geti(); P.modDepth = dr.moddepth.geti(); P.modFeedback = dr.modfb.geti(); P.modOffset = dr.modoff.geti();
        P.bass = dr.bass.geti(); P.treble = dr.treble.geti(); P.bassFreq = dr.bassf.geti(); P.trebleFreq = dr.treblef.geti();
        P.decimation = dr.srr.geti(); P.bitcrush = dr.crush.geti(); P.saturation = dr.sat.geti();
        P.delayAmount = dr.dlyamt.geti(); P.delayRate = dr.dlyrate.geti(); P.delaySync = dr.dlysync.geti(); P.delaySyncType = dr.dlysynctype.geti();
        P.delayPingPong = dr.dlypp.on(); P.delayAnalog = dr.dlyanalog.on();
        P.reverbModel = dr.rvbmodel.geti(); P.reverbAmount = dr.rvbamt.geti(); P.reverbRoom = dr.rvbroom.geti(); P.reverbDamp = dr.rvbdamp.geti();
        P.reverbWidth = dr.rvbwidth.geti(); P.reverbHPF = dr.rvbhpf.geti(); P.reverbLPF = dr.rvblpf.geti(); P.reverbPan = dr.rvbpan.geti();
        P.compThreshold = dr.cmpthr.geti(); P.compRatio = dr.cmpratio.geti(); P.compAttack = dr.cmpatt.geti(); P.compRelease = dr.cmprel.geti();
        P.compSidechainHPF = dr.cmpsc.geti(); P.compBlend = dr.cmpblend.geti();
        P.stutter = dr.stutter.on(); P.stutterRate = dr.sturate.geti(); P.stutterQuantized = dr.stuquant.on(); P.stutterReversed = dr.sturev.on(); P.stutterPingPong = dr.stupp.on();
        P.volume = dr.vol.geti();
        deluge.process (outL, outR, n, P, bpm);
    }

    for (int k = 0; k < n; ++k) ui.masterPre.push (0.5f * (outL[k] + outR[k]));
    master.process (outL, outR, scL, scR, n, mp);
    for (int k = 0; k < n; ++k)
    {
        if (! finiteF (outL[k])) outL[k] = 0; if (! finiteF (outR[k])) outR[k] = 0;
        ui.masterPost.push (0.5f * (outL[k] + outR[k])); ui.outL.push (outL[k]); ui.outR.push (outR[k]);
    }
}

void CaosmoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const auto t0 = Time::getHighResolutionTicks();
    playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = std::max (20.0, *b);
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }
    ui.bpm.store (bpm);
    if (playing && ! wasPlaying) loopPos = 0;
    wasPlaying = playing;

    const int numSamples = buffer.getNumSamples();
    bool hasSc = false;
    if (getBusCount (true) > 0 && getBus (true, 0) != nullptr && getBus (true, 0)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 0);
        if (sc.getNumChannels() > 0)
        {
            if ((int) scBufL.size() < numSamples) { scBufL.resize ((size_t) numSamples); scBufR.resize ((size_t) numSamples); }
            std::copy (sc.getReadPointer (0), sc.getReadPointer (0) + numSamples, scBufL.begin());
            const float* r = sc.getReadPointer (sc.getNumChannels() > 1 ? 1 : 0);
            std::copy (r, r + numSamples, scBufR.begin());
            hasSc = true;
        }
    }
    buffer.clear();

    if (shapesPending.load())
    {
        const SpinLock::ScopedTryLockType tl (shapeLock);
        if (tl.isLocked()) { for (int i = 0; i < kNumVenv; ++i) aShapes[i] = pShapes[i]; aKey = pKey; shapesPending = false; }
    }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
    auto it = midi.cbegin();
    int pos = 0;
    const double ppq0 = ppq;
    while (pos < numSamples)
    {
        const int n = std::min (kBlock, numSamples - pos);
        while (it != midi.cend() && (*it).samplePosition < pos + n) { handleMidi ((*it).getMessage()); ++it; }
        ppq = ppq0 + (double) pos / sr * bpm / 60.0;
        renderChunk (L + pos, R + pos, hasSc ? scBufL.data() + pos : nullptr, hasSc ? scBufR.data() + pos : nullptr, n);
        pos += n;
    }

    float pl = 0, pr = 0;
    for (int k = 0; k < numSamples; ++k) { pl = std::max (pl, std::fabs (L[k])); pr = std::max (pr, std::fabs (R[k])); }
    ui.outPkL.store (std::max (pl, ui.outPkL.load() * 0.9f)); ui.outPkR.store (std::max (pr, ui.outPkR.load() * 0.9f));
    const double el = Time::highResolutionTicksToSeconds (Time::getHighResolutionTicks() - t0);
    const float load = (float) (el / std::max (1e-6, numSamples / sr));
    ui.cpu.store (ui.cpu.load() * 0.9f + load * 0.1f);
}

//==============================================================================
void CaosmoProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CaosmoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (ValueTree::fromXml (*xml));
        ensureStateChildren(); syncShapesFromTree(); syncEqFromTree();
    }
}

void CaosmoProcessor::resetToInit()
{
    apvts.replaceState (initState.createCopy());
    ensureStateChildren(); syncShapesFromTree(); syncEqFromTree();
}

AudioProcessorEditor* CaosmoProcessor::createEditor() { return new CaosmoEditor (*this); }
} // namespace cz

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new cz::CaosmoProcessor(); }
