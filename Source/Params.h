// Caosmo — parameter catalogue
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace cz
{
constexpr int kNumLanes = 3;
constexpr int kNumEngines = 17;      // 1..17 (0 = None)
constexpr int kMaxEngP = 20;
constexpr int kMaxEngC = 3;
constexpr int kNumLfo = 8;
constexpr int kNumVenv = 4;
constexpr int kNumAttr = 4;
constexpr int kNumTrig = 4;
constexpr int kNumAttrTypes = 12;
constexpr int kNumDists = 10;
constexpr int kNumLfoWaves = 12;
constexpr int kNumSlots = 24;
constexpr int kNumMacros = 5;
constexpr int kMotorDests = 6;
constexpr int kNumBuses = 3;
constexpr int kSlotsPerBus = 3;
constexpr int kFxParams = 5;
constexpr int kEqVals = 12;

struct PSpec { const char* key; const char* name; float mn, mx, def; float centre = 0.f; bool isInt = false; };
struct CSpec { const char* key; const char* name; std::vector<const char*> opts; int def = 0; };
struct EngineSpec { const char* key; const char* name; bool pitchInert; int noteChoice; std::vector<PSpec> p; std::vector<CSpec> c; const char* blurb; };

const std::vector<EngineSpec>& engineSpecs();        // index 0 = None
juce::StringArray engineNames();

// ---------------- destinations ----------------
enum LaneDest { LD_Pitch, LD_Fine, LD_Cut, LD_Res, LD_Level, LD_Pan, LD_Width, LD_VSpread, LD_Sub1, LD_Sub2, LD_Drive,
                LD_M1, LD_M2, LD_M3, LD_M4, LD_M5, LD_M6, LD_Att, LD_Dec, LD_Sus, LD_Rel, LD_Fx1, LD_Fx2, LD_Fx3, LD_EqMorph, LD_Count };
constexpr int kDestLaneBase  = 1;
constexpr int kDestLfoRate   = kDestLaneBase + kNumLanes * LD_Count;
constexpr int kDestLfoLevel  = kDestLfoRate + kNumLfo;
constexpr int kDestVenvTime  = kDestLfoLevel + kNumLfo;
constexpr int kDestVenvLevel = kDestVenvTime + kNumVenv;
constexpr int kDestAttrRate  = kDestVenvLevel + kNumVenv;
constexpr int kDestAttrLevel = kDestAttrRate + kNumAttr;
constexpr int kDestLoopLen   = kDestAttrLevel + kNumAttr;
constexpr int kDestGlide     = kDestLoopLen + 1;
enum MasterDest { MD_LowG, MD_MidG, MD_HighG, MD_SatDrive, MD_SatMix, MD_CompThr, MD_Width, MD_Out, MD_Count };
constexpr int kDestMaster    = kDestGlide + 1;
constexpr int kDestBusLevel  = kDestMaster + MD_Count;
constexpr int kDestFxParam   = kDestBusLevel + kNumBuses;
// v0.5 additions are appended so stored routes keep their indices
constexpr int kDestTrigRate  = kDestFxParam + kNumBuses * kSlotsPerBus * kFxParams;
constexpr int kDestTrigProb  = kDestTrigRate + kNumTrig;
constexpr int kNumDests      = kDestTrigProb + kNumTrig;

// ---------------- sources ----------------
enum Source
{
    S_None,
    S_Lfo1,
    S_Venv1 = S_Lfo1 + kNumLfo,
    S_Attr1 = S_Venv1 + kNumVenv,
    S_Env1  = S_Attr1 + kNumAttr,
    S_Velocity = S_Env1 + kNumLanes, S_RelVelocity, S_Key, S_ModWheel, S_Aftertouch, S_PitchBend, S_Expression, S_Sustain, S_CC,
    S_MpePressure, S_MpeSlide, S_MpeBend,
    S_Macro1,
    S_NoteRandom = S_Macro1 + kNumMacros,
    S_Trig1,                                   // v0.5: stochastic trigger generators (appended)
    S_Count = S_Trig1 + kNumTrig
};

juce::StringArray destNames();
juce::StringArray sourceNames();
juce::String destGroup (int d);        // submenu name for nested menus
juce::String sourceGroup (int s);
bool isPolySourceKind (int s);         // sources that differ per note (not counting poly-mode modulators)

juce::String laneId (int lane, const char* key);                      // "L1_cut"
juce::String engId (int lane, int engine, const char* key);           // "L1_kur_k"
juce::String lfoId (int i, const char* key);                          // "LFO1_rate"
juce::String venvId (int i, const char* key);                         // "VENV1_time"
juce::String attrId (int i, const char* key);                         // "ATTR1_rate"
juce::String slotId (int i, const char* key);                         // "S1_src"
juce::String trigId (int i, const char* key);                         // "TRIG1_rate"
juce::String fxId (int bus, int slot, const char* key);               // "FXA1_type"
juce::String busId (int bus, const char* key);                        // "FXA_level"

// destination -> parameter id ("" if the destination has no single parameter)
juce::String destParamId (int dest, const std::function<int (int lane)>& engineOfLane);

// effect slot labels
extern const char* const kFxTypeNames[];
constexpr int kNumFxTypes = 13;
int fxParamCount (int type);
const char* fxParamName (int type, int k);
float fxParamValue (int type, int k, float norm);       // maps 0..1 to the effect's real unit
juce::String fxParamText (int type, int k, float norm);
const char* fxOptName (int type);                        // null if unused
const char* fxOptLabel (int type, bool on);

// attractor modulator labels
extern const char* const kAttrTypeNames[];
const char* attrPName (int type, int k);

// random distributions (index into kDistNames)
extern const char* const kDistNames[];

// engine knob labels that depend on the selected model/map (falls back to the spec name)
juce::String engineParamLabel (int engine, int k, int choice0);

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

extern const char* const kLfoDivNames[];
extern const float kLfoDivBeats[];
constexpr int kNumLfoDivs = 14;
extern const char* const kLoopDivNames[];
extern const float kLoopDivBeats[];
constexpr int kNumLoopDivs = 12;

// tag vocabulary for presets
extern const char* const kTagTypes[];
extern const char* const kTagCharacter[];
extern const char* const kTagMotion[];
constexpr int kNumTagTypes = 10, kNumTagCharacter = 10, kNumTagMotion = 6;
} // namespace cz
