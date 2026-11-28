// Not yet assert-confirmed to a specific file; stays at the top level.
// A thread-safe queue of (Event, owning handler) pairs: AddPendingEvent()
// clones the event and enqueues it (Deponia_Linux.asm lines 1599448-1599510),
// ProcessPendingEvents() (asm lines 1594325-1594376) dispatches and deletes
// each queued event under one global critical section. Neither method's own
// `this` matters to the queue itself (it's genuinely global state, matching
// the same "global, not per-instance" shape as TGameControl's own
// EngineEvents queue) - `this` is only threaded through as the handler each
// event's Clone() gets paired with.
#pragma once

class Event;

class EventHandler {
public:
	virtual ~EventHandler() = default;

	// Confirmed virtual (EventHandler::ProcessPendingEvents, Deponia_Linux.
	// asm line 1594346, vtable slot 2) - dispatches one queued event to its
	// owning handler; "HandleEvent" is a guess from that shape, not a
	// recovered identifier.
	virtual void HandleEvent(Event *event);

	void ProcessPendingEvents();
	void AddPendingEvent(Event *event);
};
