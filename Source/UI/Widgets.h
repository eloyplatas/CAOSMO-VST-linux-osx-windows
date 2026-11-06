// Caosmo UI — theme, shared context, controls
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"
#include "../PresetManager.h"

namespace cz
{
//==============================================================================
struct Theme
{
    juce::Colour bg, panel, panelHi, line, text, dim, accent, accent2, warn, red, green, blue;
    static Theme& c();
    static void set (int which);
    static juce::Colour engine (int e);
};

struct UiPrefs
{
    float scale = 1.f; int wheel = 0, theme = 0; bool visuals = true; int scopeMode[kNumLanes] {}; int camera = 0, squareMode = 0, palette = 0;
    void load(); void save() const;
};

class LookAndFeelCz : public juce::LookAndFeel_V4
{
public:
    LookAndFeelCz();
    void refresh();
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (12.5f); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::FontOptions (12.f, juce::Font::bold); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

//==============================================================================
struct Ctx
{
    CaosmoProcessor& proc; juce::AudioProcessorValueTreeState& s; PresetManager& pm; UiPrefs& prefs;
    int assignSource = -1;
    std::function<void()> openBrowser, openSave, refreshAll;
    std::function<void (juce::Component*, const juce::String&)> openFloating;

    Ctx (CaosmoProcessor& p, PresetManager& m, UiPrefs& u) : proc (p), s (p.apvts), pm (m), prefs (u) {}
    juce::RangedAudioParameter* param (const juce::String& id) const { return s.getParameter (id); }
    float norm (const juce::String& id) const { auto* p = param (id); return p ? p->getValue() : 0.f; }
    float real (const juce::String& id) const { auto* v = s.getRawParameterValue (id); return v ? v->load() : 0.f; }
    int  choice (const juce::String& id) const { return juce::roundToInt (real (id)); }
    void setNorm (const juce::String& id, float v) const;
    void setReal (const juce::String& id, float v) const { if (auto* p = param (id)) setNorm (id, p->convertTo0to1 (v)); }

    int destOf (const juce::String& paramId) const;
    juce::Array<int> slotsFor (int dest) const;
    int addRoute (int src, int dest, float amt);
    void removeRoute (int slot);
    static float defaultAmountFor (int dest);
    void buildModMenu (juce::PopupMenu& m, int dest, std::function<void()> afterChange);
    void handleModMenuResult (int r, int dest, const juce::String& pid);
    int learnBlinkSlot = -1;
};

//==============================================================================
// Rotary control bound to a parameter with modulation halo, right-click modulation menu, fine drag, typed entry
class Knob : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    Knob (Ctx& c, const juce::String& paramId, const juce::String& label, bool compact = false);
    ~Knob() override;
    void setLabel (const juce::String& l) { label = l; repaint(); }
    void setTextFn (std::function<juce::String (float)> f) { textFn = std::move (f); repaint(); }
    void setColour (juce::Colour c) { col = c; hasCol = true; }
    const juce::String& getParamId() const { return pid; }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void beginTyping();
private:
    void timerCallback() override;
    juce::String valueText() const;
    Ctx& ctx; juce::String pid, label; bool compact; juce::RangedAudioParameter* p;
    std::unique_ptr<juce::ParameterAttachment> att; float shown = 0, startV = 0; juce::Point<float> startPos; bool dragging = false;
    std::function<juce::String (float)> textFn; juce::Colour col; bool hasCol = false; float lastLive = 0, lastRange = 0;
    std::unique_ptr<juce::TextEditor> editor;
};

class Choice : public juce::Component
{
public:
    Choice (Ctx& c, const juce::String& paramId, const juce::String& label);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::ComboBox box;
    std::function<void (int)> onUserChange;
private:
    juce::String label; std::unique_ptr<juce::ComboBoxParameterAttachment> att;
};

class Chip : public juce::Component
{
public:
    Chip (Ctx& c, const juce::String& paramId, const juce::String& text, juce::Colour on = {});
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh() { repaint(); }
private:
    Ctx& ctx; juce::String pid, text; juce::Colour onCol;
};

// Plain clickable chip not bound to a parameter
class ActionChip : public juce::Component
{
public:
    ActionChip (const juce::String& t, std::function<void()> f) : text (t), fn (std::move (f)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (fn) fn(); }
    juce::String text; std::function<void()> fn; std::function<bool()> isOn;
};

//==============================================================================
// Titled container. Its title is a menu (Copy / Paste / Vary / Randomise [/ Copy to Lane]).
class Panel : public juce::Component
{
public:
    Panel (Ctx& c, const juce::String& title, int laneIndex = -1, bool laneMenu = false);
    juce::String title; juce::Colour stripe;
    Knob* knob (const juce::String& id, const juce::String& label);
    Choice* choice (const juce::String& id, const juce::String& label);
    Chip* chip (const juce::String& id, const juce::String& text);
    void addCustom (juce::Component* c) { addAndMakeVisible (c); flow.add (c); }
    void clearControls();
    void registerIds (const juce::StringArray& ids) { pids.addArray (ids); }
    int layoutFlow (int width);          // returns height
    void resized() override {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::OwnedArray<juce::Component> owned; juce::Array<juce::Component*> flow; juce::StringArray pids;
    juce::Rectangle<int> headerExtra;    // space reserved in the title bar for chips
    static constexpr int titleH = 22;
private:
    void showMenu();
    Ctx& ctx; int lane; bool laneMenu;
};
} // namespace cz
