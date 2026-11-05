/* The complete mutable state of the game: object records, variables, music words and the list of scheduled events - exactly what the original writes to a save game.
 *
 * Save file (the original's format, so saves are interchangeable; see serialize_state? in the analysis):
 *   0x20 bytes   header, all 0xFF
 *   the records of all objects in id order (13,400 bytes)
 *   the 205 regions of Vars in the original's save order (4,522 bytes)
 *   4 words of music state, u16 event count, 4 bytes per scheduled event
 */
#ifndef XANTH_GAMESTATE_H
#define XANTH_GAMESTATE_H

#include "xanth/objects.h"
#include "xanth/tables/vars.h"

namespace Xanth {

/** An entry of the event list (4 bytes in the save game). The fields are defined with the event system. */
struct ScheduledEvent {
	uint16 a, b;
};

class GameState {
public:
	GameState();

	/** The state of a new game. */
	void reset();

	ObjectModel objects;
	Vars v;
	uint16 music[4];                      // word_8912C, word_8911A, word_89128 (0x7F at the start), word_8912A of the original: the music state saved with the game
	std::vector<ScheduledEvent> events;   // at most 16

	/** Writes the save game bytes. */
	void save(Common::Bytes &out) const;
	/** Restores from save game bytes; false (state unchanged) if the data is not a valid save of this game. */
	bool load(const Common::Bytes &data);

	/** Size of a save game with `eventCount` scheduled events. */
	static size_t saveSize(size_t eventCount);
};

} // namespace Xanth

#endif
