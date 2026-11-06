#!/usr/bin/env python3
"""Build the Caosmo manual PDF for one language: python3 build.py es|en"""
import re, sys, json, subprocess, pathlib, html, collections
import markdown

LANG = sys.argv[1]
HERE = pathlib.Path(__file__).parent
T = {
    'es': dict(title='Manual de usuario', sub='Sintetizador polifónico de caos', ver='Versión 0.4 · noviembre de 2026',
               toc='Índice', foot='Caosmo 0.4 · Manual de usuario', lic='Software libre bajo la GNU GPL v3',
               flow=['Capa 1', 'Capa 2', 'Capa 3', 'motor · subs · filtro · env', 'Bus de capa', 'EQ · drive · width · pan',
                     'Buses FX A–C', 'Suma', 'DELUGE FX', 'Cadena master', 'Salida', 'envíos', 'retorno', 'por voz', 'por capa'],
               cat='Categoría'),
    'en': dict(title='User Manual', sub='Polyphonic chaos synthesizer', ver='Version 0.4 · November 2026',
               toc='Contents', foot='Caosmo 0.4 · User Manual', lic='Free software under the GNU GPL v3',
               flow=['Lane 1', 'Lane 2', 'Lane 3', 'engine · subs · filter · env', 'Lane bus', 'EQ · drive · width · pan',
                     'FX buses A–C', 'Sum', 'DELUGE FX', 'Master chain', 'Output', 'sends', 'return', 'per voice', 'per lane'],
               cat='Category'),
}[LANG]

src = '\n\n'.join((HERE / f'{LANG}{i}.md').read_text() for i in range(1, 5))

# ---- signal-flow figure (inline SVG, print colours) ----
f = T['flow']
def box(x, y, w, h, t1, t2='', hi=False):
    s = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" fill="{"#efeaff" if hi else "#fff"}" stroke="{"#6b4fd8" if hi else "#8a8f9c"}" stroke-width="{1.6 if hi else 1}"/>'
    if t2:
        s += f'<text x="{x+w/2}" y="{y+h/2-3}" text-anchor="middle" font-weight="600" font-size="12">{t1}</text>'
        s += f'<text x="{x+w/2}" y="{y+h/2+12}" text-anchor="middle" font-size="9.5" fill="#5b6070">{t2}</text>'
    else:
        s += f'<text x="{x+w/2}" y="{y+h/2+4}" text-anchor="middle" font-weight="600" font-size="12">{t1}</text>'
    return s
arrow = '<defs><marker id="a" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto"><path d="M0 0L10 5L0 10z" fill="#8a8f9c"/></marker></defs>'
g = [arrow]
for i, y in enumerate((20, 75, 130)):
    g.append(box(10, y, 150, 42, f[i], f[3]))
    g.append(box(195, y, 150, 42, f[4], f[5]))
    g.append(f'<path d="M160 {y+21}H193" stroke="#8a8f9c" fill="none" marker-end="url(#a)"/>')
    g.append(f'<path d="M345 {y+21}H362V96H378" stroke="#8a8f9c" fill="none"/>')
g.append('<path d="M362 96H378" stroke="#8a8f9c" marker-end="url(#a)"/>')
g.append(box(380, 75, 70, 42, f[7]))
g.append(box(380, 185, 110, 36, f[6]))
g.append('<path d="M270 172V203H378" stroke="#8a8f9c" fill="none" marker-end="url(#a)"/>')
g.append(f'<text x="280" y="198" font-size="9.5" fill="#5b6070">{f[11]}</text>')
g.append('<path d="M415 185V119" stroke="#8a8f9c" fill="none" marker-end="url(#a)"/>')
g.append(f'<text x="420" y="160" font-size="9.5" fill="#5b6070">{f[12]}</text>')
g.append('<path d="M450 96H478" stroke="#8a8f9c" marker-end="url(#a)"/>')
g.append(box(480, 75, 95, 42, f[8], '', True))
g.append('<path d="M575 96H598" stroke="#8a8f9c" marker-end="url(#a)"/>')
g.append(box(600, 75, 100, 42, f[9]))
g.append('<path d="M700 96H728" stroke="#8a8f9c" marker-end="url(#a)"/>')
g.append(f'<text x="712" y="88" font-size="9.5" fill="#5b6070" text-anchor="middle">{f[10]}</text>')
g.append(f'<text x="85" y="12" font-size="9.5" fill="#5b6070" text-anchor="middle">{f[13]}</text>')
g.append(f'<text x="270" y="12" font-size="9.5" fill="#5b6070" text-anchor="middle">{f[14]}</text>')
svg = f'<svg viewBox="0 0 740 230" xmlns="http://www.w3.org/2000/svg" font-family="Inter" style="width:100%">{"".join(g)}</svg>'
src = src.replace('<div class="figure">SIGNALFLOW</div>', f'<div class="figure">{svg}</div>')

# ---- preset catalogue ----
xml = pathlib.Path('' + str(HERE.parents[1] / "presets" / "factory_presets.xml") + '').read_text()
d = collections.OrderedDict()
for n, c in re.findall(r'<CaosmoPreset name="([^"]+)" category="([^"]+)"', xml):
    d.setdefault(c, []).append(n)
rows = [f'| {T["cat"]} | Presets |', '| --- | --- |'] + [f'| **{c}** | {", ".join(v)} |' for c, v in d.items()]
src = src.replace('PRESETLIST', '\n'.join(rows))

body = markdown.markdown(src, extensions=['tables', 'fenced_code', 'attr_list'])
# wrap tables so they can carry a class; make the first column of parameter tables nowrap-ish
body = body.replace('<table>', '<table class="t">')
body = re.sub(r'<h([12]) id="([^"]+)">(.*?)</h\1>', lambda m: f'<h{m.group(1)} id="{m.group(2)}">{m.group(3)}<span class="pm">@@{m.group(2)}@@</span></h{m.group(1)}>', body)

# ---- table of contents ----
pages = {}
pfile = HERE / f'pages_{LANG}.json'
if pfile.exists(): pages = json.loads(pfile.read_text())
toc = []
h1n = 0
for m in re.finditer(r'<h([12]) id="([^"]+)">(.*?)</h\1>', body):
    lvl, hid, txt = m.groups()
    plain = re.sub('@@.*?@@', '', re.sub('<[^>]+>', '', txt))
    if lvl == '1':
        h1n += 1; num = f'{h1n}'
    else:
        num = ''
    pg = pages.get(hid, '')
    toc.append(f'<div class="toc{lvl}"><a href="#{hid}"><span class="tn">{num}</span><span class="tt">{plain}</span><span class="dots"></span><span class="tp">{pg}</span></a></div>')

css = (HERE / 'print.css').read_text()
cover = f'''<section class="cover">
<div class="mark"><svg viewBox="0 0 200 200" width="150" height="150">{''.join(f'<ellipse cx="100" cy="100" rx="{88-i*9}" ry="{30+i*4}" transform="rotate({i*23} 100 100)" fill="none" stroke="#6b4fd8" stroke-opacity="{0.25+i*0.08}" stroke-width="1.2"/>' for i in range(8))}<circle cx="100" cy="100" r="5" fill="#6b4fd8"/></svg></div>
<h1 class="ct">CAOSMO</h1><div class="cs">{T["sub"]}</div><div class="cm">{T["title"]}</div>
<div class="cv">{T["ver"]}<br>Eloy Platas<br>{T["lic"]}</div></section>'''
head = f'<!doctype html><html lang="{LANG}"><head><meta charset="utf-8"><title>Caosmo 0.4 — {T["title"]}</title><style>{css}</style></head>'
(HERE / f'cover_{LANG}.html').write_text(f'{head}<body>{cover}</body></html>')
(HERE / f'body_{LANG}.html').write_text(f'{head}<body><section class="toc"><h1 class="tocH">{T["toc"]}</h1>{"".join(toc)}</section><main>{body}</main></body></html>')
(HERE / f'foot_{LANG}.txt').write_text(T['foot'])
