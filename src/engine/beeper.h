/*
 * beeper.h - the PC-speaker sound driver of Cadaver (BEEPER.BIN, "beeper music by Sound Images") run on the 8086 interpreter.
 *
 * The game loads the BIN to offset 0 of a segment and talks to it through far calls (entry 0 = 50 Hz tick from the timer ISR, entry 4 = init)
 * and a command area at offsets 8..0Ch ([0Ah] effect request, [0Ch] stop request).  1000:156C (id -> effect translation table at
 * ds:4064 / Cadaver 2: ds:406E, bit 7 = looping effect that is remembered in the 128-byte flag table at ds:40F0) and 1000:1613 (stop) are ported
 * as beeper_play / beeper_stop.  The driver programs PIT channel 2 (out 43h/42h) and the speaker gate (out 61h); beeper_tick returns the PIT
 * divisor that sounds after one tick (0 = silent), beeper_render synthesises 16-bit mono samples (square wave, 50 ticks per second).
 */
#ifndef CADAVER_BEEPER_H
#define CADAVER_BEEPER_H

#include <stddef.h>
#include <stdint.h>
#include "x86.h"

typedef struct Beeper {
	X86 cpu;
	uint8_t *mem;
	uint8_t port61;
	uint8_t pit[2];
	int npit;
	unsigned divisor;
	uint8_t flags[0x80];        /* ds:40F0: effect number of the looping sounds that are on (0 = off) */
	/* synthesis state */
	int rate;
	int left;                   /* samples left in the current tick */
	double phase;
	unsigned cur_div;
} Beeper;

/* `bin` = BEEPER.BIN (already unpacked).  Returns 0 on success. */
int beeper_init(Beeper *b, const uint8_t *bin, size_t len, int sample_rate);
void beeper_free(Beeper *b);
/* One 50 Hz driver tick; returns the sounding PIT divisor (0 = silent). */
unsigned beeper_tick(Beeper *b);
/* Start the sound with game id `id` (1000:156C). */
void beeper_play(Beeper *b, int id);
/* Stop the looping sound `id` (1000:1613). */
void beeper_stop(Beeper *b, int id);
/* Silence everything and forget the looping sounds (1000:15EB at a level change). */
void beeper_silence(Beeper *b);
/* Is the driver busy with an effect? ([0Ah] != 0) */
int beeper_busy(const Beeper *b);
/* Synthesise n mono samples. */
void beeper_render(Beeper *b, int16_t *out, int n);

#endif
