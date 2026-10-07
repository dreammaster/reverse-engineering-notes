#include "TGCommand.h"

#include "vstables/fieldIds.h"

// Confirmed (asm lines 261363-261550): a click (mouse event 1, 3 or 4) makes the command the
// active one of the interface. (The original first builds a copy of the event from its
// parts; the copy is what the base class gets, but it holds the same.)
void TGCommand::ExecuteEvent(TGEventInfo &info) {
	if (info.mouseEvent == 1 || info.mouseEvent == 3 || info.mouseEvent == 4) {
		TVisObjRef panel = _objRef.GetParent();

		panel.SetLink(kInterfaceActiveCommand, _objRef, true);
	}

	TManagedObject::ExecuteEvent(info);
}

// Confirmed (asm lines 261584-261618)
bool TGCommand::IsStandardCommand() const {
	return _objRef.GetParent().GetLink(kInterfaceStandardCommand) == _objRef;
}
