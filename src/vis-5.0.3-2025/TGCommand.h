// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 261322-261640, all 5 manifest-listed methods): the
// button of an interface that is a command (the verbs the player chooses: look, talk, take,
// use...). It adds nothing to THButton but what a click does: the command becomes the
// active command of the interface it is in, and whether it is the interface's standard
// command.
#pragma once

#include "THButton.h"

class TGCommand : public THButton {
public:
	TGCommand(const TVisObjRef &ref, TGInterface *owner) : THButton(ref, owner) {
	}

	/** A click makes the command the active one of its interface (then the event goes on as
	 *  usual). */
	void ExecuteEvent(TGEventInfo &info) override;
	/** Whether it is the standard command of its interface (the one that is active when no
	 *  other was chosen). */
	bool IsStandardCommand() const;
};
