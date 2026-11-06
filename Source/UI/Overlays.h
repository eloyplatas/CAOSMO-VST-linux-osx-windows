// Caosmo UI — preset browser, save dialog, settings
#pragma once
#include "Pages.h"

namespace cz
{
class Overlay : public juce::Component
{
public:
    Overlay (const juce::String& t, int w, int h) : title (t), pw (w), ph (h) { setWantsKeyboardFocus (true); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override { if (! panel().contains (e.getPosition())) close(); }
    bool keyPressed (const juce::KeyPress& k) override { if (k == juce::KeyPress::escapeKey) { close(); return true; } return false; }
    std::function<void()> onClose;
    void close() { if (onClose) onClose(); }
protected:
    juce::Rectangle<int> panel() const { return getLocalBounds().withSizeKeepingCentre (juce::jmin (pw, getWidth() - 20), juce::jmin (ph, getHeight() - 20)); }
    juce::Rectangle<int> content() const { return panel().reduced (12).withTrimmedTop (26); }
    virtual void layout (juce::Rectangle<int>) {}
    juce::String title; int pw, ph;
    juce::TextButton closeBtn { "x" };
};

class PresetBrowser : public Overlay, private juce::ListBoxModel
{
public:
    explicit PresetBrowser (Ctx& c);
private:
    void layout (juce::Rectangle<int>) override;
    void refilter();
    int getNumRows() override { return filtered.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void selectedRowsChanged (int) override { info.repaint(); }
    void userMenu (int presetIndex);
    Ctx& ctx; juce::TextEditor search; juce::ListBox list; juce::Array<int> filtered;
    juce::String category = "All"; juce::StringArray activeTags;
    juce::OwnedArray<ActionChip> cats, tags; TextLine info; juce::TextButton loadBtn { "LOAD" };
};

class SaveDialog : public Overlay
{
public:
    explicit SaveDialog (Ctx& c);
private:
    void layout (juce::Rectangle<int>) override;
    Ctx& ctx; juce::TextEditor name, author, desc, freeTags; juce::ComboBox category; juce::StringArray chosen;
    juce::OwnedArray<ActionChip> tagChips; juce::TextButton save { "SAVE" }, cancel { "CANCEL" };
    juce::Label lName, lCat, lAuthor, lDesc, lTags;
};

class SettingsPanel : public Overlay
{
public:
    SettingsPanel (Ctx& c, std::function<void()> applyUi);
private:
    void layout (juce::Rectangle<int>) override;
    Ctx& ctx; std::function<void()> apply;
    Panel ui, quality, voices, midi, presets, about;
    juce::ComboBox scale, wheel, theme; juce::OwnedArray<ActionChip> chips;
};
} // namespace cz
