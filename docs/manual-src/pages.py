import sys, json, re, pypdf
lang = sys.argv[1]
r = pypdf.PdfReader(f'body_{lang}.pdf')
pages = {}
for i, pg in enumerate(r.pages):
    for m in re.findall(r'@@([^@\s]+)@@', pg.extract_text() or ''):
        if i > 0: pages.setdefault(m, i + 1)   # skip the contents page itself
json.dump(pages, open(f'pages_{lang}.json', 'w'))
print(len(pages), 'headings,', len(r.pages), 'pages')
