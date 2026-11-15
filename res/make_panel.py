#!/usr/bin/env python3
# Draws res/TwinPeaks.svg. VCV Rack can't draw SVG <text>, so every label is
# turned into outline paths with fontTools. Positions are in mm and must match
# src/TwinPeaks.cpp (the numbers in the LAYOUT block are the same in both).
#   pip install fonttools
#   python3 res/make_panel.py
import os
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_BOLD = os.environ.get("PANEL_FONT_BOLD", "/usr/share/fonts/opentype/inter/Inter-Bold.otf")
FONT_MED = os.environ.get("PANEL_FONT_MED", "/usr/share/fonts/opentype/inter/Inter-Medium.otf")

W, H = 91.44, 128.5  # 18 HP

BG = "#1E2226"
BG_DARK = "#14171A"
LINE = "#39424A"
INK = "#EEE8DC"
SOFT = "#9FA7AD"
TEAL = "#66D6C4"     # the response curve colour
LOWC = "#F2A541"     # low peak (amber)
HIGHC = "#E8603C"    # high peak (red-orange)

# ---- LAYOUT (same numbers in src/TwinPeaks.cpp) ----
BIG_X = [24.0, 67.44]
BIG_Y = 31.0
MID_X = [15.5, 45.72, 75.94]
MID_Y = 55.0
TRIM_X = [11.0, 80.44]
TRIM_Y = 73.0
DISP_X, DISP_Y, DISP_W, DISP_H = 21.0, 64.0, 49.44, 17.0
JX = [12.0, 34.5, 57.0, 79.5]
J1_Y, J2_Y, J3_Y = 90.0, 104.0, 117.5

CONTROL = 3.2   # control label size (project standard)
JACK = 2.9      # jack label size (project standard)

fonts = {}


def text(s, x, y, size, font=FONT_BOLD, fill=INK, anchor="middle", spacing=0.04):
    """SVG path for string s; (x, y) is the baseline point in mm."""
    if font not in fonts:
        f = TTFont(font)
        fonts[font] = (f, f.getGlyphSet(), f.getBestCmap(), f["head"].unitsPerEm)
    f, gs, cmap, upm = fonts[font]
    scale = size / upm
    track = spacing * upm
    names = [cmap[ord(c)] for c in s]
    width = sum(gs[n].width + track for n in names) - track
    x0 = {"middle": x - width * scale / 2, "end": x - width * scale}.get(anchor, x)
    pen = SVGPathPen(gs)
    cx = 0.0
    for n in names:
        gs[n].draw(TransformPen(pen, (scale, 0, 0, -scale, x0 + cx * scale, y)))
        cx += gs[n].width + track
    return f'<path fill="{fill}" d="{pen.getCommands()}"/>'


parts = []
add = parts.append
add(f'<rect width="{W}" height="{H}" fill="{BG}"/>')

# title with a little two-peak curve after it
add(text("TwinPeaks", 5.0, 11.4, 5.2, FONT_BOLD, INK, anchor="start", spacing=0.03))
sx, sy = 36.5, 10.8
add(f'<path d="M{sx} {sy} C{sx + 2.0} {sy} {sx + 2.2} {sy - 4.6} {sx + 3.0} {sy - 4.6} '
    f'C{sx + 3.8} {sy - 4.6} {sx + 3.6} {sy - 2.0} {sx + 5.5} {sy - 2.0} '
    f'C{sx + 7.4} {sy - 2.0} {sx + 7.2} {sy - 4.6} {sx + 8.0} {sy - 4.6} '
    f'C{sx + 8.8} {sy - 4.6} {sx + 9.0} {sy} {sx + 11.0} {sy}" '
    f'stroke="{TEAL}" stroke-width="0.6" fill="none" stroke-linecap="round"/>')
add(text("after Rob Hordijk", W - 5.0, 11.0, 2.5, FONT_MED, SOFT, anchor="end"))

# big knobs
add(text("FREQ", BIG_X[0], BIG_Y - 11.6, CONTROL))
add(text("SPREAD", BIG_X[1], BIG_Y - 11.6, CONTROL))
add(text("20", BIG_X[0] - 9.6, BIG_Y + 10.6, 2.3, FONT_MED, SOFT))
add(text("20k", BIG_X[0] + 9.6, BIG_Y + 10.6, 2.3, FONT_MED, SOFT))
add(text("close", BIG_X[1] - 8.6, BIG_Y + 10.6, 2.3, FONT_MED, SOFT))
add(text("5 oct", BIG_X[1] + 8.6, BIG_Y + 10.6, 2.3, FONT_MED, SOFT))

# middle knobs
for x, n in zip(MID_X, ["RES", "DRIVE", "BALANCE"]):
    add(text(n, x, MID_Y - 7.6, CONTROL))
add(text("LOW", MID_X[2] - 6.4, MID_Y + 7.4, 2.3, FONT_MED, LOWC))
add(text("HIGH", MID_X[2] + 6.4, MID_Y + 7.4, 2.3, FONT_MED, HIGHC))

# trimpots
add(text("FM", TRIM_X[0], TRIM_Y - 5.0, CONTROL))
add(text("SPRD", TRIM_X[1], TRIM_Y - 5.0, CONTROL))
add(text("AMT", TRIM_X[0], TRIM_Y + 7.0, 2.3, FONT_MED, SOFT))
add(text("AMT", TRIM_X[1], TRIM_Y + 7.0, 2.3, FONT_MED, SOFT))

# screen
add(f'<rect x="{DISP_X - 0.6}" y="{DISP_Y - 0.6}" width="{DISP_W + 1.2}" height="{DISP_H + 1.2}" rx="1.0" fill="{BG_DARK}" stroke="{LINE}" stroke-width="0.3"/>')

# CV inputs
add(f'<rect x="2.6" y="83.2" width="{W - 5.2}" height="27.4" rx="1.4" fill="{BG_DARK}"/>')
for x, n in zip(JX, ["V/OCT", "FM", "SPREAD", "RES"]):
    add(text(n, x, J1_Y - 5.6, JACK, FONT_BOLD, SOFT))
for x, n in zip(JX[:3], ["DRIVE", "BAL", "FM HI"]):
    add(text(n, x, J2_Y - 5.6, JACK, FONT_BOLD, HIGHC if n == "FM HI" else SOFT))
# peaks switch (CKSS: lever down = 0 = ABOVE, up = 1 = AROUND)
add(text("AROUND", JX[3] - 3.4, J2_Y - 1.2, 2.2, FONT_MED, SOFT, anchor="end"))
add(text("ABOVE", JX[3] - 3.4, J2_Y + 2.8, 2.2, FONT_MED, SOFT, anchor="end"))
add(text("PEAKS", JX[3], J2_Y - 5.6, JACK, FONT_BOLD, SOFT))

# bottom row: audio in, outputs on a teal strip
add(text("IN", JX[0], J3_Y - 5.6, JACK, fill=INK))
ox = JX[1] - 8.6
add(f'<rect x="{ox}" y="111.4" width="{W - 2.6 - ox}" height="12.0" rx="1.4" fill="{TEAL}" fill-opacity="0.14"/>')
add(text("TWIN", JX[1], J3_Y - 5.6, JACK, fill=TEAL))
add(text("LOW", JX[2], J3_Y - 5.6, JACK, fill=LOWC))
add(text("HIGH", JX[3], J3_Y - 5.6, JACK, fill=HIGHC))

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}mm" height="{H}mm" viewBox="0 0 {W} {H}">\n'
       + "\n".join(p for p in parts if p) + "\n</svg>\n")
open(os.path.join(HERE, "TwinPeaks.svg"), "w").write(svg)
print("ok")

# --- Haer maker logo (res/haer_logo.py) on the bottom rail
import sys as _sys
_sys.path.insert(0, HERE)
import haer_logo
haer_logo.add_to_file(os.path.join(HERE, "TwinPeaks.svg"), x=W / 2, y=126.0, cap=3.0, bg=BG)
