// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 218884-219190, all 5 manifest-listed methods): the item
// the game actually creates (TGInterface::UpdateItems() makes one for each item of the
// character, 0x1C0 bytes): a TGItem that is also a TEventHandlerInterface (a second vtable at
// +0x1B8) and listens to its data object. Like the other TH* classes it is the thin layer
// that connects the data object to the running game: a change of the item's visibility,
// destination visibility and scale is taken over when it happens.
#pragma once

#include "TGItem.h"
#include "datastruct/eventhandler.h"

class THItem : public TGItem, public TEventHandlerInterface {
public:
	THItem(const TVisObjRef &ref, bool animated);
	~THItem() override;

	/** A change of a field of the item. */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;
};
