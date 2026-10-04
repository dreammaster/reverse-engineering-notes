#include "vstables/records.h"

#include <cstring>

#include "TTButton.h"
#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 1467119-1467330): makes the next command button (one that
// is a command and active) after the active one - in the order of the buttons,
// wrapping around - the active command.
void TTInterface::SetNextCommand() {
	TVisObjRef active = GetLink(kInterfaceActiveCommand);
	TVList buttons;

	GetLinks(kInterfaceButtons, TypeOrder::kValue1, buttons);

	TVList::iterator current = buttons.begin();
	while (current != buttons.end() && std::memcmp(active.GetId(), (*current)->GetId(), 4) != 0)
		++current;
	if (current == buttons.end())
		return;

	TVList::iterator next = current + 1;
	for (; next != buttons.end(); ++next) {
		TTButton button((TVisObjRef(*next)));

		if (button.IsCommand() && button.IsActive())
			break;
	}

	if (next == buttons.end()) {
		// wrap around: from the first button up to the active one
		for (next = buttons.begin(); next != buttons.end(); ++next) {
			if (std::memcmp(active.GetId(), (*next)->GetId(), 4) == 0)
				break;

			TTButton button((TVisObjRef(*next)));

			if (button.IsCommand() && button.IsActive())
				break;
		}
	}

	if (next == buttons.end() || std::memcmp(active.GetId(), (*next)->GetId(), 4) == 0)
		return;

	SetLink(kInterfaceActiveCommand, TVisObjRef(*next), true);
}
