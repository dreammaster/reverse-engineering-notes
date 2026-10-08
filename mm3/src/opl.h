/* A compact YM3812 (OPL2) FM synthesiser: 9 two-operator channels, four waveforms, ADSR envelopes, key scaling, tremolo/vibrato.
 * Written for the game's AdLib music; melodic mode only (the rhythm mode drums of the chip are not emulated: MM3's streams keep
 * register BDh at 00h).  Output is mono 16-bit at OPL_RATE. */
#ifndef MM3_OPL_H
#define MM3_OPL_H

#include <stdint.h>

#define OPL_RATE 49716

typedef struct {
	uint8_t reg[256];
	uint8_t addr;
	/* operator state (18 operators) */
	struct { uint32_t phase; double env; int stage; double fb[2]; } op[18];
	uint32_t lfo_tick;
} Opl;

void opl_reset(Opl *o);
void opl_write(Opl *o, uint8_t reg, uint8_t value);
void opl_samples(Opl *o, int16_t *out, int count);

#endif
