#include "Widgets.h"
#include <map>

using namespace juce;

namespace cz
{
//==============================================================================
static Theme makeTheme (int which)
{
    Theme t;
    if (which == 1)
    {   // Graphite
        t.bg = Colour (0xff181818); t.panel = Colour (0xff222222); t.panelHi = Colour (0xff2c2c2c); t.line = Colour (0xff3a3a3a);
        t.text = Colour (0xffe2e2e2); t.dim = Colour (0xff939393);
    }
    else
    {   // Midnight
        t.bg = Colour (0xff111318); t.panel = Colour (0xff1a1d24); t.panelHi = Colour (0xff232733); t.line = Colour (0xff2e3340);
        t.text = Colour (0xffd8dce4); t.dim = Colour (0xff8790a2);
    }
    t.accent = Colour (0xff3fd0c9); t.accent2 = Colour (0xff9b8cff); t.warn = Colour (0xfff2b84b);
    t.red = Colour (0xffff5c5c); t.green = Colour (0xff5ad17a); t.blue = Colour (0xff5aa9ff);
    return t;
}
Theme& Theme::c() { static Theme t = makeTheme (0); return t; }
void Theme::set (int which) { c() = makeTheme (which); }
Colour Theme::engine (int e)
{
    static const uint32 cols[18] = { 0xff5a6070, 0xff3fd0c9, 0xffe0875a, 0xff6fb7ff, 0xff9b8cff, 0xffc9b458, 0xffff6f91,
                                     0xff7ad36b, 0xffffc46b, 0xff4fe0a8, 0xffd59a6a, 0xffff8a4c, 0xffb0c4d8,
                                     0xffe86bd3, 0xff8fd14f, 0xff5fd7ff, 0xffc0c0c0, 0xfff2a65a };
    return Colour (cols[jlimit (0, 17, e)]);
}

static std::unique_ptr<PropertiesFile> openProps()
{
    PropertiesFile::Options o; o.applicationName = "Caosmo"; o.filenameSuffix = "settings"; o.folderName = "Caosmo"; o.osxLibrarySubFolder = "Application Support";
    return std::make_unique<PropertiesFile> (o);
}
void UiPrefs::load()
{
    auto p = openProps();
    scale = (float) p->getDoubleValue ("scale", 1.0); wheel = p->getIntValue ("wheel", 0); theme = p->getIntValue ("theme", 0);
    visuals = p->getBoolValue ("visuals", true); camera = p->getIntValue ("camera", 0); squareMode = p->getIntValue ("square", 0); palette = p->getIntValue ("palette", 0);
    for (int l = 0; l < kNumLanes; ++l) scopeMode[l] = p->getIntValue ("scope" + String (l), 0);
    scale = jlimit (0.75f, 2.f, scale);
}
void UiPrefs::save() const
{
    auto p = openProps();
    p->setValue ("scale", scale); p->setValue ("wheel", wheel); p->setValue ("theme", theme); p->setValue ("visuals", visuals);
    p->setValue ("camera", camera); p->setValue ("square", squareMode); p->setValue ("palette", palette);
    for (int l = 0; l < kNumLanes; ++l) p->setValue ("scope" + String (l), scopeMode[l]);
    p->saveIfNeeded();
}

//==============================================================================
LookAndFeelCz::LookAndFeelCz() { refresh(); }
void LookAndFeelCz::refresh()
{
    auto& t = Theme::c();
    setColour (ResizableWindow::backgroundColourId, t.bg);
    setColour (Label::textColourId, t.text);
    setColour (ComboBox::backgroundColourId, t.panelHi); setColour (ComboBox::outlineColourId, t.line);
    setColour (ComboBox::textColourId, t.text); setColour (ComboBox::arrowColourId, t.accent);
    setColour (PopupMenu::backgroundColourId, t.panel); setColour (PopupMenu::textColourId, t.text);
    setColour (PopupMenu::highlightedBackgroundColourId, t.accent.withAlpha (0.25f)); setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (PopupMenu::headerTextColourId, t.accent2);
    setColour (TextButton::buttonColourId, t.panel); setColour (TextButton::buttonOnColourId, t.accent.withAlpha (0.2f));
    setColour (TextButton::textColourOffId, t.dim); setColour (TextButton::textColourOnId, t.accent);
    setColour (TextEditor::backgroundColourId, t.panelHi); setColour (TextEditor::textColourId, t.text);
    setColour (TextEditor::outlineColourId, t.line); setColour (TextEditor::focusedOutlineColourId, t.accent);
    setColour (ScrollBar::thumbColourId, t.line); setColour (ToggleButton::textColourId, t.text); setColour (ToggleButton::tickColourId, t.accent);
    setColour (TooltipWindow::backgroundColourId, t.panelHi); setColour (TooltipWindow::textColourId, t.text); setColour (TooltipWindow::outlineColourId, t.line);
    setColour (ListBox::backgroundColourId, t.panel); setColour (AlertWindow::backgroundColourId, t.panel); setColour (AlertWindow::textColourId, t.text);
}
void LookAndFeelCz::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& b)
{
    auto& t = Theme::c();
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (b.isMouseOver() ? t.panelHi.brighter (0.06f) : t.panelHi); g.fillRoundedRectangle (r, 4.f);
    g.setColour (t.line); g.drawRoundedRectangle (r, 4.f, 1.f);
    Path a; const float cx = (float) w - 10.f, cy = (float) h * 0.5f;
    a.startNewSubPath (cx - 3.5f, cy - 1.5f); a.lineTo (cx, cy + 2.f); a.lineTo (cx + 3.5f, cy - 1.5f);
    g.setColour (t.accent); g.strokePath (a, PathStrokeType (1.5f));
}
void LookAndFeelCz::positionComboBoxText (ComboBox& b, Label& l)
{
    l.setBounds (4, 0, b.getWidth() - 18, b.getHeight());
    l.setFont (getComboBoxFont (b));
}
void LookAndFeelCz::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool)
{
    auto& t = Theme::c();
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    g.setColour (b.getToggleState() ? t.accent.withAlpha (0.16f) : (over ? t.panelHi : t.panel));
    g.fillRoundedRectangle (r, 4.f);
    if (b.getToggleState()) { g.setColour (t.accent); g.fillRect (r.removeFromBottom (2.f)); }
}
void LookAndFeelCz::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (Theme::c().panel);
    g.setColour (Theme::c().line); g.drawRect (0, 0, w, h);
}

//==============================================================================
void Ctx::setNorm (const String& id, float v) const
{
    if (auto* p = param (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (jlimit (0.f, 1.f, v)); p->endChangeGesture(); }
}

int Ctx::destOf (const String& id) const
{
    static std::map<String, int> fixed = []
    {
        std::map<String, int> m;
        for (int d = 1; d < kNumDests; ++d) { auto pid = destParamId (d, nullptr); if (pid.isNotEmpty()) m[pid] = d; }
        return m;
    }();
    auto it = fixed.find (id);
    if (it != fixed.end()) return it->second;
    if (id.length() > 3 && id[0] == 'L' && id[2] == '_')
    {
        const int l = id[1] - '1';
        if (l < 0 || l >= kNumLanes) return -1;
        const int e = proc.engineOfLane (l);
        if (e <= 0) return -1;
        const auto& sp = engineSpecs()[(size_t) e];
        for (int k = 0; k < jmin (kMotorDests, (int) sp.p.size()); ++k)
            if (engId (l, e, sp.p[(size_t) k].key) == id) return kDestLaneBase + l * LD_Count + LD_M1 + k;
    }
    return -1;
}

Array<int> Ctx::slotsFor (int dest) const
{
    Array<int> r;
    if (dest <= 0) return r;
    for (int i = 0; i < kNumSlots; ++i) if (choice (slotId (i, "dst")) == dest && choice (slotId (i, "src")) > 0) r.add (i);
    return r;
}

float Ctx::defaultAmountFor (int dest)
{
    const bool pitch = dest >= kDestLaneBase && dest < kDestLfoRate && ((dest - kDestLaneBase) % LD_Count) == LD_Pitch;
    return pitch ? 0.02f : 0.25f;
}

int Ctx::addRoute (int src, int dest, float amt)
{
    for (int i = 0; i < kNumSlots; ++i)
        if (choice (slotId (i, "src")) == 0)
        {
            proc.undo.beginNewTransaction();
            for (auto* k : { "pol", "curve", "lag", "fin", "fout", "jit", "quant" }) if (auto* p = param (slotId (i, k))) setNorm (slotId (i, k), p->getDefaultValue());
            setReal (slotId (i, "dst"), (float) dest);
            setReal (slotId (i, "amt"), amt);
            setReal (slotId (i, "src"), (float) src);
            return i;
        }
    return -1;
}

void Ctx::removeRoute (int slot)
{
    proc.undo.beginNewTransaction();
    setReal (slotId (slot, "src"), 0.f); setReal (slotId (slot, "dst"), 0.f); setReal (slotId (slot, "amt"), 0.f);
}

void Ctx::buildModMenu (PopupMenu& m, int dest, std::function<void()>)
{
    const auto srcs = sourceNames();
    if (dest > 0)
    {
        PopupMenu with;
        for (auto grp : { "LFOs", "VENV", "Attractors", "Lane Envelopes", "MIDI / MPE", "Other" })
        {
            PopupMenu sub;
            for (int s = 1; s < S_Count; ++s)
                if (sourceGroup (s) == grp && ! (s >= S_Macro1 && s < S_Macro1 + kNumMacros) && s != S_CC) sub.addItem (1000 + s, srcs[s]);
            if (String (grp) == "MIDI / MPE") { sub.addSeparator(); sub.addItem (4000, learnBlinkSlot >= 0 && proc.learnSlot.load() >= 0 ? "Cancel MIDI Learn" : "MIDI Learn (CC)"); }
            with.addSubMenu (grp, sub);
        }
        m.addSubMenu ("Modulate with", with);
        PopupMenu mac; for (int i = 0; i < kNumMacros; ++i) mac.addItem (2000 + i, "M" + String (i + 1));
        m.addSubMenu ("Send to Macro", mac);
        auto slots = slotsFor (dest);
        PopupMenu con;
        for (int sl : slots)
        {
            const float a = real (slotId (sl, "amt"));
            con.addItem (3000 + sl, "Remove  " + srcs[choice (slotId (sl, "src"))] + "  (" + (a >= 0 ? "+" : "") + String (a, 2) + ")");
        }
        m.addSubMenu ("Connected (" + String (slots.size()) + ")", con, slots.size() > 0);
        m.addSeparator();
    }
    m.addItem (5000, "Reset to default");
    m.addItem (5001, "Enter value...");
}

void Ctx::handleModMenuResult (int r, int dest, const String& pid)
{
    if (r >= 1000 && r < 2000) addRoute (r - 1000, dest, defaultAmountFor (dest));
    else if (r >= 2000 && r < 3000) addRoute (S_Macro1 + (r - 2000), dest, 0.5f);
    else if (r >= 3000 && r < 4000) removeRoute (r - 3000);
    else if (r == 4000)
    {
        if (proc.learnSlot.load() >= 0) { const int s = proc.learnSlot.exchange (-1); if (s >= 0) removeRoute (s); learnBlinkSlot = -1; }
        else { const int s = addRoute (S_CC, dest, 1.f); if (s >= 0) { proc.learnSlot = s; learnBlinkSlot = s; } }
    }
    else if (r == 5000) { if (auto* p = param (pid)) { proc.undo.beginNewTransaction(); setNorm (pid, p->getDefaultValue()); } }
}

//==============================================================================
Knob::Knob (Ctx& c, const String& paramId, const String& l, bool comp) : ctx (c), pid (paramId), label (l), compact (comp)
{
    p = ctx.param (pid);
    jassert (p != nullptr);
    att = std::make_unique<ParameterAttachment> (*p, [this] (float v) { shown = p->convertTo0to1 (v); repaint(); }, &ctx.proc.undo);
    att->sendInitialUpdate();
    setSize (compact ? 30 : 58, compact ? 30 : 66);
    setTooltip (p->getName (64));
    startTimerHz (20);
}
Knob::~Knob() { stopTimer(); }

String Knob::valueText() const
{
    if (textFn) return textFn (shown);
    return p->getCurrentValueAsText();
}

void Knob::timerCallback()
{
    const int d = ctx.destOf (pid);
    float live = 0, range = 0;
    if (d > 0)
    {
        live = ctx.proc.ui.modLive[d].load();
        for (int s : ctx.slotsFor (d)) range += std::abs (ctx.real (slotId (s, "amt")));
    }
    const bool blink = ctx.learnBlinkSlot >= 0 && ctx.proc.learnSlot.load() == ctx.learnBlinkSlot && ctx.slotsFor (d).contains (ctx.learnBlinkSlot);
    if (std::abs (live - lastLive) > 0.002f || range != lastRange || blink) { lastLive = live; lastRange = range; repaint(); }
}

void Knob::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    if (! compact)
    {
        g.setColour (t.dim); g.setFont (FontOptions (10.5f));
        g.drawText (label, r.removeFromTop (13.f), Justification::centred, true);
        auto vt = r.removeFromBottom (13.f);
        g.setColour (dragging ? t.accent : t.text); g.setFont (FontOptions (10.5f));
        g.drawText (valueText(), vt, Justification::centred, true);
    }
    const float sz = jmin (r.getWidth(), r.getHeight()) - 2.f;
    auto c = r.getCentre(); const float rad = sz * 0.5f - 3.f;
    const float a0 = MathConstants<float>::pi * 1.2f, a1 = MathConstants<float>::pi * 2.8f;
    auto angle = [&] (float v) { return a0 + jlimit (0.f, 1.f, v) * (a1 - a0); };
    Path tr; tr.addCentredArc (c.x, c.y, rad, rad, 0, a0, a1, true);
    g.setColour (t.line); g.strokePath (tr, PathStrokeType (compact ? 2.5f : 3.f, PathStrokeType::curved, PathStrokeType::rounded));
    const auto& rg = p->getNormalisableRange();
    const bool bip = rg.start < 0 && rg.end > 0;
    const float from = bip ? p->convertTo0to1 (0.f) : 0.f;
    Path va; va.addCentredArc (c.x, c.y, rad, rad, 0, angle (jmin (from, shown)), angle (jmax (from, shown)), true);
    g.setColour (hasCol ? col : t.accent); g.strokePath (va, PathStrokeType (compact ? 2.5f : 3.f, PathStrokeType::curved, PathStrokeType::rounded));
    // modulation halo
    const int d = ctx.destOf (pid);
    if (d > 0)
    {
        float pos = 0, neg = 0;
        for (int s : ctx.slotsFor (d)) { const float a = ctx.real (slotId (s, "amt")); if (a > 0) pos += a; else neg += a; }
        if (pos != 0 || neg != 0)
        {
            Path h; const float hr = rad + 3.5f;
            h.addCentredArc (c.x, c.y, hr, hr, 0, angle (shown + neg), angle (shown + pos), true);
            g.setColour (t.accent2.withAlpha (0.55f)); g.strokePath (h, PathStrokeType (2.f));
            const float lv = angle (shown + ctx.proc.ui.modLive[d].load());
            g.setColour (t.accent2); g.fillEllipse (c.x + hr * std::sin (lv) - 2.5f, c.y - hr * std::cos (lv) - 2.5f, 5.f, 5.f);
        }
        const bool blink = ctx.learnBlinkSlot >= 0 && ctx.proc.learnSlot.load() == ctx.learnBlinkSlot && ctx.slotsFor (d).contains (ctx.learnBlinkSlot);
        if (blink && (Time::getMillisecondCounter() / 250) % 2 == 0) { g.setColour (t.warn); g.drawEllipse (c.x - rad - 5, c.y - rad - 5, (rad + 5) * 2, (rad + 5) * 2, 2.f); }
    }
    g.setColour (t.panelHi); g.fillEllipse (c.x - rad * 0.66f, c.y - rad * 0.66f, rad * 1.32f, rad * 1.32f);
    Path ptr; ptr.addRoundedRectangle (-1.2f, -rad * 0.62f, 2.4f, rad * 0.42f, 1.f);
    g.setColour (t.text); g.fillPath (ptr, AffineTransform::rotation (angle (shown)).translated (c.x, c.y));
    if (ctx.assignSource >= 0 && d > 0) { g.setColour (t.warn.withAlpha (0.5f)); g.drawEllipse (c.x - rad - 2, c.y - rad - 2, (rad + 2) * 2, (rad + 2) * 2, 1.f); }
}

void Knob::mouseDown (const MouseEvent& e)
{
    const int d = ctx.destOf (pid);
    if (e.mods.isPopupMenu())
    {
        PopupMenu m; ctx.buildModMenu (m, d, {});
        SafePointer<Knob> sp (this);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [sp, d] (int r)
        {
            if (! sp || r == 0) return;
            if (r == 5001) { sp->beginTyping(); return; }
            sp->ctx.handleModMenuResult (r, d, sp->pid);
            sp->repaint();
        });
        return;
    }
    if (ctx.assignSource >= 0 && d > 0)
    {
        ctx.addRoute (ctx.assignSource, d, Ctx::defaultAmountFor (d));
        ctx.assignSource = -1;
        if (ctx.refreshAll) ctx.refreshAll();
        return;
    }
    ctx.proc.undo.beginNewTransaction();
    att->beginGesture(); dragging = true; startV = shown; startPos = e.position;
}
void Knob::mouseDrag (const MouseEvent& e)
{
    if (! dragging) return;
    const float dy = (startPos.y - e.position.y) + (e.position.x - startPos.x) * 0.25f;
    const float sens = e.mods.isShiftDown() ? 0.00025f : 0.005f;
    const float v = jlimit (0.f, 1.f, startV + dy * sens);
    att->setValueAsPartOfGesture (p->convertFrom0to1 (v));
}
void Knob::mouseUp (const MouseEvent&) { if (dragging) { att->endGesture(); dragging = false; repaint(); } }
void Knob::mouseDoubleClick (const MouseEvent&) { beginTyping(); }
void Knob::beginTyping()
{
    if (dragging) { att->endGesture(); dragging = false; }
    editor = std::make_unique<TextEditor>();
    addAndMakeVisible (*editor);
    editor->setBounds (getLocalBounds().withSizeKeepingCentre (getWidth(), 20));
    editor->setJustification (Justification::centred);
    editor->setText (String (p->convertFrom0to1 (shown), 3), false);
    editor->selectAll(); editor->grabKeyboardFocus();
    auto done = [this] (bool apply)
    {
        if (apply) { const float v = editor->getText().retainCharacters ("-0123456789.").getFloatValue(); ctx.proc.undo.beginNewTransaction(); att->setValueAsCompleteGesture (jlimit (p->getNormalisableRange().start, p->getNormalisableRange().end, v)); }
        MessageManager::callAsync ([sp = SafePointer<Knob> (this)] { if (sp) sp->editor.reset(); });
    };
    editor->onReturnKey = [done] { done (true); };
    editor->onEscapeKey = [done] { done (false); };
    editor->onFocusLost = [done] { done (true); };
}
void Knob::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (ctx.prefs.wheel == 2) { Component::mouseWheelMove (e, w); return; }
    const auto& rg = p->getNormalisableRange();
    float step = ctx.prefs.wheel == 1 ? 0.05f : (rg.interval > 0 ? rg.interval / (rg.end - rg.start) : 0.005f);
    const float dir = (w.deltaY + w.deltaX) > 0 ? 1.f : ((w.deltaY + w.deltaX) < 0 ? -1.f : 0.f);
    if (dir == 0.f) return;
    att->setValueAsCompleteGesture (p->convertFrom0to1 (jlimit (0.f, 1.f, shown + dir * step)));
}

//==============================================================================
Choice::Choice (Ctx& c, const String& pid, const String& l) : label (l)
{
    auto* p = c.param (pid);
    if (auto* ch = dynamic_cast<AudioParameterChoice*> (p)) box.addItemList (ch->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<ComboBoxParameterAttachment> (*p, box, &c.proc.undo);
    box.onChange = [this] { if (onUserChange) onUserChange (box.getSelectedItemIndex()); };
    setSize (100, 40);
}
void Choice::resized() { box.setBounds (getLocalBounds().withTrimmedTop (label.isEmpty() ? 0 : 14).reduced (2, 1)); }
void Choice::paint (Graphics& g) { g.setColour (Theme::c().dim); g.setFont (FontOptions (10.5f)); g.drawText (label, getLocalBounds().removeFromTop (13), Justification::centred); }

Chip::Chip (Ctx& c, const String& id, const String& t, Colour on) : ctx (c), pid (id), text (t), onCol (on) { setSize (46, 22); setRepaintsOnMouseActivity (true); }
void Chip::paint (Graphics& g)
{
    auto& t = Theme::c();
    const bool on = ctx.real (pid) > 0.5f;
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    const Colour oc = onCol.isTransparent() ? t.accent : onCol;
    g.setColour (on ? oc.withAlpha (0.25f) : (isMouseOver() ? t.panelHi.brighter (0.05f) : t.panelHi)); g.fillRoundedRectangle (r, 4.f);
    g.setColour (on ? oc : t.line); g.drawRoundedRectangle (r, 4.f, 1.f);
    g.setColour (on ? oc : t.dim); g.setFont (FontOptions (11.f, Font::bold)); g.drawText (text, r, Justification::centred);
}
void Chip::mouseDown (const MouseEvent&)
{
    auto* p = ctx.param (pid); if (! p) return;
    ctx.proc.undo.beginNewTransaction();
    if (auto* c = dynamic_cast<AudioParameterChoice*> (p)) ctx.setReal (pid, (float) ((c->getIndex() + 1) % c->choices.size()));
    else ctx.setNorm (pid, p->getValue() > 0.5f ? 0.f : 1.f);
    repaint();
}

void ActionChip::paint (Graphics& g)
{
    auto& t = Theme::c();
    const bool on = isOn && isOn();
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    g.setColour (on ? t.accent.withAlpha (0.25f) : (isMouseOver() ? t.panelHi.brighter (0.05f) : t.panelHi)); g.fillRoundedRectangle (r, 4.f);
    g.setColour (on ? t.accent : t.line); g.drawRoundedRectangle (r, 4.f, 1.f);
    g.setColour (on ? t.accent : t.text); g.setFont (FontOptions (11.f, Font::bold)); g.drawText (text, r, Justification::centred);
}

//==============================================================================
namespace
{
    struct Clip { std::map<String, float> vals; struct R { int src; String rel; float amt; int pol, curve; float lag; }; std::vector<R> routes; bool valid = false; };
    Clip& clip() { static Clip c; return c; }
    String rel (const String& id) { return (id.length() > 3 && id[0] == 'L' && id[2] == '_') ? id.substring (3) : id; }
    String resolve (const String& r, int lane, const String& original) { return (original.length() > 3 && original[0] == 'L' && original[2] == '_' && lane >= 0) ? laneId (lane, r.toRawUTF8()) : r; }
    bool skipForRandom (const String& id) { for (auto* k : { "power", "mute", "solo", "chan", "engine", "eqsnap", "_on" }) if (id.contains (k)) return true; return false; }
}

Panel::Panel (Ctx& c, const String& t, int lane_, bool lm) : title (t), ctx (c), lane (lane_), laneMenu (lm) {}

Knob* Panel::knob (const String& id, const String& l) { auto* k = new Knob (ctx, id, l); owned.add (k); addAndMakeVisible (k); flow.add (k); pids.add (id); return k; }
Choice* Panel::choice (const String& id, const String& l) { auto* k = new Choice (ctx, id, l); owned.add (k); addAndMakeVisible (k); flow.add (k); pids.add (id); return k; }
Chip* Panel::chip (const String& id, const String& t) { auto* k = new Chip (ctx, id, t); owned.add (k); addAndMakeVisible (k); flow.add (k); pids.add (id); return k; }
void Panel::clearControls() { flow.clear(); pids.clear(); owned.clear(); }

int Panel::layoutFlow (int width)
{
    const int pad = 6; int x = pad, y = titleH + 2, rowH = 0;
    for (auto* c : flow)
    {
        if (! c->isVisible()) continue;
        const int w = c->getWidth(), h = c->getHeight();
        if (x + w > width - pad && x > pad) { x = pad; y += rowH + 2; rowH = 0; }
        c->setTopLeftPosition (x, y);
        x += w + 2; rowH = jmax (rowH, h);
    }
    return y + rowH + pad;
}

void Panel::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.panel); g.fillRoundedRectangle (r, 6.f);
    g.setColour (t.line); g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);
    auto tr = getLocalBounds().removeFromTop (titleH).reduced (10, 0);
    if (! stripe.isTransparent()) { g.setColour (stripe); g.fillRoundedRectangle (4.f, 6.f, 3.f, (float) titleH - 10.f, 1.5f); }
    const bool over = isMouseOver (false) && getMouseXYRelative().y < titleH;
    g.setColour (over ? t.warn : t.accent2); g.setFont (FontOptions (11.5f, Font::bold));
    g.drawText (title, tr, Justification::centredLeft, true);
}

void Panel::mouseDown (const MouseEvent& e)
{
    if (e.y < titleH || e.mods.isPopupMenu()) showMenu();
}

void Panel::showMenu()
{
    PopupMenu m;
    m.addSectionHeader (title);
    m.addItem (1, "Copy");
    const int nRoutes = (int) clip().routes.size();
    m.addItem (2, "Paste" + (nRoutes > 0 ? String (" (+") + String (nRoutes) + " routes)" : String()), clip().valid);
    m.addItem (3, "Paste without routes", clip().valid && nRoutes > 0);
    m.addItem (4, "Vary"); m.addItem (5, "Randomise");
    if (laneMenu && lane >= 0)
    {
        m.addSeparator();
        for (int l = 0; l < kNumLanes; ++l) if (l != lane) m.addItem (10 + l, "Copy to Lane " + String (l + 1));
    }
    SafePointer<Panel> sp (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [sp] (int r)
    {
        if (! sp || r == 0) return;
        auto& self = *sp; auto& ctx = self.ctx;
        Random rnd;
        ctx.proc.undo.beginNewTransaction();
        if (r == 1)
        {
            auto& c = clip(); c.vals.clear(); c.routes.clear(); c.valid = true;
            for (auto& id : self.pids)
            {
                c.vals[rel (id)] = ctx.norm (id);
                const int d = ctx.destOf (id);
                for (int s : ctx.slotsFor (d))
                    c.routes.push_back ({ ctx.choice (slotId (s, "src")), rel (id), ctx.real (slotId (s, "amt")), ctx.choice (slotId (s, "pol")), ctx.choice (slotId (s, "curve")), ctx.real (slotId (s, "lag")) });
            }
        }
        else if (r == 2 || r == 3)
        {
            const String any = self.pids.isEmpty() ? String() : self.pids[0];
            for (auto& kv : clip().vals) { const auto id = resolve (kv.first, self.lane, any); if (ctx.param (id)) ctx.setNorm (id, kv.second); }
            if (r == 2)
                for (auto& rt : clip().routes)
                {
                    const auto id = resolve (rt.rel, self.lane, any);
                    const int d = ctx.destOf (id);
                    if (d > 0) { const int s = ctx.addRoute (rt.src, d, rt.amt); if (s >= 0) { ctx.setReal (slotId (s, "pol"), (float) rt.pol); ctx.setReal (slotId (s, "curve"), (float) rt.curve); ctx.setReal (slotId (s, "lag"), rt.lag); } }
                }
        }
        else if (r == 4 || r == 5)
        {
            for (auto& id : self.pids)
            {
                auto* p = ctx.param (id);
                if (! p || skipForRandom (id)) continue;
                if (dynamic_cast<AudioParameterFloat*> (p))
                    ctx.setNorm (id, r == 4 ? jlimit (0.f, 1.f, p->getValue() + (rnd.nextFloat() * 2.f - 1.f) * 0.15f) : rnd.nextFloat());
                else if (r == 5)
                    if (auto* c = dynamic_cast<AudioParameterChoice*> (p)) ctx.setReal (id, (float) rnd.nextInt (c->choices.size()));
            }
        }
        else if (r >= 10)
        {
            const int to = r - 10; const String from = "L" + String (self.lane + 1) + "_", dst = "L" + String (to + 1) + "_";
            for (auto* p : ctx.proc.getParameters())
                if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
                    if (rp->getParameterID().startsWith (from))
                        ctx.setNorm (dst + rp->getParameterID().substring (3), rp->getValue());
            const int base = kDestLaneBase + self.lane * LD_Count, nb = kDestLaneBase + to * LD_Count;
            for (int s = 0; s < kNumSlots; ++s)
            {
                const int d = ctx.choice (slotId (s, "dst"));
                if (d >= base && d < base + LD_Count && ctx.choice (slotId (s, "src")) > 0)
                    ctx.addRoute (ctx.choice (slotId (s, "src")), nb + (d - base), ctx.real (slotId (s, "amt")));
            }
        }
        if (ctx.refreshAll) ctx.refreshAll();
        self.repaint();
    });
}
} // namespace cz
