#!/usr/bin/env python3
"""Build docs/manual.html (embedded in the plugin) from the same Markdown as the PDFs: Spanish + English, no images."""
import re, pathlib, collections, html
import markdown

HERE = pathlib.Path(__file__).parent
xml = (HERE.parents[1] / 'presets' / 'factory_presets.xml').read_text()
cats = collections.OrderedDict()
for n, c in re.findall(r'<CaosmoPreset name="([^"]+)" category="([^"]+)"', xml): cats.setdefault(c, []).append(html.unescape(n))

def lang(L, cat):
    src = '\n\n'.join((HERE / f'{L}{i}.md').read_text() for i in ('1', '2', '2b', '3', '4'))
    src = re.sub(r'^!\[[^\]]*\]\([^)]*\)\s*$', '', src, flags=re.M)                       # no images in the embedded copy
    src = src.replace('<div class="figure">SIGNALFLOW</div>', '')
    src = src.replace('PRESETLIST', '\n'.join([f'| {cat} | Presets |', '| --- | --- |'] + [f'| **{c}** | {", ".join(v)} |' for c, v in cats.items()]))
    src = re.sub(r'\{#([a-z0-9-]+)\}', lambda m: '{#' + L + '-' + m.group(1) + '}', src)    # unique ids per language
    body = markdown.markdown(src, extensions=['tables', 'fenced_code', 'attr_list'])
    toc = ' '.join(f'<a href="#{i}">{re.sub("<[^>]+>", "", t)}</a>' for i, t in re.findall(r'<h1 id="([^"]+)">(.*?)</h1>', body))
    return body, toc

es, tes = lang('es', 'Categoría'); en, ten = lang('en', 'Category')
css = '''
:root{--bg:#111318;--panel:#1a1d24;--line:#2e3340;--text:#d8dce4;--dim:#8790a2;--acc:#3fd0c9;--acc2:#9b8cff}
body{background:var(--bg);color:var(--text);font:15px/1.6 system-ui,sans-serif;margin:0}
main{max-width:900px;margin:0 auto;padding:32px 20px 80px}
h1{color:var(--acc);letter-spacing:.04em;margin-top:2.4em;border-bottom:1px solid var(--line);padding-bottom:6px}
h2{color:var(--acc2);margin-top:1.8em}h3{color:var(--acc);margin-bottom:.2em}
a{color:var(--acc)}code{background:var(--panel);padding:1px 5px;border-radius:4px}
nav.toc{position:sticky;top:0;background:rgba(17,19,24,.96);border-bottom:1px solid var(--line);padding:8px 16px;font-size:13px;z-index:2;max-height:30vh;overflow:auto}
nav.toc a{display:inline-block;margin:2px 10px 2px 0;color:var(--dim);text-decoration:none}nav.toc a:hover{color:var(--acc)}
nav.toc b{color:var(--acc2);margin-right:8px}
table{border-collapse:collapse;width:100%;margin:12px 0;font-size:14px}td,th{border-bottom:1px solid rgba(128,128,128,.3);padding:6px 8px;text-align:left;vertical-align:top}
th{color:var(--dim);font-weight:600}pre{overflow-x:auto;background:var(--panel);padding:10px;border-radius:6px}
.title{font-size:34px;color:var(--acc);margin:0}.sub{color:var(--dim)}
'''
page = f'''<!doctype html><html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Caosmo 0.5 — Manual</title><style>{css}</style></head><body>
<nav class="toc"><b>ES</b>{tes}<br><b>EN</b>{ten}</nav><main>
<p class="title">Caosmo 0.5</p><p class="sub">Manual de usuario · User Manual — Eloy Platas · GNU GPL v3 · <a href="#en-intro">English below</a></p>
{es}
<hr style="margin:60px 0;border-color:#2e3340">
<p class="title" id="english">Caosmo 0.5 — User Manual</p>
{en}
</main></body></html>'''
(HERE.parent / 'manual.html').write_text(page)
print(len(page), 'bytes')
