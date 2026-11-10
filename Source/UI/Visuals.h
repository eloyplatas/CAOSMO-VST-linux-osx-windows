// Caosmo UI — scopes, curves, editors and visualisers
#pragma once
#include "Widgets.h"

namespace cz
{
void computeSpectrum (const ScopeRing& ring, float* bins, int numBins, double sr, float loHz = 30.f, float hiHz = 18000.f);

class Scope : public juce::Component, private juce::Timer
{
public:
    Scope (Ctx& c, int lane);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { if (isShowing()) repaint(); }
    Ctx& ctx; int lane;
};

class EqCurve : public juce::Component, private juce::Timer
{
public:
    EqCurve (Ctx& c, const juce::String& prefix, int lane);   // prefix "L1_" or "m_"
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    void timerCallback() override { if (isShowing()) repaint(); }
    float xOf (float hz) const; float hzOf (float x) const; float yOf (float db) const; float dbOf (float y) const;
    juce::Point<float> node (int i) const;
    juce::String id (const char* k) const { return pre + k; }
    Ctx& ctx; juce::String pre; int lane; int drag = -1;
};

class VenvCanvas : public juce::Component, private juce::Timer
{
public:
    VenvCanvas (Ctx& c, int index);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool freeDraw = false;
private:
    void timerCallback() override;
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> fromScreen (juce::Point<float>) const;
    int hit (juce::Point<float>) const;
    void commit();
    Ctx& ctx; int idx; VShape shape; juce::String loaded; int drag = -1; bool editing = false, panning = false;
    float vx0 = 0, vx1 = 1, panStart0 = 0; juce::Point<float> panFrom; juce::Array<juce::Point<float>> stroke;
};

class KeyEditor : public juce::Component, private juce::Timer
{
public:
    explicit KeyEditor (Ctx& c, bool distribution = false);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    juce::Rectangle<float> area() const;
    juce::ValueTree tree(); void store();
    Ctx& ctx; bool dist = false; KeyCurve curve; juce::String loaded; int drag = -1;
};

class AttractorView : public juce::Component, private juce::Timer
{
public:
    AttractorView (Ctx& c, int index);
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent& e) override { last = e.position; }
private:
    void timerCallback() override { if (isShowing()) repaint(); }
    Ctx& ctx; int idx; float yaw = 0.6f, pitch = 0.4f; juce::Point<float> last;
};

class LfoPreview : public juce::Component, private juce::Timer
{
public:
    LfoPreview (Ctx& c, int index);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { if (isShowing()) repaint(); }
    Ctx& ctx; int idx;
};

class LinkScope : public juce::Component, private juce::Timer
{
public:
    explicit LinkScope (Ctx& c);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    Ctx& ctx; float hist[kNumLfo][200] {}; int w = 0;
};

class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter (std::function<float()> src, bool gr = false) : get (std::move (src)), isGR (gr) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { if (isShowing()) { const float v = get(); hold = v > hold ? v : hold * 0.995f; repaint(); } }
    std::function<float()> get; bool isGR; float hold = 0;
};

class VuMeter : public juce::Component, private juce::Timer
{
public:
    explicit VuMeter (Ctx& c) : ctx (c) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { holdL = holdR = 0; tpLatched = false; }
private:
    void timerCallback() override;
    Ctx& ctx; float l = 0, r = 0, holdL = 0, holdR = 0; bool tpLatched = false;
};

class Waterfall : public juce::Component, private juce::Timer
{
public:
    explicit Waterfall (Ctx& c);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    void timerCallback() override;
    Ctx& ctx; static constexpr int NB = 96, NH = 40; float hist[NH][NB] {}; int head = 0; float zoom = 1.f;
};

class SquareVis : public juce::Component, private juce::Timer
{
public:
    explicit SquareVis (Ctx& c);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    void timerCallback() override;
    Ctx& ctx; static constexpr int NB = 64, NH = 28; float hist[NH][NB] {}; int head = 0; float zoom = 1.f, spin = 0;
};
} // namespace cz
