#!/bin/bash
# usage: ./make.sh es|en
set -e; L=$1; cd "$(dirname "$0")"
rm -f pages_$L.json
for pass in 1 2; do
  python3 build.py $L >/dev/null
  node render.js body_$L.html body_$L.pdf "$(cat foot_$L.txt)"
  python3 pages.py $L
done
node render.js cover_$L.html cover_$L.pdf ""
python3 - "$L" <<'PY'
import sys, pypdf
L = sys.argv[1]
w = pypdf.PdfWriter()
w.append(f'cover_{L}.pdf'); w.append(f'body_{L}.pdf')
w.add_metadata({'/Title': 'Caosmo 0.4 — ' + ('Manual de usuario' if L == 'es' else 'User Manual'), '/Author': 'Eloy Platas'})
w.write(f'Caosmo-0.4-Manual-{L.upper()}.pdf')
PY
ls -la Caosmo-0.4-Manual-${L^^}.pdf
