// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed only by call shape (TGameControl::LoadAndInitGame,
// Deponia_Linux.asm lines 467773-467774, 467802): TGameControl's own `this`
// pointer is passed as a TSignalSlot* when isEditor is true, nullptr
// otherwise - implying TGameControl derives from (or otherwise converts to)
// TSignalSlot in the original. That relationship isn't modeled here (like
// THGameControl's own not-yet-integrated relationship - see NOTES.md).
//
// The one virtual (vtable slot 0: TSText calls it as `slot->[0](signal, result)`, TSText.cpp)
// takes a message and fills in the answer; TMasterControl::Signal() has the same shape.
// The sound manager is the slot the texts talk to.
#pragma once

#include "TSignalData.h"

class TSignalSlot {
public:
	virtual ~TSignalSlot() = default;

	/** Handles a message; the answer (of the same type as the message, with its result in the
	 *  fields the message type says) goes to `result`. */
	virtual void Signal(const TSignalData &signal, TSignalData &result) {
		// (no sound system: it answers every question with 0)
		result = TSignalData{};
		result.type = signal.type;
		result.value = 0;
	}
};
