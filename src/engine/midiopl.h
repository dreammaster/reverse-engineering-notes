/*
 * midiopl.h - plays the MIDI stream that ROLAND.BIN (the Roland MT-32 / LAPC-1 driver of Cadaver 2) writes to the MPU-401 (port 330h) on the OPL2 synthesiser of
 * engine/opl.c: note on / off on up to nine voices, two instrument presets chosen by the channel's program number, velocity as carrier level, pitch bend ignored,
 * channel 10 (drums) ignored. An approximation meant to make the tunes audible without a MIDI device.
 */
#ifndef CADAVER_MIDIOPL_H
#define CADAVER_MIDIOPL_H

#include "opl.h"

typedef struct {
	Opl *opl;
	int status, need, have, data[2];      /* running status, bytes the message needs, bytes collected */
	int program[16];
	int vol[16];                          /* controller 7 */
	struct { int chan, note, age; } voice[9];
	int clock;
} MidiOpl;

void midiopl_init(MidiOpl *m, Opl *opl);
void midiopl_byte(MidiOpl *m, int b);
void midiopl_reset(MidiOpl *m);

#endif
