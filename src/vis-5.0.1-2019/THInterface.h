// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 246907-247200, all 5 manifest-listed methods): the
// interface the game actually creates (TGameControl::InitInterfaces() makes one for each
// interface of the game): a TGInterface that is also a TEventHandlerInterface (a second
// vtable at +0x258) and listens to the interface's record. Like the other TH* classes it is
// the thin layer that connects the data to the running game: a change of the interface's
// visibility, fade, active command, scroll position, size, offset or displacement is taken
// over when it happens (and the interfaces are put on the screen again).
#pragma once

#include "TGInterface.h"
#include "datastruct/eventhandler.h"
#include "datastruct/visobjref.h"

class THInterface : public TGInterface, public TEventHandlerInterface {
public:
	explicit THInterface(const TVisObjRef &object);
	~THInterface() override;

	/** A change of a field of the interface's record. */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;
};
