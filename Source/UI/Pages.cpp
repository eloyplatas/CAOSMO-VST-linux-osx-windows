#include "Pages.h"

using namespace juce;

namespace cz
{
namespace
{
    template <typename T> T* own (OwnedArray<Component>& o, T* c) { o.add (c); return c; }
    const float kFxDefaults[kNumFxTypes][kFxParams] = {
        { 0.5f, 0.5f, 0.5f, 0.5f, 0.8f }, { 0.6f, 0.5f, 0.1f, 0.4f, 0.8f }, { 0.75f, 0.4f, 0.5f, 0.2f, 0.7f }, { 0.6f, 0.3f, 0.8f, 0.5f, 0.5f },
        { 0.5f, 0.5f, 0.4f, 0.5f, 0.8f }, { 0.5f, 0.6f, 0.5f, 0.3f, 0.6f }, { 0.5f, 0.8f, 0.5f, 0.5f, 0.5f },
        { 0.6f, 0.2f, 0.8f, 0.5f, 0.5f }, { 0.5f, 0.4f, 0.8f, 0.5f, 0.5f }, { 0.4f, 0.25f, 0.3f, 0.8f, 0.5f }, { 0.35f, 0.4f, 0.8f, 0.5f, 0.5f },
        { 0.3f, 0.2f, 0.8f, 0.5f, 0.5f }, { 0.2f, 0.2f, 0.7f, 0.5f, 0.5f } };

    // full lane EQ editor in its own window
    class EqEditor : public Component
    {
    public:
        EqEditor (Ctx& c, int lane) : curve (c, laneId (lane, ""), lane), knobs (c, "EQ  /  LANE " + String (lane + 1))
        {
            addAndMakeVisible (curve); addAndMakeVisible (knobs);
            knobs.chip (laneId (lane, "eqon"), "ON");
            for (auto kv : { std::pair<const char*, const char*> { "hpf", "HPF" }, { "lsf", "Low F" }, { "lsg", "Low G" }, { "mf", "Mid F" }, { "mg", "Mid G" },
                             { "mq", "Mid Q" }, { "hsf", "High F" }, { "hsg", "High G" }, { "lpf", "LPF" } }) knobs.knob (laneId (lane, kv.first), kv.second);
            knobs.choice (laneId (lane, "hpfs"), "HPF Slope"); knobs.choice (laneId (lane, "lpfs"), "LPF Slope");
            knobs.knob (laneId (lane, "eqmorph"), "Morph");
            setSize (720, 420);
        }
        void resized() override { auto r = getLocalBounds().reduced (8); curve.setBounds (r.removeFromTop (240)); r.removeFromTop (8); knobs.setBounds (r.withHeight (knobs.layoutFlow (r.getWidth()))); }
        void paint (Graphics& g) override { g.fillAll (Theme::c().bg); }
        EqCurve curve; Panel knobs;
    };
}

//==============================================================================
LaneColumn::LaneColumn (Ctx& c, int l)
    : ctx (c), lane (l), header (c, "LANE " + String (l + 1), l, true), drift (c, "DRIFT", l), pitch (c, "PITCH", l), subs (c, "SUBS", l),
      motor (c, "MOTOR", l), filter (c, "FILTER", l), env (c, "ENV GEN", l), eq (c, "EQ", l), scope (c, "SCOPE", l), out (c, "OUTPUT", l)
{
    for (auto* p : { &header, &drift, &pitch, &subs, &motor, &filter, &env, &eq, &scope, &out }) addAndMakeVisible (p);
    auto* ec = header.choice (laneId (l, "engine"), "Engine"); ec->setSize (150, 40);
    header.chip (laneId (l, "power"), "PWR"); header.chip (laneId (l, "solo"), "S"); header.chip (laneId (l, "mute"), "M");
    header.choice (laneId (l, "chan"), "MIDI Ch")->setSize (80, 40);
    blurb = std::make_unique<TextLine> ([this]
    {
        const int e = ctx.proc.engineOfLane (lane);
        return e > 0 ? String (engineSpecs()[(size_t) e].blurb) : String ("No engine: pick one to bring this lane to life.");
    });
    header.addCustom (blurb.get());
    drift.choice (laneId (l, "driftp"), "Pitch")->setSize (84, 40); drift.choice (laneId (l, "driftf"), "Filter")->setSize (84, 40); drift.choice (laneId (l, "driftl"), "Level")->setSize (84, 40);
    pitch.knob (laneId (l, "pitch"), "Semi"); pitch.knob (laneId (l, "fine"), "Fine");
    subs.choice (laneId (l, "sub1oct"), "Sub1 Oct")->setSize (70, 40); subs.knob (laneId (l, "sub1lev"), "Sub1 Lev");
    subs.choice (laneId (l, "sub2oct"), "Sub2 Oct")->setSize (70, 40); subs.knob (laneId (l, "sub2lev"), "Sub2 Lev");
    filter.choice (laneId (l, "ftype"), "Type"); filter.choice (laneId (l, "fslope"), "Slope")->setSize (76, 40);
    filter.knob (laneId (l, "cut"), "Cut"); filter.knob (laneId (l, "res"), "Res"); filter.choice (laneId (l, "lpg"), "LPG Mode");

    curve = std::make_unique<EqCurve> (c, laneId (l, ""), l);
    eq.addCustom (curve.get());
    eq.chip (laneId (l, "eqon"), "ON");
    for (int s = 0; s < 4; ++s)
    {
        auto* ch = eqChips.add (new ActionChip (String::charToString ((juce_wchar) ('A' + s)), [this, s] { ctx.proc.undo.beginNewTransaction(); ctx.setReal (laneId (lane, "eqsnap"), (float) s); }));
        ch->isOn = [this, s] { return ctx.choice (laneId (lane, "eqsnap")) == s; };
        ch->setSize (26, 22); eq.addCustom (ch);
    }
    auto* rst = eqChips.add (new ActionChip ("RST", [this] { ctx.proc.undo.beginNewTransaction(); ctx.proc.eqResetSnapshots (lane); })); rst->setSize (40, 22); eq.addCustom (rst);
    auto* ed = eqChips.add (new ActionChip ("EDIT", [this] { if (ctx.openFloating) ctx.openFloating (new EqEditor (ctx, lane), "Lane " + String (lane + 1) + " EQ"); })); ed->setSize (44, 22); eq.addCustom (ed);
    eq.knob (laneId (l, "eqmorph"), "Morph");
    for (auto k : { "hpf", "hpfs", "lsf", "lsg", "mf", "mg", "mq", "hsf", "hsg", "lpf", "lpfs" }) eq.pids.add (laneId (l, k));

    static const char* sm[4] = { "2D", "WAVE", "PHASE", "GRAIN" };
    for (int m = 0; m < 4; ++m)
    {
        auto* ch = scopeChips.add (new ActionChip (sm[m], [this, m] { ctx.prefs.scopeMode[lane] = m; ctx.prefs.save(); }));
        ch->isOn = [this, m] { return ctx.prefs.scopeMode[lane] == m; };
        ch->setSize (52, 22); scope.addCustom (ch);
    }
    scopeView = std::make_unique<Scope> (c, l); scope.addCustom (scopeView.get());

    out.chip (laneId (l, "solo"), "S"); out.chip (laneId (l, "mute"), "M");
    out.knob (laneId (l, "level"), "Level"); out.knob (laneId (l, "pan"), "Pan"); out.knob (laneId (l, "width"), "Width"); out.knob (laneId (l, "vspread"), "Vsprd");
    out.knob (laneId (l, "mono"), "Mono"); out.knob (laneId (l, "drive"), "Drive"); out.choice (laneId (l, "dflav"), "Drive Fl.")->setSize (96, 40);
    out.knob (laneId (l, "fx1"), "Fx1"); out.knob (laneId (l, "fx2"), "Fx2"); out.knob (laneId (l, "fx3"), "Fx3");
    timerCallback();
    startTimerHz (10);
}

void LaneColumn::buildMotor()
{
    motor.clearControls();
    const int e = ctx.proc.engineOfLane (lane);
    const auto& sp = engineSpecs()[(size_t) jmax (0, e)];
    motor.title = e > 0 ? String ("MOTOR  /  ") + sp.name : String ("MOTOR");
    motor.stripe = Theme::engine (e);
    motorKnobs.clear(); motorChoice0 = -1;
    for (auto& p : sp.p) { auto* k = motor.knob (engId (lane, e, p.key), p.name); k->setColour (Theme::engine (e)); motorKnobs.add (k); }
    for (auto& c : sp.c) motor.choice (engId (lane, e, c.key), c.name)->setSize (96, 40);
    header.stripe = Theme::engine (e);
    header.repaint(); motor.repaint();
}

void LaneColumn::buildEnv()
{
    env.clearControls();
    const int m = ctx.choice (laneId (lane, "emode"));
    env.choice (laneId (lane, "emode"), "Mode")->setSize (84, 40);
    env.choice (laneId (lane, "esub"), "Sub Mode")->setSize (100, 40);
    auto K = [&] (const char* k, const char* l) { env.knob (laneId (lane, k), l); };
    if (m == 0) { K ("edel", "Delay"); K ("eatt", "Att"); K ("edec", "Dec"); K ("esus", "Sus"); K ("erel", "Rel");
                  env.choice (laneId (lane, "ecatt"), "Att Curve")->setSize (70, 40); env.choice (laneId (lane, "ecdec"), "Dec Curve")->setSize (70, 40); env.choice (laneId (lane, "ecrel"), "Rel Curve")->setSize (70, 40); }
    else if (m == 1) { K ("vrf", "Rise F"); K ("vff", "Fall F"); K ("vrs", "Rise S"); K ("vfs", "Fall S"); K ("vmix", "Mix"); K ("vmem", "Memory"); K ("vshape", "Shape"); K ("vdrift", "Drift"); }
    else if (m == 2) { K ("elrate", "Rate"); env.choice (laneId (lane, "elshape"), "Shape")->setSize (90, 40); K ("erel", "Rel"); }
    else { env.choice (laneId (lane, "evsrc"), "Shape")->setSize (84, 40); K ("evtime", "Time"); K ("erel", "Rel"); }
}

void LaneColumn::timerCallback()
{
    const int e = ctx.proc.engineOfLane (lane), m = ctx.choice (laneId (lane, "emode"));
    bool changed = false;
    if (e != engine) { engine = e; buildMotor(); changed = true; blurb->repaint(); }
    if (m != envMode) { envMode = m; buildEnv(); changed = true; }
    if (engine > 0 && ! engineSpecs()[(size_t) engine].c.empty())
    {   // Maps / Ecosystems: knob names follow the selected map or model
        const int c0 = ctx.choice (engId (lane, engine, engineSpecs()[(size_t) engine].c[0].key));
        if (c0 != motorChoice0) { motorChoice0 = c0; for (int k = 0; k < motorKnobs.size(); ++k) motorKnobs[k]->setLabel (engineParamLabel (engine, k, c0)); }
    }
    for (auto* c : eqChips) c->repaint();
    for (auto* c : scopeChips) c->repaint();
    if (changed && onHeightChanged) onHeightChanged();
}

int LaneColumn::layoutFor (int w)
{
    const int gap = 6; int y = 0;
    blurb->setSize (w - 16, 30);
    curve->setSize (w - 16, 110);
    scopeView->setSize (w - 16, 120);
    auto place = [&] (Panel& p, int x, int width) { const int h = p.layoutFlow (width); p.setBounds (x, y, width, h); return h; };
    y += place (header, 0, w) + gap;
    const int dw = (int) (w * 0.6f) - gap / 2;
    const int h1 = drift.layoutFlow (dw), h2 = pitch.layoutFlow (w - dw - gap);
    drift.setBounds (0, y, dw, jmax (h1, h2)); pitch.setBounds (dw + gap, y, w - dw - gap, jmax (h1, h2)); y += jmax (h1, h2) + gap;
    for (auto* p : { &subs, &motor, &filter, &env, &eq, &scope, &out }) y += place (*p, 0, w) + gap;
    setSize (w, y);
    return y;
}

MainPage::MainPage (Ctx& c) : ctx (c)
{
    for (int l = 0; l < kNumLanes; ++l)
    {
        auto* col = cols.add (new LaneColumn (c, l));
        col->onHeightChanged = [this] { relayout(); };
        body.addAndMakeVisible (col);
    }
}
void MainPage::relayout()
{
    const int w = bodyWidth(), gap = 8;
    const int cw = (w - gap * (kNumLanes + 1)) / kNumLanes;
    int maxH = 0;
    for (int l = 0; l < kNumLanes; ++l) { const int h = cols[l]->layoutFor (cw); cols[l]->setTopLeftPosition (gap + l * (cw + gap), gap); maxH = jmax (maxH, h); }
    body.setSize (w, maxH + gap * 2);
}

//==============================================================================
ModulatorsPage::ModulatorsPage (Ctx& c) : ctx (c), loop (c, "LFO BANK  /  LOOP")
{
    body.addAndMakeVisible (loop);
    loop.chip ("lfo_loop", "LOCK"); loop.knob ("lfo_looplen", "Len"); loop.choice ("lfo_loopsync", "Sync")->setSize (76, 40); loop.choice ("lfo_loopdiv", "Division")->setSize (96, 40);
    linkScope = own (extras, new LinkScope (c)); loop.addCustom (linkScope);

    auto routes = [this] (int src) -> std::function<String()>
    {
        return [this, src]
        {
            const auto dn = destNames(); StringArray s;
            for (int i = 0; i < kNumSlots; ++i) if (ctx.choice (slotId (i, "src")) == src && ctx.choice (slotId (i, "dst")) > 0) s.add (dn[ctx.choice (slotId (i, "dst"))]);
            return "ROUTING: " + (s.isEmpty() ? String ("none") : s.joinIntoString (", "));
        };
    };
    auto assign = [this] (Panel* p, int src)
    {
        auto* a = own (extras, new ActionChip ("ASSIGN", [this, src] { ctx.assignSource = ctx.assignSource == src ? -1 : src; }));
        a->isOn = [this, src] { return ctx.assignSource == src; }; a->setSize (62, 22); p->addCustom (a);
    };
    for (int i = 0; i < kNumLfo; ++i)
    {
        auto* p = lfo.add (new Panel (c, "LFO " + String (i + 1))); body.addAndMakeVisible (p);
        p->stripe = Theme::engine (i + 1);
        p->chip (lfoId (i, "on"), "ON"); assign (p, S_Lfo1 + i); p->chip (lfoId (i, "retrig"), "RETRIG")->setSize (60, 22);
        p->addCustom (own (extras, new LfoPreview (c, i)));
        p->choice (lfoId (i, "wave"), "Wave")->setSize (110, 40); p->choice (lfoId (i, "sync"), "Sync")->setSize (64, 40);
        p->knob (lfoId (i, "rate"), "Rate"); p->choice (lfoId (i, "div"), "Division")->setSize (76, 40);
        p->knob (lfoId (i, "level"), "Level"); p->knob (lfoId (i, "phase"), "Phase"); p->knob (lfoId (i, "slew"), "Slew"); p->knob (lfoId (i, "skew"), "Skew");
        p->knob (lfoId (i, "afade"), "Atk Fade"); p->knob (lfoId (i, "rfade"), "Rel Fade"); p->choice (lfoId (i, "mode"), "Mode")->setSize (76, 40);
        if ((i & 1) == 0) { p->choice (lfoId (i, "link"), "Link")->setSize (70, 40); p->choice (lfoId (i, "linkrel"), "Link Mode")->setSize (90, 40); }
        p->choice (lfoId (i, "dist"), "Dist")->setSize (90, 40);
        auto* rt = own (extras, new TextLine (routes (S_Lfo1 + i), 10.f)); routing.add (rt); p->addCustom (rt);
    }
    for (int i = 0; i < kNumVenv; ++i)
    {
        auto* p = venv.add (new Panel (c, "VENV " + String (i + 1))); body.addAndMakeVisible (p);
        p->stripe = Theme::c().accent2;
        p->chip (venvId (i, "on"), "ON"); assign (p, S_Venv1 + i); p->chip (venvId (i, "retrig"), "RETRIG")->setSize (60, 22);
        auto* sh = own (extras, new ActionChip ("SHAPE", {})); sh->setSize (64, 22);
        sh->fn = [this, i, sh] { shapeMenu (i, sh); }; p->addCustom (sh);
        auto* cv = own (extras, new VenvCanvas (c, i)); canvases.add (cv);
        auto* lines = own (extras, new ActionChip ("LINES", [cv] { cv->freeDraw = false; })); lines->isOn = [cv] { return ! cv->freeDraw; }; lines->setSize (52, 22); p->addCustom (lines);
        auto* fr = own (extras, new ActionChip ("FREE", [cv] { cv->freeDraw = true; })); fr->isOn = [cv] { return cv->freeDraw; }; fr->setSize (48, 22); p->addCustom (fr);
        auto* nm = own (extras, new TextLine ([this, i] { return ctx.proc.venvTree (i).getProperty ("name").toString(); }, 11.f)); nm->setSize (160, 22); p->addCustom (nm);
        p->addCustom (cv);
        p->knob (venvId (i, "time"), "Time"); p->choice (venvId (i, "sync"), "Sync")->setSize (64, 40); p->choice (venvId (i, "div"), "Division")->setSize (76, 40);
        p->knob (venvId (i, "level"), "Level");
        p->knob (venvId (i, "za"), "A Lvl"); p->knob (venvId (i, "zd"), "D Lvl"); p->knob (venvId (i, "zs"), "S Lvl"); p->knob (venvId (i, "zr"), "R Lvl");
        p->knob (venvId (i, "pa"), "Att Pt"); p->knob (venvId (i, "pd"), "Dec Pt"); p->knob (venvId (i, "ps"), "S/R Pt");
        p->choice (venvId (i, "mode"), "Mode")->setSize (100, 40); p->choice (venvId (i, "voicing"), "Voicing")->setSize (76, 40);
        auto* rt = own (extras, new TextLine (routes (S_Venv1 + i), 10.f)); routing.add (rt); p->addCustom (rt);
    }
    for (int i = 0; i < kNumAttr; ++i)
    {
        auto* p = attr.add (new Panel (c, "ATTRACTOR " + String (i + 1))); body.addAndMakeVisible (p);
        p->stripe = Theme::c().warn;
        p->chip (attrId (i, "on"), "ON"); assign (p, S_Attr1 + i); p->chip (attrId (i, "retrig"), "RETRIG")->setSize (60, 22);
        p->addCustom (own (extras, new AttractorView (c, i)));
        p->choice (attrId (i, "type"), "Type")->setSize (96, 40); p->knob (attrId (i, "rate"), "Rate");
        p->choice (attrId (i, "sync"), "Sync")->setSize (64, 40); p->choice (attrId (i, "div"), "Division")->setSize (76, 40);
        attrP1.add (p->knob (attrId (i, "p1"), "P1")); attrP2.add (p->knob (attrId (i, "p2"), "P2"));
        p->knob (attrId (i, "smooth"), "Smooth"); p->knob (attrId (i, "level"), "Level");
        p->choice (attrId (i, "out"), "Out")->setSize (70, 40); p->choice (attrId (i, "mode"), "Mode")->setSize (76, 40);
        auto* rt = own (extras, new TextLine (routes (S_Attr1 + i), 10.f)); routing.add (rt); p->addCustom (rt);
    }
    for (int i = 0; i < kNumTrig; ++i)
    {
        auto* p = trig.add (new Panel (c, "TRIGGER " + String (i + 1))); body.addAndMakeVisible (p);
        p->stripe = Theme::c().red;
        p->chip (trigId (i, "on"), "ON"); assign (p, S_Trig1 + i);
        auto* lamp = own (extras, new LevelMeter ([this, i] { return ctx.proc.ui.trigVal[i].load(); })); lamp->setSize (10, 40); p->addCustom (lamp);
        p->choice (trigId (i, "mode"), "Mode")->setSize (80, 40); p->knob (trigId (i, "rate"), "Rate");
        p->choice (trigId (i, "sync"), "Sync")->setSize (64, 40); p->choice (trigId (i, "div"), "Division")->setSize (76, 40);
        p->knob (trigId (i, "dev"), "Dev"); p->knob (trigId (i, "prob"), "Prob");
        p->choice (trigId (i, "shape"), "Output")->setSize (90, 40); p->knob (trigId (i, "len"), "Length"); p->knob (trigId (i, "level"), "Level");
        p->choice (trigId (i, "dist"), "Dist")->setSize (90, 40); p->choice (trigId (i, "action"), "Action")->setSize (104, 40);
        auto* rt = own (extras, new TextLine (routes (S_Trig1 + i), 10.f)); routing.add (rt); p->addCustom (rt);
    }
    startTimerHz (8);
}

void ModulatorsPage::shapeMenu (int i, Component* target)
{
    PopupMenu m, fac, usr;
    for (int b = 0; b < 10; ++b)
    {
        PopupMenu bank;
        for (int k = 0; k < 10; ++k) bank.addItem (100 + b * 10 + k, factoryShapeName (b * 10 + k));
        fac.addSubMenu ("Bank " + String::charToString ((juce_wchar) ('A' + b)), bank);
    }
    const auto names = ctx.pm.userShapeNames();
    for (int k = 0; k < names.size(); ++k) usr.addItem (1000 + k, names[k]);
    m.addSubMenu ("Factory shapes", fac);
    m.addSubMenu ("User shapes", usr, names.size() > 0);
    m.addSeparator();
    m.addItem (1, "Save", names.contains (ctx.proc.venvTree (i).getProperty ("name").toString()));
    m.addItem (2, "Save As...");
    m.addItem (3, "Browse shapes folder");
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (target), [this, i, names] (int r)
    {
        if (r >= 100 && r < 200) { ctx.proc.undo.beginNewTransaction(); ctx.proc.setShape (i, factoryShape (r - 100), factoryShapeName (r - 100)); }
        else if (r >= 1000) { VShape s; if (ctx.pm.loadUserShape (names[r - 1000], s)) { ctx.proc.undo.beginNewTransaction(); ctx.proc.setShape (i, s, names[r - 1000]); } }
        else if (r == 1) ctx.pm.saveUserShape (ctx.proc.venvTree (i).getProperty ("name").toString(), ctx.proc.getShape (i));
        else if (r == 2)
        {
            auto* w = new AlertWindow ("Save VENV shape", "Name for this shape:", MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", "My shape"); w->addButton ("Save", 1, KeyPress (KeyPress::returnKey)); w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
            w->enterModalState (true, ModalCallbackFunction::create ([this, i, w] (int res)
            {
                if (res == 1) { const auto n = w->getTextEditorContents ("name").trim(); if (n.isNotEmpty() && ctx.pm.saveUserShape (n, ctx.proc.getShape (i))) ctx.proc.setShape (i, ctx.proc.getShape (i), n); }
            }), true);
        }
        else if (r == 3) { PresetManager::venvDir().createDirectory(); PresetManager::venvDir().revealToUser(); }
    });
}

void ModulatorsPage::timerCallback()
{
    for (int i = 0; i < kNumAttr; ++i)
    {
        const int t = ctx.choice (attrId (i, "type"));
        attrP1[i]->setLabel (attrPName (t, 0)); attrP2[i]->setLabel (attrPName (t, 1));
    }
    for (auto* r : routing) r->repaint();
    for (auto* e : extras) if (auto* a = dynamic_cast<ActionChip*> (e)) a->repaint();
}

void ModulatorsPage::relayout()
{
    const int w = bodyWidth(), gap = 8; int y = gap;
    linkScope->setSize (jlimit (200, 700, w - 380), 70);
    const int lh = loop.layoutFlow (w - 2 * gap); loop.setBounds (gap, y, w - 2 * gap, lh); y += lh + gap;
    auto grid = [&] (OwnedArray<Panel>& ps, int perRow, std::function<void (int, int)> size)
    {
        const int cw = (w - gap * (perRow + 1)) / perRow;
        for (int r0 = 0; r0 < ps.size(); r0 += perRow)
        {
            int rowH = 0;
            for (int k = r0; k < jmin (ps.size(), r0 + perRow); ++k) { size (k, cw); rowH = jmax (rowH, ps[k]->layoutFlow (cw)); }
            for (int k = r0; k < jmin (ps.size(), r0 + perRow); ++k) ps[k]->setBounds (gap + (k - r0) * (cw + gap), y, cw, rowH);
            y += rowH + gap;
        }
    };
    int ri = 0;
    grid (lfo, w > 1300 ? 4 : 2, [&] (int k, int cw)
    {
        for (auto* c : lfo[k]->flow) if (dynamic_cast<LfoPreview*> (c)) c->setSize (cw - 16, 44);
        routing[k]->setSize (cw - 16, 26); (void) ri;
    });
    grid (venv, 2, [&] (int k, int cw)
    {
        canvases[k]->setSize (cw - 16, 130);
        routing[kNumLfo + k]->setSize (cw - 16, 26);
    });
    grid (attr, 2, [&] (int k, int cw)
    {
        for (auto* c : attr[k]->flow) if (dynamic_cast<AttractorView*> (c)) c->setSize (jmin (220, cw - 16), 130);
        routing[kNumLfo + kNumVenv + k]->setSize (cw - 16, 26);
    });
    grid (trig, w > 1300 ? 4 : 2, [&] (int k, int cw) { routing[kNumLfo + kNumVenv + kNumAttr + k]->setSize (cw - 16, 26); });
    body.setSize (w, y + gap);
}

//==============================================================================
MatrixRow::MatrixRow (Ctx& c, int s) : ctx (c), slot (s)
{
    src = std::make_unique<Choice> (c, slotId (s, "src"), ""); addAndMakeVisible (*src);
    pol = std::make_unique<Choice> (c, slotId (s, "pol"), ""); addAndMakeVisible (*pol);
    curve = std::make_unique<Choice> (c, slotId (s, "curve"), ""); addAndMakeVisible (*curve);
    quant = std::make_unique<Choice> (c, slotId (s, "quant"), ""); addAndMakeVisible (*quant);
    for (auto* k : { &cc, &amt, &lag, &fin, &fout, &jit })
    {
        static const char* keys[6] = { "cc", "amt", "lag", "fin", "fout", "jit" };
        const int idx = (int) (k - &cc);
        *k = std::make_unique<Knob> (c, slotId (s, keys[idx]), "", true); addAndMakeVisible (k->get());
    }
    addAndMakeVisible (dst); addAndMakeVisible (del);
    dst.onClick = [this] { destMenu(); };
    del.onClick = [this] { ctx.removeRoute (slot); };
    startTimerHz (6);
    timerCallback();
}
int MatrixRow::status (Ctx& c, int s)
{
    const int si = c.choice (slotId (s, "src")), di = c.choice (slotId (s, "dst"));
    if (si <= 0 || di <= 0 || std::abs (c.real (slotId (s, "amt"))) < 1e-6f) return 0;
    if (di >= kDestLaneBase && di < kDestLfoRate)
    {
        const int l = (di - kDestLaneBase) / LD_Count, k = (di - kDestLaneBase) % LD_Count;
        const int e = c.proc.engineOfLane (l);
        if (e <= 0) return 3;
        if (k >= LD_M1 && k <= LD_M6 && k - LD_M1 >= (int) engineSpecs()[(size_t) e].p.size()) return 3;
    }
    auto modOfSrc = [&] (int x) { if (x >= S_Lfo1 && x < S_Venv1) return x - S_Lfo1; if (x >= S_Venv1 && x < S_Attr1) return 8 + x - S_Venv1; if (x >= S_Attr1 && x < S_Env1) return 12 + x - S_Attr1;
                                  if (x >= S_Trig1 && x < S_Trig1 + kNumTrig) return 16 + x - S_Trig1; return -1; };
    auto modOfDst = [&] (int d)
    {
        if (d >= kDestLfoRate && d < kDestVenvTime) return (d - kDestLfoRate) % kNumLfo;
        if (d >= kDestVenvTime && d < kDestAttrRate) return 8 + (d - kDestVenvTime) % kNumVenv;
        if (d >= kDestAttrRate && d < kDestLoopLen) return 12 + (d - kDestAttrRate) % kNumAttr;
        if (d >= kDestTrigRate && d < kNumDests) return 16 + (d - kDestTrigRate) % kNumTrig;
        return -1;
    };
    const bool polySrc = isPolySourceKind (si)
                         || (si >= S_Lfo1 && si < S_Venv1 && c.choice (lfoId (si - S_Lfo1, "mode")) == 1)
                         || (si >= S_Venv1 && si < S_Attr1 && c.choice (venvId (si - S_Venv1, "voicing")) == 1)
                         || (si >= S_Attr1 && si < S_Env1 && c.choice (attrId (si - S_Attr1, "mode")) == 1);
    bool globalDest = di >= kDestLfoRate;
    if (di < kDestLfoRate) { const int k = (di - kDestLaneBase) % LD_Count; globalDest = k == LD_Pan || k == LD_Width || k == LD_Drive || (k >= LD_Fx1 && k <= LD_EqMorph); }
    if (polySrc && globalDest) return 4;
    const int a = modOfSrc (si), b = modOfDst (di);
    if (a >= 0 && b >= 0)
    {
        bool adj[20][20] = {};
        for (int i = 0; i < kNumSlots; ++i)
        {
            const int x = modOfSrc (c.choice (slotId (i, "src"))), y = modOfDst (c.choice (slotId (i, "dst")));
            if (x >= 0 && y >= 0) adj[x][y] = true;
        }
        bool seen[20] = {}; Array<int> stack { b };
        while (! stack.isEmpty()) { const int n = stack.removeAndReturn (stack.size() - 1); if (n == a) return 2; if (seen[n]) continue; seen[n] = true; for (int m = 0; m < 20; ++m) if (adj[n][m]) stack.add (m); }
    }
    return 1;
}
void MatrixRow::timerCallback()
{
    const int d = ctx.choice (slotId (slot, "dst"));
    const String t = d > 0 ? destNames()[d] : String ("-");
    if (dst.getButtonText() != t) dst.setButtonText (t);
    const bool isCC = ctx.choice (slotId (slot, "src")) == S_CC;
    cc->setAlpha (isCC ? 1.f : 0.25f);
    const int st = status (ctx, slot);
    if (st != lastStatus) { lastStatus = st; repaint(); }
}
void MatrixRow::destMenu()
{
    PopupMenu m; const auto dn = destNames();
    StringArray groups; for (int d = 1; d < kNumDests; ++d) groups.addIfNotAlreadyThere (destGroup (d));
    m.addItem (1000, "None");
    for (auto& gname : groups)
    {
        PopupMenu sub;
        if (gname == "FX")
        {
            for (int b = 0; b < kNumBuses; ++b)
            {
                PopupMenu bs; const String bn = String::charToString ((juce_wchar) ('A' + b));
                for (int d = kDestBusLevel; d < kNumDests; ++d) if (dn[d].startsWith ("FX " + bn)) bs.addItem (1000 + d, dn[d]);
                sub.addSubMenu ("Bus " + bn, bs);
            }
        }
        else for (int d = 1; d < kNumDests; ++d) if (destGroup (d) == gname) sub.addItem (1000 + d, dn[d]);
        m.addSubMenu (gname, sub);
    }
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&dst), [this] (int r) { if (r >= 1000) { ctx.proc.undo.beginNewTransaction(); ctx.setReal (slotId (slot, "dst"), (float) (r - 1000)); } });
}
void MatrixRow::resized()
{
    auto r = getLocalBounds().reduced (0, 2); r.removeFromLeft (34);
    src->setBounds (r.removeFromLeft (130)); cc->setBounds (r.removeFromLeft (30));
    dst.setBounds (r.removeFromLeft (170).reduced (2, 1)); amt->setBounds (r.removeFromLeft (34));
    pol->setBounds (r.removeFromLeft (86)); curve->setBounds (r.removeFromLeft (86));
    lag->setBounds (r.removeFromLeft (34)); fin->setBounds (r.removeFromLeft (34)); fout->setBounds (r.removeFromLeft (34)); jit->setBounds (r.removeFromLeft (34));
    quant->setBounds (r.removeFromLeft (92)); del.setBounds (r.removeFromLeft (26).reduced (2, 2));
}
void MatrixRow::paint (Graphics& g)
{
    auto& t = Theme::c();
    if (slot % 2) { g.setColour (t.panelHi.withAlpha (0.35f)); g.fillRect (getLocalBounds()); }
    const Colour cols[5] = { t.dim.withAlpha (0.4f), t.green, t.blue, t.warn, t.red };
    g.setColour (cols[jlimit (0, 4, lastStatus)]); g.fillEllipse (6.f, getHeight() * 0.5f - 4.f, 8.f, 8.f);
    g.setColour (t.dim); g.setFont (FontOptions (10.f)); g.drawText (String (slot + 1), 16, 0, 16, getHeight(), Justification::centred);
}

MatrixPage::MatrixPage (Ctx& c) : ctx (c), table (c, "MODULATION MATRIX"), keyPanel (c, "KEYTRACK"), macroPanel (c, "MACROS"), distPanel (c, "DRAWN DISTRIBUTION")
{
    for (auto* p : { &table, &keyPanel, &macroPanel, &distPanel }) body.addAndMakeVisible (p);
    distEd = std::make_unique<KeyEditor> (c, true); distPanel.addCustom (distEd.get());
    distPanel.choice ("nrand_dist", "Note Random")->setSize (110, 40);
    distPanel.addCustom (new TextLine ([] { return String ("Density used by every 'Drawn' choice (LFO, trigger and Note Random dists). Higher = more likely."); }, 10.f));
    distPanel.owned.add (distPanel.flow.getLast());
    header = std::make_unique<TextLine> ([] { return String ("        SOURCE                                    CC     DESTINATION                                   AMT     POLARITY            CURVE                LAG    FADE IN  FADE OUT  JITTER  QUANT"); }, 10.f);
    table.addAndMakeVisible (*header);
    for (int i = 0; i < kNumSlots; ++i) table.addAndMakeVisible (rows.add (new MatrixRow (c, i)));
    keyEd = std::make_unique<KeyEditor> (c); keyPanel.addCustom (keyEd.get());
    keyPanel.addCustom (new TextLine ([] { return String ("Draw how each MIDI note maps to the Keytrack source. Double-click adds a point, right-click removes it."); }, 10.f));
    keyPanel.owned.add (keyPanel.flow.getLast());
    for (int i = 0; i < kNumMacros; ++i) macroPanel.knob ("macro" + String (i + 1), "M" + String (i + 1));
    macroRoutes = std::make_unique<TextLine> ([this]
    {
        const auto dn = destNames(); String s;
        for (int m = 0; m < kNumMacros; ++m)
        {
            StringArray d;
            for (int i = 0; i < kNumSlots; ++i) if (ctx.choice (slotId (i, "src")) == S_Macro1 + m && ctx.choice (slotId (i, "dst")) > 0)
                d.add (dn[ctx.choice (slotId (i, "dst"))] + " " + String (ctx.real (slotId (i, "amt")), 2));
            s << "M" << (m + 1) << ": " << (d.isEmpty() ? String ("-") : d.joinIntoString (", ")) << "\n";
        }
        return s;
    }, 10.f);
    macroPanel.addCustom (macroRoutes.get());
}
void MatrixPage::relayout()
{
    const int w = bodyWidth(), gap = 8;
    const int tableW = jmin (w - 2 * gap, 960);
    const bool side = w - tableW - 3 * gap >= 320;
    header->setBounds (6, Panel::titleH, tableW - 12, 18);
    for (int i = 0; i < kNumSlots; ++i) rows[i]->setBounds (4, Panel::titleH + 20 + i * 32, tableW - 8, 32);
    const int th = Panel::titleH + 24 + kNumSlots * 32;
    table.setBounds (gap, gap, tableW, th);
    const int sx = side ? tableW + 2 * gap : gap, sy = side ? gap : th + 2 * gap;
    const int sw = side ? w - tableW - 3 * gap : w - 2 * gap;
    keyEd->setSize (sw - 16, 150); keyPanel.flow.getLast()->setSize (sw - 16, 30);
    const int kh = keyPanel.layoutFlow (sw); keyPanel.setBounds (sx, sy, sw, kh);
    macroRoutes->setSize (sw - 16, 90);
    const int mh = macroPanel.layoutFlow (sw); macroPanel.setBounds (sx, sy + kh + gap, sw, mh);
    distEd->setSize (sw - 16, 110); distPanel.flow.getLast()->setSize (sw - 16, 30);
    const int dh = distPanel.layoutFlow (sw); distPanel.setBounds (sx, sy + kh + mh + 2 * gap, sw, dh);
    body.setSize (w, jmax (th + gap * 2, sy + kh + mh + dh + gap * 3));
}

//==============================================================================
MasterPage::MasterPage (Ctx& c)
    : ctx (c), chainBtn ("CHAIN", [this] { view = 0; relayout(); }), fxBtn ("FX", [this] { view = 1; relayout(); }), delugeBtn ("DELUGE", [this] { view = 2; relayout(); }),
      eqP (c, "EQ"), satP (c, "SATURATOR"), compP (c, "COMPRESSOR"), widthP (c, "WIDTH"), limP (c, "LIMITER"), outP (c, "OUTPUT"), visP (c, "VISUALISERS")
{
    chainBtn.isOn = [this] { return view == 0; }; fxBtn.isOn = [this] { return view == 1; }; delugeBtn.isOn = [this] { return view == 2; };
    body.addAndMakeVisible (chainBtn); body.addAndMakeVisible (fxBtn); body.addAndMakeVisible (delugeBtn);
    // ---- DELUGE FX: effects ported from the Synthstrom Audible Deluge firmware (GPL-3.0) ----
    {
        auto P = [&] (const char* t) { auto* p = dlP.add (new Panel (c, t)); body.addAndMakeVisible (p); p->stripe = Theme::c().warn; return p; };
        auto* top = P ("DELUGE FX  /  insert before the master chain");
        top->chip ("dl_on", "ON"); top->knob ("dl_vol", "Volume");
        auto* credit = new TextLine ([] { return String ("Mod FX, EQ, decimation, bitcrush, saturation, delay, reverb, compressor and stutter ported from the "
                                                         "Synthstrom Audible Deluge community firmware (GPL-3.0). Values use the Deluge's 0-50 menu scale."); }, 10.f);
        top->owned.add (credit); credit->setSize (560, 30); top->addCustom (credit);
        auto* mod = P ("MOD FX"); mod->choice ("dl_modtype", "Type")->setSize (120, 40);
        mod->knob ("dl_modrate", "Rate"); mod->knob ("dl_moddepth", "Depth"); mod->knob ("dl_modfb", "Feedback"); mod->knob ("dl_modoff", "Offset");
        auto* eq = P ("EQ"); eq->knob ("dl_bass", "Bass"); eq->knob ("dl_bassf", "Bass F"); eq->knob ("dl_treble", "Treble"); eq->knob ("dl_treblef", "Treble F");
        auto* lofi = P ("DESTRUCTION"); lofi->knob ("dl_srr", "Decimate"); lofi->knob ("dl_crush", "Bitcrush"); lofi->knob ("dl_sat", "Saturation");
        auto* dly = P ("DELAY"); dly->knob ("dl_dlyamt", "Amount"); dly->knob ("dl_dlyrate", "Rate"); dly->choice ("dl_dlysync", "Sync")->setSize (80, 40);
        dly->choice ("dl_dlysynctype", "Type")->setSize (80, 40); dly->chip ("dl_dlypp", "PING-PONG")->setSize (84, 22); dly->chip ("dl_dlyanalog", "ANALOG")->setSize (70, 22);
        auto* rvb = P ("REVERB"); rvb->choice ("dl_rvbmodel", "Model")->setSize (100, 40); rvb->knob ("dl_rvbamt", "Amount"); rvb->knob ("dl_rvbroom", "Room");
        rvb->knob ("dl_rvbdamp", "Damping"); rvb->knob ("dl_rvbwidth", "Width"); rvb->knob ("dl_rvbhpf", "HPF"); rvb->knob ("dl_rvblpf", "LPF"); rvb->knob ("dl_rvbpan", "Pan");
        auto* cmp = P ("COMPRESSOR"); cmp->knob ("dl_cmpthr", "Threshold"); cmp->knob ("dl_cmpratio", "Ratio"); cmp->knob ("dl_cmpatt", "Attack");
        cmp->knob ("dl_cmprel", "Release"); cmp->knob ("dl_cmpsc", "SC HPF"); cmp->knob ("dl_cmpblend", "Blend");
        dlGr = std::make_unique<LevelMeter> ([this] { return ctx.proc.delugeCompGR(); }, true); dlGr->setSize (16, 66); cmp->addCustom (dlGr.get());
        auto* stu = P ("STUTTER"); stu->chip ("dl_stutter", "STUTTER")->setSize (80, 22); stu->knob ("dl_sturate", "Rate");
        stu->chip ("dl_stuquant", "QUANTIZED")->setSize (84, 22); stu->chip ("dl_sturev", "REVERSE")->setSize (70, 22); stu->chip ("dl_stupp", "PING-PONG")->setSize (84, 22);
    }
    for (auto* p : { &eqP, &satP, &compP, &widthP, &limP, &outP, &visP }) body.addAndMakeVisible (p);
    curve = std::make_unique<EqCurve> (c, "m_", -1); eqP.addCustom (curve.get());
    eqP.chip ("m_eqon", "ON"); eqP.choice ("m_analyzer", "Analyzer")->setSize (90, 40);
    for (auto kv : { std::pair<const char*, const char*> { "m_hpf", "HPF" }, { "m_lsf", "Low F" }, { "m_lsg", "Low G" }, { "m_mf", "Mid F" }, { "m_mg", "Mid G" },
                     { "m_mq", "Mid Q" }, { "m_hsf", "High F" }, { "m_hsg", "High G" }, { "m_lpf", "LPF" } }) eqP.knob (kv.first, kv.second);
    eqP.choice ("m_hpfs", "HPF Slope")->setSize (76, 40); eqP.choice ("m_lpfs", "LPF Slope")->setSize (76, 40);
    satP.chip ("m_saton", "ON"); satP.knob ("m_satdrive", "Drive"); satP.knob ("m_satmix", "Mix"); satP.choice ("m_satflav", "Flavour")->setSize (76, 40); satP.choice ("m_satos", "Overs")->setSize (64, 40);
    compP.chip ("m_compon", "ON"); compP.chip ("m_sc", "SIDECHAIN")->setSize (80, 22);
    for (auto kv : { std::pair<const char*, const char*> { "m_thr", "Thres" }, { "m_ratio", "Ratio" }, { "m_att", "Att" }, { "m_rel", "Rel" }, { "m_knee", "Knee" },
                     { "m_cgain", "Gain" }, { "m_glue", "Glue" }, { "m_schpf", "Sc Hpf" } }) compP.knob (kv.first, kv.second);
    compGr = std::make_unique<LevelMeter> ([this] { return ctx.proc.masterChain().compGR.load(); }, true); compGr->setSize (16, 66); compP.addCustom (compGr.get());
    widthP.chip ("m_widthon", "ON"); widthP.knob ("m_ms", "M/S"); widthP.knob ("m_mono", "Mono");
    limP.chip ("m_limon", "ON"); limP.knob ("m_ceil", "Ceiling"); limP.knob ("m_lrel", "Rel"); limP.choice ("m_lrelmode", "Release")->setSize (76, 40);
    limGr = std::make_unique<LevelMeter> ([this] { return ctx.proc.masterChain().limGR.load(); }, true); limGr->setSize (16, 66); limP.addCustom (limGr.get());
    outP.chip ("m_bypass", "BYPASS MASTER CHAIN")->setSize (150, 22); outP.knob ("out_gain", "Level");
    vu = std::make_unique<VuMeter> (c); vu->setSize (130, 210); outP.addCustom (vu.get());
    water = std::make_unique<Waterfall> (c); square = std::make_unique<SquareVis> (c);
    visP.addCustom (water.get()); visP.addCustom (square.get());

    for (int b = 0; b < kNumBuses; ++b)
    {
        const String bn = String::charToString ((juce_wchar) ('A' + b));
        auto* bp = busP.add (new Panel (c, "BUS " + bn)); body.addAndMakeVisible (bp);
        bp->knob (busId (b, "level"), "Bus Level");
        auto* m = own (extras, new LevelMeter ([this, b] { return ctx.proc.ui.busPk[b].load(); })); m->setSize (12, 66); bp->addCustom (m);
        for (int s = 0; s < kSlotsPerBus; ++s)
        {
            auto* sp = slotP.add (new Panel (c, "FX " + String (s + 1))); body.addAndMakeVisible (sp);
            auto* ch = sp->choice (fxId (b, s, "type"), "Type"); ch->setSize (110, 40);
            ch->onUserChange = [this, b, s] (int t) { for (int k = 0; k < kFxParams; ++k) ctx.setNorm (fxId (b, s, ("p" + String (k + 1)).toRawUTF8()), kFxDefaults[jlimit (0, kNumFxTypes - 1, t)][k]); };
            sp->chip (fxId (b, s, "on"), "ON");
            optChips.add (sp->chip (fxId (b, s, "opt"), "OPT")); optChips.getLast()->setSize (80, 22);
            for (int k = 0; k < kFxParams; ++k)
            {
                auto* kn = sp->knob (fxId (b, s, ("p" + String (k + 1)).toRawUTF8()), "P" + String (k + 1));
                kn->setTextFn ([this, b, s, k] (float v) { return fxParamText (ctx.choice (fxId (b, s, "type")), k, v); });
                fxKnobs.add (kn);
            }
            auto* mi = own (extras, new LevelMeter ([this, b, s] { return ctx.proc.fxSlot (b, s).inPk.load(); })); mi->setSize (10, 66); sp->addCustom (mi);
            auto* mo = own (extras, new LevelMeter ([this, b, s] { return ctx.proc.fxSlot (b, s).outPk.load(); })); mo->setSize (10, 66); sp->addCustom (mo);
        }
    }
    startTimerHz (6);
}

void MasterPage::timerCallback()
{
    String sig; for (int b = 0; b < kNumBuses; ++b) for (int s = 0; s < kSlotsPerBus; ++s) sig << ctx.choice (fxId (b, s, "type"));
    for (int b = 0; b < kNumBuses; ++b)
        for (int s = 0; s < kSlotsPerBus; ++s)
        {
            const int t = ctx.choice (fxId (b, s, "type")), si = b * kSlotsPerBus + s;
            for (int k = 0; k < kFxParams; ++k) { auto* kn = fxKnobs[si * kFxParams + k]; kn->setVisible (k < fxParamCount (t)); kn->setLabel (fxParamName (t, k)); }
            auto* oc = optChips[si];
            const bool hasOpt = fxOptName (t) != nullptr;
            oc->setVisible (hasOpt);
            slotP[si]->title = "FX " + String (s + 1) + "  /  " + kFxTypeNames[jlimit (0, kNumFxTypes - 1, t)] + (hasOpt ? String ("  (OPT = ") + fxOptLabel (t, true) + ")" : String());
            slotP[si]->repaint();
        }
    chainBtn.repaint(); fxBtn.repaint(); delugeBtn.repaint();
    if (sig != typeSig) { typeSig = sig; relayout(); }
}

void MasterPage::relayout()
{
    const int w = bodyWidth(), gap = 8; int y = gap;
    chainBtn.setBounds (gap, y, 80, 24); fxBtn.setBounds (gap + 86, y, 60, 24); delugeBtn.setBounds (gap + 152, y, 80, 24); y += 32;
    for (auto* p : { &eqP, &satP, &compP, &widthP, &limP, &outP, &visP }) p->setVisible (view == 0);
    for (auto* p : busP) p->setVisible (view == 1);
    for (auto* p : slotP) p->setVisible (view == 1);
    for (auto* p : dlP) p->setVisible (view == 2);
    if (view == 2)
    {
        int x = gap, rowH = 0;
        for (auto* p : dlP)
        {
            int pw = 0; for (auto* c : p->flow) pw += c->getWidth() + 2;
            pw = jlimit (180, w - 2 * gap, pw + 16);
            if (x + pw > w - gap && x > gap) { x = gap; y += rowH + gap; rowH = 0; }
            const int h = p->layoutFlow (pw); p->setBounds (x, y, pw, h); x += pw + gap; rowH = jmax (rowH, h);
        }
        y += rowH + gap;
    }
    else if (view == 0)
    {
        curve->setSize (jmin (520, w - 40), 130);
        int x = gap, rowH = 0;
        auto put = [&] (Panel& p, int pw)
        {
            pw = jmin (pw, w - 2 * gap);
            if (x + pw > w - gap && x > gap) { x = gap; y += rowH + gap; rowH = 0; }
            const int h = p.layoutFlow (pw); p.setBounds (x, y, pw, h); x += pw + gap; rowH = jmax (rowH, h);
        };
        put (eqP, jmin (540, w - 2 * gap)); put (outP, 300); put (satP, 260); put (compP, 400); put (widthP, 200); put (limP, 300);
        y += rowH + gap;
        const int vw = w - 2 * gap;
        square->setSize (jmin (240, vw / 3), 220); water->setSize (vw - 16 - square->getWidth() - 4, 220);
        const int vh = visP.layoutFlow (vw); visP.setBounds (gap, y, vw, vh); y += vh + gap;
    }
    else
    {
        const int cw = (w - gap * (kNumBuses + 1)) / kNumBuses;
        int maxY = y;
        for (int b = 0; b < kNumBuses; ++b)
        {
            int yy = y; const int x = gap + b * (cw + gap);
            const int bh = busP[b]->layoutFlow (cw); busP[b]->setBounds (x, yy, cw, bh); yy += bh + gap;
            for (int s = 0; s < kSlotsPerBus; ++s) { auto* sp = slotP[b * kSlotsPerBus + s]; const int h = sp->layoutFlow (cw); sp->setBounds (x, yy, cw, h); yy += h + gap; }
            maxY = jmax (maxY, yy);
        }
        y = maxY;
    }
    body.setSize (w, y + gap);
}
} // namespace cz
