#include "opl.h"
#include <math.h>
#include <string.h>

static const int op_off[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
static const double mult_tab[16] = {0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15};

void opl_init(Opl *o, int rate) {
	memset(o, 0, sizeof(*o));
	o->rate = rate;
}

uint8_t opl_read_status(const Opl *o) { return o->status; }
void opl_write_index(Opl *o, uint8_t v) { o->index = v; }

void opl_write_data(Opl *o, uint8_t v) {
	int r = o->index;
	if (r == 4) {                                        /* timers: 80h = reset flags; 21h starts timer 1 (the driver's detection waits for the flags) */
		if (v & 0x80) o->status = 0;
		else if (v & 1) o->status = 0xC0;
		o->reg[4] = v;
		return;
	}
	o->reg[r] = v;
	if (r >= 0xB0 && r <= 0xB8) {
		int c = r - 0xB0, on = (v >> 5) & 1;
		if (on && !o->keyon[c]) { for (int k = 0; k < 2; k++) { o->op[c][k].stage = 1; o->op[c][k].phase = 0; o->op[c][k].env = 0; } }
		else if (!on && o->keyon[c]) { for (int k = 0; k < 2; k++) if (o->op[c][k].stage) o->op[c][k].stage = 4; }
		o->keyon[c] = on;
	}
}

static double rate_time(int r) {                         /* seconds for a full swing at rate r (0 = never, 15 = instant) */
	if (r <= 0) return 1e9;
	if (r >= 15) return 0.0004;
	return 2.8 / pow(2.0, r - 1) * 1.0 + 0.0004;
}

static double env_step(Opl *o, OplOp *p, int ar, int dr, int sl, int rr, int sustain) {
	double dt = 1.0 / o->rate;
	switch (p->stage) {
	case 1: p->env += dt / rate_time(ar); if (p->env >= 1.0) { p->env = 1.0; p->stage = 2; } break;
	case 2: { double target = sl == 15 ? 0.0 : pow(10.0, -(sl * 3.0) / 20.0);
	          p->env -= dt / rate_time(dr) * (1.0 - target) * 4.0;
	          if (p->env <= target) { p->env = target; p->stage = sustain ? 3 : 4; } break; }
	case 3: break;
	case 4: p->env -= dt / rate_time(rr) * 4.0; if (p->env <= 0.0) { p->env = 0.0; p->stage = 0; } break;
	default: p->env = 0; break;
	}
	return p->env;
}

static double wave(int w, double ph) {
	double s = sin(ph * 2.0 * M_PI);
	switch (w & 3) {
	case 1: return s > 0 ? s : 0;
	case 2: return fabs(s);
	case 3: return fmod(ph, 1.0) < 0.5 ? fabs(s) : 0;
	default: return s;
	}
}

void opl_render(Opl *o, int16_t *out, int n) {
	int wsel = o->reg[1] & 0x20;
	for (int i = 0; i < n; i++) {
		o->lfo_a += 3.7 / o->rate; o->lfo_v += 6.1 / o->rate;
		double acc = 0;
		for (int c = 0; c < 9; c++) {
			OplOp *m = &o->op[c][0], *k = &o->op[c][1];
			if (!m->stage && !k->stage) continue;
			int om = op_off[c], ok = op_off[c] + 3;
			int fn = o->reg[0xA0 + c] | ((o->reg[0xB0 + c] & 3) << 8), blk = (o->reg[0xB0 + c] >> 2) & 7;
			double f = fn * 49716.0 / (double)(1 << (20 - blk));
			int fb = (o->reg[0xC0 + c] >> 1) & 7, conn = o->reg[0xC0 + c] & 1;
			double fm = f * mult_tab[o->reg[0x20 + om] & 15], fk = f * mult_tab[o->reg[0x20 + ok] & 15];
			if (o->reg[0x20 + om] & 0x40) fm *= 1.0 + 0.007 * sin(o->lfo_v * 2 * M_PI);
			if (o->reg[0x20 + ok] & 0x40) fk *= 1.0 + 0.007 * sin(o->lfo_v * 2 * M_PI);
			double em = env_step(o, m, o->reg[0x60 + om] >> 4, o->reg[0x60 + om] & 15, o->reg[0x80 + om] >> 4, o->reg[0x80 + om] & 15, o->reg[0x20 + om] & 0x20);
			double ek = env_step(o, k, o->reg[0x60 + ok] >> 4, o->reg[0x60 + ok] & 15, o->reg[0x80 + ok] >> 4, o->reg[0x80 + ok] & 15, o->reg[0x20 + ok] & 0x20);
			double tlm = pow(10.0, -((o->reg[0x40 + om] & 63) * 0.75) / 20.0), tlk = pow(10.0, -((o->reg[0x40 + ok] & 63) * 0.75) / 20.0);
			if (o->reg[0x20 + om] & 0x80) em *= 1.0 - 0.2 * (0.5 + 0.5 * sin(o->lfo_a * 2 * M_PI));
			if (o->reg[0x20 + ok] & 0x80) ek *= 1.0 - 0.2 * (0.5 + 0.5 * sin(o->lfo_a * 2 * M_PI));
			double fbk = fb ? (m->out1 + m->out2) * 0.5 * (M_PI / 16.0) * (double)(1 << (fb - 1)) / (2.0 * M_PI) : 0;
			double mo = wave(wsel ? o->reg[0xE0 + om] : 0, m->phase + fbk) * em * tlm;
			m->out2 = m->out1; m->out1 = mo;
			m->phase += fm / o->rate; if (m->phase > 1e6) m->phase = 0;
			double mod = conn ? 0.0 : mo * 1.2;                 /* phase modulation in cycles */
			double ko = wave(wsel ? o->reg[0xE0 + ok] : 0, k->phase + mod) * ek * tlk;
			k->phase += fk / o->rate; if (k->phase > 1e6) k->phase = 0;
			acc += conn ? (mo + ko) * 0.5 : ko;
		}
		double v = acc * 5000.0;
		out[i] = (int16_t)(v > 32000 ? 32000 : v < -32000 ? -32000 : v);
	}
}
