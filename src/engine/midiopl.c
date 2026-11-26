#include "midiopl.h"
#include <math.h>
#include <string.h>

static const int mod_op[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};

static void w(Opl *o, int reg, int v) { opl_write_index(o, (uint8_t)reg); opl_write_data(o, (uint8_t)v); }

void midiopl_init(MidiOpl *m, Opl *opl) {
	memset(m, 0, sizeof(*m));
	m->opl = opl;
	for (int i = 0; i < 16; i++) m->vol[i] = 100;
	for (int v = 0; v < 9; v++) m->voice[v].chan = -1;
}

void midiopl_reset(MidiOpl *m) {
	for (int v = 0; v < 9; v++) { w(m->opl, 0xB0 + v, 0); m->voice[v].chan = -1; }
}

/* two presets: a sustained, organ-like one and a plucked one (pianos, guitars, harps, mallets, basses, strings plucked) */
static int plucked(int prog) { return prog < 32 || (prog >= 104 && prog < 120) || prog == 45; }

static void setup_voice(MidiOpl *m, int v, int prog, int vel, int chvol) {
	Opl *o = m->opl;
	int mo = mod_op[v], co = mo + 3;
	int lvl = 63 - (int)(63.0 * (vel / 127.0) * (chvol / 127.0));       /* attenuation 0..63 */
	if (plucked(prog)) {
		w(o, 0x20 + mo, 0x01); w(o, 0x40 + mo, 0x18); w(o, 0x60 + mo, 0xF3); w(o, 0x80 + mo, 0x74); w(o, 0xE0 + mo, 0);
		w(o, 0x20 + co, 0x01); w(o, 0x40 + co, lvl);  w(o, 0x60 + co, 0xD5); w(o, 0x80 + co, 0x64); w(o, 0xE0 + co, 0);
		w(o, 0xC0 + v, 0x04);
	} else {
		w(o, 0x20 + mo, 0x21); w(o, 0x40 + mo, 0x1C); w(o, 0x60 + mo, 0xA1); w(o, 0x80 + mo, 0x11); w(o, 0xE0 + mo, 0);
		w(o, 0x20 + co, 0x21); w(o, 0x40 + co, lvl);  w(o, 0x60 + co, 0xA2); w(o, 0x80 + co, 0x15); w(o, 0xE0 + co, 0);
		w(o, 0xC0 + v, 0x06);
	}
}

static void key(MidiOpl *m, int v, int note, int on) {
	double f = 440.0 * pow(2.0, (note - 69) / 12.0);
	int block = 0, fn = 0;
	for (block = 0; block < 8; block++) {
		fn = (int)(f * (1 << (20 - block)) / 49716.0 + 0.5);
		if (fn < 1024) break;
	}
	if (block > 7) { block = 7; fn = 1023; }
	w(m->opl, 0xA0 + v, fn & 0xFF);
	w(m->opl, 0xB0 + v, (on ? 0x20 : 0) | (block << 2) | (fn >> 8));
}

static void note_on(MidiOpl *m, int ch, int note, int vel) {
	if (ch == 9) return;                                   /* drums */
	int v = -1, oldest = 0;
	for (int i = 0; i < 9; i++) if (m->voice[i].chan < 0) { v = i; break; }
	if (v < 0) { for (int i = 1; i < 9; i++) if (m->voice[i].age < m->voice[oldest].age) oldest = i; v = oldest; w(m->opl, 0xB0 + v, 0); }
	m->voice[v].chan = ch; m->voice[v].note = note; m->voice[v].age = ++m->clock;
	setup_voice(m, v, m->program[ch], vel, m->vol[ch]);
	key(m, v, note, 1);
}

static void note_off(MidiOpl *m, int ch, int note) {
	for (int i = 0; i < 9; i++)
		if (m->voice[i].chan == ch && m->voice[i].note == note) { key(m, i, note, 0); m->voice[i].chan = -1; return; }
}

static void message(MidiOpl *m) {
	int ch = m->status & 15;
	switch (m->status >> 4) {
	case 8: note_off(m, ch, m->data[0]); break;
	case 9: if (m->data[1]) note_on(m, ch, m->data[0], m->data[1]); else note_off(m, ch, m->data[0]); break;
	case 11:
		if (m->data[0] == 7) m->vol[ch] = m->data[1];
		else if (m->data[0] == 123 || m->data[0] == 120) for (int i = 0; i < 9; i++) if (m->voice[i].chan == ch) { key(m, i, m->voice[i].note, 0); m->voice[i].chan = -1; }
		break;
	case 12: m->program[ch] = m->data[0]; break;
	default: break;
	}
}

void midiopl_byte(MidiOpl *m, int b) {
	b &= 0xFF;
	if (b >= 0xF8) return;                                  /* realtime */
	if (b >= 0x80) {
		m->status = b; m->have = 0;
		int hi = b >> 4;
		m->need = (hi == 12 || hi == 13) ? 1 : (hi >= 8 && hi <= 14) ? 2 : 0;       /* system messages are skipped */
		return;
	}
	if (!m->need) return;
	m->data[m->have++] = b;
	if (m->have == m->need) { message(m); m->have = 0; }
}
