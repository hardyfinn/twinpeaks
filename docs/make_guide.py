#!/usr/bin/env python3
# Builds the TwinPeaks guide PDF:  python3 docs/make_guide.py out.pdf
# (needs Playwright + Chromium; uses panel-preview.png from the repo root)
import os, sys, base64, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "TwinPeaks-Guide.pdf")
img = base64.b64encode(open(os.path.join(ROOT, "panel-preview.png"), "rb").read()).decode()

html = f"""<!doctype html><html><head><meta charset="utf-8"><style>
@page {{ size: A4; margin: 16mm; }}
body {{ font-family: Inter, 'Helvetica Neue', Arial, sans-serif; color: #1d1d1f; font-size: 10.5pt; line-height: 1.45; }}
h1 {{ font-size: 26pt; margin: 0 0 2mm; letter-spacing: 0.02em; }}
h1 span {{ color: #2a9d8f; }}
h2 {{ font-size: 13.5pt; margin: 7mm 0 2mm; border-bottom: 1.5px solid #66d6c4; padding-bottom: 1mm; }}
h3 {{ font-size: 11pt; margin: 4mm 0 1mm; }}
.lede {{ font-size: 11.5pt; color: #444; }}
.cols {{ display: flex; gap: 8mm; align-items: flex-start; }}
.cols img {{ width: 78mm; border-radius: 2mm; }}
table {{ border-collapse: collapse; width: 100%; font-size: 9.6pt; }}
td, th {{ border-bottom: 1px solid #e3e0d8; padding: 1.3mm 2mm; text-align: left; vertical-align: top; }}
th {{ background: #eef7f5; }}
td:first-child {{ font-weight: 700; white-space: nowrap; }}
.recipe {{ background: #f3f8f7; border-left: 3px solid #66d6c4; padding: 2.5mm 4mm; margin: 3mm 0; page-break-inside: avoid; }}
.recipe b.t {{ display: block; font-size: 11pt; margin-bottom: 1mm; }}
.flow {{ font-weight: 700; background: #1e2226; color: #eee8dc; padding: 2.5mm 4mm; border-radius: 1.5mm; }}
.small {{ font-size: 9pt; color: #555; }}
.break {{ page-break-before: always; }}
</style></head><body>

<h1>Twin<span>Peaks</span></h1>
<p class="lede">A two-peak resonant filter for VCV Rack, after Rob Hordijk's Twin Peak filter, the filter idea behind the
Benjolin and the Blippoo Box.</p>

<div class="cols">
<img src="data:image/png;base64,{img}">
<div>
<h3>How it works</h3>
<p class="flow">IN &rarr; DRIVE &rarr; two resonant low-pass filters &rarr; HIGH minus LOW &rarr; TWIN</p>
<p>The same sound goes into two resonant low-pass filters, one tuned higher than the other.
TwinPeaks then subtracts the low one from the high one. Below the low filter's cutoff both filters
let the sound through, so it cancels out. Above the high filter's cutoff both have cut it, so it's gone.
What's left is the band in between, with a <b>resonant peak at each edge</b>.</p>
<p>That gives a vowel-like, talking quality that one filter can't, because there are two formants you
can move together (FREQ) or pull apart (SPREAD). The little screen shows the shape live: the amber tick is
the low peak, the red tick the high peak.</p>
<p>Turn RES past about 93% and both peaks <b>whistle on their own</b>: TwinPeaks becomes two sine
oscillators a SPREAD apart. DRIVE pushes the sound into a soft clipper before the filters, and the
resonance itself squashes rather than blows up, which is where the rounded Hordijk scream comes from.</p>
</div></div>

<h2>Controls</h2>
<table>
<tr><th>Control</th><th>What it does</th></tr>
<tr><td>FREQ</td><td>Where the peaks sit, 20 Hz to 20 kHz. Moves both together.</td></tr>
<tr><td>SPREAD</td><td>How far apart the peaks are: close (2 semitones) to 5 octaves. Close = one narrow nasal band; wide = a thin top and bottom with a hollow middle.</td></tr>
<tr><td>PEAKS switch</td><td>ABOVE (down): the low peak sits on FREQ and SPREAD pushes the high peak up. AROUND (up): the peaks open out either side of FREQ, so SPREAD sweeps keep the centre steady.</td></tr>
<tr><td>RES</td><td>Height of both peaks. Noon is gentle, 3 o'clock is vocal, from about 93% they self-oscillate.</td></tr>
<tr><td>DRIVE</td><td>Input level into the soft clipper. Fully left is clean and quiet, fully right is heavily squashed.</td></tr>
<tr><td>BALANCE</td><td>Level of each peak in TWIN. Left favours the LOW peak, right the HIGH peak; noon is equal.</td></tr>
<tr><td>FM + AMT</td><td>Modulates both peaks (1 V/oct at full AMT, either direction). Audio rate is fine and encouraged.</td></tr>
<tr><td>SPRD AMT</td><td>Amount and direction of the SPREAD CV (10 V = 5 octaves at full).</td></tr>
</table>

<h2>Jacks</h2>
<table>
<tr><th>Jack</th><th>What it does</th></tr>
<tr><td>V/OCT</td><td>Moves both peaks, 1 V/oct. With RES up, it plays the two whistles in tune.</td></tr>
<tr><td>FM</td><td>Through FM AMT, both peaks.</td></tr>
<tr><td>SPREAD, RES, DRIVE, BAL</td><td>CV for those knobs (RES and DRIVE: 10 V = full; BAL: &plusmn;5 V = full).</td></tr>
<tr><td>FM HI</td><td>Moves only the high peak, 1 V/oct, no knob. This is the Hordijk-style cross-patch point (see recipes).</td></tr>
<tr><td>IN</td><td>Audio in. All jacks are polyphonic.</td></tr>
<tr><td>TWIN</td><td>The two-peak output (high minus low). This is the main one.</td></tr>
<tr><td>LOW / HIGH</td><td>Each resonant low-pass on its own, for a normal filter or for cross-patching.</td></tr>
</table>
<p class="small">Right-click: <b>Clean resonance</b> lets the peaks get much louder before they squash: cleaner whistles, less grit. Keep an eye on levels.</p>

<h2 class="break">Recipes</h2>

<div class="recipe"><b class="t">1. Talking acid bass</b>
Saw from your bass voice &rarr; IN. FREQ 10 o'clock, SPREAD 11 o'clock, PEAKS ABOVE, RES 2 o'clock, DRIVE noon.
Your accent envelope &rarr; FM with AMT at 2 o'clock. Each accent opens both formants together, so the line "says" something
instead of just getting brighter. <i>Variation:</i> send the envelope to SPREAD instead (SPRD AMT up): the vowel changes shape rather than pitch.</div>

<div class="recipe"><b class="t">2. Detroit pad that breathes</b>
Chords (the Changes or a Hapax chord into a poly saw) &rarr; IN, polyphonic. PEAKS AROUND, FREQ 1 o'clock, RES noon.
A slow LFO (60 bars) &rarr; SPREAD, SPRD AMT 10 o'clock. With AROUND, the centre stays put while the band breathes wider and narrower,
so the chord gets hollow and full over the section. Good for steering an arrangement without touching the notes.</div>

<div class="recipe"><b class="t">3. Two-tone whistle voice (no input needed)</b>
RES fully right, nothing in IN (a short trigger into IN kicks it if it's silent). Sequencer pitch &rarr; V/OCT.
SPREAD sets the interval: fully left = two semitones (a rough, beating pair), about 8 o'clock = a fifth, 9 o'clock = an octave, noon = two and a half octaves. Fine-tune it by ear.
Run a VCA after TWIN with your envelope. <i>Variation:</i> random CV into SPREAD per step and you get an Autechre-ish
two-note chime whose interval mutates.</div>

<div class="recipe"><b class="t">4. Hordijk cross-patch (the chaos one)</b>
Noise or a drum loop &rarr; IN, RES 3 o'clock. Patch <b>LOW out &rarr; FM HI</b>. Now the low filter's output wobbles the high peak at
audio rate, which changes what comes out, which changes the wobble. Small changes to FREQ and DRIVE flip it between purr, growl and
squeal. Add HIGH &rarr; FM with a little AMT for a fully self-patched loop, Benjolin-style. Record a long take and edit the best bits.</div>

<div class="recipe"><b class="t">5. Hats and percussion into vowels</b>
808 Metal or a hat loop &rarr; IN, FREQ 2 o'clock, SPREAD 9 o'clock, RES 3 o'clock, BALANCE right. Random8 or Marbles stepped CV &rarr; FM.
Each hit lands on a different pair of resonances, so a static hat pattern gets a pitched, talking variation that doesn't loop-fatigue.</div>

<h2>Sweet spots</h2>
<ul>
<li>Bass: FREQ 9&ndash;11 o'clock, SPREAD 10&ndash;noon, DRIVE noon. TWIN removes the very lowest end, so blend in LOW (or the dry bass) under it if the sub goes missing.</li>
<li>Vowels: SPREAD about 1.5&ndash;2.5 octaves, RES 2&ndash;3 o'clock. Move FREQ slowly.</li>
<li>Very close SPREAD with low RES cancels a lot of the sound: it's meant to, turn up DRIVE or RES to bring it back.</li>
</ul>
<p class="small">TwinPeaks 2.0.0 &middot; Haer &middot; original code; the twin-peak filter idea is Rob Hordijk's. Plugin file: TwinPeaks-2.0.0-mac-arm64.vcvplugin. Drag it onto the Rack window, or copy it into Documents/Rack2/plugins-mac-arm64.</p>
</body></html>"""

tmp = out + ".html"
open(tmp, "w").write(html)
js = f"""const {{chromium}} = require('playwright');
(async () => {{ const b = await chromium.launch(); const p = await b.newPage();
await p.goto('file://{tmp}'); await p.pdf({{path: '{out}', format: 'A4', printBackground: true}}); await b.close(); }})();"""
open(out + ".js", "w").write(js)
subprocess.run(["node", out + ".js"], check=True, cwd="/tmp")
os.remove(out + ".js"); os.remove(tmp)
print("wrote", out)
