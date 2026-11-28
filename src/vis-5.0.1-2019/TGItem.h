// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 260668-261289, all 9 manifest-listed
// methods): an item of an interface (an inventory item). It is a TMObject (the sprite, the
// object's animation) that is drawn centred on a position the interface gives it
// (SetCenteredPosition()), gets the events of the player on it forwarded to the place holder
// (button) of the interface that shows it, and, when it is clicked without an action to
// run, becomes the item the player has picked up (kGameUsedItem), or - for an item that can
// be dragged - the one that is moved with the cursor.
//
// Original layout (TMObject ends at +0x1AC): +0x1AC/+0x1B0 the centred position; THItem
// adds its event handler at +0x1B8.
#pragma once

#include "TMObject.h"

class TGItem : public TMObject {
public:
	TGItem(const TVisObjRef &ref, bool animated);
	~TGItem() override = default;

	/** Draws the item centred at its position. */
	void Draw() override;
	/** Passes the event on to the place holder of the interface that has the item. */
	void HandlePreExecution(TGEventInfo &info) override;
	/** A click that no action was run for picks the item up (see the class comment). */
	void HandlePostExecution(TGEventInfo &info, const TGActionInfo &actionInfo) override;

	/** Shows the item's animation again (when the item is active and has one that is not
	 *  shown yet). */
	void StartAnimation();

	/** The position the item is drawn centred on. */
	void SetCenteredPosition(const wxPoint &position) {
		_centeredPosition = position;
	}
	/** The position next to the right edge of the item, as high as its centre (where the
	 *  cursor is put to hold it). */
	wxPoint GetPositionNextToItem() const;

private:
	wxPoint _centeredPosition;  // +0x1AC
};
