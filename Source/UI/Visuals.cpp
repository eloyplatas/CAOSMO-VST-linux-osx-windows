#include "Visuals.h"
#include <complex>

using namespace juce;

namespace cz
{
//==============================================================================
void computeSpectrum (const ScopeRing& ring, float* bins, int nb, double sr, float lo, float hi)
{
    static dsp::FFT fft (11);
    static std::vector<float> buf (4096), win;
    if (win.empty()) { win.resize (2048); for (int i = 0; i < 2048; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * (float) i / 2047.f); }
    ring.latest (buf.data(), 2048);
    for (int i = 0; i < 2048; ++i) buf[(size_t) i] *= win[(size_t) i];
    std::fill (buf.begin() + 2048, buf.end(), 0.f);
    fft.performFrequencyOnlyForwardTransform (buf.data());
    const float binHz = (float) sr / 2048.f;
    for (int b = 0; b < nb; ++b)
    {
        const float f0 = lo * std::pow (hi / lo, (float) b / (float) nb), f1 = lo * std::pow (hi / lo, (float) (b + 1) / (float) nb);
        int i0 = jlimit (1, 1023, (int) (f0 / binHz)), i1 = jlimit (i0, 1023, (int) (f1 / binHz));
        float m = 0; for (int i = i0; i <= i1; ++i) m = jmax (m, buf[(size_t) i]);
        const float db = Decibels::gainToDecibels (m / 512.f, -96.f);
        bins[b] = jlimit (0.f, 1.f, (db + 84.f) / 84.f);
    }
}

static Colour palette (int pal, float v)
{
    auto& t = Theme::c();
    v = jlimit (0.f, 1.f, v);
    if (pal == 1) return Colour (0xff3a6ea5).interpolatedWith (Colours::white, v);
    return t.accent.interpolatedWith (t.accent2, 1.f - v).withMultipliedBrightness (0.6f + 0.6f * v);
}

//==============================================================================
Scope::Scope (Ctx& c, int l) : ctx (c), lane (l) { startTimerHz (30); }
void Scope::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    g.setColour (t.line.withAlpha (0.6f)); g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight()); g.drawVerticalLine ((int) r.getCentreX(), r.getY(), r.getBottom());
    const Colour col = Theme::engine (ctx.proc.engineOfLane (lane));
    const int mode = ctx.prefs.scopeMode[lane];
    static float L[2048], R[2048];
    Path p;
    if (mode == 0)
    {
        ctx.proc.ui.laneL[lane].latest (L, 1024); ctx.proc.ui.laneR[lane].latest (R, 1024);
        float mx = 1e-4f; for (int i = 0; i < 1024; ++i) mx = jmax (mx, std::abs (L[i]), std::abs (R[i]));
        const float k = 0.45f * jmin (r.getWidth(), r.getHeight()) / jmax (mx, 0.05f);
        for (int i = 0; i < 1024; ++i)
        {
            const float x = r.getCentreX() + (L[i] - R[i]) * 0.7071f * k, y = r.getCentreY() - (L[i] + R[i]) * 0.7071f * k;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
    }
    else if (mode == 1)
    {
        ctx.proc.ui.laneL[lane].latest (L, 2048); ctx.proc.ui.laneR[lane].latest (R, 2048);
        int start = 512; for (int i = 512; i < 1500; ++i) if (L[i - 1] + R[i - 1] <= 0 && L[i] + R[i] > 0) { start = i; break; }
        float mx = 1e-4f; for (int i = start; i < start + 512; ++i) mx = jmax (mx, std::abs (L[i] + R[i]) * 0.5f);
        for (int i = 0; i < 512; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / 511.f, y = r.getCentreY() - (L[start + i] + R[start + i]) * 0.5f / jmax (mx, 0.05f) * r.getHeight() * 0.42f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
    }
    else if (mode == 2)
    {
        ctx.proc.ui.engine[lane].latest (L, 1536);
        int zc = 0; for (int i = 1; i < 1536; ++i) if (L[i - 1] <= 0 && L[i] > 0) ++zc;
        const int tau = jlimit (2, 200, zc > 1 ? 1536 / zc / 4 : 12);
        float mx = 1e-4f; for (int i = 0; i < 1536; ++i) mx = jmax (mx, std::abs (L[i]));
        const float k = 0.45f * jmin (r.getWidth(), r.getHeight()) / jmax (mx, 0.02f);
        for (int i = tau; i < 1536; ++i)
        {
            const float x = r.getCentreX() + L[i] * k, y = r.getCentreY() - L[i - tau] * k;
            if (i == tau) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
    }
    else
    {
        if (ctx.proc.engineOfLane (lane) != 9) { g.setColour (t.dim); g.setFont (FontOptions (11.f)); g.drawText ("Grain view needs Wave Grains", r, Justification::centred); return; }
        for (int i = 0; i < 16; ++i)
        {
            const float ph = ctx.proc.ui.grains[lane][i][0].load(), a = ctx.proc.ui.grains[lane][i][1].load(), pan = ctx.proc.ui.grains[lane][i][2].load();
            if (ph < 0) continue;
            const float x = r.getX() + 6 + ph * (r.getWidth() - 12), y = r.getCentreY() - pan * r.getHeight() * 0.35f, s = 3.f + a * 10.f;
            g.setColour (col.withAlpha (0.25f + 0.6f * a)); g.fillEllipse (x - s * 0.5f, y - s * 0.5f, s, s);
        }
        return;
    }
    g.setColour (col.withAlpha (0.85f)); g.strokePath (p, PathStrokeType (1.f));
}

//==============================================================================
EqCurve::EqCurve (Ctx& c, const String& prefix, int l) : ctx (c), pre (prefix), lane (l) { startTimerHz (20); }
float EqCurve::xOf (float hz) const { return 4.f + (getWidth() - 8.f) * std::log (hz / 20.f) / std::log (1000.f); }
float EqCurve::hzOf (float x) const { return 20.f * std::pow (1000.f, jlimit (0.f, 1.f, (x - 4.f) / (getWidth() - 8.f))); }
float EqCurve::yOf (float db) const { return getHeight() * 0.5f - db / 20.f * (getHeight() * 0.45f); }
float EqCurve::dbOf (float y) const { return (getHeight() * 0.5f - y) / (getHeight() * 0.45f) * 20.f; }
Point<float> EqCurve::node (int i) const
{
    switch (i)
    {
        case 0: return { xOf (ctx.real (id ("hpf"))), yOf (0) };
        case 1: return { xOf (ctx.real (id ("lsf"))), yOf (ctx.real (id ("lsg"))) };
        case 2: return { xOf (ctx.real (id ("mf"))), yOf (ctx.real (id ("mg"))) };
        case 3: return { xOf (ctx.real (id ("hsf"))), yOf (ctx.real (id ("hsg"))) };
        default: return { xOf (ctx.real (id ("lpf"))), yOf (0) };
    }
}
static float bqDb (const BQ& b, float f, float sr)
{
    const float w = MathConstants<float>::twoPi * f / sr;
    const std::complex<float> z1 = std::polar (1.f, -w), z2 = std::polar (1.f, -2.f * w);
    const auto num = b.b0 + b.b1 * z1 + b.b2 * z2, den = 1.f + b.a1 * z1 + b.a2 * z2;
    return 20.f * std::log10 (std::max (1e-6f, std::abs (num) / std::max (1e-9f, std::abs (den))));
}
void EqCurve::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    g.setColour (t.line.withAlpha (0.5f));
    for (float f : { 100.f, 1000.f, 10000.f }) g.drawVerticalLine ((int) xOf (f), 2.f, r.getBottom() - 2.f);
    for (float d : { -12.f, 0.f, 12.f }) g.drawHorizontalLine ((int) yOf (d), 2.f, r.getRight() - 2.f);
    const float sr = (float) ctx.proc.getSr();
    // analyser
    const bool isMaster = lane < 0;
    const int an = isMaster ? ctx.choice ("m_analyzer") : 2;
    if (ctx.prefs.visuals && an > 0)
    {
        float bins[120];
        auto draw = [&] (const ScopeRing& ring, Colour c)
        {
            computeSpectrum (ring, bins, 120, sr, 20.f, 20000.f);
            Path p; p.startNewSubPath (4.f, r.getBottom());
            for (int b = 0; b < 120; ++b) p.lineTo (4.f + (getWidth() - 8.f) * (float) b / 119.f, r.getBottom() - bins[b] * r.getHeight() * 0.9f);
            p.lineTo (r.getRight() - 4.f, r.getBottom()); p.closeSubPath();
            g.setColour (c.withAlpha (0.18f)); g.fillPath (p);
        };
        if (! isMaster) draw (ctx.proc.ui.laneL[lane], Theme::engine (ctx.proc.engineOfLane (lane)));
        else { if (an == 1 || an == 3) draw (ctx.proc.ui.masterPre, t.dim); if (an >= 2) draw (ctx.proc.ui.masterPost, t.accent); }
    }
    // response
    BQ hp, ls, bell, hs, lp;
    const float v[11] = { ctx.real (id ("hpf")), (float) ctx.choice (id ("hpfs")), ctx.real (id ("lsf")), ctx.real (id ("lsg")), ctx.real (id ("mf")), ctx.real (id ("mg")),
                          ctx.real (id ("mq")), ctx.real (id ("hsf")), ctx.real (id ("hsg")), ctx.real (id ("lpf")), (float) ctx.choice (id ("lpfs")) };
    hp.set (1, v[0], sr, 0.7071f, 0); ls.set (3, v[2], sr, 0.7071f, v[3]); bell.set (2, v[4], sr, v[6], v[5]); hs.set (4, v[7], sr, 0.7071f, v[8]); lp.set (0, v[9], sr, 0.7071f, 0);
    const int hpN = v[0] > 20.5f ? (int) v[1] + 1 : 0, lpN = v[9] < 19900.f ? (int) v[10] + 1 : 0;
    Path resp;
    for (int i = 0; i <= 160; ++i)
    {
        const float f = 20.f * std::pow (1000.f, (float) i / 160.f);
        float db = hpN * bqDb (hp, f, sr) + lpN * bqDb (lp, f, sr);
        if (std::abs (v[3]) > 0.05f) db += bqDb (ls, f, sr);
        if (std::abs (v[5]) > 0.05f) db += bqDb (bell, f, sr);
        if (std::abs (v[8]) > 0.05f) db += bqDb (hs, f, sr);
        const float y = jlimit (2.f, r.getBottom() - 2.f, yOf (db));
        if (i == 0) resp.startNewSubPath (xOf (f), y); else resp.lineTo (xOf (f), y);
    }
    g.setColour (t.accent); g.strokePath (resp, PathStrokeType (1.6f));
    static const uint32 nc[5] = { 0xff5aa9ff, 0xff7ad36b, 0xfff2b84b, 0xffff6f91, 0xff9b8cff };
    for (int i = 0; i < 5; ++i)
    {
        auto p = node (i);
        g.setColour (Colour (nc[i]).withAlpha (drag == i ? 1.f : 0.85f)); g.fillEllipse (p.x - 4.5f, p.y - 4.5f, 9.f, 9.f);
    }
}
void EqCurve::mouseDown (const MouseEvent& e)
{
    drag = -1; float best = 12.f;
    for (int i = 0; i < 5; ++i) { const float d = node (i).getDistanceFrom (e.position); if (d < best) { best = d; drag = i; } }
    if (drag >= 0)
    {
        ctx.proc.undo.beginNewTransaction();
        static const char* fk[5] = { "hpf", "lsf", "mf", "hsf", "lpf" }; static const char* gk[5] = { "", "lsg", "mg", "hsg", "" };
        if (auto* p = ctx.param (id (fk[drag]))) p->beginChangeGesture();
        if (*gk[drag]) if (auto* p = ctx.param (id (gk[drag]))) p->beginChangeGesture();
    }
}
void EqCurve::mouseDrag (const MouseEvent& e)
{
    if (drag < 0) return;
    static const char* fk[5] = { "hpf", "lsf", "mf", "hsf", "lpf" }; static const char* gk[5] = { "", "lsg", "mg", "hsg", "" };
    if (auto* p = ctx.param (id (fk[drag]))) p->setValueNotifyingHost (p->convertTo0to1 (jlimit (p->getNormalisableRange().start, p->getNormalisableRange().end, hzOf (e.position.x))));
    if (*gk[drag]) if (auto* p = ctx.param (id (gk[drag]))) p->setValueNotifyingHost (p->convertTo0to1 (jlimit (-18.f, 18.f, dbOf (e.position.y))));
    repaint();
}
void EqCurve::mouseUp (const MouseEvent&)
{
    if (drag < 0) return;
    static const char* fk[5] = { "hpf", "lsf", "mf", "hsf", "lpf" }; static const char* gk[5] = { "", "lsg", "mg", "hsg", "" };
    if (auto* p = ctx.param (id (fk[drag]))) p->endChangeGesture();
    if (*gk[drag]) if (auto* p = ctx.param (id (gk[drag]))) p->endChangeGesture();
    drag = -1;
}
void EqCurve::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    int n = -1; float best = 14.f;
    for (int i = 0; i < 5; ++i) { const float d = node (i).getDistanceFrom (e.position); if (d < best) { best = d; n = i; } }
    if (n < 0) return;
    const float dir = w.deltaY > 0 ? 1.f : -1.f;
    if (n == 2) ctx.setReal (id ("mq"), jlimit (0.2f, 8.f, ctx.real (id ("mq")) * (dir > 0 ? 1.15f : 1.f / 1.15f)));
    else if (n == 0) ctx.setReal (id ("hpfs"), (float) jlimit (0, 3, ctx.choice (id ("hpfs")) + (int) dir));
    else if (n == 4) ctx.setReal (id ("lpfs"), (float) jlimit (0, 3, ctx.choice (id ("lpfs")) + (int) dir));
}

//==============================================================================
VenvCanvas::VenvCanvas (Ctx& c, int i) : ctx (c), idx (i) { startTimerHz (30); timerCallback(); }
void VenvCanvas::timerCallback()
{
    if (! editing)
    {
        const auto s = ctx.proc.venvTree (idx).getProperty ("pts").toString();
        if (s != loaded) { loaded = s; shape = VShape::fromString (s); }
    }
    if (isShowing()) repaint();
}
Point<float> VenvCanvas::toScreen (float x, float y) const
{
    auto a = getLocalBounds().toFloat().reduced (6.f, 5.f);
    return { a.getX() + (x - vx0) / (vx1 - vx0) * a.getWidth(), a.getBottom() - y * a.getHeight() };
}
Point<float> VenvCanvas::fromScreen (Point<float> p) const
{
    auto a = getLocalBounds().toFloat().reduced (6.f, 5.f);
    return { vx0 + (p.x - a.getX()) / a.getWidth() * (vx1 - vx0), (a.getBottom() - p.y) / a.getHeight() };
}
int VenvCanvas::hit (Point<float> p) const
{
    for (int i = 0; i < shape.n; ++i) if (toScreen (shape.x[i], shape.y[i]).getDistanceFrom (p) < 7.f) return i;
    return -1;
}
void VenvCanvas::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    const float pa = ctx.real (venvId (idx, "pa")), pd = jmax (pa, ctx.real (venvId (idx, "pd"))), ps = jmax (pd, ctx.real (venvId (idx, "ps")));
    const float zx[5] = { 0.f, pa, pd, ps, 1.f };
    for (int z = 0; z < 4; ++z)
    {
        const float x0 = toScreen (zx[z], 0).x, x1 = toScreen (zx[z + 1], 0).x;
        g.setColour (t.panelHi.withAlpha (z % 2 ? 0.55f : 0.25f)); g.fillRect (x0, r.getY(), x1 - x0, r.getHeight());
        g.setColour (t.dim.withAlpha (0.7f)); g.setFont (FontOptions (9.5f)); g.drawText (String::charToString ("ADSR"[z]), (int) x0 + 2, 2, 12, 10, Justification::left);
    }
    if (ctx.choice (venvId (idx, "mode")) == 2) { g.setColour (t.warn.withAlpha (0.55f)); const float x0 = toScreen (pd, 0).x, x1 = toScreen (ps, 0).x; g.fillRect (x0, r.getY() + 1, x1 - x0, 3.f); }
    Path p;
    for (int i = 0; i <= 240; ++i)
    {
        const float x = vx0 + (vx1 - vx0) * (float) i / 240.f;
        auto s = toScreen (x, shape.eval (x));
        if (i == 0) p.startNewSubPath (s); else p.lineTo (s);
    }
    g.setColour (t.accent); g.strokePath (p, PathStrokeType (1.6f));
    for (int i = 0; i < shape.n; ++i) { auto s = toScreen (shape.x[i], shape.y[i]); g.setColour (i == drag ? t.warn : t.text); g.fillEllipse (s.x - 3, s.y - 3, 6, 6); }
    if (! stroke.isEmpty()) { Path sp; sp.startNewSubPath (stroke[0]); for (auto& q : stroke) sp.lineTo (q); g.setColour (t.warn); g.strokePath (sp, PathStrokeType (1.f)); }
    const float ph = ctx.proc.ui.venvPos[idx].load();
    if (ph > 0.f && ph < 1.f) { const float x = toScreen (ph, 0).x; g.setColour (t.accent2.withAlpha (0.8f)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom()); }
}
void VenvCanvas::mouseDown (const MouseEvent& e)
{
    if (e.mods.isShiftDown() && ! freeDraw) { panning = true; panFrom = e.position; panStart0 = vx0; return; }
    const int h = hit (e.position);
    if (e.mods.isPopupMenu())
    {
        if (h < 0) return;
        PopupMenu m; m.addItem (1, "Delete point", h > 0 && h < shape.n - 1); m.addSeparator();
        m.addItem (2, "Curve: Linear"); m.addItem (3, "Curve: Exponential"); m.addItem (4, "Curve: Logarithmic");
        SafePointer<VenvCanvas> sp (this);
        m.showMenuAsync (PopupMenu::Options(), [sp, h] (int r)
        {
            if (! sp || r == 0) return;
            auto& s = sp->shape;
            if (r == 1) { for (int i = h; i < s.n - 1; ++i) { s.x[i] = s.x[i + 1]; s.y[i] = s.y[i + 1]; s.c[i] = s.c[i + 1]; } --s.n; }
            else s.c[h] = r == 2 ? 0.f : (r == 3 ? 0.6f : -0.6f);
            sp->commit();
        });
        return;
    }
    editing = true;
    if (freeDraw) { stroke.clear(); stroke.add (e.position); return; }
    drag = h;
}
void VenvCanvas::mouseDrag (const MouseEvent& e)
{
    if (panning)
    {
        const float span = vx1 - vx0, dx = (panFrom.x - e.position.x) / (float) getWidth() * span;
        vx0 = jlimit (0.f, 1.f - span, panStart0 + dx); vx1 = vx0 + span; repaint(); return;
    }
    if (freeDraw) { stroke.add (e.position); repaint(); return; }
    if (drag < 0) return;
    auto q = fromScreen (e.position);
    if (e.mods.isAltDown()) { q.x = std::round (q.x * 16.f) / 16.f; q.y = std::round (q.y * 8.f) / 8.f; }
    if (drag == 0) q.x = 0; else if (drag == shape.n - 1) q.x = 1;
    else q.x = jlimit (shape.x[drag - 1], shape.x[drag + 1], q.x);
    shape.x[drag] = q.x; shape.y[drag] = jlimit (0.f, 1.f, q.y);
    repaint();
}
void VenvCanvas::mouseUp (const MouseEvent&)
{
    if (panning) { panning = false; return; }
    if (freeDraw && stroke.size() > 2)
    {
        Array<Point<float>> pts; for (auto& s : stroke) { auto q = fromScreen (s); pts.add ({ jlimit (0.f, 1.f, q.x), jlimit (0.f, 1.f, q.y) }); }
        std::sort (pts.begin(), pts.end(), [] (auto& a, auto& b) { return a.x < b.x; });
        const float lo = pts.getFirst().x, hi = pts.getLast().x;
        VShape ns; ns.n = 0;
        auto add = [&] (float x, float y, float c) { if (ns.n < VShape::kMax && (ns.n == 0 || x >= ns.x[ns.n - 1])) { ns.x[ns.n] = x; ns.y[ns.n] = y; ns.c[ns.n] = c; ++ns.n; } };
        for (int i = 0; i < shape.n; ++i) if (shape.x[i] < lo) add (shape.x[i], shape.y[i], shape.c[i]);
        const int want = jmin (40, pts.size()); const int room = VShape::kMax - ns.n - 4;
        const int cnt = jmin (want, room);
        for (int i = 0; i < cnt; ++i) { auto q = pts[(int) ((float) i / jmax (1, cnt - 1) * (pts.size() - 1))]; add (q.x, q.y, 0.f); }
        for (int i = 0; i < shape.n; ++i) if (shape.x[i] > hi) add (shape.x[i], shape.y[i], shape.c[i]);
        if (ns.n >= 2) shape = ns;
        stroke.clear();
        commit();
    }
    else if (drag >= 0) commit();
    drag = -1; editing = false;
}
void VenvCanvas::mouseDoubleClick (const MouseEvent& e)
{
    if (freeDraw || hit (e.position) >= 0 || shape.n >= VShape::kMax) return;
    auto q = fromScreen (e.position); q.x = jlimit (0.001f, 0.999f, q.x);
    int at = 1; while (at < shape.n && shape.x[at] < q.x) ++at;
    for (int i = shape.n; i > at; --i) { shape.x[i] = shape.x[i - 1]; shape.y[i] = shape.y[i - 1]; shape.c[i] = shape.c[i - 1]; }
    shape.x[at] = q.x; shape.y[at] = jlimit (0.f, 1.f, q.y); shape.c[at] = 0; ++shape.n;
    commit();
}
void VenvCanvas::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const float span = vx1 - vx0, cx = fromScreen (e.position).x;
    const float ns = jlimit (0.05f, 1.f, span * (w.deltaY > 0 ? 0.85f : 1.f / 0.85f));
    const float rel = (cx - vx0) / span;
    vx0 = jlimit (0.f, 1.f - ns, cx - rel * ns); vx1 = vx0 + ns;
    if (ns >= 0.999f) { vx0 = 0; vx1 = 1; }
    repaint();
}
void VenvCanvas::commit()
{
    shape.sanitise();
    ctx.proc.undo.beginNewTransaction();
    ctx.proc.setShape (idx, shape, "User shape");
    loaded = ctx.proc.venvTree (idx).getProperty ("pts").toString();
    editing = false; repaint();
}

//==============================================================================
KeyEditor::KeyEditor (Ctx& c, bool distribution) : ctx (c), dist (distribution) { startTimerHz (10); timerCallback(); }
ValueTree KeyEditor::tree() { return dist ? ctx.proc.distTree() : ctx.proc.keyTree(); }
void KeyEditor::store() { ctx.proc.undo.beginNewTransaction(); if (dist) ctx.proc.setDistCurve (curve); else ctx.proc.setKeyCurve (curve); loaded = tree().getProperty ("pts").toString(); }
void KeyEditor::timerCallback()
{
    if (drag >= 0) return;
    const auto s = tree().getProperty ("pts").toString();
    if (s != loaded) { loaded = s; curve = KeyCurve::fromString (s); repaint(); }
}
Rectangle<float> KeyEditor::area() const { return getLocalBounds().toFloat().reduced (6.f, 4.f).withTrimmedBottom (14.f); }
void KeyEditor::paint (Graphics& g)
{
    auto& t = Theme::c();
    g.setColour (t.bg); g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.f);
    auto a = area();
    g.setColour (t.line); g.drawHorizontalLine ((int) a.getCentreY(), a.getX(), a.getRight());
    // piano strip (keytrack) or a filled density (distribution)
    auto kb = getLocalBounds().toFloat().reduced (6.f, 2.f).removeFromBottom (12.f);
    if (dist)
    {
        Path fill; fill.startNewSubPath (a.getX(), a.getBottom());
        for (int i = 0; i < curve.n; ++i) fill.lineTo (a.getX() + curve.x[i] * a.getWidth(), a.getCentreY() - curve.y[i] * a.getHeight() * 0.5f);
        fill.lineTo (a.getRight(), a.getBottom()); fill.closeSubPath();
        g.setColour (t.accent.withAlpha (0.18f)); g.fillPath (fill);
        g.setColour (t.dim); g.setFont (FontOptions (9.f)); g.drawText ("low values", kb, Justification::centredLeft); g.drawText ("high values", kb, Justification::centredRight);
    }
    else for (int n = 0; n < 128; ++n)
    {
        const int pc = n % 12; const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        g.setColour (black ? t.bg : t.dim.withAlpha (0.6f));
        g.fillRect (kb.getX() + kb.getWidth() * n / 128.f, kb.getY(), kb.getWidth() / 128.f, kb.getHeight());
    }
    Path p;
    for (int i = 0; i < curve.n; ++i)
    {
        const Point<float> s (a.getX() + curve.x[i] * a.getWidth(), a.getCentreY() - curve.y[i] * a.getHeight() * 0.5f);
        if (i == 0) p.startNewSubPath (s); else p.lineTo (s);
    }
    g.setColour (t.accent); g.strokePath (p, PathStrokeType (1.6f));
    for (int i = 0; i < curve.n; ++i)
    {
        const Point<float> s (a.getX() + curve.x[i] * a.getWidth(), a.getCentreY() - curve.y[i] * a.getHeight() * 0.5f);
        g.setColour (i == drag ? t.warn : t.text); g.fillEllipse (s.x - 3, s.y - 3, 6, 6);
    }
}
void KeyEditor::mouseDown (const MouseEvent& e)
{
    auto a = area(); drag = -1;
    for (int i = 0; i < curve.n; ++i)
        if (Point<float> (a.getX() + curve.x[i] * a.getWidth(), a.getCentreY() - curve.y[i] * a.getHeight() * 0.5f).getDistanceFrom (e.position) < 7.f) drag = i;
    if (e.mods.isPopupMenu() && drag > 0 && drag < curve.n - 1)
    {
        for (int i = drag; i < curve.n - 1; ++i) { curve.x[i] = curve.x[i + 1]; curve.y[i] = curve.y[i + 1]; }
        --curve.n; drag = -1; store();
    }
}
void KeyEditor::mouseDrag (const MouseEvent& e)
{
    if (drag < 0) return;
    auto a = area();
    float x = jlimit (0.f, 1.f, (e.position.x - a.getX()) / a.getWidth()), y = jlimit (-1.f, 1.f, (a.getCentreY() - e.position.y) / (a.getHeight() * 0.5f));
    if (drag == 0) x = 0; else if (drag == curve.n - 1) x = 1; else x = jlimit (curve.x[drag - 1], curve.x[drag + 1], x);
    curve.x[drag] = x; curve.y[drag] = y; repaint();
}
void KeyEditor::mouseUp (const MouseEvent&)
{
    if (drag >= 0) store();
    drag = -1;
}
void KeyEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (curve.n >= KeyCurve::kMax) return;
    auto a = area();
    const float x = jlimit (0.001f, 0.999f, (e.position.x - a.getX()) / a.getWidth()), y = jlimit (-1.f, 1.f, (a.getCentreY() - e.position.y) / (a.getHeight() * 0.5f));
    int at = 1; while (at < curve.n && curve.x[at] < x) ++at;
    for (int i = curve.n; i > at; --i) { curve.x[i] = curve.x[i - 1]; curve.y[i] = curve.y[i - 1]; }
    curve.x[at] = x; curve.y[at] = y; ++curve.n;
    store(); repaint();
}

//==============================================================================
AttractorView::AttractorView (Ctx& c, int i) : ctx (c), idx (i) { startTimerHz (30); }
void AttractorView::mouseDrag (const MouseEvent& e) { yaw += (e.position.x - last.x) * 0.01f; pitch = jlimit (-1.5f, 1.5f, pitch + (e.position.y - last.y) * 0.01f); last = e.position; repaint(); }
void AttractorView::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    const auto& tr = ctx.proc.ui.attrTrail[idx];
    const int w = ctx.proc.ui.attrW[idx].load();
    const float cy = std::cos (yaw), sy = std::sin (yaw), cp = std::cos (pitch), sp = std::sin (pitch);
    const float sc = jmin (r.getWidth(), r.getHeight()) * 0.42f;
    Point<float> prev; bool has = false;
    for (int k = 0; k < 256; ++k)
    {
        const int i = (w + k) & 255;
        const float x = tr[i][0], y = tr[i][1], z = tr[i][2];
        const float x1 = x * cy - z * sy, z1 = x * sy + z * cy, y1 = y * cp - z1 * sp;
        const Point<float> s (r.getCentreX() + x1 * sc, r.getCentreY() - y1 * sc);
        if (has) { g.setColour (t.accent2.withAlpha ((float) k / 256.f * 0.9f)); g.drawLine ({ prev, s }, 1.2f); }
        prev = s; has = true;
    }
    if (has) { g.setColour (t.warn); g.fillEllipse (prev.x - 3, prev.y - 3, 6, 6); }
}

//==============================================================================
LfoPreview::LfoPreview (Ctx& c, int i) : ctx (c), idx (i) { startTimerHz (25); }
void LfoPreview::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    const int wave = ctx.choice (lfoId (idx, "wave")); const float skew = ctx.real (lfoId (idx, "skew"));
    const bool rnd = LfoEval::isRandom (wave);
    Path p;
    for (int i = 0; i <= 120; ++i)
    {
        const float x = (float) i / 120.f;
        const float v = LfoEval::value (x * (rnd ? 4.0 : 1.0), wave, skew, 11u);
        const Point<float> s (r.getX() + 3 + x * (r.getWidth() - 6), r.getCentreY() - v * r.getHeight() * 0.4f);
        if (i == 0) p.startNewSubPath (s); else p.lineTo (s);
    }
    g.setColour (ctx.real (lfoId (idx, "on")) > 0.5f ? t.accent : t.dim); g.strokePath (p, PathStrokeType (1.4f));
    const float lv = ctx.proc.ui.lfoVal[idx].load();
    g.setColour (t.warn); g.fillEllipse (r.getRight() - 8.f, r.getCentreY() - lv * r.getHeight() * 0.4f - 3.f, 6.f, 6.f);
}

LinkScope::LinkScope (Ctx& c) : ctx (c) { startTimerHz (30); }
void LinkScope::timerCallback()
{
    for (int i = 0; i < kNumLfo; ++i) hist[i][w] = ctx.proc.ui.lfoVal[i].load();
    w = (w + 1) % 200;
    if (isShowing()) repaint();
}
void LinkScope::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    g.setColour (t.dim); g.setFont (FontOptions (10.f, Font::bold)); g.drawText ("LINK SCOPE", r.reduced (6, 2), Justification::topLeft);
    for (int l = 0; l < kNumLfo; ++l)
    {
        if (ctx.real (lfoId (l, "on")) < 0.5f) continue;
        Path p;
        for (int k = 0; k < 200; ++k)
        {
            const float v = hist[l][(w + k) % 200];
            const Point<float> s (r.getX() + r.getWidth() * k / 199.f, r.getCentreY() - v * r.getHeight() * 0.42f);
            if (k == 0) p.startNewSubPath (s); else p.lineTo (s);
        }
        g.setColour (Theme::engine (l + 1).withAlpha (0.85f)); g.strokePath (p, PathStrokeType (1.f));
    }
}

//==============================================================================
void LevelMeter::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 2.f);
    const float v = get();
    if (isGR)
    {
        const float f = jlimit (0.f, 1.f, v / 24.f);
        g.setColour (t.warn); g.fillRect (r.withHeight (r.getHeight() * f));
        g.setColour (t.text); g.setFont (FontOptions (9.5f)); g.drawText (String (v, 1), r.removeFromBottom (12.f), Justification::centred);
        return;
    }
    auto toF = [] (float x) { return jlimit (0.f, 1.f, (Decibels::gainToDecibels (x, -60.f) + 60.f) / 66.f); };
    const float f = toF (v);
    auto bar = r.withTop (r.getBottom() - r.getHeight() * f);
    g.setColour (v > 1.f ? t.red : t.accent); g.fillRect (bar);
    g.setColour (t.text); g.fillRect (r.getX(), r.getBottom() - r.getHeight() * toF (hold) - 1.f, r.getWidth(), 1.5f);
}

void VuMeter::timerCallback()
{
    l = ctx.proc.ui.outPkL.load(); r = ctx.proc.ui.outPkR.load();
    holdL = jmax (l, holdL * 0.997f); holdR = jmax (r, holdR * 0.997f);
    const float ceil = ctx.real ("m_limon") > 0.5f ? Decibels::decibelsToGain (ctx.real ("m_ceil")) : 1.f;
    if (ctx.proc.masterChain().tpPeak.load() > ceil * 1.001f) tpLatched = true;
    if (isShowing()) repaint();
}
void VuMeter::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto a = getLocalBounds().toFloat().reduced (4.f);
    auto bottom = a.removeFromBottom (28.f);
    auto toF = [] (float x) { return jlimit (0.f, 1.f, (Decibels::gainToDecibels (x, -60.f) + 60.f) / 66.f); };
    const float bw = (a.getWidth() - 30.f) * 0.5f;
    for (int c = 0; c < 2; ++c)
    {
        auto b = Rectangle<float> (a.getX() + c * (bw + 4.f), a.getY() + 12.f, bw, a.getHeight() - 26.f);
        g.setColour (t.bg); g.fillRect (b);
        const float v = c == 0 ? l : r, h = c == 0 ? holdL : holdR;
        g.setColour (v > 1.f ? t.red : t.accent); g.fillRect (b.withTop (b.getBottom() - b.getHeight() * toF (v)));
        g.setColour (t.text); g.fillRect (b.getX(), b.getBottom() - b.getHeight() * toF (h) - 1.f, b.getWidth(), 1.5f);
        g.setFont (FontOptions (9.5f)); g.drawText (String (Decibels::gainToDecibels (h, -60.f), 1), (int) b.getX() - 4, (int) a.getY(), (int) b.getWidth() + 8, 11, Justification::centred);
        g.drawText (c == 0 ? "L" : "R", (int) b.getX(), (int) b.getBottom() + 1, (int) b.getWidth(), 12, Justification::centred);
    }
    auto sc = Rectangle<float> (a.getRight() - 24.f, a.getY() + 12.f, 24.f, a.getHeight() - 26.f);
    g.setColour (t.dim); g.setFont (FontOptions (8.5f));
    for (float d : { 0.f, -6.f, -12.f, -24.f, -48.f }) { const float y = sc.getBottom() - sc.getHeight() * (d + 60.f) / 66.f; g.drawText (String ((int) d), sc.withY (y - 5).withHeight (10), Justification::right); }
    // true peak + correlation
    auto tp = bottom.removeFromTop (12.f);
    g.setColour (tpLatched ? t.red : t.panelHi); g.fillRoundedRectangle (tp.removeFromLeft (12.f).reduced (1.f), 3.f);
    g.setColour (t.dim); g.setFont (FontOptions (9.5f, Font::bold)); g.drawText ("TRUE PEAK", tp.withTrimmedLeft (4), Justification::centredLeft);
    auto cr = bottom.reduced (0, 2);
    g.setColour (t.bg); g.fillRect (cr);
    const float corr = jlimit (-1.f, 1.f, ctx.proc.masterChain().corr.load());
    const float cx = cr.getCentreX(), x = cr.getX() + (corr + 1.f) * 0.5f * cr.getWidth();
    g.setColour (corr < 0 ? t.red : t.green); g.fillRect (Rectangle<float>::leftTopRightBottom (jmin (cx, x), cr.getY(), jmax (cx, x), cr.getBottom()));
    g.setColour (t.dim); g.drawVerticalLine ((int) cx, cr.getY(), cr.getBottom());
}

//==============================================================================
Waterfall::Waterfall (Ctx& c) : ctx (c) { startTimerHz (25); }
void Waterfall::timerCallback()
{
    if (! isShowing() || ! ctx.prefs.visuals) return;
    computeSpectrum (ctx.proc.ui.masterPost, hist[head], NB, ctx.proc.getSr());
    head = (head + 1) % NH; repaint();
}
void Waterfall::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    if (! ctx.prefs.visuals) { g.setColour (t.dim); g.drawText ("Visualisers off (Settings > Quality)", r, Justification::centred); return; }
    static const float cams[3][2] = { { 3.0f, -2.6f }, { -3.0f, -2.6f }, { 0.f, -3.4f } };
    const auto cam = cams[ctx.prefs.camera % 3];
    const float baseW = r.getWidth() - std::abs (cam[0]) * NH - 20.f;
    const float h = r.getHeight() * 0.55f * zoom;
    for (int k = NH - 1; k >= 0; --k)
    {
        const int row = (head - 1 - k + NH * 2) % NH;
        const float ox = r.getX() + 10.f + (cam[0] < 0 ? -cam[0] * NH : 0.f) + cam[0] * k, oy = r.getBottom() - 8.f + cam[1] * k;
        Path p; p.startNewSubPath (ox, oy);
        for (int b = 0; b < NB; ++b) p.lineTo (ox + baseW * b / (NB - 1.f), oy - hist[row][b] * h);
        p.lineTo (ox + baseW, oy); p.closeSubPath();
        float avg = 0; for (int b = 0; b < NB; ++b) avg += hist[row][b]; avg /= NB;
        g.setColour (t.bg.withAlpha (0.9f)); g.fillPath (p);
        g.setColour (palette (ctx.prefs.palette, avg * 2.f).withAlpha (1.f - (float) k / NH * 0.85f)); g.strokePath (p, PathStrokeType (1.f));
    }
}
void Waterfall::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) ctx.prefs.palette = 1 - ctx.prefs.palette; else ctx.prefs.camera = (ctx.prefs.camera + 1) % 3;
    ctx.prefs.save(); repaint();
}
void Waterfall::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) { zoom = jlimit (0.3f, 3.f, zoom * (w.deltaY > 0 ? 1.1f : 0.9f)); repaint(); }

SquareVis::SquareVis (Ctx& c) : ctx (c) { startTimerHz (25); }
void SquareVis::timerCallback()
{
    if (! isShowing() || ! ctx.prefs.visuals) return;
    computeSpectrum (ctx.proc.ui.masterPost, hist[head], NB, ctx.proc.getSr());
    head = (head + 1) % NH; spin += 0.01f; repaint();
}
void SquareVis::paint (Graphics& g)
{
    auto& t = Theme::c();
    auto r = getLocalBounds().toFloat();
    g.setColour (t.bg); g.fillRoundedRectangle (r, 4.f);
    if (! ctx.prefs.visuals) return;
    const int mode = ctx.prefs.squareMode % 3;
    static const char* names[3] = { "PHASE CLOUD", "TERRAIN", "GALAXY" };
    g.setColour (t.dim); g.setFont (FontOptions (9.5f, Font::bold)); g.drawText (names[mode], r.reduced (6, 3), Justification::topLeft);
    const float sc = jmin (r.getWidth(), r.getHeight()) * 0.45f * zoom;
    if (mode == 0)
    {
        static float L[1024], R[1024];
        ctx.proc.ui.outL.latest (L, 1024); ctx.proc.ui.outR.latest (R, 1024);
        for (int i = 0; i < 1024; i += 2)
        {
            const float x = r.getCentreX() + (L[i] - R[i]) * 0.7071f * sc, y = r.getCentreY() - (L[i] + R[i]) * 0.7071f * sc;
            g.setColour (palette (ctx.prefs.palette, (float) i / 1024.f).withAlpha (0.5f)); g.fillRect (x, y, 1.5f, 1.5f);
        }
    }
    else if (mode == 1)
    {
        for (int k = NH - 1; k >= 0; --k)
        {
            const int row = (head - 1 - k + NH * 2) % NH;
            const float oy = r.getY() + 16.f + (r.getHeight() - 30.f) * (1.f - (float) k / NH);
            Path p; p.startNewSubPath (r.getX() + 8, oy);
            for (int b = 0; b < NB; ++b) p.lineTo (r.getX() + 8 + (r.getWidth() - 16) * b / (NB - 1.f), oy - hist[row][b] * 22.f * zoom);
            g.setColour (t.bg); p.lineTo (r.getRight() - 8, oy + 2); p.lineTo (r.getX() + 8, oy + 2); p.closeSubPath(); g.fillPath (p);
            g.setColour (palette (ctx.prefs.palette, 1.f - (float) k / NH)); g.strokePath (p, PathStrokeType (1.f));
        }
    }
    else
    {
        const int row = (head - 1 + NH) % NH;
        for (int b = 0; b < NB; ++b)
            for (int arm = 0; arm < 3; ++arm)
            {
                const float a = (float) b / NB * 4.f * MathConstants<float>::pi + spin + arm * MathConstants<float>::twoPi / 3.f;
                const float rad = (0.12f + 0.88f * (float) b / NB) * sc, m = hist[row][b];
                const float s = 1.5f + m * 6.f;
                g.setColour (palette (ctx.prefs.palette, m).withAlpha (0.2f + 0.8f * m));
                g.fillEllipse (r.getCentreX() + rad * std::cos (a) - s * 0.5f, r.getCentreY() + rad * std::sin (a) - s * 0.5f, s, s);
            }
    }
}
void SquareVis::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) ctx.prefs.palette = 1 - ctx.prefs.palette; else ctx.prefs.squareMode = (ctx.prefs.squareMode + 1) % 3;
    ctx.prefs.save(); repaint();
}
void SquareVis::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) { zoom = jlimit (0.3f, 3.f, zoom * (w.deltaY > 0 ? 1.1f : 0.9f)); repaint(); }
} // namespace cz
