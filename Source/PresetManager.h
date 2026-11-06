// Caosmo — presets: factory bank (embedded), user presets on disk, favourites, tags, VENV shape files
#pragma once
#include "PluginProcessor.h"

namespace cz
{
struct PresetInfo
{
    juce::String name, category, author, description; juce::StringArray tags;
    bool factory = false; juce::File file; int factoryIndex = -1;
    juce::String key() const { return (factory ? "F:" : "U:") + category + "/" + name; }
};

class PresetManager
{
public:
    explicit PresetManager (CaosmoProcessor& p);

    static juce::File rootDir();
    static juce::File userDir() { return rootDir().getChildFile ("User"); }
    static juce::File venvDir() { return rootDir().getChildFile ("VENV"); }

    void scan();
    const juce::Array<PresetInfo>& all() const { return presets; }
    juce::StringArray categories() const;
    int indexOfCurrent() const;
    bool load (int index);
    void step (int delta);
    bool saveUser (const juce::String& name, const juce::String& category, const juce::String& author,
                   const juce::String& description, const juce::StringArray& tags);
    bool overwriteCurrent();          // false if the current preset is factory or none
    bool remove (int index);
    bool rename (int index, const juce::String& newName);
    bool editInfo (int index, const juce::String& category, const juce::String& author, const juce::String& description, const juce::StringArray& tags);
    void resetToInit();

    bool isFavourite (const PresetInfo& p) const { return favourites.contains (p.key()); }
    void toggleFavourite (const PresetInfo& p);

    juce::String currentName() const;
    juce::StringArray suggestTags() const;     // character tags from the active engines

    // VENV user shapes
    juce::StringArray userShapeNames() const;
    bool saveUserShape (const juce::String& name, const VShape& s);
    bool loadUserShape (const juce::String& name, VShape& s) const;

    // shared helpers (used by the factory generator too)
    static juce::ValueTree wrap (const juce::ValueTree& state, const PresetInfo& meta);
    static juce::ValueTree factoryBank();      // parsed embedded bank

private:
    void loadFavourites(); void saveFavourites();
    CaosmoProcessor& proc;
    juce::Array<PresetInfo> presets; juce::StringArray favourites;
};
} // namespace cz
