// Not yet assert-confirmed to a specific file; stays alongside composedfile.h.
//
// Confirmed layout (Deponia_Linux.asm lines 548597-548703: Clone() and the
// destructors; TComposedFile::WriteToDisk() fills one in): a 0x88-byte Event
// of type 0x1D (29) posted to an EventHandler after each entry of a composed
// file is written, carrying the entry's index. Its other fields are only
// confirmed as constants WriteToDisk() sets (a "phase" of 3 at +0x44, an
// empty text at +0x48, zeros at +0x50/0x54/0x5C) and copied by Clone(); the
// five qwords at +0x60-0x80 are copied but never written by anything
// reversed.
#pragma once

#include "Event.h"
#include "WxStub.h"

class BuildProgressEvent : public Event {
public:
	static const int kEventType = 0x1D;

	BuildProgressEvent(int phase, int entryIndex) : _phase(phase), _entryIndex(entryIndex) {
	}

	int GetEventType() const {
		return _eventType;
	}
	int GetEntryIndex() const {
		return _entryIndex;
	}
	// Confirmed (asm lines 548597-548654): a field-by-field copy.
	Event *Clone() const override {
		return new BuildProgressEvent(*this);
	}

private:
	int _eventType = kEventType;
	int _phase;
	wxString _text;
	int _field50 = 0;
	int _field54 = 0;
	int _entryIndex;
	int _field5C = 0;
	long long _extra[5] = {0, 0, 0, 0, 0};
};
