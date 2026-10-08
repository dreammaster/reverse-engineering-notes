#include "opl.h"

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <string.h>

enum { ST_OFF, ST_ATTACK, ST_DECAY, ST_SUSTAIN, ST_RELEASE };

static const int MULT_X2[16] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30 };
static const int KSL_ROM[16] = { 0, 32, 40, 45, 48, 51, 53, 55, 56, 58, 59, 60, 61, 62, 63, 64 };
static const int KSL_SHIFT[4] = { 31, 1, 2, 0 };
static const int CH_MOD[9] = { 0, 1, 2, 6, 7, 8, 12, 13, 14 };

void opl_reset(Opl *o) {
	memset(o, 0, sizeof *o);
	for (int i = 0; i < 18; i++) o->op[i].env = 511;
}

void opl_write(Opl *o, uint8_t reg, uint8_t v) {
	uint8_t old = o->reg[reg];
	o->reg[reg] = v;
	if (reg >= 0xB0 && reg <= 0xB8) { /* key on / off */
		int ch = reg - 0xB0;
		if ((v & 0x20) && !(old & 0x20)) {
			for (int k = 0; k < 2; k++) {
				int op = CH_MOD[ch] + 3 * k;
				o->op[op].phase = 0;
				o->op[op].stage = ST_ATTACK;
			}
		} else if (!(v & 0x20) && (old & 0x20)) {
			for (int k = 0; k < 2; k++) {
				int op = CH_MOD[ch] + 3 * k;
				if (o->op[op].stage != ST_OFF) o->op[op].stage = ST_RELEASE;
			}
		}
	}
}

static double wave(int form, unsigned index) {
	double s = sin(2.0 * M_PI * (double)(index & 1023) / 1024.0);
	switch (form & 3) {
	case 0: return s;
	case 1: return s > 0 ? s : 0;
	case 2: return fabs(s);
	default: return (index & 512) ? 0 : fabs(s);
	}
}

static unsigned op_regoff(int op) { /* register offset of operator index */
	int group = op / 6, i = op % 6;
	return (unsigned)(group * 8 + i);
}

/* per-operator parameters that depend on the channel */
static void op_rates(Opl *o, int op, int ch, double *a_step, double *d_step, double *r_step, double *sl_att) {
	unsigned off = op_regoff(op);
	uint8_t r20 = o->reg[0x20 + off], r60 = o->reg[0x60 + off], r80 = o->reg[0x80 + off];
	int block = (o->reg[0xB0 + ch] >> 2) & 7, fnum9 = (o->reg[0xB0 + ch] >> 1) & 1;
	int rof = (block << 1) | fnum9;
	if (!(r20 & 0x10)) rof >>= 2;
	int ar = r60 >> 4, dr = r60 & 15, sl = r80 >> 4, rr = r80 & 15;
	double rate = OPL_RATE;
	#define EFF(r) ((r) ? 4 * (r) + rof : 0)
	int ea = EFF(ar), ed = EFF(dr), er = EFF(rr);
	/* attack: time constant; decay/release: linear in attenuation over the full 96 dB */
	*a_step = ar == 0 ? 0 : (ar >= 15 ? -1 : 1.0 / (2.82624e-3 * pow(2.0, -(ea - 4) / 4.0) * rate / 5.0));
	*d_step = dr == 0 ? 0 : 511.0 / (39.28064 * pow(2.0, -(ed - 4) / 4.0) * rate / 1000.0);
	*r_step = rr == 0 ? 0 : 511.0 / (39.28064 * pow(2.0, -(er - 4) / 4.0) * rate / 1000.0);
	*sl_att = sl == 15 ? 496 : sl * 16;
}

static void env_step(Opl *o, int op, int ch) {
	double a, d, r, sl;
	op_rates(o, op, ch, &a, &d, &r, &sl);
	unsigned off = op_regoff(op);
	int sustained = (o->reg[0x20 + off] & 0x20) != 0;
	double *e = &o->op[op].env;
	switch (o->op[op].stage) {
	case ST_ATTACK:
		if (a < 0) { *e = 0; } else if (a > 0) { *e -= (*e + 1.0) * a * 3.0; }
		if (*e < 0.5 || a < 0) { *e = 0; o->op[op].stage = ST_DECAY; }
		break;
	case ST_DECAY:
		*e += d;
		if (*e >= sl) { *e = sl; o->op[op].stage = ST_SUSTAIN; }
		break;
	case ST_SUSTAIN:
		if (!sustained) { *e += r; if (*e > 511) *e = 511; }
		break;
	case ST_RELEASE:
		*e += r;
		if (*e >= 511) { *e = 511; o->op[op].stage = ST_OFF; }
		break;
	default: break;
	}
	if (*e > 511) *e = 511;
}

static double op_out(Opl *o, int op, int ch, double modulation, double lfo_am, double lfo_vib) {
	unsigned off = op_regoff(op);
	uint8_t r20 = o->reg[0x20 + off], r40 = o->reg[0x40 + off];
	int fnum = o->reg[0xA0 + ch] | ((o->reg[0xB0 + ch] & 3) << 8), block = (o->reg[0xB0 + ch] >> 2) & 7;
	double vib = (r20 & 0x40) ? lfo_vib : 0.0;
	double inc = (double)(((unsigned)fnum << block) * MULT_X2[r20 & 15]) / 2.0 * (1.0 + vib);
	o->op[op].phase = (uint32_t)(o->op[op].phase + (uint32_t)inc) & 0xFFFFF;
	if (o->op[op].stage == ST_OFF) return 0;
	int form = (o->reg[1] & 0x20) ? (o->reg[0xE0 + off] & 3) : 0;
	unsigned index = (o->op[op].phase >> 10) + (unsigned)(int)floor(modulation * 2048.0);
	double w = wave(form, index);
	int ksl_sel = r40 >> 6;
	int ksl = (KSL_ROM[fnum >> 6] << 2) - ((8 - block) << 5);
	ksl = ksl < 0 ? 0 : ksl >> KSL_SHIFT[ksl_sel];
	if (ksl_sel == 0) ksl = 0;
	double att = o->op[op].env + (r40 & 63) * 4 + ksl + ((r20 & 0x80) ? lfo_am : 0.0);
	if (att > 511) att = 511;
	return w * pow(2.0, -att / 32.0);
}

void opl_samples(Opl *o, int16_t *out, int count) {
	for (int n = 0; n < count; n++) {
		o->lfo_tick++;
		double t = (double)o->lfo_tick / OPL_RATE;
		double tri = fabs(fmod(t * 3.7 * 2.0, 2.0) - 1.0); /* 0..1 triangle at 3.7 Hz */
		double lfo_am = tri * ((o->reg[0xBD] & 0x80) ? 25.6 : 5.3);
		double lfo_vib = sin(2.0 * M_PI * 6.1 * t) * ((o->reg[0xBD] & 0x40) ? 0.0081 : 0.0040);
		double mix = 0;
		for (int ch = 0; ch < 9; ch++) {
			int m = CH_MOD[ch], c = m + 3;
			env_step(o, m, ch); env_step(o, c, ch);
			int fb = (o->reg[0xC0 + ch] >> 1) & 7;
			double fbmod = fb ? (o->op[m].fb[0] + o->op[m].fb[1]) / 2.0 * ((1 << (fb - 1)) / 64.0) : 0.0;
			double mo = op_out(o, m, ch, fbmod, lfo_am, lfo_vib);
			o->op[m].fb[1] = o->op[m].fb[0]; o->op[m].fb[0] = mo;
			double co;
			if (o->reg[0xC0 + ch] & 1) { co = op_out(o, c, ch, 0.0, lfo_am, lfo_vib); mix += mo + co; }
			else { co = op_out(o, c, ch, mo, lfo_am, lfo_vib); mix += co; }
		}
		double s = mix * 9000.0;
		out[n] = (int16_t)(s > 32767 ? 32767 : (s < -32768 ? -32768 : s));
	}
}
