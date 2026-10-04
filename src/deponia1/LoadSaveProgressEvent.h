// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed layout (Deponia_Linux.asm lines 620683-620960: destructor, Clone;
// built in TVisionaire::BinaryLoad): a wxCommandEvent-shaped object (0x58
// bytes: a base of event type/flags up to +0x48, the command string at +0x48)
// whose own payload is the table number being loaded (+0x50, -1 for the game
// object itself) and a flag byte (+0x54, always 1 where it is posted). The
// editor's loading progress bar listens for it; the player never does. The
// event type the loader gives it is 0x26; the wx base fields are not modelled.
#pragma once

#include "Event.h"

class LoadSaveProgressEvent : public Event {
public:
	LoadSaveProgressEvent(int table, bool flag) : _table(table), _flag(flag) {
	}

	Event *Clone() const override {
		return new LoadSaveProgressEvent(*this);
	}

	int GetTable() const {
		return _table;
	}
	bool GetFlag() const {
		return _flag;
	}

private:
	int _eventType = 0x26;
	int _table;
	bool _flag;
};
