// Offline checks for the TwinPeaks engine (no Rack needed).
//   make test     (or: c++ -O2 -std=c++17 test/render.cpp -o /tmp/tp && /tmp/tp out_dir)
// Prints a pass/fail line per check and writes demo WAVs into out_dir.
#include "../src/TwinPeaksDSP.hpp"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <random>

using namespace twinpeaks;
static const float SR = 48000.f;
static int failures = 0;

static void check(bool ok, const char* what) {
	printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok) failures++;
}

static void writeWav(const std::string& path, const std::vector<float>& l, const std::vector<float>& r) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) return;
	uint32_t n = l.size(), dataBytes = n * 4, sr = (uint32_t) SR, br = sr * 4;
	uint16_t ch = 2, bits = 16, align = 4, fmt = 1;
	uint32_t riff = 36 + dataBytes, sixteen = 16;
	fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
	fwrite(&sixteen, 4, 1, f); fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
	fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
	fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f);
	for (uint32_t i = 0; i < n; i++) {
		int16_t a = (int16_t) std::max(-32767.f, std::min(32767.f, l[i] / 10.f * 32767.f));
		int16_t b = (int16_t) std::max(-32767.f, std::min(32767.f, r[i] / 10.f * 32767.f));
		fwrite(&a, 2, 1, f); fwrite(&b, 2, 1, f);
	}
	fclose(f);
}

// RMS of the TWIN output for a sine at freq hz.
static float sineGain(Controls c, float hz) {
	Engine e; e.setSampleRate(SR);
	Channel ch;
	double acc = 0; int n = 0;
	for (int i = 0; i < 24000; i++) {
		float in = 1.f * std::sin(2 * PI * hz * i / SR);   // 1 V: stays in the clean region
		e.process(ch, c, in);
		if (i >= 12000) { acc += ch.outTwin * ch.outTwin; n++; }
	}
	return std::sqrt(acc / n) / (1.f / std::sqrt(2.f));
}

// Zero-crossing-free pitch estimate: strongest DFT bin between lo and hi Hz.
static float strongest(const std::vector<float>& x, float lo, float hi) {
	float best = 0, bestHz = 0;
	for (float hz = lo; hz <= hi; hz *= 1.005f) {
		double re = 0, im = 0;
		for (size_t i = 0; i < x.size(); i++) {
			re += x[i] * std::cos(2 * PI * hz * i / SR);
			im += x[i] * std::sin(2 * PI * hz * i / SR);
		}
		float m = re * re + im * im;
		if (m > best) { best = m; bestHz = hz; }
	}
	return bestHz;
}

int main(int argc, char** argv) {
	std::string out = argc > 1 ? argv[1] : ".";

	// 1. Two peaks: low at 400 Hz, high at 1600 Hz (2 octaves), resonance 0.8
	{
		Controls c; c.freqOct = std::log2(400.f / 20.f); c.spreadOct = 2.f; c.res = 0.8f; c.drive = 0.f;
		float gLo = sineGain(c, 400), gMid = sineGain(c, 800), gHi = sineGain(c, 1600);
		float gBass = sineGain(c, 50), gTop = sineGain(c, 12000);
		printf("     gains: 50 Hz %.3f  400 Hz %.3f  800 Hz %.3f  1600 Hz %.3f  12 kHz %.3f\n", gBass, gLo, gMid, gHi, gTop);
		check(gLo > 1.5f * gMid && gHi > 1.5f * gMid, "two resonant peaks stand above the middle of the band");
		check(gBass < 0.1f * gLo && gTop < 0.1f * gHi, "lows cancel and highs are filtered away");
	}
	// 2. Centred mode puts the peaks either side of FREQ
	{
		Engine e; e.setSampleRate(SR);
		Controls c; c.freqOct = std::log2(1000.f / 20.f); c.spreadOct = 2.f; c.centred = true;
		float fl, fh; e.peakFreqs(c, fl, fh);
		check(std::fabs(fl - 500.f) < 1.f && std::fabs(fh - 2000.f) < 2.f, "centred: 1 kHz with 2 oct spread -> 500 Hz and 2 kHz");
	}
	// 3. Self-oscillation: full resonance, a single click in, the two whistles ring on
	{
		Engine e; e.setSampleRate(SR);
		Channel ch;
		Controls c; c.freqOct = std::log2(300.f / 20.f); c.spreadOct = 1.5f; c.res = 1.f; c.drive = 0.f;
		std::vector<float> lo, hi;
		float peak = 0;
		for (int i = 0; i < 48000; i++) {
			e.process(ch, c, i == 0 ? 5.f : 0.f);
			if (i >= 40000) { lo.push_back(ch.outLo); hi.push_back(ch.outHi); peak = std::max(peak, std::fabs(ch.outTwin)); }
		}
		float fl = strongest(lo, 150, 600), fh = strongest(hi, 400, 1800);
		printf("     whistles at %.1f Hz and %.1f Hz (expected 300 and %.1f), level %.2f V\n", fl, fh, 300 * std::pow(2.f, 1.5f), peak);
		check(std::fabs(std::log2(fl / 300.f)) < 0.08f, "low peak self-oscillates near its pitch");
		check(std::fabs(std::log2(fh / (300.f * std::pow(2.f, 1.5f)))) < 0.08f, "high peak self-oscillates near its pitch");
		check(peak > 1.f && peak < 12.f, "self-oscillation is loud but bounded");
	}
	// 4. Abuse: noise, full drive, full resonance, wild audio-rate FM. Must stay finite and bounded.
	{
		Engine e; e.setSampleRate(SR);
		Channel ch;
		std::mt19937 rng(1);
		std::uniform_real_distribution<float> u(-10.f, 10.f);
		bool ok = true; float mx = 0;
		for (int i = 0; i < 96000; i++) {
			Controls c; c.res = 1.f; c.drive = 1.f; c.spreadOct = 3.f;
			c.freqOct = 5.f + 6.f * std::sin(2 * PI * 900.f * i / SR);
			c.hiFmOct = 3.f * std::sin(2 * PI * 1300.f * i / SR);
			e.process(ch, c, u(rng));
			if (!std::isfinite(ch.outTwin)) ok = false;
			mx = std::max(mx, std::fabs(ch.outTwin));
		}
		printf("     abuse peak %.2f V\n", mx);
		check(ok && mx < 40.f, "stays finite and bounded under noise + max drive + audio FM");
	}

	// Demo: a saw bassline through the filter while FREQ sweeps and SPREAD opens
	{
		Engine e; e.setSampleRate(SR);
		Channel cl, cr;
		std::vector<float> L, R;
		const float notes[8] = {43.65f, 43.65f, 87.31f, 43.65f, 51.91f, 43.65f, 65.41f, 58.27f}; // F1-ish
		float ph = 0;
		int total = (int) (SR * 12);
		for (int i = 0; i < total; i++) {
			float t = i / SR;
			int step = (int) (t * 8.f) % 8;   // 16ths at 120 bpm
			ph += notes[step] / SR; ph -= std::floor(ph);
			float saw = 5.f * (2.f * ph - 1.f);
			float stepT = std::fmod(t * 8.f, 1.f);
			float env = std::exp(-stepT * 4.f);
			Controls c;
			c.freqOct = 3.5f + 2.5f * (0.5f - 0.5f * std::cos(2 * PI * t / 12.f)) + 1.0f * env;
			c.spreadOct = 0.6f + 2.4f * (t / 12.f);
			c.res = 0.55f + 0.4f * (0.5f - 0.5f * std::cos(2 * PI * t / 6.f));
			c.drive = 0.45f;
			e.process(cl, c, saw);
			c.balance = 0.3f;
			e.process(cr, c, saw);
			L.push_back(cl.outTwin); R.push_back(cr.outTwin);
		}
		writeWav(out + "/twinpeaks-demo.wav", L, R);
	}
	printf(failures ? "%d check(s) failed\n" : "all checks passed\n", failures);
	return failures ? 1 : 0;
}
