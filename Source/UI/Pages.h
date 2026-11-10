// Caosmo UI — pages
#pragma once
#include "Visuals.h"

namespace cz
{
// Scrollable page: a Viewport around a body that lays itself out
class ScrollPage : public juce::Component
{
public:
    ScrollPage() { view.setViewedComponent (&body, false); view.setScrollBarsShown (true, false); addAndMakeVisible (view); }
    void resized() override { view.setBounds (getLocalBounds()); relayout(); }
    virtual void relayout() = 0;
    void scrollToFraction (float f) { view.setViewPosition (0, (int) (f * (float) juce::jmax (0, body.getHeight() - view.getHeight()))); }
protected:
    struct Body : juce::Component { void paint (juce::Graphics& g) override { g.fillAll (Theme::c().bg); } };
    juce::Viewport view; Body body;
    int bodyWidth() const { return juce::jmax (200, view.getWidth() - view.getScrollBarThickness()); }
};

class TextLine : public juce::Component
{
public:
    TextLine (std::function<juce::String()> f, float size = 10.5f) : fn (std::move (f)), fontSize (size) {}
    void paint (juce::Graphics& g) override { g.setColour (Theme::c().dim); g.setFont (juce::FontOptions (fontSize)); g.drawFittedText (fn(), getLocalBounds().reduced (4, 0), juce::Justification::centredLeft, 3); }
    std::function<juce::String()> fn; float fontSize;
};

//==============================================================================
class LaneColumn : public juce::Component, private juce::Timer
{
public:
    LaneColumn (Ctx& c, int lane);
    int layoutFor (int width);           // returns height
    std::function<void()> onHeightChanged;
private:
    void timerCallback() override;
    void buildMotor(); void buildEnv();
    Ctx& ctx; int lane; int engine = -1, envMode = -1;
    Panel header, drift, pitch, subs, motor, filter, env, eq, scope, out;
    juce::Array<Knob*> motorKnobs; int motorChoice0 = -1;
    std::unique_ptr<TextLine> blurb; std::unique_ptr<EqCurve> curve; std::unique_ptr<Scope> scopeView;
    juce::OwnedArray<ActionChip> eqChips, scopeChips;
};

class MainPage : public ScrollPage
{
public:
    explicit MainPage (Ctx& c);
    void relayout() override;
private:
    Ctx& ctx; juce::OwnedArray<LaneColumn> cols;
};

//==============================================================================
class ModulatorsPage : public ScrollPage, private juce::Timer
{
public:
    explicit ModulatorsPage (Ctx& c);
    void relayout() override;
private:
    void timerCallback() override;
    void shapeMenu (int i, juce::Component* target);
    Ctx& ctx; Panel loop;
    juce::OwnedArray<Panel> lfo, venv, attr, trig;
    juce::Array<Knob*> attrP1, attrP2;
    juce::OwnedArray<juce::Component> extras;
    juce::Array<VenvCanvas*> canvases; juce::Array<TextLine*> routing;
    LinkScope* linkScope = nullptr;
};

//==============================================================================
class MatrixRow : public juce::Component, private juce::Timer
{
public:
    MatrixRow (Ctx& c, int slot);
    void resized() override;
    void paint (juce::Graphics&) override;
    static int status (Ctx& c, int slot);   // 0 off, 1 active, 2 loop, 3 engine missing, 4 invalid
private:
    void timerCallback() override;
    void destMenu();
    Ctx& ctx; int slot;
    std::unique_ptr<Choice> src, pol, curve, quant;
    std::unique_ptr<Knob> cc, amt, lag, fin, fout, jit;
    juce::TextButton dst, del { "x" };
    int lastStatus = -1;
};

class MatrixPage : public ScrollPage
{
public:
    explicit MatrixPage (Ctx& c);
    void relayout() override;
private:
    Ctx& ctx; juce::OwnedArray<MatrixRow> rows; Panel table, keyPanel, macroPanel, distPanel; std::unique_ptr<KeyEditor> keyEd, distEd;
    std::unique_ptr<TextLine> macroRoutes, header;
};

//==============================================================================
class MasterPage : public ScrollPage, private juce::Timer
{
public:
    explicit MasterPage (Ctx& c);
    void relayout() override;
    void setView (int v) { view = v; relayout(); }
private:
    void timerCallback() override;
    Ctx& ctx; int view = 0; juce::String typeSig;   // 0 chain, 1 fx, 2 deluge
    ActionChip chainBtn, fxBtn, delugeBtn;
    juce::OwnedArray<Panel> dlP; std::unique_ptr<LevelMeter> dlGr;
    Panel eqP, satP, compP, widthP, limP, outP, visP;
    std::unique_ptr<EqCurve> curve; std::unique_ptr<VuMeter> vu; std::unique_ptr<Waterfall> water; std::unique_ptr<SquareVis> square;
    std::unique_ptr<LevelMeter> compGr, limGr;
    juce::OwnedArray<Panel> busP, slotP;
    juce::Array<Knob*> fxKnobs; juce::Array<Chip*> optChips;
    juce::OwnedArray<juce::Component> extras;
};
} // namespace cz
