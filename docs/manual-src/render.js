// node render.js in.html out.pdf "footer text"
const { chromium } = require('playwright');
const path = require('path');
(async () => {
  const [inp, out, foot] = process.argv.slice(2);
  const b = await chromium.launch();
  const p = await b.newPage();
  await p.goto('file://' + path.resolve(inp), { waitUntil: 'load' });
  await p.evaluate(() => document.fonts.ready);
  const footer = foot ? `<div style="font-family:Inter,sans-serif;font-size:7.5pt;color:#8a93a3;width:100%;padding:0 18mm;display:flex;justify-content:space-between;">
    <span>${foot}</span><span><span class="pageNumber"></span></span></div>` : "<div></div>";
  await p.pdf({ path: out, format: 'A4', printBackground: true, preferCSSPageSize: true, displayHeaderFooter: !!foot,
    headerTemplate: '<div></div>', footerTemplate: footer, outline: true, tagged: true });
  await b.close();
})();
