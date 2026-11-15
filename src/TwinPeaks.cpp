// TwinPeaks: a twin-peak resonant filter for VCV Rack, after Rob Hordijk's
// Twin Peak filter. Two resonant low-pass filters hear the same input; the
// output is the high one minus the low one, which leaves a band with a
// resonant peak at each edge. The sound engine is in TwinPeaksDSP.hpp.
#include "plugin.hpp"
#include "TwinPeaksDSP.hpp"

// ---- LAYOUT (same numbers in res/make_panel.py) ----
static const float BIG_X[2] = {24.0f, 67.44f};
static const float BIG_Y = 31.0f;
static const float MID_X[3] = {15.5f, 45.72f, 75.94f};
static const float MID_Y = 55.0f;
static const float TRIM_X[2] = {11.0f, 80.44f};
static const float TRIM_Y = 73.0f;
static const float DISP_X = 21.0f, DISP_Y = 64.0f, DISP_W = 49.44f, DISP_H = 17.0f;
static const float JX[4] = {12.0f, 34.5f, 57.0f, 79.5f};
static const float J1_Y = 90.0f, J2_Y = 104.0f, J3_Y = 117.5f;

struct TwinPeaks : Module {
	enum ParamIds {
		FREQ_PARAM, SPREAD_PARAM,
		RES_PARAM, DRIVE_PARAM, BAL_PARAM,
		FM_ATV_PARAM, SPREAD_ATV_PARAM,
		CENTRE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		VOCT_INPUT, FM_INPUT, SPREAD_INPUT, RES_INPUT,
		DRIVE_INPUT, BAL_INPUT, FMHI_INPUT,
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TWIN_OUTPUT, LOW_OUTPUT, HIGH_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	twinpeaks::Engine engine;
	twinpeaks::Channel ch[16];
	bool clean = false;        // right-click: cleaner, louder resonance

	// what the little screen shows (channel 1)
	float dispLo = 400.f, dispHi = 1600.f, dispRes = 0.5f, dispBal = 0.f;

	TwinPeaks() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.45f, "Frequency", " Hz", 1024.f, twinpeaks::FREQ_MIN);
		configParam(SPREAD_PARAM, 0.f, 1.f, 0.4f, "Spread (distance between the peaks)", " oct", 0.f, twinpeaks::SPREAD_MAX);
		configParam(RES_PARAM, 0.f, 1.f, 0.7f, "Resonance (whistles from about 93%)", "%", 0.f, 100.f);
		configParam(DRIVE_PARAM, 0.f, 1.f, 0.3f, "Drive", "%", 0.f, 100.f);
		configParam(BAL_PARAM, -1.f, 1.f, 0.f, "Balance (left: low peak, right: high peak)", "%", 0.f, 100.f);
		configParam(FM_ATV_PARAM, -1.f, 1.f, 0.f, "FM amount", "%", 0.f, 100.f);
		configParam(SPREAD_ATV_PARAM, -1.f, 1.f, 0.f, "Spread CV amount", "%", 0.f, 100.f);
		configSwitch(CENTRE_PARAM, 0.f, 1.f, 0.f, "Peaks",
		             {"Above FREQ (low peak sits on FREQ)", "Around FREQ (peaks either side)"});

		configInput(VOCT_INPUT, "V/Oct (moves both peaks)");
		configInput(FM_INPUT, "FM (both peaks, through FM amount; audio rate is fine)");
		configInput(SPREAD_INPUT, "Spread CV (through its amount knob, 10 V = 5 octaves)");
		configInput(RES_INPUT, "Resonance CV (10 V = full)");
		configInput(DRIVE_INPUT, "Drive CV (10 V = full)");
		configInput(BAL_INPUT, "Balance CV (+/-5 V = full)");
		configInput(FMHI_INPUT, "FM HI (high peak only, 1 V/oct; try patching LOW out here)");
		configInput(IN_INPUT, "Audio");
		configOutput(TWIN_OUTPUT, "Twin peaks (high minus low)");
		configOutput(LOW_OUTPUT, "Low peak filter (resonant low-pass)");
		configOutput(HIGH_OUTPUT, "High peak filter (resonant low-pass)");
		configBypass(IN_INPUT, TWIN_OUTPUT);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		engine.setSampleRate(e.sampleRate);
	}
	void onAdd(const AddEvent& e) override {
		engine.setSampleRate(APP->engine->getSampleRate());
	}
	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		for (auto& c : ch) c.reset();
	}

	static float softLimit(float v) {
		float a = std::fabs(v);
		if (a < 8.f) return v;
		float y = 8.f + 3.f * std::tanh((a - 8.f) / 3.f);
		return v < 0.f ? -y : y;
	}

	void process(const ProcessArgs& args) override {
		int channels = 1;
		for (int i = 0; i < NUM_INPUTS; i++)
			channels = std::max(channels, inputs[i].getChannels());

		float freqK = params[FREQ_PARAM].getValue() * twinpeaks::FREQ_OCTAVES;
		float spreadK = params[SPREAD_PARAM].getValue() * twinpeaks::SPREAD_MAX;
		float fmAtv = params[FM_ATV_PARAM].getValue();
		float spAtv = params[SPREAD_ATV_PARAM].getValue();
		bool centred = params[CENTRE_PARAM].getValue() > 0.5f;

		for (int c = 0; c < channels; c++) {
			twinpeaks::Controls k;
			k.freqOct = freqK + inputs[VOCT_INPUT].getPolyVoltage(c) + fmAtv * inputs[FM_INPUT].getPolyVoltage(c);
			k.hiFmOct = inputs[FMHI_INPUT].getPolyVoltage(c);
			k.spreadOct = clamp(spreadK + spAtv * inputs[SPREAD_INPUT].getPolyVoltage(c) * 0.5f, 0.f, 6.f);
			k.res = clamp(params[RES_PARAM].getValue() + inputs[RES_INPUT].getPolyVoltage(c) * 0.1f, 0.f, 1.f);
			k.drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_INPUT].getPolyVoltage(c) * 0.1f, 0.f, 1.f);
			k.balance = clamp(params[BAL_PARAM].getValue() + inputs[BAL_INPUT].getPolyVoltage(c) * 0.2f, -1.f, 1.f);
			k.centred = centred;
			k.clean = clean;

			engine.process(ch[c], k, inputs[IN_INPUT].getPolyVoltage(c));
			outputs[TWIN_OUTPUT].setVoltage(softLimit(ch[c].outTwin), c);
			outputs[LOW_OUTPUT].setVoltage(softLimit(ch[c].outLo), c);
			outputs[HIGH_OUTPUT].setVoltage(softLimit(ch[c].outHi), c);

			if (c == 0) {
				engine.peakFreqs(k, dispLo, dispHi);
				dispRes = k.res;
				dispBal = k.balance;
			}
		}
		for (int o = 0; o < NUM_OUTPUTS; o++)
			outputs[o].setChannels(channels);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "clean", json_boolean(clean));
		return root;
	}
	void dataFromJson(json_t* root) override {
		json_t* j;
		if ((j = json_object_get(root, "clean"))) clean = json_boolean_value(j);
	}
};

// The little screen: the shape of the TWIN response from 20 Hz to 20 kHz,
// with a tick under each peak.
struct PeaksDisplay : Widget {
	TwinPeaks* module = nullptr;

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1) {
			Widget::drawLayer(args, layer);
			return;
		}
		float lo = 400.f, hi = 1600.f, res = 0.6f, bal = 0.f;
		if (module) {
			lo = module->dispLo; hi = module->dispHi; res = module->dispRes; bal = module->dispBal;
		}
		float w = box.size.x, h = box.size.y;
		const float dbTop = 24.f, dbBot = -36.f;
		auto xOf = [&](float f) { return w * std::log2(f / 20.f) / std::log2(1000.f); };
		auto yOf = [&](float db) { return h * (dbTop - clamp(db, dbBot, dbTop)) / (dbTop - dbBot); };

		nvgScissor(args.vg, 0, 0, w, h);
		// 0 dB line
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 0, yOf(0.f));
		nvgLineTo(args.vg, w, yOf(0.f));
		nvgStrokeColor(args.vg, nvgRGBA(241, 233, 218, 40));
		nvgStrokeWidth(args.vg, 0.8f);
		nvgStroke(args.vg);

		// response curve
		nvgBeginPath(args.vg);
		const int N = 120;
		for (int i = 0; i <= N; i++) {
			float f = 20.f * std::pow(1000.f, i / (float) N);
			float m = twinpeaks::Engine::twinResponse(f, lo, hi, res, bal);
			float db = 20.f * std::log10(std::max(m, 1e-5f));
			float x = w * i / N, y = yOf(db);
			if (i == 0) nvgMoveTo(args.vg, x, y);
			else nvgLineTo(args.vg, x, y);
		}
		nvgLineTo(args.vg, w, h);
		nvgLineTo(args.vg, 0, h);
		nvgClosePath(args.vg);
		nvgFillColor(args.vg, nvgRGBA(102, 214, 196, 50));
		nvgFill(args.vg);

		nvgBeginPath(args.vg);
		for (int i = 0; i <= N; i++) {
			float f = 20.f * std::pow(1000.f, i / (float) N);
			float m = twinpeaks::Engine::twinResponse(f, lo, hi, res, bal);
			float db = 20.f * std::log10(std::max(m, 1e-5f));
			float x = w * i / N, y = yOf(db);
			if (i == 0) nvgMoveTo(args.vg, x, y);
			else nvgLineTo(args.vg, x, y);
		}
		nvgStrokeColor(args.vg, nvgRGB(102, 214, 196));
		nvgStrokeWidth(args.vg, 1.2f);
		nvgStroke(args.vg);

		// peak ticks
		float px[2] = {xOf(lo), xOf(hi)};
		NVGcolor pc[2] = {nvgRGB(242, 165, 65), nvgRGB(232, 96, 60)};
		for (int i = 0; i < 2; i++) {
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, px[i], h);
			nvgLineTo(args.vg, px[i], h - 4.f);
			nvgStrokeColor(args.vg, pc[i]);
			nvgStrokeWidth(args.vg, 1.6f);
			nvgStroke(args.vg);
		}
		nvgResetScissor(args.vg);
		Widget::drawLayer(args, layer);
	}
};

struct TwinPeaksWidget : ModuleWidget {
	TwinPeaksWidget(TwinPeaks* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/TwinPeaks.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(BIG_X[0], BIG_Y)), module, TwinPeaks::FREQ_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(BIG_X[1], BIG_Y)), module, TwinPeaks::SPREAD_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(MID_X[0], MID_Y)), module, TwinPeaks::RES_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(MID_X[1], MID_Y)), module, TwinPeaks::DRIVE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(MID_X[2], MID_Y)), module, TwinPeaks::BAL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(TRIM_X[0], TRIM_Y)), module, TwinPeaks::FM_ATV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(TRIM_X[1], TRIM_Y)), module, TwinPeaks::SPREAD_ATV_PARAM));

		PeaksDisplay* d = createWidget<PeaksDisplay>(mm2px(Vec(DISP_X, DISP_Y)));
		d->box.size = mm2px(Vec(DISP_W, DISP_H));
		d->module = module;
		addChild(d);

		const int j1[4] = {TwinPeaks::VOCT_INPUT, TwinPeaks::FM_INPUT, TwinPeaks::SPREAD_INPUT, TwinPeaks::RES_INPUT};
		const int j2[3] = {TwinPeaks::DRIVE_INPUT, TwinPeaks::BAL_INPUT, TwinPeaks::FMHI_INPUT};
		for (int i = 0; i < 4; i++)
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(JX[i], J1_Y)), module, j1[i]));
		for (int i = 0; i < 3; i++)
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(JX[i], J2_Y)), module, j2[i]));
		addParam(createParamCentered<CKSS>(mm2px(Vec(JX[3], J2_Y)), module, TwinPeaks::CENTRE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(JX[0], J3_Y)), module, TwinPeaks::IN_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(JX[1], J3_Y)), module, TwinPeaks::TWIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(JX[2], J3_Y)), module, TwinPeaks::LOW_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(JX[3], J3_Y)), module, TwinPeaks::HIGH_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		TwinPeaks* module = dynamic_cast<TwinPeaks*>(this->module);
		if (!module)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem("Clean resonance (less squash, louder whistles)", "", &module->clean));
	}
};

Model* modelTwinPeaks = createModel<TwinPeaks, TwinPeaksWidget>("TwinPeaks");
