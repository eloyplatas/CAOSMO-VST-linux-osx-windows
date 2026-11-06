#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "UI/Overlays.h"

namespace cz
{
class FloatingWindow : public juce::DocumentWindow
{
public:
    FloatingWindow (const juce::String& name, juce::Component* content, std::function<void (FloatingWindow*)> onClose);
    void closeButtonPressed() override { if (closed) closed (this); }
private:
    std::function<void (FloatingWindow*)> closed;
};

class CaosmoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CaosmoEditor (CaosmoProcessor&);
    ~CaosmoEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void showPage (int idx);
    int currentPage() const { return page; }
    void testOverlay (int which);   // used by the offline snapshot test
private:
    struct Root : juce::Component
    {
        CaosmoEditor& ed; explicit Root (CaosmoEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override; void resized() override;
    };
    void timerCallback() override;
    void layoutRoot();
    void applyUi();
    void presetMenu();
    void openOverlay (Overlay* o);
    juce::Component* makePage (int idx);
    void detach();

    static constexpr int kW = 1440, kH = 900;
    CaosmoProcessor& proc;
    UiPrefs prefs; LookAndFeelCz lnf; PresetManager pm; Ctx ctx;
    Root root { *this };
    juce::TooltipWindow tips { this, 600 };
    // top bar
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, nameBtn, saveBtn { "SAVE" }, saveAsBtn { "SAVE AS" }, browseBtn { "BROWSE" };
    juce::OwnedArray<juce::TextButton> tabs;
    juce::TextButton undoBtn { "UNDO" }, redoBtn { "REDO" }, detachBtn { "DETACH" }, setBtn { "SETTINGS" };
    std::unique_ptr<Knob> outKnob, glideKnob;
    std::unique_ptr<LevelMeter> meterL, meterR;
    // footer
    juce::OwnedArray<Knob> macros;
    std::unique_ptr<juce::Component> pageComp;
    std::unique_ptr<Overlay> overlay;
    juce::OwnedArray<FloatingWindow> floating;
    int page = 0;
};
} // namespace cz
