// Not yet assert-confirmed to a specific file; stays at the top level.
// A queued event object, owned by the vec<pair<Event*,EventHandler*>>
// EventHandler::AddPendingEvent()/ProcessPendingEvents() maintain (see
// EventHandler.h) - ProcessPendingEvents() deletes each one (via its own
// virtual destructor) once dispatched.
#pragma once

class Event {
public:
	virtual ~Event() = default;

	// Confirmed virtual (EventHandler::AddPendingEvent, Deponia_Linux.asm
	// line 1599467, vtable slot 2) - its return value, not the original
	// event pointer, is what actually gets queued; "Clone" is a guess from
	// that shape, not a recovered identifier.
	virtual Event *Clone() const;
};
