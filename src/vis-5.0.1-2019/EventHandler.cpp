#include "EventHandler.h"

#include <utility>
#include <vector>

#include "Event.h"
#include "WxStub.h"

namespace {
wxCriticalSection eventSection;
std::vector<std::pair<Event *, EventHandler *>> vec;
}  // namespace

void EventHandler::HandleEvent(Event */*event*/) {
}

void EventHandler::ProcessPendingEvents() {
	// Confirmed (Deponia_Linux.asm lines 1594325-1594376).
	wxCriticalSectionLocker locker(eventSection);
	for (auto &entry : vec) {
		entry.second->HandleEvent(entry.first);
		delete entry.first;
	}
	vec.clear();
}

void EventHandler::AddPendingEvent(Event *event) {
	// Confirmed (Deponia_Linux.asm lines 1599448-1599510).
	wxCriticalSectionLocker locker(eventSection);
	vec.push_back({event->Clone(), this});
}
