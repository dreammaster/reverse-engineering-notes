#include "engine/opl2.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Ultima6 {

namespace {

const double PI = 3.14159265358979323846;
const int MULT_X2[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30};
const int KSL_ROM[16] = {0, 32, 40, 45, 48, 51, 53, 55, 56, 58, 59, 60, 61, 62, 63, 64};
const int KSL_SHIFT[4] = {8, 1, 2, 0};

// Time in samples to cross the whole envelope at an effective rate (datasheet: 39280.64 ms for a decay at rate 4 of the 64, 2826.24 ms for an attack; each 4 rates halve it and
// the four rates in between are 4/5, 4/6 and 4/7 of one another)
double sweepSamples(int rate, double base) {
	int hi = rate >> 2, lo = rate & 3;
	double ms = base * 4.0 / (4 + lo) / std::pow(2.0, hi - 1);
	return ms * Opl2::SAMPLE_RATE / 1000.0;
}

double attenuationAmplitude(double units) {
	static double table[1200];
	static bool ready = false;
	if (!ready) {
		for (int i = 0; i < 1200; ++i)
			table[i] = std::pow(10.0, -i * 0.1875 / 20.0);
		ready = true;
	}
	int i = (int)units;
	return i < 0 ? 1.0 : i >= 1200 ? 0.0 : table[i];
}

double waveform(int wave, int index) {
	double s = std::sin(2.0 * PI * (index + 0.5) / 1024.0);
	switch (wave) {
	case 1:
		return s > 0 ? s : 0;
	case 2:
		return std::fabs(s);
	case 3:
		return (index & 256) ? 0 : std::fabs(s);
	default:
		return s;
	}
}

} // namespace

Opl2::Opl2() {
	reset();
}

void Opl2::reset() {
	std::memset(_regs, 0, sizeof(_regs));
	for (Voice &v : _voice)
		v = Voice();
	_waveSelect = _noteSelect = _deepTremolo = _deepVibrato = false;
	_tremoloPhase = _vibratoPhase = 0;
}

void Opl2::keyChange(Voice &v, bool on) {
	if (on == v.keyOn)
		return;
	v.keyOn = on;
	for (Op &o : v.op) {
		if (on) {
			o.state = ATTACK;
			o.phase = 0;
		} else if (o.state != OFF) {
			o.state = RELEASE;
		}
	}
}

void Opl2::write(int reg, int value) {
	reg &= 0xFF;
	value &= 0xFF;
	_regs[reg] = (uint8_t)value;
	if (reg == 0x01) {
		_waveSelect = (value & 0x20) != 0;
		return;
	}
	if (reg == 0x08) {
		_noteSelect = (value & 0x40) != 0;
		return;
	}
	if (reg == 0xBD) {
		_deepTremolo = (value & 0x80) != 0;
		_deepVibrato = (value & 0x40) != 0;
		return;
	}
	if (reg >= 0xA0 && reg <= 0xA8) {
		Voice &v = _voice[reg - 0xA0];
		v.fnum = (v.fnum & 0x300) | value;
		return;
	}
	if (reg >= 0xB0 && reg <= 0xB8) {
		Voice &v = _voice[reg - 0xB0];
		v.fnum = (v.fnum & 0xFF) | ((value & 3) << 8);
		v.block = (value >> 2) & 7;
		keyChange(v, (value & 0x20) != 0);
		return;
	}
	if (reg >= 0xC0 && reg <= 0xC8) {
		Voice &v = _voice[reg - 0xC0];
		v.additive = (value & 1) != 0;
		v.feedback = (value >> 1) & 7;
		return;
	}
	if (reg < 0x20 || reg > 0xF5)
		return;
	int off = reg & 0x1F, idx = off & 7, group = off >> 3;
	if (idx >= 6 || group > 2)
		return;
	Voice &v = _voice[group * 3 + idx % 3];
	Op &o = v.op[idx / 3];
	switch (reg & 0xE0) {
	case 0x20:
		o.am = (value & 0x80) != 0;
		o.vib = (value & 0x40) != 0;
		o.sustained = (value & 0x20) != 0;
		o.ksr = (value & 0x10) != 0;
		o.mult = value & 15;
		break;
	case 0x40:
		o.ksl = value >> 6;
		o.tl = value & 63;
		break;
	case 0x60:
		o.ar = value >> 4;
		o.dr = value & 15;
		break;
	case 0x80:
		o.sl = value >> 4;
		o.rr = value & 15;
		break;
	case 0xE0:
		o.wave = value & 3;
		break;
	default:
		break;
	}
}

double Opl2::rateEffective(const Voice &v, const Op &o, int rate) const {
	if (rate == 0)
		return 0;
	int keyScale = (v.block << 1) | (_noteSelect ? (v.fnum >> 8) & 1 : (v.fnum >> 9) & 1);
	int r = rate * 4 + (o.ksr ? keyScale : keyScale >> 2);
	return r > 63 ? 63 : r;
}

void Opl2::updateEnvelope(const Voice &v, Op &o) {
	switch (o.state) {
	case ATTACK: {
		int r = (int)rateEffective(v, o, o.ar);
		if (r == 0)
			break;
		if (r >= 60) {
			o.env = 0;
		} else {
			double n = sweepSamples(r, 2826.24);
			o.env = (o.env + 1.0) * std::exp(-std::log(512.0) / n) - 1.0;
		}
		if (o.env <= 0) {
			o.env = 0;
			o.state = DECAY;
		}
		break;
	}
	case DECAY: {
		int r = (int)rateEffective(v, o, o.dr);
		double target = (o.sl == 15 ? 31 : o.sl) * 16.0;
		if (r > 0)
			o.env += 512.0 / sweepSamples(r, 39280.64);
		if (o.env >= target) {
			o.env = target;
			o.state = SUSTAIN;
		}
		break;
	}
	case SUSTAIN:
		if (!o.sustained) {
			int r = (int)rateEffective(v, o, o.rr);
			if (r > 0)
				o.env += 512.0 / sweepSamples(r, 39280.64);
			if (o.env >= 511) {
				o.env = 511;
				o.state = OFF;
			}
		}
		break;
	case RELEASE: {
		int r = (int)rateEffective(v, o, o.rr);
		if (r > 0)
			o.env += 512.0 / sweepSamples(r, 39280.64);
		if (o.env >= 511) {
			o.env = 511;
			o.state = OFF;
		}
		break;
	}
	default:
		break;
	}
}

// One sample of an operator: `modulation` is the phase offset in cycles
double Opl2::step(const Voice &v, Op &o, double modulation, double tremolo, double vibrato, bool, int) {
	updateEnvelope(v, o);
	if (o.state == OFF)
		return 0;
	int ksl = (KSL_ROM[v.fnum >> 6] << 2) - ((8 - v.block) << 5);
	if (ksl < 0)
		ksl = 0;
	ksl = KSL_SHIFT[o.ksl] >= 8 ? 0 : ksl >> KSL_SHIFT[o.ksl];
	double att = o.env + o.tl * 4 + ksl + (o.am ? tremolo : 0);
	double inc = (double)(v.fnum << v.block) * MULT_X2[o.mult] / 2.0;
	if (o.vib)
		inc *= vibrato;
	int index = (((int)o.phase >> 10) + (int)std::floor(modulation * 1024.0)) & 1023;
	double out = attenuationAmplitude(att) * waveform(_waveSelect ? o.wave : 0, index);
	o.phase += inc;
	if (o.phase >= 1048576.0)
		o.phase = std::fmod(o.phase, 1048576.0);
	return out;
}

void Opl2::generate(int16_t *out, int count) {
	for (int n = 0; n < count; ++n) {
		_tremoloPhase += 3.7 / SAMPLE_RATE;
		_vibratoPhase += 6.1 / SAMPLE_RATE;
		if (_tremoloPhase >= 1)
			_tremoloPhase -= 1;
		if (_vibratoPhase >= 1)
			_vibratoPhase -= 1;
		double tri = _tremoloPhase < 0.5 ? _tremoloPhase * 2 : 2 - _tremoloPhase * 2;
		double tremolo = tri * (_deepTremolo ? 25.6 : 5.33);
		double vtri = _vibratoPhase < 0.5 ? _vibratoPhase * 4 - 1 : 3 - _vibratoPhase * 4;
		double vibrato = std::pow(2.0, vtri * (_deepVibrato ? 14.0 : 7.0) / 1200.0);
		double mix = 0;
		for (Voice &v : _voice) {
			Op &m = v.op[0], &c = v.op[1];
			if (m.state == OFF && c.state == OFF)
				continue;
			double fb = v.feedback ? (m.prev1 + m.prev2) * std::pow(2.0, v.feedback - 7) : 0;
			double mo = step(v, m, fb, tremolo, vibrato, true, v.feedback);
			m.prev2 = m.prev1;
			m.prev1 = mo;
			if (v.additive)
				mix += mo + step(v, c, 0, tremolo, vibrato, false, 0);
			else
				mix += step(v, c, mo * 2.0, tremolo, vibrato, false, 0);
		}
		double s = mix * 2400.0;
		out[n] = (int16_t)(s > 32767 ? 32767 : s < -32768 ? -32768 : s);
	}
}

} // namespace Ultima6
