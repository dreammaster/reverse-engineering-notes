// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 256950-258121): the game control the
// program actually instantiates (Init() constructs a THGameControl, not a plain
// TGameControl): a TGameControl that is also a TEventHandlerInterface (a second
// vtable at +0xA90) and registers itself with the game object to hear when a
// field of the game settings changes (RegisterEventHandler()), applying the
// change to the running game in OnEvent().
#pragma once

#include "datastruct/eventhandler.h"
#include "vsplayer/control/gameControl.h"

class THGameControl : public TGameControl, public TEventHandlerInterface {
public:
	THGameControl();
	~THGameControl() override;

	/** Registers for the changes of the game object's fields. Does nothing while
	 *  there is no game. */
	void RegisterEventHandler();

	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;
};
