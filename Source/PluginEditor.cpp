#include "PluginEditor.h"
#include <map>

using namespace juce;

namespace cz
{
FloatingWindow::FloatingWindow (const String& name, Component* content, std::function<void (FloatingWindow*)> onClose)
    : DocumentWindow (name, Theme::c().bg, DocumentWindow::closeButton), closed (std::move (onClose))
{
    setUsingNativeTitleBar (true);
    if (content->getWidth() == 0) content->setSize (1200, 760);
    setContentOwned (content, true);
    setResizable (true, false);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

static const char* kPageNames[4] = { "MAIN", "MODULATORS", "MATRIX", "MASTER" };

CaosmoEditor::CaosmoEditor (CaosmoProcessor& p) : AudioProcessorEditor (p), proc (p), pm (p), ctx (p, pm, prefs)
{
    prefs.load(); Theme::set (prefs.theme); lnf.refresh();
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);
    ctx.openBrowser = [this] { openOverlay (new PresetBrowser (ctx)); };
    ctx.openSave = [this] { openOverlay (new SaveDialog (ctx)); };
    ctx.refreshAll = [this] { root.repaint(); };
    ctx.openFloating = [this] (Component* c, const String& n)
    {
        floating.add (new FloatingWindow (n, c, [this] (FloatingWindow* w) { MessageManager::callAsync ([sp = SafePointer<CaosmoEditor> (this), w] { if (sp) sp->floating.removeObject (w); }); }));
    };

    for (auto* b : { &prevBtn, &nextBtn, &nameBtn, &saveBtn, &saveAsBtn, &browseBtn, &undoBtn, &redoBtn, &detachBtn, &setBtn }) root.addAndMakeVisible (b);
    prevBtn.onClick = [this] { pm.step (-1); };
    nextBtn.onClick = [this] { pm.step (1); };
    nameBtn.onClick = [this] { presetMenu(); };
    saveBtn.onClick = [this] { if (! pm.overwriteCurrent()) ctx.openSave(); };
    saveAsBtn.onClick = [this] { ctx.openSave(); };
    browseBtn.onClick = [this] { ctx.openBrowser(); };
    undoBtn.onClick = [this] { proc.undo.undo(); };
    redoBtn.onClick = [this] { proc.undo.redo(); };
    detachBtn.onClick = [this] { detach(); };
    setBtn.onClick = [this] { openOverlay (new SettingsPanel (ctx, [this] { applyUi(); })); };
    saveBtn.setTooltip ("Overwrite the current user preset (opens Save As for factory presets)");
    browseBtn.setTooltip ("Open the preset browser");
    detachBtn.setTooltip ("Pop the current page out into its own window");
    for (int i = 0; i < 4; ++i)
    {
        auto* t = tabs.add (new TextButton (kPageNames[i]));
        t->setClickingTogglesState (true); t->setRadioGroupId (77);
        t->onClick = [this, i] { if (tabs[i]->getToggleState()) showPage (i); };
        root.addAndMakeVisible (t);
    }
    outKnob = std::make_unique<Knob> (ctx, "out_gain", "OUT", true); root.addAndMakeVisible (*outKnob);
    glideKnob = std::make_unique<Knob> (ctx, "glide", "GLIDE", true); root.addAndMakeVisible (*glideKnob);
    meterL = std::make_unique<LevelMeter> ([this] { return proc.ui.outPkL.load(); }); root.addAndMakeVisible (*meterL);
    meterR = std::make_unique<LevelMeter> ([this] { return proc.ui.outPkR.load(); }); root.addAndMakeVisible (*meterR);
    for (int i = 0; i < kNumMacros; ++i) { auto* k = macros.add (new Knob (ctx, "macro" + String (i + 1), "M" + String (i + 1), true)); k->setColour (Theme::c().accent2); root.addAndMakeVisible (k); }

    addMouseListener (this, true);
    setWantsKeyboardFocus (true);
    tabs[0]->setToggleState (true, dontSendNotification);
    applyUi();
    startTimerHz (15);
}

CaosmoEditor::~CaosmoEditor()
{
    stopTimer();
    removeMouseListener (this);
    overlay.reset(); floating.clear(); pageComp.reset();
    setLookAndFeel (nullptr);
}

void CaosmoEditor::applyUi()
{
    Theme::set (prefs.theme); lnf.refresh();
    const float s = jlimit (0.75f, 2.f, prefs.scale);
    root.setTransform (AffineTransform::scale (s));
    setSize (roundToInt (kW * s), roundToInt (kH * s));
    showPage (page);
    sendLookAndFeelChange();
    repaint();
}

void CaosmoEditor::resized() { root.setBounds (0, 0, kW, kH); }

Component* CaosmoEditor::makePage (int idx)
{
    switch (idx)
    {
        case 1: return new ModulatorsPage (ctx);
        case 2: return new MatrixPage (ctx);
        case 3: return new MasterPage (ctx);
        default: return new MainPage (ctx);
    }
}

void CaosmoEditor::showPage (int idx)
{
    page = idx;
    for (int i = 0; i < tabs.size(); ++i) tabs[i]->setToggleState (i == idx, dontSendNotification);
    pageComp.reset (makePage (idx));
    root.addAndMakeVisible (*pageComp);
    pageComp->setBounds (0, 56, kW, kH - 56 - 50);
    if (overlay) overlay->toFront (true);
}

void CaosmoEditor::detach()
{
    auto* c = makePage (page);
    c->setSize (1280, 800);
    ctx.openFloating (c, String ("Caosmo - ") + kPageNames[page]);
}

void CaosmoEditor::openOverlay (Overlay* o)
{
    overlay.reset (o);
    o->onClose = [this] { MessageManager::callAsync ([sp = SafePointer<CaosmoEditor> (this)] { if (sp) sp->overlay.reset(); }); };
    root.addAndMakeVisible (o);
    o->setBounds (0, 0, kW, kH);
    o->grabKeyboardFocus();
}

void CaosmoEditor::presetMenu()
{
    PopupMenu m, fac, usr;
    const auto& all = pm.all();
    std::map<String, PopupMenu> fcat, ucat; StringArray forder, uorder;
    const int cur = pm.indexOfCurrent();
    for (int i = 0; i < all.size(); ++i)
    {
        auto& p = all.getReference (i);
        auto& mapRef = p.factory ? fcat : ucat; auto& order = p.factory ? forder : uorder;
        order.addIfNotAlreadyThere (p.category);
        mapRef[p.category].addItem (1 + i, p.name, true, i == cur);
    }
    for (auto& c : forder) fac.addSubMenu (c, fcat[c]);
    for (auto& c : uorder) usr.addSubMenu (c, ucat[c]);
    m.addSubMenu ("Factory", fac); m.addSubMenu ("User", usr, uorder.size() > 0);
    m.addSeparator(); m.addItem (100000, "Reset to Default");
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&nameBtn), [this] (int r)
    {
        if (r == 100000) pm.resetToInit();
        else if (r > 0) pm.load (r - 1);
    });
}

bool CaosmoEditor::keyPressed (const KeyPress& k)
{
    const bool cmd = k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown();
    if (cmd && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z'))
    {
        if (k.getModifiers().isShiftDown()) proc.undo.redo(); else proc.undo.undo();
        return true;
    }
    if (cmd && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { proc.undo.redo(); return true; }
    if (k == KeyPress::escapeKey)
    {
        if (overlay) { overlay.reset(); return true; }
        if (ctx.assignSource >= 0) { ctx.assignSource = -1; return true; }
    }
    return false;
}

void CaosmoEditor::mouseDown (const MouseEvent&) { proc.undo.beginNewTransaction(); }

void CaosmoEditor::testOverlay (int which)
{
    if (which == 0) ctx.openBrowser(); else if (which == 1) ctx.openSave();
    else if (which == 2) openOverlay (new SettingsPanel (ctx, [this] { applyUi(); }));
    else overlay.reset();
}

void CaosmoEditor::timerCallback()
{
    const int ls = proc.learnedSlot.exchange (-1);
    if (ls >= 0) { ctx.setReal (slotId (ls, "cc"), (float) proc.learnedCC.load()); ctx.learnBlinkSlot = -1; }
    const String n = pm.currentName();
    if (nameBtn.getButtonText() != n) nameBtn.setButtonText (n);
    undoBtn.setEnabled (proc.undo.canUndo()); redoBtn.setEnabled (proc.undo.canRedo());
    root.repaint (0, 0, kW, 56);
    root.repaint (0, kH - 50, kW, 50);
}

//==============================================================================
void CaosmoEditor::Root::resized() { ed.layoutRoot(); }

void CaosmoEditor::layoutRoot()
{
    int x = 160;
    prevBtn.setBounds (x, 13, 24, 28); x += 26;
    nameBtn.setBounds (x, 13, 170, 28); x += 172;
    nextBtn.setBounds (x, 13, 24, 28); x += 30;
    saveBtn.setBounds (x, 13, 50, 28); x += 52; saveAsBtn.setBounds (x, 13, 66, 28); x += 68; browseBtn.setBounds (x, 13, 66, 28); x += 76;
    for (auto* t : tabs) { t->setBounds (x, 10, 94, 34); x += 96; }
    int r = kW - 10;
    setBtn.setBounds (r - 78, 13, 78, 28); r -= 82;
    detachBtn.setBounds (r - 62, 13, 62, 28); r -= 66;
    redoBtn.setBounds (r - 48, 13, 48, 28); r -= 50;
    undoBtn.setBounds (r - 48, 13, 48, 28); r -= 54;
    glideKnob->setBounds (r - 72, 12, 30, 30); r -= 78;
    r -= 72;   // BPM / CPU text
    outKnob->setBounds (r - 32, 12, 30, 30); r -= 36;
    meterR->setBounds (r - 7, 12, 6, 30); meterL->setBounds (r - 15, 12, 6, 30);
    for (int i = 0; i < macros.size(); ++i) macros[i]->setBounds (48 + i * 140, kH - 42, 34, 34);
    if (pageComp) pageComp->setBounds (0, 56, kW, kH - 56 - 50);
    if (overlay) overlay->setBounds (0, 0, kW, kH);
}

void CaosmoEditor::Root::paint (Graphics& g)
{
    auto& t = Theme::c();
    g.fillAll (t.bg);
    g.setColour (t.panel); g.fillRect (0, 0, kW, 54); g.fillRect (0, kH - 50, kW, 50);
    g.setColour (t.line); g.drawHorizontalLine (54, 0, (float) kW); g.drawHorizontalLine (kH - 50, 0, (float) kW);
    g.setColour (t.accent); g.setFont (FontOptions (24.f, Font::bold)); g.drawText ("CAOSMO", 18, 8, 140, 34, Justification::centredLeft);
    g.setColour (t.dim); g.setFont (FontOptions (9.f)); g.drawText ("chaos synthesizer", 20, 36, 140, 12, Justification::centredLeft);
    const int textX = ed.outKnob->getRight() + 10;
    g.setColour (t.dim); g.setFont (FontOptions (11.f));
    g.drawText ("BPM " + String (ed.proc.ui.bpm.load(), 1), textX, 10, 70, 16, Justification::centredLeft);
    const float cpu = ed.proc.ui.cpu.load() * 100.f;
    g.setColour (cpu > 70.f ? t.red : t.dim); g.drawText ("CPU " + String (roundToInt (cpu)) + "%", textX, 28, 90, 16, Justification::centredLeft);
    g.setColour (t.dim); g.drawText ("GLIDE", ed.glideKnob->getRight() + 4, 20, 44, 14, Justification::centredLeft);
    for (int i = 0; i < kNumMacros; ++i)
    {
        const int x = 18 + i * 140;
        g.setColour (t.accent2); g.setFont (FontOptions (12.f, Font::bold)); g.drawText ("M" + String (i + 1), x, kH - 40, 28, 30, Justification::centredLeft);
        g.setColour (t.text); g.setFont (FontOptions (11.f));
        g.drawText (String (roundToInt (ed.ctx.real ("macro" + String (i + 1)) * 100.f)) + "%", x + 66, kH - 40, 60, 30, Justification::centredLeft);
    }
    const float vals[3] = { ed.proc.ui.mpePress.load(), ed.proc.ui.mpeSlide.load(), std::abs (ed.proc.ui.mpeGlide.load()) };
    static const char* nm[3] = { "PRESS", "SLIDE", "GLIDE" };
    int mx = 760;
    g.setColour (t.dim); g.setFont (FontOptions (10.f, Font::bold)); g.drawText ("MPE", mx, kH - 40, 30, 30, Justification::centredLeft); mx += 34;
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (t.dim); g.setFont (FontOptions (9.5f)); g.drawText (nm[i], mx, kH - 44, 60, 14, Justification::centredLeft);
        g.setColour (t.bg); g.fillRect (mx, kH - 26, 56, 6);
        g.setColour (t.accent); g.fillRect ((float) mx, (float) kH - 26, 56.f * jlimit (0.f, 1.f, vals[i]), 6.f);
        mx += 70;
    }
    g.setColour (t.dim); g.setFont (FontOptions (11.f));
    g.drawText (String (ed.proc.ui.activeVoices.load()) + " voices", kW - 120, kH - 40, 100, 30, Justification::centredRight);
    if (ed.ctx.assignSource >= 0)
    {
        g.setColour (t.warn); g.setFont (FontOptions (12.f, Font::bold));
        g.drawText ("ASSIGN " + sourceNames()[ed.ctx.assignSource] + ": click a control  (Esc cancels)", 1010, kH - 40, 300, 30, Justification::centredLeft);
    }
}
} // namespace cz
