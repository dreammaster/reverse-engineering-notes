/*
 * opl.h - a small YM3812 (OPL2) software synthesiser for the AdLib driver (ADLIB.BIN writes the registers through ports 388h / 389h).
 * Approximate: 9 two-operator voices with the four waveforms, ADSR envelopes from the rate tables' orders of magnitude, total level, multiples,
 * feedback, FM / additive connection, tremolo and vibrato as plain LFOs; no rhythm section and no key-scale tables. Meant to make the music audible,
 * not to be sample exact.
 */
#ifndef CADAVER_OPL_H
#define CADAVER_OPL_H

#include <stdint.h>

typedef struct {
	double phase, env, out1, out2;
	int stage;                 /* 0 off, 1 attack, 2 decay, 3 sustain, 4 release */
} OplOp;

typedef struct {
	uint8_t reg[256];
	uint8_t index;
	uint8_t status;
	OplOp op[9][2];
	int keyon[9];
	double rate;
	double lfo_a, lfo_v;
} Opl;

void opl_init(Opl *o, int sample_rate);
void opl_write_index(Opl *o, uint8_t v);
void opl_write_data(Opl *o, uint8_t v);
uint8_t opl_read_status(const Opl *o);
/* mono samples, 16 bit */
void opl_render(Opl *o, int16_t *out, int n);

#endif
