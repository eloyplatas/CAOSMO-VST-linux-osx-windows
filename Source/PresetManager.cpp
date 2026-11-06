#include "PresetManager.h"
#include <BinaryData.h>

using namespace juce;

namespace cz
{
PresetManager::PresetManager (CaosmoProcessor& p) : proc (p) { loadFavourites(); scan(); }

File PresetManager::rootDir()
{
    return File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Caosmo Presets");
}

ValueTree PresetManager::factoryBank()
{
    static ValueTree bank = []
    {
        auto xml = parseXML (String::fromUTF8 (BinaryData::factory_presets_xml, BinaryData::factory_presets_xmlSize));
        return xml ? ValueTree::fromXml (*xml) : ValueTree ("CaosmoFactory");
    }();
    return bank;
}

ValueTree PresetManager::wrap (const ValueTree& state, const PresetInfo& m)
{
    ValueTree t ("CaosmoPreset");
    t.setProperty ("name", m.name, nullptr); t.setProperty ("category", m.category, nullptr);
    t.setProperty ("author", m.author, nullptr); t.setProperty ("description", m.description, nullptr);
    t.setProperty ("tags", m.tags.joinIntoString (";"), nullptr); t.setProperty ("version", 1, nullptr);
    auto s = state.createCopy();
    s.setProperty ("presetName", m.name, nullptr); s.setProperty ("presetCategory", m.category, nullptr);
    s.setProperty ("presetKey", (m.factory ? "F:" : "U:") + m.category + "/" + m.name, nullptr);
    t.appendChild (s, nullptr);
    return t;
}

static PresetInfo infoFrom (const ValueTree& t)
{
    PresetInfo i;
    i.name = t.getProperty ("name").toString(); i.category = t.getProperty ("category").toString();
    i.author = t.getProperty ("author").toString(); i.description = t.getProperty ("description").toString();
    i.tags = StringArray::fromTokens (t.getProperty ("tags").toString(), ";", ""); i.tags.removeEmptyStrings();
    return i;
}

void PresetManager::scan()
{
    presets.clear();
    auto bank = factoryBank();
    for (int i = 0; i < bank.getNumChildren(); ++i)
    {
        auto info = infoFrom (bank.getChild (i)); info.factory = true; info.factoryIndex = i;
        presets.add (info);
    }
    for (auto& f : userDir().findChildFiles (File::findFiles, true, "*.caosmo"))
        if (auto xml = parseXML (f))
        {
            auto info = infoFrom (ValueTree::fromXml (*xml)); info.file = f;
            if (info.name.isEmpty()) info.name = f.getFileNameWithoutExtension();
            if (info.category.isEmpty()) info.category = f.getParentDirectory().getFileName();
            presets.add (info);
        }
}

StringArray PresetManager::categories() const
{
    StringArray c; for (auto& p : presets) c.addIfNotAlreadyThere (p.category);
    return c;
}

String PresetManager::currentName() const
{
    const auto n = proc.apvts.state.getProperty ("presetName").toString();
    return n.isEmpty() ? "Default" : n;
}

int PresetManager::indexOfCurrent() const
{
    const auto key = proc.apvts.state.getProperty ("presetKey").toString();
    for (int i = 0; i < presets.size(); ++i) if (presets[i].key() == key) return i;
    return -1;
}

bool PresetManager::load (int index)
{
    if (! isPositiveAndBelow (index, presets.size())) return false;
    const auto& p = presets.getReference (index);
    ValueTree wrapped;
    if (p.factory) wrapped = factoryBank().getChild (p.factoryIndex);
    else if (auto xml = parseXML (p.file)) wrapped = ValueTree::fromXml (*xml);
    if (! wrapped.isValid() || wrapped.getNumChildren() == 0) return false;
    // merge onto INIT so presets may store only what they change
    auto saved = wrapped.getChild (0);
    auto state = proc.getInitState().createCopy();
    for (int i = 0; i < saved.getNumChildren(); ++i)
    {
        auto c = saved.getChild (i);
        if (c.hasType ("PARAM"))
        {
            auto d = state.getChildWithProperty ("id", c.getProperty ("id"));
            if (d.isValid()) d.setProperty ("value", c.getProperty ("value"), nullptr);
        }
        else
        {
            auto old = state.getChildWithName (c.getType());
            if (old.isValid()) state.removeChild (old, nullptr);
            state.appendChild (c.createCopy(), nullptr);
        }
    }
    state.setProperty ("presetName", p.name, nullptr); state.setProperty ("presetCategory", p.category, nullptr); state.setProperty ("presetKey", p.key(), nullptr);
    proc.undo.beginNewTransaction();
    MemoryBlock mb; { std::unique_ptr<XmlElement> x (state.createXml()); AudioProcessor::copyXmlToBinary (*x, mb); }
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    return true;
}

void PresetManager::step (int d)
{
    if (presets.isEmpty()) return;
    int i = indexOfCurrent();
    i = i < 0 ? (d > 0 ? 0 : presets.size() - 1) : (i + d + presets.size()) % presets.size();
    load (i);
}

bool PresetManager::saveUser (const String& name, const String& category, const String& author, const String& description, const StringArray& tags)
{
    if (name.trim().isEmpty()) return false;
    PresetInfo m; m.name = name.trim(); m.category = category.trim().isEmpty() ? "User" : category.trim(); m.author = author; m.description = description; m.tags = tags;
    auto dir = userDir().getChildFile (File::createLegalFileName (m.category));
    dir.createDirectory();
    auto f = dir.getChildFile (File::createLegalFileName (m.name) + ".caosmo");
    auto state = proc.apvts.copyState();
    auto t = wrap (state, m);
    std::unique_ptr<XmlElement> xml (t.createXml());
    if (! xml || ! xml->writeTo (f)) return false;
    proc.apvts.state.setProperty ("presetName", m.name, nullptr);
    proc.apvts.state.setProperty ("presetCategory", m.category, nullptr);
    proc.apvts.state.setProperty ("presetKey", "U:" + m.category + "/" + m.name, nullptr);
    scan();
    return true;
}

bool PresetManager::overwriteCurrent()
{
    const int i = indexOfCurrent();
    if (i < 0 || presets[i].factory) return false;
    const auto p = presets[i];
    return saveUser (p.name, p.category, p.author, p.description, p.tags);
}

bool PresetManager::remove (int index)
{
    if (! isPositiveAndBelow (index, presets.size()) || presets[index].factory) return false;
    const bool ok = presets[index].file.deleteFile();
    scan(); return ok;
}

bool PresetManager::rename (int index, const String& newName)
{
    if (! isPositiveAndBelow (index, presets.size()) || presets[index].factory || newName.trim().isEmpty()) return false;
    auto p = presets[index];
    auto xml = parseXML (p.file); if (! xml) return false;
    auto t = ValueTree::fromXml (*xml); t.setProperty ("name", newName.trim(), nullptr);
    if (t.getNumChildren() > 0) t.getChild (0).setProperty ("presetName", newName.trim(), nullptr);
    auto nf = p.file.getSiblingFile (File::createLegalFileName (newName.trim()) + ".caosmo");
    std::unique_ptr<XmlElement> out (t.createXml());
    if (! out->writeTo (nf)) return false;
    if (nf != p.file) p.file.deleteFile();
    scan(); return true;
}

bool PresetManager::editInfo (int index, const String& category, const String& author, const String& description, const StringArray& tags)
{
    if (! isPositiveAndBelow (index, presets.size()) || presets[index].factory) return false;
    auto p = presets[index];
    auto xml = parseXML (p.file); if (! xml) return false;
    auto t = ValueTree::fromXml (*xml);
    t.setProperty ("category", category, nullptr); t.setProperty ("author", author, nullptr);
    t.setProperty ("description", description, nullptr); t.setProperty ("tags", tags.joinIntoString (";"), nullptr);
    auto dir = userDir().getChildFile (File::createLegalFileName (category.isEmpty() ? "User" : category)); dir.createDirectory();
    auto nf = dir.getChildFile (p.file.getFileName());
    std::unique_ptr<XmlElement> out (t.createXml());
    if (! out->writeTo (nf)) return false;
    if (nf != p.file) p.file.deleteFile();
    scan(); return true;
}

void PresetManager::resetToInit()
{
    proc.undo.beginNewTransaction();
    proc.resetToInit();
    proc.apvts.state.setProperty ("presetName", "Default", nullptr);
    proc.apvts.state.setProperty ("presetKey", "", nullptr);
}

void PresetManager::loadFavourites()
{
    favourites = StringArray::fromLines (rootDir().getChildFile ("favourites.txt").loadFileAsString());
    favourites.removeEmptyStrings();
}
void PresetManager::saveFavourites()
{
    rootDir().createDirectory();
    rootDir().getChildFile ("favourites.txt").replaceWithText (favourites.joinIntoString ("\n"));
}
void PresetManager::toggleFavourite (const PresetInfo& p)
{
    if (favourites.contains (p.key())) favourites.removeString (p.key()); else favourites.add (p.key());
    saveFavourites();
}

StringArray PresetManager::suggestTags() const
{
    static const char* map[kNumEngines + 1][2] = { { "", "" }, { "Glassy", "Airy" }, { "Gritty", "Organic" }, { "Organic", "Warm" }, { "Dark", "Airy" },
                                                   { "Noisy", "Gritty" }, { "Gritty", "Harsh" }, { "Noisy", "Organic" }, { "Warm", "Glassy" },
                                                   { "Airy", "Glassy" }, { "Organic", "Warm" }, { "Warm", "Gritty" }, { "Metallic", "Bright" } };
    StringArray t;
    for (int l = 0; l < kNumLanes; ++l)
    {
        const int e = proc.engineOfLane (l);
        if (e > 0 && e <= kNumEngines && proc.apvts.getRawParameterValue (laneId (l, "power"))->load() > 0.5f)
            for (auto* s : map[e]) t.addIfNotAlreadyThere (s);
    }
    return t;
}

StringArray PresetManager::userShapeNames() const
{
    StringArray n;
    for (auto& f : venvDir().findChildFiles (File::findFiles, false, "*.caosvenv")) n.add (f.getFileNameWithoutExtension());
    n.sort (true);
    return n;
}
bool PresetManager::saveUserShape (const String& name, const VShape& s)
{
    venvDir().createDirectory();
    return venvDir().getChildFile (File::createLegalFileName (name) + ".caosvenv").replaceWithText (s.toString());
}
bool PresetManager::loadUserShape (const String& name, VShape& s) const
{
    auto f = venvDir().getChildFile (File::createLegalFileName (name) + ".caosvenv");
    if (! f.existsAsFile()) return false;
    s = VShape::fromString (f.loadFileAsString()); return true;
}
} // namespace cz
