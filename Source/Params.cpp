#include "Params.h"

namespace cz
{
using namespace juce;

const char* const kLfoDivNames[] = { "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T", "1/4D", "1/8D" };
const float kLfoDivBeats[] = { 32.f, 16.f, 8.f, 4.f, 2.f, 1.f, 0.5f, 0.25f, 0.125f, 2.f / 3.f, 1.f / 3.f, 1.f / 6.f, 1.5f, 0.75f };
const char* const kLoopDivNames[] = { "1/4 bar", "1/2 bar", "1/2T bar", "3/4 bar", "1 bar", "1T bar", "1.5 bars", "2 bars", "3 bars", "4 bars", "6 bars", "8 bars" };
const float kLoopDivBeats[] = { 1.f, 2.f, 4.f / 3.f, 3.f, 4.f, 8.f / 3.f, 6.f, 8.f, 12.f, 16.f, 24.f, 32.f };

const char* const kTagTypes[] = { "Pad", "Bass", "Lead", "Keys", "Pluck", "Drone", "Texture", "FX", "Sequence", "Percussive" };
const char* const kTagCharacter[] = { "Warm", "Dark", "Bright", "Metallic", "Noisy", "Glassy", "Organic", "Gritty", "Airy", "Harsh" };
const char* const kTagMotion[] = { "Static", "Slow", "Evolving", "Rhythmic", "Chaotic", "Pulsing" };

const std::vector<EngineSpec>& engineSpecs()
{
    static const std::vector<EngineSpec> specs = {
        { "none", "None", true, -1, {}, {}, "" },
        { "kur", "Kuramoto", false, -1,
          { { "k", "K", 0.f, 15.f, 2.f }, { "spread", "Spread", 0.f, 1.f, 0.5f }, { "nosc", "N Osc", 3.f, 12.f, 6.f, 0.f, true } },
          { { "out", "Out", { "Mean", "Coherent" } }, { "ratio", "Ratio", { "Asymmetric", "Linear", "Golden", "Harmonic", "Random" } } },
          "Coupled phase oscillators: scattered partials at low K, a locked glassy tone at high K." },
        { "duf", "Duffing", false, -1,
          { { "gamma", "Gamma", 0.f, 1.f, 0.3f }, { "delta", "Delta", 0.01f, 1.f, 0.1f, 0.2f }, { "omega", "Omega", 0.25f, 4.f, 1.f, 1.f },
            { "stiff", "Stiff", 0.5f, 3.f, 1.f }, { "fb", "FB", 0.f, 1.f, 0.f } },
          { { "out", "Out", { "Position", "Velocity", "Blend" } }, { "drive", "Drive", { "Sine", "Square", "Triangle", "Saw", "Noise" } } },
          "Forced double-well oscillator: resonant when calm, leaping between wells when pushed." },
        { "spr", "Sprott", false, -1,
          { { "chaos", "Chaos", 0.1f, 3.f, 1.f, 1.f }, { "damp", "Damp", 0.1f, 3.f, 1.f, 1.f }, { "omega", "Omega", 0.25f, 4.f, 1.f, 1.f }, { "fb", "FB", 0.f, 1.f, 0.f } },
          { { "out", "Out", { "X", "Y", "Z", "Blend" } } },
          "Jerk flow: smooth sinuous orbits, period-doubling into turbulence." },
        { "lr", "Lorenz+Rossler", true, -1,
          { { "lor", "Lorenz Lvl", 0.f, 1.f, 0.7f }, { "ros", "Rossler Lvl", 0.f, 1.f, 0.7f }, { "sigma", "Sigma", 5.f, 20.f, 10.f }, { "c", "C", 4.f, 8.f, 5.7f } },
          { { "lorout", "Lorenz Out", { "X", "Y", "Z", "Blend" } }, { "rosout", "Rossler Out", { "X", "Y", "Z", "Blend" } } },
          "Two strange attractors mixed. Pitch-inert: use Sub 2 for a pitch." },
        { "map", "Logistic/Henon", true, -1,
          { { "morph", "Morph", 0.f, 1.f, 0.5f }, { "r", "R", 3.6f, 3.95f, 3.9f }, { "a", "A", 0.8f, 1.4f, 1.4f }, { "b", "B", 0.f, 0.4f, 0.3f },
            { "bright", "Smooth", 0.f, 1.f, 0.f }, { "drift", "Drift", 0.f, 1.f, 0.f }, { "iter", "Iter Oct", -2.f, 7.f, 3.f } },
          { { "clock", "Clock", { "Per Sample", "Note Step", "Note Linear", "Note Cubic" } } },
          "Discrete chaotic maps. Per Sample: grainy noise. Note clocks iterate in step with the note and make it pitched." },
        { "chua", "Chua", false, -1,
          { { "alpha", "Alpha", 8.5f, 11.f, 10.f }, { "beta", "Beta", 13.5f, 18.5f, 15.f }, { "omega", "Omega", 0.5f, 8.f, 3.5f, 2.5f } }, {},
          "The double-scroll circuit: bifurcates into crunchy, restless tones." },
        { "gs", "Gray-Scott", true, -1,
          { { "f", "Feed", 0.02f, 0.06f, 0.0367f }, { "k", "Kill", 0.04f, 0.075f, 0.0649f }, { "drift", "Drift", 0.f, 1.f, 0.3f },
            { "bpf", "BPF", 0.f, 1.f, 0.f }, { "q", "Q", 0.5f, 15.f, 5.f, 4.f } }, {},
          "Reaction-diffusion ring: crackling cell patterns. Keep Drift above 0; BPF gives it a pitch." },
        { "spec", "Lorenz Additive", false, -1,
          { { "chaos", "Chaos", 0.5f, 2.f, 1.f }, { "dens", "Density", 4.f, 32.f, 32.f, 0.f, true }, { "smooth", "Smooth", 0.f, 1.f, 0.3f }, { "drive", "Drive", 0.1f, 2.f, 0.7f } }, {},
          "32 sine partials stirred by a slow Lorenz attractor. Benefits from Engine HQ." },
        { "wg", "Wave Grains", false, -1,
          { { "x", "X", 0.f, 1.f, 0.3f }, { "y", "Y", 0.f, 1.f, 0.2f }, { "pwm", "PWM", 0.05f, 0.95f, 0.5f }, { "fold", "Fold", 0.f, 1.f, 0.f },
            { "size", "Size ms", 1.f, 1000.f, 200.f, 150.f }, { "ratio", "Formant", 0.25f, 16.f, 1.f, 1.5f }, { "crate", "Chaos Rate", 0.02f, 20.f, 0.4f, 1.f },
            { "camx", "Chaos>X", -1.f, 1.f, 0.3f }, { "camy", "Chaos>Y", -1.f, 1.f, 0.2f }, { "camf", "Chaos>Fold", -1.f, 1.f, 0.f },
            { "tau", "Tau", 0.f, 1.f, 0.4f }, { "count", "Count", 1.f, 16.f, 6.f, 0.f, true }, { "shape", "Grain Shape", 0.f, 1.f, 0.5f },
            { "scatter", "Scatter", 0.f, 1.f, 0.3f }, { "jitter", "Jitter", 0.f, 1.f, 0.1f }, { "spray", "Spray", 0.f, 1.f, 0.4f }, { "fb", "Grain FB", 0.f, 1.f, 0.f } },
          { { "ctype", "Chaos", { "Lorenz", "Rossler", "Chua", "Duffing" } }, { "gmode", "Grain", { "Frozen", "Continuous" } }, { "rev", "Reverse", { "Off", "On" } } },
          "2-D morphing wavetable woven into a grain cloud, rewritten by its own chaos." },
        { "bow", "Bowed String", false, 0,
          { { "press", "Press", 0.f, 1.f, 0.5f }, { "speed", "Speed", 0.f, 1.f, 0.5f }, { "pos", "Pos", 0.f, 1.f, 0.3f }, { "loss", "Loss", 0.f, 1.f, 0.4f },
            { "rasp", "Rasp", 0.f, 1.f, 0.2f }, { "bright", "Bright", 0.f, 1.f, 0.5f }, { "stiff", "Stiff", 0.f, 1.f, 0.f }, { "body", "Body", 0.f, 1.f, 0.4f } },
          { { "note", "Note", { "Retrig", "Free" } } },
          "Physically modelled bowed string. Press or Speed at 0 stops it." },
        { "mg", "Mackey-Glass", false, 2,
          { { "drive", "Drive", 0.f, 1.f, 0.5f }, { "speed", "Speed", 0.f, 1.f, 0.5f }, { "tap", "Tap", 0.f, 1.f, 0.3f }, { "loss", "Loss", 0.f, 1.f, 0.8f },
            { "sharp", "Sharp", 0.f, 1.f, 0.2f }, { "bright", "Bright", 0.f, 1.f, 0.5f }, { "crackle", "Crackle", 0.f, 1.f, 0.f },
            { "tap2", "Tap2", 0.f, 1.f, 0.5f }, { "res", "Res", 0.f, 1.f, 0.f } },
          { { "curve", "Curve", { "Tuned", "Classic" } }, { "octave", "Octave", { "Fixed", "Free" } }, { "note", "Note", { "Retrig", "Free" } } },
          "Delay loop with a steep non-linearity, self-tuned to the note. Sharp walks into chaos." },
        { "cm", "Circle Map", false, 0,
          { { "lock", "Lock", 0.f, 1.f, 0.5f }, { "ratio", "Ratio", 0.f, 1.f, 0.5f }, { "bias", "Bias", 0.f, 1.f, 0.5f }, { "slew", "Slew", 0.f, 1.f, 0.4f },
            { "tongue", "Tongue", 0.f, 1.f, 0.2f }, { "shape", "Shape", 0.f, 1.f, 0.5f }, { "slave", "Slave", 0.5f, 2.f, 1.f, 1.f }, { "couple", "Couple", 0.f, 1.f, 0.3f } },
          { { "note", "Note", { "Retrig", "Free" } } },
          "Circle map locking onto musical ratios like meshing gears; Ratio 0.5 is the note." },
        // ---- v0.5: generators studied in the SuperCollider documentation ----
        { "gen", "Gendy", false, -1,
          { { "ascale", "Amp Step", 0.f, 1.f, 0.3f }, { "dscale", "Dur Step", 0.f, 1.f, 0.15f }, { "apar", "Amp Shape", 0.0001f, 1.f, 0.5f },
            { "dpar", "Dur Shape", 0.0001f, 1.f, 0.5f }, { "spread", "Spread", 0.f, 24.f, 3.f, 6.f }, { "points", "Points", 2.f, 32.f, 12.f, 0.f, true },
            { "stereo", "Stereo", 0.f, 1.f, 0.3f } },
          { { "adist", "Amp Dist", { "Linear", "Cauchy", "Logist", "Hyperbcos", "Arcsine", "Expon", "Sinus" }, 1 },
            { "ddist", "Dur Dist", { "Linear", "Cauchy", "Logist", "Hyperbcos", "Arcsine", "Expon", "Sinus" }, 1 },
            { "interp", "Interp", { "Linear", "Cubic" } } },
          "Xenakis' dynamic stochastic synthesis (GENDYN): a polygon wave whose points random-walk every cycle." },
        { "mz", "Maps", false, 2,
          { { "oct", "Iter Oct", -2.f, 7.f, 3.f }, { "a", "A", 0.f, 1.f, 0.5f }, { "b", "B", 0.f, 1.f, 0.5f }, { "c", "C", 0.f, 1.f, 0.5f },
            { "d", "D", 0.f, 1.f, 0.5f }, { "smooth", "Smooth", 0.f, 1.f, 0.f } },
          { { "map", "Map", { "Latoocarfian", "Standard", "Gingerbread", "Cusp", "Quad", "LinCong", "FBSine", "Crackle", "Logistic", "Henon" } },
            { "interp", "Interp", { "Step", "Linear", "Cubic" }, 2 }, { "note", "Note", { "Retrig", "Free" } } },
          "Ten chaotic maps from SuperCollider, iterated in step with the note: tones, buzz and crackle." },
        { "eco", "Ecosystems", false, 2,
          { { "p1", "P1", 0.f, 1.f, 0.5f }, { "p2", "P2", 0.f, 1.f, 0.5f }, { "force", "Force", 0.f, 1.f, 0.f }, { "speed", "Speed", 0.25f, 4.f, 1.f, 1.f },
            { "kick", "Kick", 0.f, 1.f, 0.f }, { "drive", "Drive", 0.f, 1.f, 0.3f } },
          { { "model", "Model", { "FitzHugh-Nagumo", "Brusselator", "Oregonator", "Lotka-Volterra", "Arneodo-Coullet-Tresser", "Spruce Budworm" } },
            { "out", "Out", { "X", "Y", "Z", "Blend" } }, { "note", "Note", { "Retrig", "Free" } } },
          "Neurons, chemical clocks and predator-prey ecologies, tuned to the note. Force and Kick push them into chaos." },
        { "nz", "Noise", true, -1,
          { { "tone", "Tone", -1.f, 1.f, 0.f }, { "dens", "Density", 1.f, 20000.f, 300.f, 300.f }, { "chaos", "Chaos", 1.f, 2.05f, 1.9f },
            { "stereo", "Stereo", 0.f, 1.f, 0.6f }, { "bpf", "Key BPF", 0.f, 1.f, 0.f }, { "q", "Q", 0.5f, 30.f, 6.f, 6.f } },
          { { "type", "Type", { "White", "Pink", "Brown", "Gray", "Clip", "Crackle", "Dust", "Dust2" } }, { "clock", "Clock", { "Free", "Note Step", "Note Smooth" } } },
          "Plain and chaotic noise: white, pink, brown, gray, clip, Crackle and Dust impulses. Key BPF gives it a pitch." },
        { "mb", "Membrane", false, -1,
          { { "gravity", "Gravity", 0.5f, 40.f, 6.f, 6.f }, { "bloss", "Bounce Loss", 0.f, 1.f, 0.35f }, { "height", "Height", 0.f, 1.f, 0.5f },
            { "scatter", "Scatter", 0.f, 1.f, 0.1f }, { "decay", "Decay", 0.02f, 8.f, 0.8f, 0.8f }, { "strike", "Strike", 0.f, 1.f, 0.5f },
            { "tension", "Tension", 0.f, 1.f, 0.f }, { "modes", "Modes", 2.f, 16.f, 10.f, 0.f, true } },
          { { "body", "Body", { "Circle Membrane", "Square Membrane", "Free Bar", "Bell" } } },
          "A bouncing ball striking a membrane, bar or bell whose modes are tuned to the note." },
    };
    return specs;
}

StringArray engineNames() { StringArray s; for (auto& e : engineSpecs()) s.add (e.name); return s; }

//==============================================================================
const char* const kFxTypeNames[] = { "Empty", "Reverb", "Delay", "Freq Shift", "Granular", "Howl", "Ring Mod",
                                     "Diffuse", "Mag Noise", "Rand Comb", "Noise Synth", "Wave Loss", "Squiz" };
int fxParamCount (int t) { static const int n[] = { 0, 5, 5, 3, 5, 5, 2, 3, 3, 4, 3, 3, 3 }; return n[jlimit (0, kNumFxTypes - 1, t)]; }
const char* fxParamName (int t, int k)
{
    static const char* n[kNumFxTypes][5] = { { "", "", "", "", "" }, { "Size", "Damp", "Pre", "Mod", "Level" }, { "Time", "FB", "Damp", "Wobble", "Level" },
                                   { "Shift", "FB", "Level", "", "" }, { "Size", "Density", "Scatter", "Pitch", "Level" },
                                   { "Freq", "FB", "Damp", "Drive", "Level" }, { "Freq", "Level", "", "", "" },
                                   { "Amount", "Rate", "Level", "", "" }, { "Amount", "Smooth", "Level", "", "" }, { "Density", "Rate", "Smear", "Level", "" },
                                   { "Thresh", "Memory", "Level", "", "" }, { "Drop", "Burst", "Level", "", "" }, { "Ratio", "Cycles", "Level", "", "" } };
    return n[jlimit (0, kNumFxTypes - 1, t)][jlimit (0, 4, k)];
}
float fxParamValue (int t, int k, float v)
{
    switch (t)
    {
        case 1: return k == 2 ? v * 200.f : v;
        case 2: return k == 0 ? std::pow (2000.f, v) : (k == 1 ? v * 0.98f : v);
        case 3: { if (k == 0) { float s = 2.f * v - 1.f; return s * s * s * 2000.f; } return v; }
        case 4: return k == 0 ? 10.f * std::pow (50.f, v) : (k == 1 ? std::pow (40.f, v) : (k == 3 ? std::round (v * 48.f - 24.f) : v));
        case 5: return k == 0 ? 40.f * std::pow (100.f, v) : (k == 1 ? v * 1.2f : v);
        case 6: return k == 0 ? std::pow (4000.f, v) : v;
        case 7: case 9: return k == 1 ? 0.05f * std::pow (400.f, v) : v;        // re-randomise rate 0.05-20 Hz
        case 10: return k == 1 ? std::round (1.f + v * 15.f) : v;               // frames of memory
        case 11: return k == 1 ? std::round (1.f + v * 15.f) : v;               // burst length in cycles
        case 12: return k == 0 ? 1.f + v * 9.f : (k == 1 ? std::round (1.f + v * 9.f) : v);
        default: return v;
    }
}
String fxParamText (int t, int k, float v)
{
    const float x = fxParamValue (t, k, v);
    auto pct = [] (float a) { return String (roundToInt (a * 100.f)) + "%"; };
    auto hz = [] (float a) { return std::abs (a) >= 1000.f ? String (a / 1000.f, 2) + " kHz" : (std::abs (a) < 10.f ? String (a, 2) : String (roundToInt (a))) + " Hz"; };
    switch (t)
    {
        case 1: return k == 2 ? String (roundToInt (x)) + " ms" : pct (x);
        case 2: return k == 0 ? (x >= 1000.f ? String (x / 1000.f, 2) + " s" : String (roundToInt (x)) + " ms") : pct (k == 1 ? v : x);
        case 3: return k == 0 ? hz (x) : pct (x);
        case 4: return k == 0 ? String (roundToInt (x)) + " ms" : (k == 1 ? String (x, 1) + "/s" : (k == 3 ? String ((int) x) + " st" : pct (x)));
        case 5: return k == 0 ? hz (x) : (k == 1 ? pct (v) : pct (x));
        case 6: return k == 0 ? hz (x) : pct (x);
        case 7: case 9: return k == 1 ? (x < 0.995f ? String (x, 2) + " Hz" : String (x, 1) + " Hz") : pct (x);
        case 10: return k == 1 ? String ((int) x) + " fr" : pct (x);
        case 11: return k == 1 ? String ((int) x) + " cyc" : pct (x);
        case 12: return k == 0 ? String (x, 2) + "x" : (k == 1 ? String ((int) x) + " zc" : pct (x));
        default: return pct (v);
    }
}
const char* fxOptName (int t) { return t == 2 ? "Mode" : (t == 4 || t == 7 || t == 9 ? "Capture" : (t == 10 ? "Keep" : (t == 11 ? "Pattern" : nullptr))); }
const char* fxOptLabel (int t, bool on)
{
    if (t == 2) return on ? "Ping-Pong" : "Stereo";
    if (t == 10) return on ? "Stable" : "Unstable";
    if (t == 11) return on ? "Regular" : "Random";
    return on ? "Freeze" : "Live";
}

const char* const kAttrTypeNames[] = { "Lorenz", "Rossler", "Chua", "Duffing", "Sprott B",
                                       "Henon", "Latoocarfian", "Standard", "Gingerbread", "FitzHugh", "Lotka-Volterra", "ACT" };
const char* attrPName (int t, int k)
{
    static const char* n[kNumAttrTypes][2] = { { "Sigma", "Rho" }, { "A", "C" }, { "Alpha", "Beta" }, { "Gamma", "Delta" }, { "A", "B" },
                                               { "A", "B" }, { "A", "B" }, { "K", "Init" }, { "X0", "Y0" }, { "Current", "Eps" }, { "Prey", "Pred" }, { "Alpha", "Immig" } };
    return n[jlimit (0, kNumAttrTypes - 1, t)][k & 1];
}

const char* const kDistNames[] = { "Uniform", "Gauss", "Low", "High", "Mean", "Exponential", "Cauchy", "Beta", "Poisson", "Drawn" };

String engineParamLabel (int e, int k, int c0)
{
    const auto& sp = engineSpecs()[(size_t) jlimit (0, kNumEngines, e)];
    const String base = k < (int) sp.p.size() ? String (sp.p[(size_t) k].name) : String();
    if (e == 14 && k >= 1 && k <= 4)   // Maps: A-D mean different things per map
    {
        static const char* n[10][4] = { { "A", "B", "C", "D" }, { "K", "-", "-", "-" }, { "X0", "Y0", "-", "-" }, { "A", "B", "-", "-" },
                                        { "A", "B", "C", "-" }, { "Mult", "Inc", "Mod", "-" }, { "Index", "FB", "Mult", "Inc" },
                                        { "Param", "-", "-", "-" }, { "R", "-", "-", "-" }, { "A", "B", "-", "-" } };
        return n[jlimit (0, 9, c0)][k - 1];
    }
    if (e == 15 && k <= 1)             // Ecosystems: P1/P2 per model
    {
        static const char* n[6][2] = { { "Current", "Eps" }, { "A", "B-Hopf" }, { "F", "Eps" }, { "Prey", "Pred" }, { "Alpha", "Immig" }, { "Step", "K1" } };
        return n[jlimit (0, 5, c0)][k];
    }
    return base;
}

//==============================================================================
static const char* laneDestName (int d)
{
    static const char* n[LD_Count] = { "Pitch", "Fine", "Cutoff", "Res", "Level", "Pan", "Width", "V.Spread", "Sub1 Lev", "Sub2 Lev", "Drive",
                                       "Motor 1", "Motor 2", "Motor 3", "Motor 4", "Motor 5", "Motor 6", "Env Att", "Env Dec", "Env Sus", "Env Rel",
                                       "FX1 Send", "FX2 Send", "FX3 Send", "EQ Morph" };
    return n[d];
}
static const char* kBusNames[] = { "A", "B", "C" };

StringArray destNames()
{
    StringArray s; s.add ("None");
    for (int l = 0; l < kNumLanes; ++l) for (int d = 0; d < LD_Count; ++d) s.add ("L" + String (l + 1) + " " + laneDestName (d));
    for (int i = 0; i < kNumLfo; ++i) s.add ("LFO" + String (i + 1) + " Rate");
    for (int i = 0; i < kNumLfo; ++i) s.add ("LFO" + String (i + 1) + " Level");
    for (int i = 0; i < kNumVenv; ++i) s.add ("VENV" + String (i + 1) + " Time");
    for (int i = 0; i < kNumVenv; ++i) s.add ("VENV" + String (i + 1) + " Level");
    for (int i = 0; i < kNumAttr; ++i) s.add ("Attr" + String (i + 1) + " Rate");
    for (int i = 0; i < kNumAttr; ++i) s.add ("Attr" + String (i + 1) + " Level");
    s.add ("LFO Loop Len"); s.add ("Glide");
    for (auto* n : { "Master Low Gain", "Master Mid Gain", "Master High Gain", "Sat Drive", "Sat Mix", "Comp Thresh", "Master Width", "Output Gain" }) s.add (n);
    for (int b = 0; b < kNumBuses; ++b) s.add (String ("FX ") + kBusNames[b] + " Level");
    for (int b = 0; b < kNumBuses; ++b) for (int sl = 0; sl < kSlotsPerBus; ++sl) for (int k = 0; k < kFxParams; ++k)
        s.add (String ("FX ") + kBusNames[b] + String (sl + 1) + " P" + String (k + 1));
    for (int i = 0; i < kNumTrig; ++i) s.add ("Trig" + String (i + 1) + " Rate");
    for (int i = 0; i < kNumTrig; ++i) s.add ("Trig" + String (i + 1) + " Prob");
    jassert (s.size() == kNumDests);
    return s;
}

String destGroup (int d)
{
    if (d <= 0) return {};
    if (d < kDestLfoRate) return "Lane " + String ((d - kDestLaneBase) / LD_Count + 1);
    if (d < kDestVenvTime) return "LFOs";
    if (d < kDestAttrRate) return "VENV";
    if (d < kDestLoopLen) return "Attractors";
    if (d < kDestMaster) return "Global";
    if (d < kDestBusLevel) return "Master";
    if (d < kDestTrigRate) return "FX";
    return "Triggers";
}

StringArray sourceNames()
{
    StringArray s; s.add ("None");
    for (int i = 0; i < kNumLfo; ++i) s.add ("LFO " + String (i + 1));
    for (int i = 0; i < kNumVenv; ++i) s.add ("VENV " + String (i + 1));
    for (int i = 0; i < kNumAttr; ++i) s.add ("Attractor " + String (i + 1));
    for (int l = 0; l < kNumLanes; ++l) s.add ("Lane " + String (l + 1) + " Env");
    s.addArray (StringArray { "Velocity", "Release Vel", "Keytrack", "Mod Wheel", "Aftertouch", "Pitch Bend", "Expression", "Sustain", "CC",
                              "MPE Pressure", "MPE Slide", "MPE Bend" });
    for (int i = 0; i < kNumMacros; ++i) s.add ("Macro " + String (i + 1));
    s.add ("Note Random");
    for (int i = 0; i < kNumTrig; ++i) s.add ("Trig " + String (i + 1));
    jassert (s.size() == S_Count);
    return s;
}

String sourceGroup (int s)
{
    if (s >= S_Lfo1 && s < S_Venv1) return "LFOs";
    if (s >= S_Venv1 && s < S_Attr1) return "VENV";
    if (s >= S_Attr1 && s < S_Env1) return "Attractors";
    if (s >= S_Env1 && s < S_Velocity) return "Lane Envelopes";
    if (s >= S_Velocity && s < S_Macro1) return "MIDI / MPE";
    if (s >= S_Macro1 && s < S_NoteRandom) return "Macros";
    if (s >= S_Trig1 && s < S_Trig1 + kNumTrig) return "Triggers";
    return "Other";
}

bool isPolySourceKind (int s)
{
    return (s >= S_Env1 && s < S_Env1 + kNumLanes) || s == S_Velocity || s == S_RelVelocity || s == S_Key || s == S_NoteRandom
           || s == S_MpePressure || s == S_MpeSlide || s == S_MpeBend;
}

String laneId (int lane, const char* key) { return "L" + String (lane + 1) + "_" + key; }
String engId (int lane, int e, const char* key) { return "L" + String (lane + 1) + "_" + engineSpecs()[(size_t) e].key + "_" + key; }
String lfoId (int i, const char* key) { return "LFO" + String (i + 1) + "_" + key; }
String venvId (int i, const char* key) { return "VENV" + String (i + 1) + "_" + key; }
String attrId (int i, const char* key) { return "ATTR" + String (i + 1) + "_" + key; }
String slotId (int i, const char* key) { return "S" + String (i + 1) + "_" + key; }
String trigId (int i, const char* key) { return "TRIG" + String (i + 1) + "_" + key; }
String fxId (int b, int s, const char* key) { return String ("FX") + kBusNames[b] + String (s + 1) + "_" + key; }
String busId (int b, const char* key) { return String ("FX") + kBusNames[b] + "_" + key; }

String destParamId (int d, const std::function<int (int)>& engineOfLane)
{
    if (d <= 0 || d >= kNumDests) return {};
    if (d < kDestLfoRate)
    {
        const int l = (d - kDestLaneBase) / LD_Count, k = (d - kDestLaneBase) % LD_Count;
        static const char* keys[LD_Count] = { "pitch", "fine", "cut", "res", "level", "pan", "width", "vspread", "sub1lev", "sub2lev", "drive",
                                              "", "", "", "", "", "", "eatt", "edec", "esus", "erel", "fx1", "fx2", "fx3", "eqmorph" };
        if (k >= LD_M1 && k <= LD_M6)
        {
            const int e = engineOfLane ? engineOfLane (l) : 0;
            if (e <= 0) return {};
            const auto& sp = engineSpecs()[(size_t) e];
            if (k - LD_M1 >= (int) sp.p.size()) return {};
            return engId (l, e, sp.p[(size_t) (k - LD_M1)].key);
        }
        return laneId (l, keys[k]);
    }
    if (d < kDestLfoLevel) return lfoId (d - kDestLfoRate, "rate");
    if (d < kDestVenvTime) return lfoId (d - kDestLfoLevel, "level");
    if (d < kDestVenvLevel) return venvId (d - kDestVenvTime, "time");
    if (d < kDestAttrRate) return venvId (d - kDestVenvLevel, "level");
    if (d < kDestAttrLevel) return attrId (d - kDestAttrRate, "rate");
    if (d < kDestLoopLen) return attrId (d - kDestAttrLevel, "level");
    if (d == kDestLoopLen) return "lfo_looplen";
    if (d == kDestGlide) return "glide";
    if (d < kDestBusLevel) { static const char* m[MD_Count] = { "m_lsg", "m_mg", "m_hsg", "m_satdrive", "m_satmix", "m_thr", "m_ms", "out_gain" }; return m[d - kDestMaster]; }
    if (d < kDestFxParam) return busId (d - kDestBusLevel, "level");
    if (d >= kDestTrigProb) return trigId (d - kDestTrigProb, "prob");
    if (d >= kDestTrigRate) return trigId (d - kDestTrigRate, "rate");
    const int i = d - kDestFxParam, b = i / (kSlotsPerBus * kFxParams), s = (i / kFxParams) % kSlotsPerBus, k = i % kFxParams;
    return fxId (b, s, ("p" + String (k + 1)).toRawUTF8());
}

//==============================================================================
namespace
{
    NormalisableRange<float> mkRange (float mn, float mx, float centre = 0.f, bool isInt = false)
    {
        NormalisableRange<float> r (mn, mx, isInt ? 1.f : 0.f);
        if (centre > mn && centre < mx) r.setSkewForCentre (centre);
        return r;
    }
    String fmtValue (float v, const String& unit, bool isInt)
    {
        if (unit == "dB" && v <= -59.9f) return "-inf";
        if (unit == "Hz" && v >= 1000.f) return String (v / 1000.f, v >= 10000.f ? 1 : 2) + " kHz";
        if (unit == "ms" && v >= 1000.f) return String (v / 1000.f, 2) + " s";
        if (unit == "%") return String (roundToInt (v * 100.f)) + "%";
        const float a = std::abs (v);
        String t = (isInt || a >= 100.f) ? String ((int) std::lround (v)) : String (v, a < 0.1f && a > 0.f ? 3 : (a < 10.f ? 2 : 1));
        return unit.isEmpty() ? t : t + " " + unit;
    }
    template <typename G>
    void addF (G& g, const String& id, const String& name, float mn, float mx, float def, float centre = 0.f, bool isInt = false, const String& unit = {})
    {
        auto fmt = [unit, isInt] (float v, int) { return fmtValue (v, unit, isInt); };
        auto attrs = AudioParameterFloatAttributes().withLabel (unit == "%" ? String() : unit).withStringFromValueFunction (fmt);
        g.addChild (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, mkRange (mn, mx, centre, isInt), def, attrs));
    }
    template <typename G>
    void addC (G& g, const String& id, const String& name, const StringArray& opts, int def)
    {
        g.addChild (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, opts, def));
    }
    template <typename G>
    void addB (G& g, const String& id, const String& name, bool def)
    {
        g.addChild (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    }
    template <typename G>
    void addEq (G& g, const String& pre, const String& P)
    {
        const StringArray slopes { "12 dB", "24 dB", "36 dB", "48 dB" };
        addF (g, pre + "hpf", P + "EQ HPF", 20.f, 2000.f, 20.f, 200.f, false, "Hz");
        addC (g, pre + "hpfs", P + "EQ HPF Slope", slopes, 0);
        addF (g, pre + "lsf", P + "EQ Low Freq", 20.f, 1000.f, 120.f, 150.f, false, "Hz");
        addF (g, pre + "lsg", P + "EQ Low Gain", -18.f, 18.f, 0.f, 0.f, false, "dB");
        addF (g, pre + "mf", P + "EQ Mid Freq", 100.f, 10000.f, 1000.f, 1000.f, false, "Hz");
        addF (g, pre + "mg", P + "EQ Mid Gain", -18.f, 18.f, 0.f, 0.f, false, "dB");
        addF (g, pre + "mq", P + "EQ Mid Q", 0.2f, 8.f, 1.f, 1.f);
        addF (g, pre + "hsf", P + "EQ High Freq", 1000.f, 18000.f, 6000.f, 5000.f, false, "Hz");
        addF (g, pre + "hsg", P + "EQ High Gain", -18.f, 18.f, 0.f, 0.f, false, "dB");
        addF (g, pre + "lpf", P + "EQ LPF", 200.f, 20000.f, 20000.f, 4000.f, false, "Hz");
        addC (g, pre + "lpfs", P + "EQ LPF Slope", slopes, 0);
    }
}

AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    const StringArray curves { "Lin", "Exp", "Log" };
    const StringArray drift { "Off", "Subtle", "Moderate", "Heavy" };
    StringArray chans { "Omni" }; for (int i = 1; i <= 16; ++i) chans.add (String (i));
    const StringArray slopes { "12 dB", "24 dB", "36 dB", "48 dB" };
    StringArray divs; for (int i = 0; i < kNumLfoDivs; ++i) divs.add (kLfoDivNames[i]);
    StringArray ldivs; for (int i = 0; i < kNumLoopDivs; ++i) ldivs.add (kLoopDivNames[i]);

    // ---------------- global ----------------
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("global", "Global", "|");
        addF (*g, "out_gain", "Output Gain", -60.f, 12.f, 0.f, 0.f, false, "dB");
        addC (*g, "voices", "Voices", { "1", "4", "8", "16", "32", "64" }, 3);
        addB (*g, "legato", "Legato", false);
        addF (*g, "glide", "Glide", 0.f, 5000.f, 0.f, 300.f, false, "ms");
        addC (*g, "glide_mode", "Glide Mode", { "Fingered", "Always" }, 0);
        addF (*g, "tune", "Tuning A", 415.f, 466.f, 440.f, 0.f, false, "Hz");
        addF (*g, "pb_range", "PB Range", 0.f, 24.f, 2.f, 0.f, true, "st");
        addF (*g, "pb_note", "MPE PB Range", 0.f, 96.f, 48.f, 0.f, true, "st");
        addC (*g, "mpe", "MPE Mode", { "Off", "Lower", "Upper" }, 0);
        addB (*g, "hq", "Engine HQ", true);
        for (int i = 0; i < kNumMacros; ++i) addF (*g, "macro" + String (i + 1), "Macro " + String (i + 1), 0.f, 1.f, 0.f, 0.f, false, "%");
        addB (*g, "lfo_loop", "LFO Loop Lock", false);
        addF (*g, "lfo_looplen", "LFO Loop Len", 0.25f, 16.f, 4.f, 2.f, false, "s");
        addC (*g, "lfo_loopsync", "LFO Loop Sync", { "Free", "Host" }, 0);
        addC (*g, "lfo_loopdiv", "LFO Loop Division", ldivs, 4);
        StringArray dn; for (int d = 0; d < kNumDists; ++d) dn.add (kDistNames[d]);
        addC (*g, "nrand_dist", "Note Random Dist", dn, 0);
        layout.add (std::move (g));
    }

    // ---------------- lanes ----------------
    const auto& specs = engineSpecs();
    for (int l = 0; l < kNumLanes; ++l)
    {
        const String P = "L" + String (l + 1) + " ";
        auto g = std::make_unique<AudioProcessorParameterGroup> ("lane" + String (l + 1), "Lane " + String (l + 1), "|");
        addC (*g, laneId (l, "engine"), P + "Engine", engineNames(), l == 0 ? 1 : 0);
        addB (*g, laneId (l, "power"), P + "Power", true);
        addC (*g, laneId (l, "chan"), P + "MIDI Ch", chans, 0);
        addC (*g, laneId (l, "driftp"), P + "Drift Pitch", drift, 0);
        addC (*g, laneId (l, "driftf"), P + "Drift Filter", drift, 0);
        addC (*g, laneId (l, "driftl"), P + "Drift Level", drift, 0);
        addF (*g, laneId (l, "pitch"), P + "Pitch", -36.f, 36.f, 0.f, 0.f, true, "st");
        addF (*g, laneId (l, "fine"), P + "Fine", -100.f, 100.f, 0.f, 0.f, false, "ct");
        addC (*g, laneId (l, "sub1oct"), P + "Sub1 Oct", { "-1", "-2", "-3" }, 0);
        addF (*g, laneId (l, "sub1lev"), P + "Sub1 Lev", 0.f, 1.f, 0.f, 0.f, false, "%");
        addC (*g, laneId (l, "sub2oct"), P + "Sub2 Oct", { "-1", "-2" }, 0);
        addF (*g, laneId (l, "sub2lev"), P + "Sub2 Lev", 0.f, 1.f, 0.f, 0.f, false, "%");
        addC (*g, laneId (l, "ftype"), P + "Filter Type", { "LPF", "HPF", "BPF", "Notch", "Peak", "Low Shelf", "High Shelf", "All-Pass", "Ladder", "Comb", "Formant", "Phaser", "Friction", "Spring" }, 0);
        addC (*g, laneId (l, "fslope"), P + "Filter Slope", slopes, 0);
        addF (*g, laneId (l, "cut"), P + "Cutoff", 20.f, 20000.f, 20000.f, 1000.f, false, "Hz");
        addF (*g, laneId (l, "res"), P + "Res", 0.f, 1.f, 0.1f, 0.f, false, "%");
        addC (*g, laneId (l, "emode"), P + "Env Mode", { "ADSR", "Vactrol", "LFO", "VENV" }, 0);
        addC (*g, laneId (l, "esub"), P + "Env Sub Mode", { "Sustain", "One-Shot", "Sustain-Loop" }, 0);
        addF (*g, laneId (l, "edel"), P + "Delay", 0.f, 2000.f, 0.f, 200.f, false, "ms");
        addF (*g, laneId (l, "eatt"), P + "Attack", 0.3f, 10000.f, 5.f, 300.f, false, "ms");
        addF (*g, laneId (l, "edec"), P + "Decay", 1.f, 10000.f, 300.f, 500.f, false, "ms");
        addF (*g, laneId (l, "esus"), P + "Sustain", 0.f, 1.f, 0.8f, 0.f, false, "%");
        addF (*g, laneId (l, "erel"), P + "Release", 1.f, 15000.f, 400.f, 800.f, false, "ms");
        addC (*g, laneId (l, "ecatt"), P + "Att Curve", curves, 0);
        addC (*g, laneId (l, "ecdec"), P + "Dec Curve", curves, 1);
        addC (*g, laneId (l, "ecrel"), P + "Rel Curve", curves, 1);
        addF (*g, laneId (l, "vrf"), P + "Rise Fast", 0.5f, 500.f, 5.f, 30.f, false, "ms");
        addF (*g, laneId (l, "vff"), P + "Fall Fast", 1.f, 2000.f, 80.f, 150.f, false, "ms");
        addF (*g, laneId (l, "vrs"), P + "Rise Slow", 5.f, 2000.f, 120.f, 250.f, false, "ms");
        addF (*g, laneId (l, "vfs"), P + "Fall Slow", 20.f, 8000.f, 900.f, 1000.f, false, "ms");
        addF (*g, laneId (l, "vmix"), P + "Vac Mix", 0.f, 1.f, 0.5f, 0.f, false, "%");
        addF (*g, laneId (l, "vmem"), P + "Memory", 0.f, 1.f, 0.3f, 0.f, false, "%");
        addF (*g, laneId (l, "vshape"), P + "Vac Shape", 0.f, 1.f, 0.4f, 0.f, false, "%");
        addF (*g, laneId (l, "vdrift"), P + "Vac Drift", 0.f, 1.f, 0.1f, 0.f, false, "%");
        addF (*g, laneId (l, "elrate"), P + "Env LFO Rate", 0.05f, 40.f, 2.f, 2.f, false, "Hz");
        addC (*g, laneId (l, "elshape"), P + "Env LFO Shape", { "Sine", "Triangle", "Saw Down", "Square", "Ramp Up" }, 0);
        addC (*g, laneId (l, "evsrc"), P + "Env VENV Shape", { "VENV 1", "VENV 2", "VENV 3", "VENV 4" }, 0);
        addF (*g, laneId (l, "evtime"), P + "Env VENV Time", 0.01f, 30.f, 1.f, 2.f, false, "s");
        addC (*g, laneId (l, "lpg"), P + "LPG Mode", { "Amp", "Amp + Filt" }, 0);
        addB (*g, laneId (l, "eqon"), P + "EQ On", true);
        addEq (*g, laneId (l, ""), P);
        addC (*g, laneId (l, "eqsnap"), P + "EQ Snapshot", { "A", "B", "C", "D" }, 0);
        addF (*g, laneId (l, "eqmorph"), P + "EQ Morph", 0.f, 1.f, 0.f, 0.f, false, "%");
        addB (*g, laneId (l, "mute"), P + "Mute", false);
        addB (*g, laneId (l, "solo"), P + "Solo", false);
        addF (*g, laneId (l, "level"), P + "Level", -60.f, 12.f, 0.f, -12.f, false, "dB");
        addF (*g, laneId (l, "pan"), P + "Pan", -1.f, 1.f, 0.f);
        addF (*g, laneId (l, "width"), P + "Width", 0.f, 2.f, 1.f);
        addF (*g, laneId (l, "vspread"), P + "Voice Spread", -1.f, 1.f, 0.f);
        addF (*g, laneId (l, "mono"), P + "Bass Mono", 0.f, 500.f, 0.f, 120.f, false, "Hz");
        addF (*g, laneId (l, "drive"), P + "Drive", 0.f, 1.f, 0.f, 0.f, false, "%");
        addC (*g, laneId (l, "dflav"), P + "Drive Flavour", { "Tape", "Clip", "Fold", "Transformer", "Wavefolder" }, 0);
        for (int f = 1; f <= kNumBuses; ++f)
            addF (*g, laneId (l, ("fx" + String (f)).toRawUTF8()), P + "FX" + String (f) + " Send", -60.f, 0.f, -60.f, -12.f, false, "dB");

        for (int e = 1; e <= kNumEngines; ++e)
        {
            const auto& sp = specs[(size_t) e];
            for (auto& p : sp.p) addF (*g, engId (l, e, p.key), P + sp.name + " " + p.name, p.mn, p.mx, p.def, p.centre, p.isInt);
            for (auto& c : sp.c) { StringArray o; for (auto* s : c.opts) o.add (s); addC (*g, engId (l, e, c.key), P + sp.name + " " + c.name, o, c.def); }
        }
        layout.add (std::move (g));
    }

    // ---------------- modulators ----------------
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("lfos", "LFOs", "|");
        for (int i = 0; i < kNumLfo; ++i)
        {
            const String P = "LFO" + String (i + 1) + " ";
            addB (*g, lfoId (i, "on"), P + "On", true);
            addC (*g, lfoId (i, "wave"), P + "Wave", { "Sine", "Triangle", "Saw Up", "Saw Down", "Pulse", "Random S&H", "Random Smooth", "Perlin",
                                                       "Brownian", "Geo Brownian", "Noise Ring", "Markov" }, 0);
            addF (*g, lfoId (i, "rate"), P + "Rate", 0.01f, 50.f, 1.f, 2.f, false, "Hz");
            addC (*g, lfoId (i, "sync"), P + "Sync", { "Free", "Host" }, 0);
            addC (*g, lfoId (i, "div"), P + "Division", divs, 5);
            addF (*g, lfoId (i, "level"), P + "Level", 0.f, 1.f, 1.f, 0.f, false, "%");
            addF (*g, lfoId (i, "phase"), P + "Phase", 0.f, 1.f, 0.f, 0.f, false, "%");
            addF (*g, lfoId (i, "slew"), P + "Slew", 0.f, 1.f, 0.f, 0.f, false, "%");
            addF (*g, lfoId (i, "skew"), P + "Skew", 0.f, 1.f, 0.5f, 0.f, false, "%");
            addF (*g, lfoId (i, "afade"), P + "Atk Fade", 0.f, 5000.f, 0.f, 500.f, false, "ms");
            addF (*g, lfoId (i, "rfade"), P + "Rel Fade", 0.f, 5000.f, 0.f, 500.f, false, "ms");
            addC (*g, lfoId (i, "mode"), P + "Mode", { "Global", "Poly" }, 0);
            addB (*g, lfoId (i, "retrig"), P + "Retrig", false);
            if ((i & 1) == 0)
            {
                addC (*g, lfoId (i, "link"), P + "Link", i == 0 ? StringArray { "Off", "Dual", "Quad", "Oct" } : (i == 4 ? StringArray { "Off", "Dual", "Quad" } : StringArray { "Off", "Dual" }), 0);
                addC (*g, lfoId (i, "linkrel"), P + "Link Mode", { "Sync", "Phase", "Divide", "Pendulum", "Mirror" }, 1);
            }
            StringArray dn; for (int d = 0; d < kNumDists; ++d) dn.add (kDistNames[d]);
            addC (*g, lfoId (i, "dist"), P + "Dist", dn, 0);
        }
        layout.add (std::move (g));

        auto v = std::make_unique<AudioProcessorParameterGroup> ("venv", "VENV", "|");
        for (int i = 0; i < kNumVenv; ++i)
        {
            const String P = "VENV" + String (i + 1) + " ";
            addB (*v, venvId (i, "on"), P + "On", true);
            addF (*v, venvId (i, "time"), P + "Time", 0.01f, 30.f, 2.f, 2.f, false, "s");
            addC (*v, venvId (i, "sync"), P + "Sync", { "Free", "Host" }, 0);
            addC (*v, venvId (i, "div"), P + "Division", divs, 3);
            addF (*v, venvId (i, "level"), P + "Level", 0.f, 1.f, 1.f, 0.f, false, "%");
            addF (*v, venvId (i, "za"), P + "Attack Lvl", 0.f, 2.f, 1.f, 0.f, false, "");
            addF (*v, venvId (i, "zd"), P + "Decay Lvl", 0.f, 2.f, 1.f, 0.f, false, "");
            addF (*v, venvId (i, "zs"), P + "Sustain Lvl", 0.f, 2.f, 1.f, 0.f, false, "");
            addF (*v, venvId (i, "zr"), P + "Release Lvl", 0.f, 2.f, 1.f, 0.f, false, "");
            addF (*v, venvId (i, "pa"), P + "Attack Pt", 0.f, 1.f, 0.15f, 0.f, false, "%");
            addF (*v, venvId (i, "pd"), P + "Decay Pt", 0.f, 1.f, 0.4f, 0.f, false, "%");
            addF (*v, venvId (i, "ps"), P + "Sus-Rel Pt", 0.f, 1.f, 0.7f, 0.f, false, "%");
            addC (*v, venvId (i, "mode"), P + "Mode", { "1-Shot", "Sustain", "Sustain-Loop" }, 1);
            addC (*v, venvId (i, "voicing"), P + "Voicing", { "Global", "Poly" }, 1);
            addB (*v, venvId (i, "retrig"), P + "Retrig", true);
        }
        layout.add (std::move (v));

        auto a = std::make_unique<AudioProcessorParameterGroup> ("attr", "Attractors", "|");
        StringArray at; for (int t = 0; t < kNumAttrTypes; ++t) at.add (kAttrTypeNames[t]);
        for (int i = 0; i < kNumAttr; ++i)
        {
            const String P = "Attr" + String (i + 1) + " ";
            addB (*a, attrId (i, "on"), P + "On", true);
            addC (*a, attrId (i, "type"), P + "Type", at, i % 5);
            addF (*a, attrId (i, "rate"), P + "Rate", 0.01f, 20.f, 0.5f, 1.f, false, "Hz");
            addC (*a, attrId (i, "sync"), P + "Sync", { "Free", "Host" }, 0);
            addC (*a, attrId (i, "div"), P + "Division", divs, 3);
            addF (*a, attrId (i, "p1"), P + "P1", 0.f, 1.f, 0.5f, 0.f, false, "%");
            addF (*a, attrId (i, "p2"), P + "P2", 0.f, 1.f, 0.5f, 0.f, false, "%");
            addF (*a, attrId (i, "smooth"), P + "Smooth", 0.f, 1.f, 0.2f, 0.f, false, "%");
            addF (*a, attrId (i, "level"), P + "Level", 0.f, 1.f, 1.f, 0.f, false, "%");
            addC (*a, attrId (i, "out"), P + "Out", { "X", "Y", "Z", "Blend" }, 0);
            addC (*a, attrId (i, "mode"), P + "Mode", { "Global", "Poly" }, 0);
            addB (*a, attrId (i, "retrig"), P + "Retrig", false);
        }
        layout.add (std::move (a));

        // v0.5: stochastic trigger generators (Dust, GaussTrig, CoinGate, LorenzTrig)
        auto t = std::make_unique<AudioProcessorParameterGroup> ("trig", "Triggers", "|");
        StringArray dn; for (int d = 0; d < kNumDists; ++d) dn.add (kDistNames[d]);
        for (int i = 0; i < kNumTrig; ++i)
        {
            const String P = "Trig" + String (i + 1) + " ";
            addB (*t, trigId (i, "on"), P + "On", true);
            addC (*t, trigId (i, "mode"), P + "Mode", { "Dust", "Gauss", "Coin", "Lorenz", "Clock" }, 0);
            addF (*t, trigId (i, "rate"), P + "Rate", 0.05f, 50.f, 2.f, 2.f, false, "Hz");
            addC (*t, trigId (i, "sync"), P + "Sync", { "Free", "Host" }, 0);
            addC (*t, trigId (i, "div"), P + "Division", divs, 7);
            addF (*t, trigId (i, "dev"), P + "Deviation", 0.f, 1.f, 0.3f, 0.f, false, "%");
            addF (*t, trigId (i, "prob"), P + "Probability", 0.f, 1.f, 1.f, 0.f, false, "%");
            addC (*t, trigId (i, "shape"), P + "Output", { "Gate", "Decay", "S&H", "S&H Decay" }, 1);
            addF (*t, trigId (i, "len"), P + "Length", 1.f, 2000.f, 120.f, 150.f, false, "ms");
            addC (*t, trigId (i, "dist"), P + "Dist", dn, 0);
            addC (*t, trigId (i, "action"), P + "Action", { "None", "Retrig Envs", "Restart LFOs", "Stutter" }, 0);
            addF (*t, trigId (i, "level"), P + "Level", 0.f, 1.f, 1.f, 0.f, false, "%");
        }
        layout.add (std::move (t));
    }

    // ---------------- matrix ----------------
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("matrix", "Matrix", "|");
        const auto srcs = sourceNames(); const auto dsts = destNames();
        for (int i = 0; i < kNumSlots; ++i)
        {
            const String P = "Slot" + String (i + 1) + " ";
            addC (*g, slotId (i, "src"), P + "Source", srcs, 0);
            addC (*g, slotId (i, "dst"), P + "Dest", dsts, 0);
            addF (*g, slotId (i, "amt"), P + "Amount", -1.f, 1.f, 0.f);
            addC (*g, slotId (i, "pol"), P + "Polarity", { "Auto", "Unipolar", "Bipolar" }, 0);
            addC (*g, slotId (i, "curve"), P + "Curve", { "Linear", "Exp", "Log", "S-Curve" }, 0);
            addF (*g, slotId (i, "lag"), P + "Lag", 0.f, 1000.f, 0.f, 100.f, false, "ms");
            addF (*g, slotId (i, "fin"), P + "Fade In", 0.f, 5000.f, 0.f, 500.f, false, "ms");
            addF (*g, slotId (i, "fout"), P + "Fade Out", 0.f, 5000.f, 0.f, 500.f, false, "ms");
            addF (*g, slotId (i, "jit"), P + "Jitter", 0.f, 1.f, 0.f, 0.f, false, "%");
            addC (*g, slotId (i, "quant"), P + "Quant", { "Off", "Semitone", "Major", "Minor", "Penta", "Chromatic" }, 0);
            addF (*g, slotId (i, "cc"), P + "CC", 0.f, 127.f, 1.f, 0.f, true);
        }
        layout.add (std::move (g));
    }

    // ---------------- master chain ----------------
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("master", "Master", "|");
        addB (*g, "m_bypass", "Bypass Master Chain", false);
        addB (*g, "m_eqon", "Master EQ On", true);
        addEq (*g, "m_", "Master ");
        addC (*g, "m_analyzer", "Analyzer", { "Off", "Pre", "Post", "Pre+Post" }, 2);
        addB (*g, "m_saton", "Sat On", true);
        addF (*g, "m_satdrive", "Sat Drive", 0.f, 1.f, 0.f, 0.f, false, "%");
        addF (*g, "m_satmix", "Sat Mix", 0.f, 1.f, 1.f, 0.f, false, "%");
        addC (*g, "m_satflav", "Sat Flavour", { "Trans", "Tube" }, 0);
        addC (*g, "m_satos", "Sat Oversampling", { "Off", "x2", "x4" }, 1);
        addB (*g, "m_compon", "Comp On", true);
        addF (*g, "m_thr", "Comp Thresh", -60.f, 0.f, 0.f, -18.f, false, "dB");
        addF (*g, "m_ratio", "Comp Ratio", 1.f, 20.f, 1.f, 4.f, false, ":1");
        addF (*g, "m_att", "Comp Attack", 0.1f, 200.f, 10.f, 20.f, false, "ms");
        addF (*g, "m_rel", "Comp Release", 10.f, 2000.f, 150.f, 200.f, false, "ms");
        addF (*g, "m_knee", "Comp Knee", 0.f, 24.f, 6.f, 0.f, false, "dB");
        addF (*g, "m_cgain", "Comp Makeup", 0.f, 24.f, 0.f, 0.f, false, "dB");
        addF (*g, "m_glue", "Comp Glue", 0.f, 1.f, 0.f, 0.f, false, "%");
        addF (*g, "m_schpf", "Comp SC HPF", 20.f, 500.f, 20.f, 100.f, false, "Hz");
        addB (*g, "m_sc", "Comp Ext Sidechain", false);
        addB (*g, "m_widthon", "Width On", true);
        addF (*g, "m_ms", "Master Width", 0.f, 2.f, 1.f);
        addF (*g, "m_mono", "Master Bass Mono", 0.f, 500.f, 0.f, 120.f, false, "Hz");
        addB (*g, "m_limon", "Limiter On", false);
        addF (*g, "m_ceil", "Limiter Ceiling", -12.f, 0.f, -1.f, 0.f, false, "dBTP");
        addF (*g, "m_lrel", "Limiter Release", 1.f, 1000.f, 100.f, 100.f, false, "ms");
        addC (*g, "m_lrelmode", "Limiter Release Mode", { "Manual", "Auto" }, 1);
        layout.add (std::move (g));
    }

    // ---------------- DELUGE FX (effects from the Synthstrom Audible Deluge firmware, GPL-3.0) ----------------
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("deluge", "Deluge FX", "|");
        auto I = [&] (const char* id, const char* name, int mn, int mx, int def) { addF (*g, id, name, (float) mn, (float) mx, (float) def, 0.f, true); };
        addB (*g, "dl_on", "Deluge FX On", false);
        addC (*g, "dl_modtype", "Deluge Mod FX", { "Off", "Flanger", "Chorus", "Phaser", "Stereo Chorus", "Warble", "Dimension", "Grain" }, 0);
        I ("dl_modrate", "Deluge Mod Rate", 0, 50, 25); I ("dl_moddepth", "Deluge Mod Depth", 0, 50, 25);
        I ("dl_modfb", "Deluge Mod Feedback", 0, 50, 0); I ("dl_modoff", "Deluge Mod Offset", 0, 50, 25);
        I ("dl_bass", "Deluge Bass", -25, 25, 0); I ("dl_treble", "Deluge Treble", -25, 25, 0);
        I ("dl_bassf", "Deluge Bass Freq", 0, 50, 25); I ("dl_treblef", "Deluge Treble Freq", 0, 50, 25);
        I ("dl_srr", "Deluge Decimation", 0, 50, 0); I ("dl_crush", "Deluge Bitcrush", 0, 50, 0); I ("dl_sat", "Deluge Saturation", 0, 15, 0);
        I ("dl_dlyamt", "Deluge Delay Amount", 0, 50, 0); I ("dl_dlyrate", "Deluge Delay Rate", 0, 50, 25);
        // Deluge SyncLevel n spans 3 << (9 - n) ticks at 96 ticks per quarter (Song::getNoteLengthName at the default magnitude)
        addC (*g, "dl_dlysync", "Deluge Delay Sync", { "Off", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128" }, 6);
        addC (*g, "dl_dlysynctype", "Deluge Delay Sync Type", { "Even", "Triplet", "Dotted" }, 0);
        addB (*g, "dl_dlypp", "Deluge Delay Ping-Pong", true); addB (*g, "dl_dlyanalog", "Deluge Delay Analog", false);
        addC (*g, "dl_rvbmodel", "Deluge Reverb Model", { "Freeverb", "Mutable", "Digital" }, 0);
        I ("dl_rvbamt", "Deluge Reverb Amount", 0, 50, 0); I ("dl_rvbroom", "Deluge Reverb Room", 0, 50, 25);
        I ("dl_rvbdamp", "Deluge Reverb Damping", 0, 50, 25); I ("dl_rvbwidth", "Deluge Reverb Width", 0, 50, 50);
        I ("dl_rvbhpf", "Deluge Reverb HPF", 0, 50, 0); I ("dl_rvblpf", "Deluge Reverb LPF", 0, 50, 50); I ("dl_rvbpan", "Deluge Reverb Pan", -25, 25, 0);
        I ("dl_cmpthr", "Deluge Comp Threshold", 0, 50, 0); I ("dl_cmpratio", "Deluge Comp Ratio", 0, 50, 25);
        I ("dl_cmpatt", "Deluge Comp Attack", 0, 50, 25); I ("dl_cmprel", "Deluge Comp Release", 0, 50, 25);
        I ("dl_cmpsc", "Deluge Comp Sidechain HPF", 0, 50, 0); I ("dl_cmpblend", "Deluge Comp Blend", 0, 50, 50);
        addB (*g, "dl_stutter", "Deluge Stutter", false); I ("dl_sturate", "Deluge Stutter Rate", 0, 50, 25);
        addB (*g, "dl_stuquant", "Deluge Stutter Quantized", true); addB (*g, "dl_sturev", "Deluge Stutter Reversed", false); addB (*g, "dl_stupp", "Deluge Stutter Ping-Pong", false);
        I ("dl_vol", "Deluge Volume", -25, 25, 0);
        layout.add (std::move (g));
    }

    // ---------------- FX buses ----------------
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("fx", "FX", "|");
        StringArray types; for (int t = 0; t < kNumFxTypes; ++t) types.add (kFxTypeNames[t]);
        for (int b = 0; b < kNumBuses; ++b)
        {
            const String P = String ("FX ") + kBusNames[b] + " ";
            addF (*g, busId (b, "level"), P + "Bus Level", -60.f, 6.f, 0.f, -6.f, false, "dB");
            for (int s = 0; s < kSlotsPerBus; ++s)
            {
                const String Q = P + "Slot" + String (s + 1) + " ";
                const int def = (s == 0) ? (b == 0 ? 1 : (b == 1 ? 2 : 0)) : 0;
                addC (*g, fxId (b, s, "type"), Q + "Type", types, def);
                addB (*g, fxId (b, s, "on"), Q + "On", true);
                for (int k = 0; k < kFxParams; ++k) addF (*g, fxId (b, s, ("p" + String (k + 1)).toRawUTF8()), Q + "P" + String (k + 1), 0.f, 1.f, k == 4 ? 0.8f : 0.5f, 0.f, false, "%");
                addB (*g, fxId (b, s, "opt"), Q + "Option", false);
            }
        }
        layout.add (std::move (g));
    }
    return layout;
}
} // namespace cz
