#include "vstables/records.h"

#include <cstdlib>
#include <cstring>

#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static bool sameId(const TVisObjRef &ref, const TVisionaireObject *object) {
	return std::memcmp(ref.GetId(), object->GetId(), 4) == 0;
}

// Confirmed (asm lines 1475142-1475414): adds an item to the inventory (false
// if the character already has it). With `scrollIntoView` the first interface
// that has item buttons is scrolled, in steps of its scroll step size, until
// the end of the item list is in view.
bool TTCharacter::AddItem(const TVisObjRef &item, bool scrollIntoView) {
	TVList items;

	GetLinks(kCharacterItems, TypeOrder::kValue0, items);
	for (TVisionaireObject *existing : items) {
		if (sameId(item, existing))
			return false;
	}

	items.push_back(item);
	SetValue(kCharacterItems, items, true);

	if (!scrollIntoView)
		return true;

	TVList interfaces;
	GetLinks(kCharacterInterfaces, TypeOrder::kValue0, interfaces);
	for (TVisionaireObject *object : interfaces) {
		TVisObjRef interfaceRef(object);
		TVList buttons;
		int itemButtons = 0;

		interfaceRef.GetLinks(kInterfaceButtons, TypeOrder::kValue0, buttons);
		for (TVisionaireObject *button : buttons) {
			if (button->GetInt(kButtonType) == 0)
				itemButtons++;
		}
		if (itemButtons == 0)
			continue;

		int step = std::abs(interfaceRef.GetInt(kInterfaceScrollStepSize));
		int scroll = interfaceRef.GetInt(kInterfaceItemsScrollPosition);

		if (step != 0 && itemButtons + scroll < (int)items.size()) {
			do {
				scroll += step;
			} while (itemButtons + scroll < (int)items.size());
			interfaceRef.SetValue(kInterfaceItemsScrollPosition, scroll, TSendEventEnum::kSendEvent);
		}
		break;
	}
	return true;
}

// Confirmed (asm lines 1475415-1475548): adds the items the character doesn't
// have yet.
void TTCharacter::AddItems(const TVList &newItems) {
	TVList items;

	GetLinks(kCharacterItems, TypeOrder::kValue0, items);
	for (TVisionaireObject *item : newItems) {
		bool found = false;

		for (TVisionaireObject *existing : items) {
			if (std::memcmp(item->GetId(), existing->GetId(), 4) == 0) {
				found = true;
				break;
			}
		}
		if (!found)
			items.push_back(TVisObjRef(item));
	}

	SetValue(kCharacterItems, items, true);
}

// Confirmed (asm lines 1475549-1475645)
bool TTCharacter::RemoveItem(const TVisObjRef &item) {
	TVList items;

	GetLinks(kCharacterItems, TypeOrder::kValue0, items);
	for (TVList::iterator it = items.begin(); it != items.end(); ++it) {
		if (sameId(item, *it)) {
			items.erase(it);
			SetValue(kCharacterItems, items, true);
			return true;
		}
	}
	return false;
}
