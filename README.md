# TwinPeaks

A twin-peak resonant filter for VCV Rack, after Rob Hordijk's Twin Peak filter
(the filter idea behind the Benjolin and Blippoo Box). Part of the Haer plugins.

Two resonant low-pass filters hear the same input, one tuned higher than the
other. The output is the high one minus the low one: the lows cancel, the highs
are filtered away, and you hear a band with a resonant peak at each edge.
Push RES past about 93 % and both peaks whistle on their own as two sine tones.

Controls: FREQ, SPREAD (+ amount), RES, DRIVE, BALANCE, FM (+ amount), FM HI
(high peak only), PEAKS switch (ABOVE: low peak on FREQ; AROUND: peaks either side).
Outputs: TWIN, LOW, HIGH. Polyphonic. Right-click: Clean resonance.

Build: `make dist RACK_DIR=path/to/Rack-SDK`. GitHub Actions builds the Mac
(Apple Silicon) plugin on every push. Engine checks without Rack: `make test`.
Panel: `python3 res/make_panel.py`; preview: `python3 tools/preview.py out.png`.

Original code (MIT licence); the twin-peak idea is credited to Rob Hordijk.

## Licence

MIT (see `LICENSE.txt`): you're free to use, change, port (VST/CLAP, other platforms, hardware) and share this code, as long as the copyright notice and licence text come along. Fonts on the panel (Inter, Climate Crisis) are under the SIL Open Font Licence.
