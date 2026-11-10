#include "Overlays.h"
#include <BinaryData.h>

using namespace juce;

namespace cz
{
void Overlay::paint (Graphics& g)
{
    auto& t = Theme::c();
    g.fillAll (Colours::black.withAlpha (0.55f));
    auto p = panel().toFloat();
    g.setColour (t.panel); g.fillRoundedRectangle (p, 8.f);
    g.setColour (t.line); g.drawRoundedRectangle (p.reduced (0.5f), 8.f, 1.f);
    g.setColour (t.accent2); g.setFont (FontOptions (14.f, Font::bold));
    g.drawText (title, panel().reduced (14, 8).removeFromTop (20), Justification::centredLeft);
}
void Overlay::resized()
{
    closeBtn.setBounds (panel().getRight() - 34, panel().getY() + 6, 26, 22);
    if (closeBtn.getParentComponent() == nullptr) { addAndMakeVisible (closeBtn); closeBtn.onClick = [this] { close(); }; }
    layout (content());
}

//==============================================================================
PresetBrowser::PresetBrowser (Ctx& c)
    : Overlay ("PRESETS", 1000, 690), ctx (c),
      info ([this]
      {
          const int r = list.getSelectedRow();
          if (! isPositiveAndBelow (r, filtered.size())) return String ("Select a preset to see its details. Double-click to load.");
          const auto& p = ctx.pm.all()[filtered[r]];
          return p.name + "  -  " + p.category + (p.factory ? "  (factory)" : "  (user)") + "\nAuthor: " + p.author + "\n" + p.description + "\nTags: " + p.tags.joinIntoString (", ");
      }, 11.f)
{
    ctx.pm.scan();
    addAndMakeVisible (search); search.setTextToShowWhenEmpty ("Search names, authors, descriptions, tags...", Theme::c().dim);
    search.onTextChange = [this] { refilter(); };
    list.setModel (this); list.setRowHeight (26); list.setColour (ListBox::backgroundColourId, Theme::c().bg);
    addAndMakeVisible (list); addAndMakeVisible (info); addAndMakeVisible (loadBtn);
    loadBtn.onClick = [this] { const int r = list.getSelectedRow(); if (isPositiveAndBelow (r, filtered.size())) { ctx.pm.load (filtered[r]); close(); } };
    StringArray catNames { "All", "Favorites" }; catNames.addArray (ctx.pm.categories());
    for (auto& cn : catNames)
    {
        int n = 0; for (auto& p : ctx.pm.all()) if (cn == "All" || p.category == cn || (cn == "Favorites" && ctx.pm.isFavourite (p))) ++n;
        auto* ch = cats.add (new ActionChip (cn + "  (" + String (n) + ")", [this, cn] { category = cn; refilter(); for (auto* k : cats) k->repaint(); }));
        ch->isOn = [this, cn] { return category == cn; };
        addAndMakeVisible (ch);
    }
    auto addTags = [this] (const char* const* v, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const String tg (v[i]);
            auto* ch = tags.add (new ActionChip (tg, [this, tg] { if (activeTags.contains (tg)) activeTags.removeString (tg); else activeTags.add (tg); refilter(); for (auto* k : tags) k->repaint(); }));
            ch->isOn = [this, tg] { return activeTags.contains (tg); };
            addAndMakeVisible (ch);
        }
    };
    addTags (kTagTypes, kNumTagTypes); addTags (kTagCharacter, kNumTagCharacter); addTags (kTagMotion, kNumTagMotion);
    refilter();
    const int cur = ctx.pm.indexOfCurrent();
    if (cur >= 0) { const int r = filtered.indexOf (cur); if (r >= 0) list.selectRow (r); }
}

void PresetBrowser::layout (Rectangle<int> r)
{
    auto side = r.removeFromLeft (230); r.removeFromLeft (10);
    int y = side.getY();
    for (auto* c : cats) { c->setBounds (side.getX(), y, side.getWidth(), 20); y += 22; }
    y += 8;
    int x = side.getX();
    const int tw = (side.getWidth() - 6) / 4;
    for (int i = 0; i < tags.size(); ++i)
    {
        if (i == kNumTagTypes || i == kNumTagTypes + kNumTagCharacter) { x = side.getX(); y += 26; }
        if (x + tw > side.getRight() + 1) { x = side.getX(); y += 21; }
        tags[i]->setBounds (x, y, tw - 2, 19); x += tw;
    }
    search.setBounds (r.removeFromTop (28)); r.removeFromTop (8);
    auto bottom = r.removeFromBottom (74);
    loadBtn.setBounds (bottom.removeFromRight (90).withSizeKeepingCentre (84, 30));
    info.setBounds (bottom);
    list.setBounds (r);
}

void PresetBrowser::refilter()
{
    filtered.clear();
    const auto q = search.getText().trim().toLowerCase();
    const auto& all = ctx.pm.all();
    for (int i = 0; i < all.size(); ++i)
    {
        const auto& p = all.getReference (i);
        if (category == "Favorites" && ! ctx.pm.isFavourite (p)) continue;
        if (category != "All" && category != "Favorites" && p.category != category) continue;
        bool ok = true; for (auto& t : activeTags) if (! p.tags.contains (t, true)) ok = false;
        if (! ok) continue;
        if (q.isNotEmpty() && ! (p.name + " " + p.author + " " + p.description + " " + p.category + " " + p.tags.joinIntoString (" ")).toLowerCase().contains (q)) continue;
        filtered.add (i);
    }
    list.updateContent(); list.repaint(); info.repaint();
}

void PresetBrowser::paintListBoxItem (int row, Graphics& g, int w, int h, bool sel)
{
    if (! isPositiveAndBelow (row, filtered.size())) return;
    auto& t = Theme::c();
    const auto& p = ctx.pm.all()[filtered[row]];
    if (sel) g.fillAll (t.accent.withAlpha (0.18f)); else if (row % 2) g.fillAll (t.panelHi.withAlpha (0.3f));
    g.setColour (t.text); g.setFont (FontOptions (13.f)); g.drawText (p.name, 10, 0, w / 2, h, Justification::centredLeft);
    g.setColour (t.dim); g.setFont (FontOptions (11.f)); g.drawText (p.category, w / 2, 0, w / 4, h, Justification::centredLeft);
    if (p.factory) { g.setColour (t.accent2.withAlpha (0.8f)); g.drawText ("FACTORY", w * 3 / 4 - 10, 0, 70, h, Justification::centredLeft); }
    const bool fav = ctx.pm.isFavourite (p);
    Path heart; const float cx = (float) w - 18.f, cy = h * 0.5f;
    heart.startNewSubPath (cx, cy + 5); heart.cubicTo (cx - 9, cy - 2, cx - 4, cy - 9, cx, cy - 3); heart.cubicTo (cx + 4, cy - 9, cx + 9, cy - 2, cx, cy + 5);
    if (fav) { g.setColour (t.red); g.fillPath (heart); } else { g.setColour (t.dim); g.strokePath (heart, PathStrokeType (1.2f)); }
}

void PresetBrowser::listBoxItemClicked (int row, const MouseEvent& e)
{
    if (! isPositiveAndBelow (row, filtered.size())) return;
    const int idx = filtered[row];
    if (e.mods.isPopupMenu()) { if (! ctx.pm.all()[idx].factory) userMenu (idx); return; }
    if (e.x > list.getWidth() - 34) { ctx.pm.toggleFavourite (ctx.pm.all()[idx]); list.repaint(); }
}
void PresetBrowser::listBoxItemDoubleClicked (int row, const MouseEvent&)
{
    if (isPositiveAndBelow (row, filtered.size())) { ctx.pm.load (filtered[row]); close(); }
}

void PresetBrowser::userMenu (int idx)
{
    PopupMenu m; m.addItem (1, "Edit Info..."); m.addItem (2, "Rename..."); m.addItem (3, "Delete");
    m.showMenuAsync (PopupMenu::Options(), [this, idx] (int r)
    {
        const auto p = ctx.pm.all()[idx];
        if (r == 1)
        {
            auto* w = new AlertWindow ("Edit preset info", p.name, MessageBoxIconType::NoIcon);
            w->addTextEditor ("cat", p.category, "Category"); w->addTextEditor ("author", p.author, "Author");
            w->addTextEditor ("desc", p.description, "Description"); w->addTextEditor ("tags", p.tags.joinIntoString (";"), "Tags (;)");
            w->addButton ("Save", 1); w->addButton ("Cancel", 0);
            w->enterModalState (true, ModalCallbackFunction::create ([this, w, idx] (int res)
            {
                if (res == 1) { auto tg = StringArray::fromTokens (w->getTextEditorContents ("tags"), ";", ""); tg.trim(); tg.removeEmptyStrings();
                                ctx.pm.editInfo (idx, w->getTextEditorContents ("cat"), w->getTextEditorContents ("author"), w->getTextEditorContents ("desc"), tg); refilter(); }
            }), true);
        }
        else if (r == 2)
        {
            auto* w = new AlertWindow ("Rename preset", "New name:", MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", p.name); w->addButton ("Rename", 1); w->addButton ("Cancel", 0);
            w->enterModalState (true, ModalCallbackFunction::create ([this, w, idx] (int res) { if (res == 1) { ctx.pm.rename (idx, w->getTextEditorContents ("name")); refilter(); } }), true);
        }
        else if (r == 3)
        {
            auto opts = MessageBoxOptions().withIconType (MessageBoxIconType::WarningIcon).withTitle ("Delete preset").withMessage ("Delete \"" + p.name + "\"?").withButton ("Delete").withButton ("Cancel");
            AlertWindow::showAsync (opts, [this, idx] (int res) { if (res == 1) { ctx.pm.remove (idx); refilter(); } });
        }
    });
}

//==============================================================================
SaveDialog::SaveDialog (Ctx& c) : Overlay ("SAVE PRESET AS", 640, 470), ctx (c)
{
    auto lab = [this] (Label& l, const String& t) { l.setText (t, dontSendNotification); l.setColour (Label::textColourId, Theme::c().dim); addAndMakeVisible (l); };
    lab (lName, "Name"); lab (lCat, "Category"); lab (lAuthor, "Author"); lab (lDesc, "Description"); lab (lTags, "Tags");
    const int cur = ctx.pm.indexOfCurrent();
    const PresetInfo info = cur >= 0 ? ctx.pm.all()[cur] : PresetInfo();
    name.setText (cur >= 0 ? info.name + " 2" : String ("My Preset")); author.setText (info.author.isEmpty() || info.factory ? String ("User") : info.author);
    desc.setText (info.description); desc.setMultiLine (true);
    category.setEditableText (true); category.addItemList (ctx.pm.categories(), 1); category.setText (info.category.isEmpty() || info.factory ? String ("User") : info.category);
    for (auto* e : { &name, &author, &desc, &freeTags }) addAndMakeVisible (e);
    addAndMakeVisible (category);
    chosen = info.tags;
    if (chosen.isEmpty()) chosen = ctx.pm.suggestTags();
    auto addChips = [this] (const char* const* v, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const String tg (v[i]);
            auto* ch = tagChips.add (new ActionChip (tg, [this, tg] { if (chosen.contains (tg)) chosen.removeString (tg); else chosen.add (tg); for (auto* k : tagChips) k->repaint(); }));
            ch->isOn = [this, tg] { return chosen.contains (tg); }; addAndMakeVisible (ch);
        }
    };
    addChips (kTagTypes, kNumTagTypes); addChips (kTagCharacter, kNumTagCharacter); addChips (kTagMotion, kNumTagMotion);
    freeTags.setTextToShowWhenEmpty ("your own tags, separated by ;", Theme::c().dim);
    addAndMakeVisible (save); addAndMakeVisible (cancel);
    cancel.onClick = [this] { close(); };
    save.onClick = [this]
    {
        StringArray tg = chosen; auto extra = StringArray::fromTokens (freeTags.getText(), ";", ""); extra.trim(); extra.removeEmptyStrings(); tg.addArray (extra);
        if (ctx.pm.saveUser (name.getText(), category.getText(), author.getText(), desc.getText(), tg)) close();
    };
}
void SaveDialog::layout (Rectangle<int> r)
{
    auto row = [&] (Label& l, Component& c, int h) { auto a = r.removeFromTop (h); l.setBounds (a.removeFromLeft (90)); c.setBounds (a); r.removeFromTop (6); };
    row (lName, name, 26); row (lCat, category, 26); row (lAuthor, author, 26); row (lDesc, desc, 60);
    auto tagsArea = r.removeFromTop (140); lTags.setBounds (tagsArea.removeFromLeft (90).withHeight (24));
    int x = tagsArea.getX(), y = tagsArea.getY();
    for (int i = 0; i < tagChips.size(); ++i)
    {
        if (i == kNumTagTypes || i == kNumTagTypes + kNumTagCharacter) { x = tagsArea.getX(); y += 28; }
        if (x + 84 > tagsArea.getRight()) { x = tagsArea.getX(); y += 24; }
        tagChips[i]->setBounds (x, y, 80, 22); x += 84;
    }
    freeTags.setBounds (r.removeFromTop (26).withTrimmedLeft (90)); r.removeFromTop (10);
    auto b = r.removeFromTop (32);
    save.setBounds (b.removeFromRight (100)); b.removeFromRight (8); cancel.setBounds (b.removeFromRight (100));
}

//==============================================================================
SettingsPanel::SettingsPanel (Ctx& c, std::function<void()> a)
    : Overlay ("SETTINGS", 900, 600), ctx (c), apply (std::move (a)),
      ui (c, "UI SIZE & INTERFACE"), quality (c, "QUALITY"), voices (c, "VOICES / GLIDE / TUNING"), midi (c, "MIDI / MPE"), presets (c, "PRESETS"), about (c, "ABOUT")
{
    for (auto* p : { &ui, &quality, &voices, &midi, &presets, &about }) addAndMakeVisible (p);
    const int sc[] = { 75, 90, 100, 110, 125, 150, 175, 200 };
    for (int i = 0; i < 8; ++i) scale.addItem (String (sc[i]) + "%", sc[i]);
    scale.setSelectedId (roundToInt (ctx.prefs.scale * 100.f), dontSendNotification);
    if (scale.getSelectedId() == 0) scale.setSelectedId (100, dontSendNotification);
    scale.onChange = [this] { ctx.prefs.scale = scale.getSelectedId() / 100.f; ctx.prefs.save(); if (apply) MessageManager::callAsync (apply); };
    wheel.addItemList ({ "Scroll wheel: Fine", "Scroll wheel: Coarse", "Scroll wheel: Off" }, 1); wheel.setSelectedItemIndex (ctx.prefs.wheel, dontSendNotification);
    wheel.onChange = [this] { ctx.prefs.wheel = wheel.getSelectedItemIndex(); ctx.prefs.save(); };
    theme.addItemList ({ "Background: Midnight", "Background: Graphite" }, 1); theme.setSelectedItemIndex (ctx.prefs.theme, dontSendNotification);
    theme.onChange = [this] { ctx.prefs.theme = theme.getSelectedItemIndex(); ctx.prefs.save(); if (apply) MessageManager::callAsync (apply); };
    scale.setSize (120, 26); wheel.setSize (180, 26); theme.setSize (190, 26);
    ui.addCustom (&scale); ui.addCustom (&wheel); ui.addCustom (&theme);
    auto* vis = chips.add (new ActionChip ("VISUALISERS", [this] { ctx.prefs.visuals = ! ctx.prefs.visuals; ctx.prefs.save(); for (auto* k : chips) k->repaint(); }));
    vis->isOn = [this] { return ctx.prefs.visuals; }; vis->setSize (110, 24); quality.addCustom (vis);
    quality.chip ("hq", "ENGINE HQ")->setSize (100, 24);
    voices.choice ("voices", "Voices")->setSize (70, 40); voices.chip ("legato", "LEGATO")->setSize (70, 22);
    voices.choice ("glide_mode", "Glide Mode")->setSize (96, 40); voices.knob ("glide", "Glide"); voices.knob ("tune", "A =");
    midi.choice ("mpe", "MPE")->setSize (76, 40); midi.knob ("pb_range", "PB Global"); midi.knob ("pb_note", "PB Note");
    for (int l = 0; l < kNumLanes; ++l) midi.choice (laneId (l, "chan"), "Lane " + String (l + 1) + " Ch")->setSize (80, 40);
    auto* of = chips.add (new ActionChip ("OPEN PRESETS FOLDER", [] { PresetManager::userDir().createDirectory(); PresetManager::rootDir().revealToUser(); })); of->setSize (170, 24); presets.addCustom (of);
    auto* ri = chips.add (new ActionChip ("RESET TO INIT", [this]
    {
        auto opts = MessageBoxOptions().withIconType (MessageBoxIconType::QuestionIcon).withTitle ("Reset to INIT").withMessage ("Replace the current patch with the initial one?").withButton ("Reset").withButton ("Cancel");
        AlertWindow::showAsync (opts, [this] (int r) { if (r == 1) ctx.pm.resetToInit(); });
    }));
    ri->setSize (130, 24); presets.addCustom (ri);
    auto* txt = new TextLine ([] { return String ("Caosmo 0.5  -  polyphonic chaos synthesizer  -  Eloy Platas  -  GNU GPL v3\nBuilt with JUCE. DELUGE FX: effects from the Synthstrom Audible Deluge firmware (GPL-3.0).\nv0.5 generators after SuperCollider UGens (Xenakis/Collins Gendy, SLUGens, MCLDUGens, ChaosUGens, BhobUGens), GPL."); }, 11.f);
    about.owned.add (txt); txt->setSize (420, 40); about.addCustom (txt);
    auto* man = chips.add (new ActionChip ("OPEN MANUAL", []
    {
        auto f = File::getSpecialLocation (File::tempDirectory).getChildFile ("Caosmo Manual.html");
        f.replaceWithData (BinaryData::manual_html, (size_t) BinaryData::manual_htmlSize);
        URL (f).launchInDefaultBrowser();
    }));
    man->setSize (120, 24); about.addCustom (man);
}
void SettingsPanel::layout (Rectangle<int> r)
{
    const int cw = (r.getWidth() - 10) / 2;
    int yl = r.getY(), yr = r.getY();
    for (auto* p : { &ui, &quality, &voices }) { const int h = p->layoutFlow (cw); p->setBounds (r.getX(), yl, cw, h); yl += h + 8; }
    for (auto* p : { &midi, &presets, &about }) { const int h = p->layoutFlow (cw); p->setBounds (r.getX() + cw + 10, yr, cw, h); yr += h + 8; }
}
} // namespace cz
