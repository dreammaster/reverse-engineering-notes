#ifndef ULTIMA6_FILES_MUSIC_H
#define ULTIMA6_FILES_MUSIC_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * One piece of music (ULTIMA.M, BOOTUP.M, INTRO.M, CREATE.M, FOREST.M, HORNPIPE.M, ENGAGE.M,
 * STONES.M, DUNGEON.M, BRIT.M, GARGOYLE.M, END.M): an LZW stream (see lzw.h) holding one track of
 * events. The sound drivers (U6ADLIB.DRV and friends) play it from their timer interrupt, which
 * the driver programs to 60 Hz (PIT divisor 4DAEh), by jumping through a 16-entry table indexed
 * by the high nibble of each event byte. Event byte = command << 4 | channel:
 *
 *   0-7   one data byte: 0 note off, 1 note on, 2 set pitch, 3 set level, 4 modulation, 5 slide,
 *         6 vibrato, 7 program change (names after Nuvie's parser; the layout is what the driver reads)
 *   8     sub-command in the channel field: 1 call (u8 repeat count, u16 offset), 2 delay (u8 ticks),
 *         3 load instrument (u8 slot + 11 bytes of OPL instrument), 5 fade out, 6 fade in (u8 each);
 *         other values have no data
 *   E     set the loop point    F  return (ends a subroutine, or loops back to the loop point)
 *
 * All twelve shipped tracks consist of valid events from first to last byte and every call target
 * is inside the track.
 */
struct MusicTrack {
	enum Command {
		NOTE_OFF = 0, NOTE_ON, SET_PITCH, SET_LEVEL, MODULATION, SLIDE, VIBRATO, PROGRAM,
		SUBCOMMAND = 8,
		LOOP_POINT = 0xE,
		RETURN = 0xF
	};
	enum Sub { CALL = 1, DELAY = 2, LOAD_INSTRUMENT = 3, FADE_OUT = 5, FADE_IN = 6 };

	struct Event {
		size_t offset;
		int command;
		int channel;
		size_t dataOffset; // first data byte
		size_t dataLength;
	};

	Bytes data;
	std::vector<Event> events;
	int totalDelay; // sum of the delay sub-commands, in ticks (one pass, subroutines counted where they are defined)

	/** Decodes the track; false if an event is not recognised or runs past the end, or a call leaves the track. */
	bool load(const Bytes &lzwFile);
};

} // namespace Ultima6

#endif
