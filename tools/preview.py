#!/usr/bin/env python3
# Picture of the panel with Rack's own knob/jack graphics and the screen drawn
# in, so the layout can be checked without Rack.
#   python3 tools/preview.py out.png [path/to/Rack/res/ComponentLibrary]
import os, sys, math, re, base64, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "preview.png")
LIB = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, "..", "..", "Rack-src", "res", "ComponentLibrary")
svg = open(os.path.join(HERE, "..", "res", "TwinPeaks.svg")).read()
L = {}
src = open(os.path.join(HERE, "..", "res", "make_panel.py")).read()
exec(src.split("# ---- LAYOUT")[1].split("CONTROL =")[0].split("\n", 1)[1], L)
W = 91.44
PX = 25.4 / 75.0
g = []

def comp(name, x, y, angle=0.0):
    p = os.path.join(LIB, name + ".svg")
    s = open(p).read()
    m = re.search(r'width="([0-9.]+)(px|mm)?"', s); n = re.search(r'height="([0-9.]+)(px|mm)?"', s)
    w = float(m.group(1)) * (1 if m.group(2) == "mm" else PX); h = float(n.group(1)) * (1 if n.group(2) == "mm" else PX)
    data = base64.b64encode(s.encode()).decode()
    g.append(f'<g transform="rotate({angle} {x} {y})"><image href="data:image/svg+xml;base64,{data}" x="{x - w/2}" y="{y - h/2}" width="{w}" height="{h}"/></g>')

def knob(base, x, y, v):
    if os.path.exists(os.path.join(LIB, base + "_bg.svg")):
        comp(base + "_bg", x, y)
    comp(base, x, y, -135 + 270 * v)

knob("RoundHugeBlackKnob", L["BIG_X"][0], L["BIG_Y"], 0.45)
knob("RoundBigBlackKnob", L["BIG_X"][1], L["BIG_Y"], 0.4)
for x, v in zip(L["MID_X"], [0.7, 0.3, 0.5]): knob("RoundBlackKnob", x, L["MID_Y"], v)
for x in L["TRIM_X"]: knob("Trimpot", x, L["TRIM_Y"], 0.5)
for y in (L["J1_Y"],): 
    for x in L["JX"]: comp("PJ301M", x, y)
for x in L["JX"][:3]: comp("PJ301M", x, L["J2_Y"])
for x in L["JX"]: comp("PJ301M", x, L["J3_Y"])
comp("CKSS_0", L["JX"][3], L["J2_Y"])
for x in (5.08, W - 2 * 5.08):
    for y in (0.0, 128.5 - 5.08):
        comp("ScrewSilver", x + 2.54, y + 2.54)

# screen: same maths as Engine::twinResponse
def resp(f, lo, hi, res, bal):
    k = max(0.06, 2 * (1 - res) ** 1.5 - 0.06 * res ** 4)
    gL, gH = min(1, 1 - bal), min(1, 1 + bal)
    def lp(fc):
        w = f / fc
        return 1 / complex(1 - w * w, k * w)
    return abs(gH * lp(hi) - gL * lp(lo))
fq = 20 * 1024 ** 0.45; lo, hi = fq, fq * 2 ** 2.0
dx, dy, dw, dh = L["DISP_X"], L["DISP_Y"], L["DISP_W"], L["DISP_H"]
pts = []
for i in range(121):
    f = 20 * 1000 ** (i / 120)
    db = max(-36, min(24, 20 * math.log10(max(resp(f, lo, hi, 0.7, 0), 1e-5))))
    pts.append((dx + dw * i / 120, dy + dh * (24 - db) / 60))
d = "M" + " L".join(f"{x:.2f} {y:.2f}" for x, y in pts)
g.append(f'<path d="{d} L{dx+dw} {dy+dh} L{dx} {dy+dh} Z" fill="#66D6C4" fill-opacity="0.2"/>')
g.append(f'<path d="{d}" stroke="#66D6C4" stroke-width="0.4" fill="none"/>')
for f, c in ((lo, "#F2A541"), (hi, "#E8603C")):
    x = dx + dw * math.log2(f / 20) / math.log2(1000)
    g.append(f'<line x1="{x}" y1="{dy+dh}" x2="{x}" y2="{dy+dh-1.4}" stroke="{c}" stroke-width="0.5"/>')

svg = svg.replace("</svg>", "\n".join(g) + "\n</svg>")
tmp = OUT + ".svg"; open(tmp, "w").write(svg)
pw, ph = 457, 643
open(tmp + ".html", "w").write(f'<html><body style="margin:0;background:#111"><img src="{os.path.basename(tmp)}" style="width:{pw}px;height:{ph}px"></body></html>')
js = f"""const {{chromium}} = require('playwright');
(async () => {{ const b = await chromium.launch(); const p = await b.newPage({{viewport: {{width: {pw}, height: {ph}}}, deviceScaleFactor: 2}});
await p.goto('file://{tmp}.html'); await p.waitForTimeout(400);
await p.screenshot({{path: '{OUT}'}}); await b.close(); }})();"""
open(OUT + ".js", "w").write(js)
subprocess.run(["node", OUT + ".js"], check=True, cwd="/tmp")
os.remove(OUT + ".js"); os.remove(tmp); os.remove(tmp + ".html")
print("wrote", OUT)
