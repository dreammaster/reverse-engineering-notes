#include "THInterface.h"

#include "AppGlobals.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Confirmed (asm lines 247172-247200)
THInterface::THInterface(const TVisObjRef &object) : TGInterface(object) {
	_ref.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 247088-247114)
THInterface::~THInterface() {
	_ref.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 246907-247056). Which field changed is all that is given: the new
// value is read from the data.
void THInterface::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	switch (field) {
	case kInterfaceVisibility:
		SetDestAlpha(_ref.GetInt(kInterfaceVisibility), 0);
		break;
	case kInterfaceDestVisibility:
		SetDestAlpha(_ref.GetInt(kInterfaceDestVisibility), _ref.GetInt(kInterfaceTimeToDestVisibility));
		break;
	case kInterfaceVisible:
		UpdateActiveStatus();
		gameControl()->AdjustInterfacesOnScreen(true, this);
		break;
	case kInterfaceDisplacement:
	case kInterfaceSize:
	case kInterfaceOffset:
		gameControl()->AdjustInterfacesOnScreen(false, this);
		break;
	case kInterfaceActiveCommand:
		SetActiveCommand(_ref.GetLink(kInterfaceActiveCommand), true);
		break;
	case kInterfaceItemsScrollPosition: {
		// the scroll position stays at an item
		int position = _ref.GetInt(kInterfaceItemsScrollPosition);

		if (position < 0 || _items.empty())
			_ref.SetValue(kInterfaceItemsScrollPosition, 0, TSendEventEnum::kNoEvent);
		else if (position >= (int)_items.size())
			_ref.SetValue(kInterfaceItemsScrollPosition, (int)_items.size() - 1, TSendEventEnum::kNoEvent);

		TestActiveObjects();
		break;
	}
	default:
		break;
	}
}
