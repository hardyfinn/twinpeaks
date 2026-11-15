// TwinPeaksDSP.hpp: the sound engine for TwinPeaks, with no VCV Rack code in it
// (so test/render.cpp can run it offline).
//
// The idea comes from Rob Hordijk's Twin Peak filter: two resonant low-pass
// filters run side by side on the same input, one tuned higher than the other,
// and the output is the HIGH one minus the LOW one. Everything below the low
// cutoff cancels out, everything above the high cutoff is filtered away, and
// what is left is a band with a resonant peak at each edge: two peaks.
// Turn the resonance up far enough and each peak whistles on its own, so the
// filter becomes two sine oscillators a set distance apart.
//
// Each filter is a 2-pole state-variable filter (Simper/Zavalishin
// "trapezoidal" form) with a soft limiter on its band-pass state. That limiter
// is what lets the resonance go past self-oscillation without blowing up and
// gives the screaming, rounded overdrive Hordijk's filters are known for.
// The whole thing runs at twice the sample rate so audio-rate FM stays clean.
#pragma once
#include <cmath>
#include <algorithm>

namespace twinpeaks {

static const float PI = 3.14159265358979f;
static const float FREQ_MIN = 20.f;     // FREQ knob fully left
static const float FREQ_OCTAVES = 10.f; // ...fully right = 20 Hz * 2^10 = 20.48 kHz
static const float SPREAD_MAX = 5.f;    // SPREAD knob fully right = 5 octaves apart
static const float SPREAD_MIN = 1.f / 6.f; // never closer than 2 semitones (they would cancel)

// Soft limiter used inside the filter loop: straight line for small signals,
// bends smoothly toward +/-lim for big ones.
static inline float soft(float x, float lim) {
	float y = x / lim;
	return lim * y / std::sqrt(1.f + y * y);
}

// Cheap tanh for the input drive stage.
static inline float tanhFast(float x) {
	x = std::max(-3.f, std::min(3.f, x));
	float x2 = x * x;
	return x * (27.f + x2) / (27.f + 9.f * x2);
}

struct SVF {
	float ic1 = 0.f, ic2 = 0.f;   // the two integrator memories
	float lp = 0.f;
	void reset() { ic1 = ic2 = lp = 0.f; }
	// g = tan(pi fc / fs), k = damping (2 = no resonance, 0 = self-oscillates)
	inline float process(float v0, float g, float k, float sat) {
		float a1 = 1.f / (1.f + g * (g + k));
		float a2 = g * a1;
		float a3 = g * a2;
		float v3 = v0 - ic2;
		float v1 = a1 * ic1 + a2 * v3;
		float v2 = ic2 + a2 * ic1 + a3 * v3;
		ic1 = soft(2.f * v1 - ic1, sat);
		ic2 = 2.f * v2 - ic2;
		if (!std::isfinite(ic1) || !std::isfinite(ic2)) reset();
		lp = v2;
		return v2;
	}
};

// Everything the panel sets, already turned into plain numbers.
struct Controls {
	float freqOct = 5.f;     // octaves above 20 Hz for the low peak (or the centre)
	float hiFmOct = 0.f;     // extra octaves for the high peak only (FM HI input)
	float spreadOct = 2.f;   // distance between the peaks in octaves
	float res = 0.5f;        // 0..1, about 0.93 starts self-oscillation
	float drive = 0.3f;      // 0..1
	float balance = 0.f;     // -1 = low peak only .. +1 = high peak only
	bool centred = false;    // true: peaks sit either side of FREQ; false: low peak on FREQ
	bool clean = false;      // true: much higher limiter ceiling (cleaner, louder resonance)
};

struct Channel {
	SVF lo, hi;
	float lastIn = 0.f;
	float outTwin = 0.f, outLo = 0.f, outHi = 0.f;
	void reset() { lo.reset(); hi.reset(); lastIn = 0.f; }
};

struct Engine {
	float sampleRate = 48000.f;
	void setSampleRate(float sr) { sampleRate = sr; }

	// Turn controls into the two cutoff frequencies (Hz).
	void peakFreqs(const Controls& c, float& fLo, float& fHi) const {
		float spread = std::max(SPREAD_MIN, c.spreadOct);
		float oLo = c.centred ? c.freqOct - 0.5f * spread : c.freqOct;
		float oHi = oLo + spread + c.hiFmOct;
		fLo = FREQ_MIN * std::pow(2.f, std::max(-4.f, std::min(11.f, oLo)));
		fHi = FREQ_MIN * std::pow(2.f, std::max(-4.f, std::min(11.f, oHi)));
	}

	static float damping(float res) {
		res = std::max(0.f, std::min(1.f, res));
		// Gentle at first, steep near the top: Q about 1.4 at noon, 4.5 at 3 o'clock,
		// crosses zero (self-oscillation) at about 0.93; a little negative above
		// that so the whistle sustains, held in check by the limiter.
		return 2.f * std::pow(1.f - res, 1.5f) - 0.06f * res * res * res * res;
	}

	// in: input voltage. Results land in ch.outTwin/outLo/outHi (volts).
	void process(Channel& ch, const Controls& c, float in) {
		float fLo, fHi;
		peakFreqs(c, fLo, fHi);
		float fs2 = 2.f * sampleRate;
		float nyq = 0.45f * fs2;
		float gLo = std::tan(PI * std::min(fLo, nyq) / fs2);
		float gHi = std::tan(PI * std::min(fHi, nyq) / fs2);
		float k = damping(c.res);
		float sat = c.clean ? 8.f : 2.5f;

		// DRIVE: 0 -> x0.5 (clean), 1 -> x16 (heavily squashed)
		float gain = 0.5f * std::pow(32.f, std::max(0.f, std::min(1.f, c.drive)));
		float gLevel = std::min(1.f, 1.f - c.balance);   // low peak level
		float gHiLevel = std::min(1.f, 1.f + c.balance); // high peak level

		float x1 = in * 0.2f;                            // 5 V -> 1.0
		float x0 = 0.5f * (ch.lastIn + x1);              // in-between sample
		ch.lastIn = x1;
		float sumT = 0.f, sumL = 0.f, sumH = 0.f;
		float xs[2] = {x0, x1};
		for (int i = 0; i < 2; i++) {
			float x = tanhFast(xs[i] * gain);
			if (gain < 1.f) x = xs[i] * gain;           // below unity: keep it clean
			float l = ch.lo.process(x, gLo, k, sat);
			float h = ch.hi.process(x, gHi, k, sat);
			sumL += l;
			sumH += h;
			sumT += gHiLevel * h - gLevel * l;
		}
		// back to volts (and average the two half-steps)
		ch.outTwin = 2.5f * sumT;
		ch.outLo = 2.5f * sumL;
		ch.outHi = 2.5f * sumH;
	}

	// Magnitude response (linear) of the TWIN output at frequency f, for the
	// little screen. Uses the ideal analogue filters, ignoring the limiter.
	static float twinResponse(float f, float fLo, float fHi, float res, float balance) {
		float k = std::max(0.06f, damping(res));
		float gL = std::min(1.f, 1.f - balance), gH = std::min(1.f, 1.f + balance);
		auto lp = [&](float fc, float& re, float& im) {
			float w = f / fc;
			// 1 / (1 - w^2 + j k w)
			float dr = 1.f - w * w, di = k * w;
			float d = dr * dr + di * di;
			re = dr / d;
			im = -di / d;
		};
		float lr, li, hr, hi;
		lp(fLo, lr, li);
		lp(fHi, hr, hi);
		float re = gH * hr - gL * lr, im = gH * hi - gL * li;
		return std::sqrt(re * re + im * im);
	}
};

} // namespace twinpeaks
