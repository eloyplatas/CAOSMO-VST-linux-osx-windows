#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "Engines.h"
#include "Modulators.h"
#include "MasterFX.h"
#include "DelugeFX.h"

namespace cz
{
constexpr int kMaxVoices = 64;
constexpr int kBlock = 16;
constexpr int kEqN = 11;

struct PRef
{
    std::atomic<float>* raw = nullptr; juce::RangedAudioParameter* p = nullptr;
    void bind (juce::AudioProcessorValueTreeState& s, const juce::String& id) { raw = s.getRawParameterValue (id); p = s.getParameter (id); jassert (raw && p); }
    inline float get() const { return raw->load (std::memory_order_relaxed); }
    inline int geti() const { return (int) std::lround (get()); }
    inline bool on() const { return get() > 0.5f; }
    inline float mod (float m) const
    {
        if (std::abs (m) < 1e-7f) return get();
        return p->convertFrom0to1 (clampf (p->convertTo0to1 (get()) + m, 0.f, 1.f));
    }
};

struct LaneRefs
{
    PRef engine, power, chan, driftp, driftf, driftl, pitch, fine, sub1oct, sub1lev, sub2oct, sub2lev;
    PRef ftype, fslope, cut, res, emode, esub, edel, eatt, edec, esus, erel, ecatt, ecdec, ecrel;
    PRef vrf, vff, vrs, vfs, vmix, vmem, vshape, vdrift, elrate, elshape, evsrc, evtime, lpg;
    PRef eqon, eq[kEqN], eqsnap, eqmorph;
    PRef mute, solo, level, pan, width, vspread, mono, drive, dflav, fx[kNumBuses];
    PRef ep[kNumEngines + 1][kMaxEngP]; PRef ec[kNumEngines + 1][kMaxEngC];
    int np[kNumEngines + 1] = {}, nc[kNumEngines + 1] = {};
};
struct LfoRefs { PRef on, wave, rate, sync, div, level, phase, slew, skew, afade, rfade, mode, retrig, link, linkrel; };
struct VenvRefs { PRef on, time, sync, div, level, za, zd, zs, zr, pa, pd, ps, mode, voicing, retrig; };
struct AttrRefs { PRef on, type, rate, sync, div, p1, p2, smooth, level, out, mode, retrig; };
struct SlotRefs { PRef src, dst, amt, pol, curve, lag, fin, fout, jit, quant, cc; };
struct FxRefs { PRef type, on, p[kFxParams], opt; };
struct MasterRefs
{
    PRef bypass, eqon, eq[kEqN], analyzer, saton, satdrive, satmix, satflav, satos, compon, thr, ratio, att, rel, knee, cgain, glue, schpf, sc;
    PRef widthon, ms, mono, limon, ceil, lrel, lrelmode;
};

struct LaneVoice
{
    bool active = false; int type = 0;
    EngineBank eng, sub;
    VoiceFilter filt; Env env; DCBlock dcL, dcR;
    float driftP = 0, driftF = 0, driftL = 1.f, envOut = 0, sub2ph = 0, sub2phR = 0;
    float levelSm = 1.f;
};

struct Voice
{
    bool active = false, gate = false, sustained = false;
    int note = 60, chan = 1; float vel = 1.f, relVel = 0.f; uint64_t age = 0;
    float curNote = 60.f, targetNote = 60.f; float noteRand = 0; int vsIdx = 0;
    float pressure = 0, slide = 0, bend = 0;
    LaneVoice lane[kNumLanes];
    LfoState lfo[kNumLfo]; float lfoVal[kNumLfo] = {};
    VenvPlayer venv[kNumVenv]; float venvVal[kNumVenv] = {};
    AttrMod attr[kNumAttr]; float attrVal[kNumAttr] = {};
    float slotLag[kNumSlots] = {}, slotFade[kNumSlots] = {}, slotJit[kNumSlots] = {}; bool lagInit = false;
    Rng rng;
};

struct LaneBus
{
    EqBank eq; SVF monoHp; SVFCoefs monoC; float monoCache = -1.f; float muteSm = 1.f, powerSm = 1.f;
};

struct ScopeRing
{
    static constexpr int N = 4096;
    float d[N] {}; std::atomic<int> w { 0 };
    inline void push (float x) { const int i = w.load (std::memory_order_relaxed); d[i] = x; w.store ((i + 1) & (N - 1), std::memory_order_relaxed); }
    void latest (float* out, int n) const { const int e = w.load (std::memory_order_relaxed); for (int k = 0; k < n; ++k) out[k] = d[(e - n + k) & (N - 1)]; }
};

// Everything the editor reads for display. Written by the audio thread, read without locks (tearing is harmless).
struct UiData
{
    ScopeRing laneL[kNumLanes], laneR[kNumLanes], engine[kNumLanes], masterPre, masterPost, outL, outR;
    std::atomic<float> lanePeak[kNumLanes] {}, outPkL { 0 }, outPkR { 0 }, cpu { 0 };
    std::atomic<float> modLive[kNumDests] {}, srcLive[S_Count] {};
    std::atomic<float> lfoVal[kNumLfo] {}, venvPos[kNumVenv] {}, venvVal[kNumVenv] {}, attrVal[kNumAttr] {};
    float attrTrail[kNumAttr][256][3] {}; std::atomic<int> attrW[kNumAttr] {};
    std::atomic<float> grains[kNumLanes][16][3] {};          // phase, amplitude, pan of the newest voice's grains
    std::atomic<int> activeVoices { 0 }; std::atomic<float> mpePress { 0 }, mpeSlide { 0 }, mpeGlide { 0 };
    std::atomic<float> busPk[kNumBuses] {};
    std::atomic<double> bpm { 120.0 };
};

class CaosmoProcessor : public juce::AudioProcessor,
                        private juce::AudioProcessorValueTreeState::Listener,
                        private juce::ValueTree::Listener,
                        private juce::AsyncUpdater
{
public:
    CaosmoProcessor();
    ~CaosmoProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Caosmo"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    UiData ui;

    // ---- shapes & curves stored in the state tree (message thread) ----
    juce::ValueTree venvTree (int i);
    juce::ValueTree keyTree();
    VShape getShape (int i);
    void setShape (int i, const VShape& s, const juce::String& name, bool undoable = true);
    KeyCurve getKeyCurve();
    void setKeyCurve (const KeyCurve& k);

    // ---- EQ snapshots (message thread) ----
    int eqUsedMask (int lane) const { return eqUsed[lane].load(); }
    void eqResetSnapshots (int lane);

    // ---- presets ----
    void resetToInit();
    juce::ValueTree getInitState() const { return initState; }

    // ---- MIDI learn ----
    std::atomic<int> learnSlot { -1 }, learnedSlot { -1 }, learnedCC { -1 };

    int engineOfLane (int l) const { return lr[l].engine.geti(); }
    MasterChain& masterChain() { return master; }
    float delugeCompGR() const { return deluge.compressorReduction(); }
    FxSlot& fxSlot (int b, int s) { return *fx[b][s]; }
    double getSr() const { return sr; }

private:
    void bindRefs();
    void ensureStateChildren();
    void syncShapesFromTree();
    void syncEqFromTree();
    void writeEqToTree (int lane);
    void parameterChanged (const juce::String& id, float v) override;
    void handleAsyncUpdate() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { shapesDirtyFromTree = true; triggerAsyncUpdate(); }
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override { shapesDirtyFromTree = true; triggerAsyncUpdate(); }
    void valueTreeRedirected (juce::ValueTree&) override { shapesDirtyFromTree = true; eqReload = true; triggerAsyncUpdate(); }

    void handleMidi (const juce::MidiMessage& m);
    void noteOn (int note, float vel, int chan);
    void noteOff (int note, float relVel, int chan);
    void releaseVoice (Voice& v);
    void startVoice (Voice& v, int note, float vel, int chan, bool fingered);
    int laneMaskFor (int chan) const;
    bool isMpe() const { return mpeMode.geti() != 0; }
    bool isMasterChan (int ch) const { return (mpeMode.geti() == 1 && ch == 1) || (mpeMode.geti() == 2 && ch == 16); }
    void renderChunk (float* outL, float* outR, const float* scL, const float* scR, int n);
    void advanceLfos (LfoState* st, float* out, bool poly, bool gate, int n);
    void computeSources (const Voice* v, float* src) const;
    void accumulateMods (const float* src, float* mod, Voice* v, int n, bool global);
    float lfoCyclesPerSample (int i, float rateMod) const;
    float modRateHz (const PRef& rate, const PRef& sync, const PRef& div, float rateMod) const;
    void eqValues (int lane, float morphMod, float* out) const;

    double sr = 48000.0; int maxBlock = 512;
    LaneRefs lr[kNumLanes]; LfoRefs lfoR[kNumLfo]; VenvRefs venvR[kNumVenv]; AttrRefs attrR[kNumAttr]; SlotRefs slotR[kNumSlots];
    FxRefs fxR[kNumBuses][kSlotsPerBus]; PRef busLevel[kNumBuses]; MasterRefs mr;
    PRef outGain, voicesP, legato, glide, glideMode, tune, pbRange, pbNote, mpeMode, hqP, macro[kNumMacros], loopOn, loopLen, loopSync, loopDiv;

    std::vector<std::unique_ptr<Voice>> voices;
    LaneBus bus[kNumLanes];
    std::unique_ptr<FxSlot> fx[kNumBuses][kSlotsPerBus];
    MasterChain master;
    DelugeRack deluge; DelugeParams dlp;
    struct DelugeRefs { PRef on, modtype, modrate, moddepth, modfb, modoff, bass, treble, bassf, treblef, srr, crush, sat, dlyamt, dlyrate, dlysync, dlysynctype, dlypp, dlyanalog,
                        rvbmodel, rvbamt, rvbroom, rvbdamp, rvbwidth, rvbhpf, rvblpf, rvbpan, cmpthr, cmpratio, cmpatt, cmprel, cmpsc, cmpblend, stutter, sturate, stuquant, sturev, stupp, vol; } dr;
    LfoState gLfo[kNumLfo]; float gLfoVal[kNumLfo] = {};
    VenvPlayer gVenv[kNumVenv]; float gVenvVal[kNumVenv] = {};
    AttrMod gAttr[kNumAttr]; float gAttrVal[kNumAttr] = {};
    float gSlotLag[kNumSlots] = {}, gSlotJit[kNumSlots] = {}; bool gLagInit = false; Rng gRng;
    float globalMod[kNumDests] = {};
    struct Link { int master = -1, k = 0, n = 1, rel = 0; } link[kNumLfo];
    double loopPos = 0; bool wasPlaying = false; double ppq = 0; bool playing = false;

    // shapes (audio-thread copies) and their hand-off
    VShape aShapes[kNumVenv]; KeyCurve aKey;
    VShape pShapes[kNumVenv]; KeyCurve pKey; juce::SpinLock shapeLock; std::atomic<bool> shapesPending { false };
    std::atomic<bool> shapesDirtyFromTree { true }, eqReload { true };

    // EQ snapshots
    std::atomic<float> eqSnap[kNumLanes][4][kEqN] {}; std::atomic<int> eqUsed[kNumLanes] {}; int eqSel[kNumLanes] = {}; std::atomic<bool> eqSelPending { false };

    // performance state
    float modWheel = 0, aftertouch = 0, pitchBend = 0, expression = 1.f, slideGlobal = 0; bool sustainDown = false;
    float cc[128] = {}; float chanBend[17] = {}, chanPress[17] = {}, chanSlide[17] = {};
    int held[128] = {}; int heldStack[128] = {}; int heldCount = 0;
    int lastNote = -1; int vspreadCounter = 0; uint64_t ageCounter = 0; double bpm = 120.0;
    Voice* newest[kNumLanes] = {};
    std::vector<float> laneL[kNumLanes], laneR[kNumLanes], sendL[kNumBuses], sendR[kNumBuses], scBufL, scBufR;
    float scratch[8][kBlock * 2] = {};
    bool limWasOn = false; float busTail[kNumBuses] = {};
    juce::ValueTree initState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaosmoProcessor)
};
} // namespace cz
